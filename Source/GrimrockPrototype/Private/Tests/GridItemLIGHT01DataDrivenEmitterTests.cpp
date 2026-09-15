#if WITH_DEV_AUTOMATION_TESTS

#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridItemLIGHT01Tests
{
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
				FName(*FString::Printf(TEXT("ITEM_LIGHT01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemLIGHT01DataDrivenEmitterTest, "Grimrock.Items.LIGHT01.DataDrivenEmitter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemLIGHT01DataDrivenEmitterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("ITEM_LIGHT01_TestLamp");
	TestFalse(TEXT("A default item definition has no light emitter"), Definition->HasLightEmitter());
	TestFalse(TEXT("A default item definition is not lit by default"), Definition->IsLightEnabledByDefault());

	FGridLightEmitterConfig& LightConfig = Definition->LightEmitter;
	LightConfig.bDefaultEnabled = true;
	LightConfig.bUsePointLight = true;
	LightConfig.PointLightRelativeLocation = FVector(3.0f, 4.0f, 5.0f);
	LightConfig.PointLightRelativeRotation = FRotator(1.0f, 2.0f, 3.0f);
	LightConfig.LightIntensity = 777.0f;
	LightConfig.LightRadius = 444.0f;
	LightConfig.bEnableLightFlicker = false;
	LightConfig.bEnableLightPositionFlicker = false;
	LightConfig.bEnableLightColorFlicker = false;

	TestTrue(TEXT("A point-light-only definition owns a valid emitter"), Definition->HasLightEmitter());
	TestTrue(TEXT("The definition exposes its authored default enabled state"), Definition->IsLightEnabledByDefault());
	TestTrue(TEXT("The light emitter configuration is valid"), LightConfig.IsValid());
	TestTrue(TEXT("The item definition remains valid with a data-driven light emitter"), Definition->IsValidDefinition());

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	TestTrue(TEXT("The light definition registers in the item registry"), Inventory->RegisterItemDefinition(Definition));
	FGridItemInstance ItemInstance;
	ItemInstance.RuntimeObjectId = FGuid::NewGuid();
	ItemInstance.ItemDefinitionId = Definition->ItemDefinitionId;
	ItemInstance.Quantity = 1;
	TestTrue(TEXT("Applying the item definition succeeds"), Inventory->ApplyItemDefinitionToInstance(ItemInstance));
	TestTrue(TEXT("The item instance receives the authored default light state"), ItemInstance.bLightsEnabled);

	GridItemLIGHT01Tests::FTestWorld TestWorld;
	if (!TestWorld.World)
	{
		AddError(TEXT("ITEM-LIGHT01 could not create a transient game world."));
		return false;
	}

	AGridItemActor* ItemActor = TestWorld.World->SpawnActor<AGridItemActor>();
	TestNotNull(TEXT("The generic item actor is spawned"), ItemActor);
	if (!ItemActor || !ItemActor->LightEmitterComponent)
	{
		return false;
	}

	ItemActor->InitializeFromItemDefinition(Definition, ItemInstance.RuntimeObjectId);
	TestTrue(TEXT("The generic item actor receives the definition emitter config"), ItemActor->LightEmitterComponent->HasConfiguredEmitter());
	TestFalse(TEXT("Actor initialization does not bypass the instance-owned light state"), ItemActor->AreItemLightsEnabled());
	TestFalse(TEXT("A disabled emitter does not tick"), ItemActor->LightEmitterComponent->IsComponentTickEnabled());

	ItemActor->SetItemLightsEnabled(ItemInstance.bLightsEnabled);
	TestTrue(TEXT("The item instance can enable the generic emitter"), ItemActor->AreItemLightsEnabled());
	TestFalse(TEXT("A steady point light does not waste a runtime tick"), ItemActor->LightEmitterComponent->IsComponentTickEnabled());

	UPointLightComponent* PointLight = ItemActor->FindComponentByClass<UPointLightComponent>();
	TestNotNull(TEXT("Enabling the generic emitter creates its point light"), PointLight);
	if (PointLight)
	{
		TestTrue(TEXT("The point light is visible while enabled"), PointLight->IsVisible());
		TestTrue(TEXT("The point light uses the authored relative location"),
			PointLight->GetRelativeLocation().Equals(LightConfig.PointLightRelativeLocation, KINDA_SMALL_NUMBER));
		TestTrue(TEXT("The point light uses the authored relative rotation"),
			PointLight->GetRelativeRotation().Equals(LightConfig.PointLightRelativeRotation, KINDA_SMALL_NUMBER));
		TestTrue(TEXT("The point light uses the authored intensity"), FMath::IsNearlyEqual(PointLight->Intensity, LightConfig.LightIntensity));
		TestTrue(TEXT("The point light uses the authored radius"), FMath::IsNearlyEqual(PointLight->AttenuationRadius, LightConfig.LightRadius));
	}

	FGridLightEmitterConfig FlickerConfig = LightConfig;
	FlickerConfig.bEnableLightFlicker = true;
	ItemActor->LightEmitterComponent->ApplyConfig(FlickerConfig);
	TestTrue(TEXT("An enabled flickering emitter ticks"), ItemActor->LightEmitterComponent->IsComponentTickEnabled());
	TestTrue(TEXT("Applying a new config preserves the enabled runtime state"), ItemActor->AreItemLightsEnabled());

	ItemActor->SetItemLightsEnabled(false);
	TestFalse(TEXT("Disabling the item disables the generic emitter"), ItemActor->AreItemLightsEnabled());
	TestFalse(TEXT("A disabled emitter stops ticking"), ItemActor->LightEmitterComponent->IsComponentTickEnabled());
	if (PointLight)
	{
		TestFalse(TEXT("The point light is hidden while disabled"), PointLight->IsVisible());
	}

	FGridLightEmitterConfig EmptyConfig;
	ItemActor->LightEmitterComponent->ApplyConfig(EmptyConfig);
	ItemActor->SetItemLightsEnabled(true);
	TestFalse(TEXT("An item with no configured emitter cannot be force-enabled"), ItemActor->AreItemLightsEnabled());
	TestFalse(TEXT("An unconfigured emitter never ticks"), ItemActor->LightEmitterComponent->IsComponentTickEnabled());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
