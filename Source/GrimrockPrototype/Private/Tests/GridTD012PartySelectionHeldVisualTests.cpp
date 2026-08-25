#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridTD012Tests
{
	const FName CharacterZeroLightItemId(TEXT("TD012_CharacterZeroLight"));

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
				FName(*FString::Printf(TEXT("TD012_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num, &Values);
			if (!World || !GEngine)
			{
				return;
			}

			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
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

	FGridItemInstance MakeCharacterZeroLightItem(const FGuid& CharacterId)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = CharacterZeroLightItemId;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = EGridEquipmentSlot::MainHand;
		Item.bLightsEnabled = true;
		return Item;
	}

	AGrimrockPartyPawn* SpawnPartyWithTwoCharacters(FAutomationTestBase& Test, UWorld* World)
	{
		AGrimrockPartyPawn* PartyPawn = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
		if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
		{
			Test.AddError(TEXT("TD01.2 could not spawn a party pawn with its inventory component."));
			return nullptr;
		}

		UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.ActiveCharacters.SetNum(2);
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
		Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId = FGuid::NewGuid();
		Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId = FGuid::NewGuid();
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.MaxActiveCharacters = 2;
		Inventory->PartyInventoryState.ActiveEquipment[0].MainHand =
			MakeCharacterZeroLightItem(Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId);

		PartyPawn->DefaultHeldItemDefinitionId = CharacterZeroLightItemId;
		PartyPawn->HeldTorchActorClass = AGridItemActor::StaticClass();
		PartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();
		return PartyPawn;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD012SelectionChangeTest, "Grimrock.TechnicalDebt.TD01_2.PartySelectionHeldVisual.SelectionChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD012SelectionChangeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	GridTD012Tests::FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("TD01.2 could not create a transient game world."));
		return false;
	}

	AGrimrockPartyPawn* PartyPawn = GridTD012Tests::SpawnPartyWithTwoCharacters(*this, TestWorld.World);
	if (!PartyPawn)
	{
		return false;
	}
	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;

	TestEqual(TEXT("Character 0 initially presents its equipped light"), PartyPawn->GetHeldItemDefinitionId(), GridTD012Tests::CharacterZeroLightItemId);
	TestTrue(TEXT("Character 0 initially owns the held light presentation"), PartyPawn->bHasTorchInHand);

	TestTrue(TEXT("Selection changes from character 0 to 1 through the authoritative setter"), Inventory->SetSelectedCharacterIndex(1));
	TestEqual(TEXT("Character 1 becomes selected"), Inventory->GetSelectedCharacterIndex(), 1);
	TestTrue(TEXT("Character 1 has no held visual after the selection notification"), PartyPawn->GetHeldItemDefinitionId().IsNone());
	TestFalse(TEXT("Character 1 has no held light presentation"), PartyPawn->bHasTorchInHand);

	TestTrue(TEXT("Selection changes from character 1 back to 0 through the authoritative setter"), Inventory->SetSelectedCharacterIndex(0));
	TestEqual(TEXT("Character 0 becomes selected again"), Inventory->GetSelectedCharacterIndex(), 0);
	TestEqual(TEXT("Character 0 held visual is restored automatically"), PartyPawn->GetHeldItemDefinitionId(), GridTD012Tests::CharacterZeroLightItemId);
	TestTrue(TEXT("Character 0 held light presentation is restored automatically"), PartyPawn->bHasTorchInHand);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD012SelectedCharacterFilterTest,
	"Grimrock.TechnicalDebt.TD01_2.PartySelectionHeldVisual.SelectedCharacterFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD012SelectedCharacterFilterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	GridTD012Tests::FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("TD01.2 could not create a transient game world."));
		return false;
	}

	AGrimrockPartyPawn* PartyPawn = GridTD012Tests::SpawnPartyWithTwoCharacters(*this, TestWorld.World);
	if (!PartyPawn)
	{
		return false;
	}
	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
	TestEqual(TEXT("Selected character 0 starts with its held light"), PartyPawn->GetHeldItemDefinitionId(), GridTD012Tests::CharacterZeroLightItemId);

	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = FGridItemInstance();
	Inventory->NotifyPartyInventoryChanged(1);
	TestEqual(TEXT("An unrelated character notification does not resynchronize the selected held visual"), PartyPawn->GetHeldItemDefinitionId(),
		GridTD012Tests::CharacterZeroLightItemId);
	TestTrue(TEXT("Unrelated character notification preserves current presentation state"), PartyPawn->bHasTorchInHand);

	Inventory->NotifyPartyInventoryChanged(0);
	TestTrue(TEXT("The selected character notification resynchronizes the held visual"), PartyPawn->GetHeldItemDefinitionId().IsNone());
	TestFalse(TEXT("The selected character notification clears stale held-light state"), PartyPawn->bHasTorchInHand);

	Inventory->PartyInventoryState.ActiveEquipment[0].MainHand =
		GridTD012Tests::MakeCharacterZeroLightItem(Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId);
	Inventory->NotifyPartyInventoryChanged(INDEX_NONE);
	TestEqual(TEXT("Party-wide selection or registry notification resynchronizes the held visual"), PartyPawn->GetHeldItemDefinitionId(),
		GridTD012Tests::CharacterZeroLightItemId);
	TestTrue(TEXT("Party-wide notification restores the selected held-light state"), PartyPawn->bHasTorchInHand);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
