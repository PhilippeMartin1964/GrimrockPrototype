#if WITH_DEV_AUTOMATION_TESTS

#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridPartyLIGHT01Tests
{
	const FName WeakLightId(TEXT("PARTY_LIGHT01_Weak"));
	const FName StrongLightId(TEXT("PARTY_LIGHT01_Strong"));

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("PARTY_LIGHT01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}

		~FTestWorld()
		{
			if (!World)
			{
				return;
			}

			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
		}
	};

	UGridItemDefinitionAsset* MakeLightDefinition(UObject* Outer, FName Id, float Intensity, float Radius)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Outer);
		Definition->ItemDefinitionId = Id;
		Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
		Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::OffHand);
		Definition->LightEmitter.bUsePointLight = true;
		Definition->LightEmitter.bDefaultEnabled = true;
		Definition->LightEmitter.PointLightRelativeLocation = FVector(91.0f, 37.0f, 22.0f);
		Definition->LightEmitter.PointLightRelativeRotation = FRotator(7.0f, 11.0f, 13.0f);
		Definition->LightEmitter.LightIntensity = Intensity;
		Definition->LightEmitter.LightRadius = Radius;
		Definition->LightEmitter.LightColor = FLinearColor(0.9f, 0.5f, 0.2f, 1.0f);
		Definition->LightEmitter.bEnableLightFlicker = false;
		Definition->LightEmitter.bEnableLightPositionFlicker = false;
		Definition->LightEmitter.bEnableLightColorFlicker = false;
		return Definition;
	}

	FGridItemInstance MakeEquippedItem(FName Id, const FGuid& CharacterId, int32 CharacterIndex, EGridEquipmentSlot Slot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Id;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = CharacterId;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = Slot;
		Item.bLightsEnabled = true;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyLIGHT01EquipmentDrivenIlluminationTest,
	"Grimrock.Party.LIGHT01.EquipmentDrivenIllumination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyLIGHT01EquipmentDrivenIlluminationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	GridPartyLIGHT01Tests::FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("PARTY-LIGHT01 could not create a transient game world."));
		return false;
	}

	AGrimrockPartyPawn* PartyPawn = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	TestNotNull(TEXT("Party pawn is spawned"), PartyPawn);
	if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
	{
		return false;
	}

	TestNull(TEXT("A bare C++ party does not receive an automatic illumination component"),
		PartyPawn->FindComponentByClass<UGridPartyIlluminationComponent>());

	UGridPartyIlluminationComponent* PartyLight = NewObject<UGridPartyIlluminationComponent>(PartyPawn, TEXT("PartyIlluminationTest"));
	TestNotNull(TEXT("The test can add an explicit party illumination component"), PartyLight);
	if (!PartyLight)
	{
		return false;
	}
	PartyLight->SetupAttachment(PartyPawn->GetRootComponent());
	PartyPawn->AddInstanceComponent(PartyLight);
	PartyLight->RegisterComponent();
	TestEqual(TEXT("The explicitly authored illumination component is discoverable"),
		PartyPawn->FindComponentByClass<UGridPartyIlluminationComponent>(), PartyLight);

	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
	Inventory->PartyInventoryState = FGridPartyInventoryState();
	Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId = FGuid::NewGuid();
	Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId = FGuid::NewGuid();
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
	Inventory->PartyInventoryState.MaxActiveCharacters = 2;

	UGridItemDefinitionAsset* WeakDefinition = GridPartyLIGHT01Tests::MakeLightDefinition(Inventory, GridPartyLIGHT01Tests::WeakLightId, 100.0f, 250.0f);
	UGridItemDefinitionAsset* StrongDefinition = GridPartyLIGHT01Tests::MakeLightDefinition(Inventory, GridPartyLIGHT01Tests::StrongLightId, 450.0f, 625.0f);
	TestTrue(TEXT("Weak light definition registers"), Inventory->RegisterItemDefinition(WeakDefinition));
	TestTrue(TEXT("Strong light definition registers"), Inventory->RegisterItemDefinition(StrongDefinition));

	Inventory->PartyInventoryState.ActiveEquipment[1].OffHand = GridPartyLIGHT01Tests::MakeEquippedItem(
		GridPartyLIGHT01Tests::StrongLightId, Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId, 1, EGridEquipmentSlot::OffHand);
	PartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();

	TestTrue(TEXT("A light held by a non-selected party member illuminates the party"), PartyLight->HasActiveIlluminationSource());
	TestEqual(TEXT("The non-selected equipped source is selected"), PartyLight->ActiveSourceId, GridPartyLIGHT01Tests::StrongLightId);
	TestTrue(TEXT("Selected character without a light has no first-person held light visual"), PartyPawn->GetHeldItemDefinitionId().IsNone());
	TestTrue(TEXT("Source intensity is transmitted to the party proxy"), FMath::IsNearlyEqual(PartyLight->RuntimeConfig.LightIntensity, 450.0f));
	TestTrue(TEXT("Source radius is transmitted to the party proxy"), FMath::IsNearlyEqual(PartyLight->RuntimeConfig.LightRadius, 625.0f));
	TestTrue(TEXT("Physical item point-light offset is not copied to the party proxy"), PartyLight->RuntimeConfig.PointLightRelativeLocation.IsNearlyZero());
	TestTrue(TEXT("Physical item point-light rotation is not copied to the party proxy"), PartyLight->RuntimeConfig.PointLightRelativeRotation.IsNearlyZero());
	TestTrue(TEXT("Party illumination never owns source Niagara"), PartyLight->RuntimeConfig.NiagaraSystem.IsNull());
	TestFalse(TEXT("Party illumination is shadowless by default for first-person ergonomics"), PartyLight->GetPointLightCastShadows());

	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = GridPartyLIGHT01Tests::MakeEquippedItem(
		GridPartyLIGHT01Tests::WeakLightId, Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, 0, EGridEquipmentSlot::MainHand);
	PartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();

	TestEqual(TEXT("The selected character shows its own held light visual"), PartyPawn->GetHeldItemDefinitionId(), GridPartyLIGHT01Tests::WeakLightId);
	TestNotNull(TEXT("Held item actor exists"), PartyPawn->HeldItemActor.Get());
	if (PartyPawn->HeldItemActor && PartyPawn->HeldItemActor->LightEmitterComponent)
	{
		TestTrue(TEXT("Held item remains logically lit"), PartyPawn->HeldItemActor->AreItemLightsEnabled());
		TestTrue(TEXT("Held item keeps its Niagara presentation channel"), PartyPawn->HeldItemActor->LightEmitterComponent->IsNiagaraPresentationEnabled());
		TestFalse(TEXT("Held item delegates its local PointLight to party illumination"), PartyPawn->HeldItemActor->LightEmitterComponent->IsPointLightPresentationEnabled());
	}
	TestEqual(TEXT("Stronger party source wins independently of selected character"), PartyLight->ActiveSourceId, GridPartyLIGHT01Tests::StrongLightId);

	Inventory->PartyInventoryState.ActiveEquipment[1].OffHand = FGridItemInstance();
	Inventory->NotifyPartyInventoryChanged(1);
	TestEqual(TEXT("Removing the stronger source falls back to the remaining hand light"), PartyLight->ActiveSourceId, GridPartyLIGHT01Tests::WeakLightId);
	TestTrue(TEXT("Fallback source intensity is transmitted"), FMath::IsNearlyEqual(PartyLight->RuntimeConfig.LightIntensity, 100.0f));

	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = FGridItemInstance();
	Inventory->NotifyPartyInventoryChanged(0);
	TestFalse(TEXT("Removing the last equipped source disables party illumination"), PartyLight->HasActiveIlluminationSource());
	TestTrue(TEXT("Clearing the last source clears its identity"), PartyLight->ActiveSourceId.IsNone());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
