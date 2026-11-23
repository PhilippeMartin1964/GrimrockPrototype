#if WITH_DEV_AUTOMATION_TESTS

#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemActor.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridItemLIGHT01Tests
{

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemLIGHT01DataDrivenEmitterTest, "Grimrock.Items.LIGHT01.DataDrivenEmitter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemLIGHT01DataDrivenEmitterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FName LampId(TEXT("ITEM_LIGHT01_TestLamp"));
	FGridLightEmitterConfig LightConfig;
	TestFalse(TEXT("A default light configuration has no emitter"), LightConfig.HasEmitter());
	TestFalse(TEXT("A default light configuration is disabled"), LightConfig.bDefaultEnabled);

	LightConfig.bDefaultEnabled = true;
	LightConfig.bUsePointLight = true;
	LightConfig.PointLightRelativeLocation = FVector(3.0f, 4.0f, 5.0f);
	LightConfig.PointLightRelativeRotation = FRotator(1.0f, 2.0f, 3.0f);
	LightConfig.LightIntensity = 777.0f;
	LightConfig.LightRadius = 444.0f;
	LightConfig.bEnableLightFlicker = false;
	LightConfig.bEnableLightPositionFlicker = false;
	LightConfig.bEnableLightColorFlicker = false;

	TestTrue(TEXT("The light emitter configuration is valid"), LightConfig.IsValid());
	FGridCanonicalWeightTestWorld TestWorld({}, [&](UGridItemSystemAsset& Root)
	{
		auto* Lamp = NewObject<UGridNonArmamentDefinitionAsset>(&Root);
		Lamp->ItemId = LampId;
		Lamp->DisplayName = FText::FromName(LampId);
		Lamp->LightEmitter = LightConfig;
		Root.ItemDefinitions.Add(Lamp);
	});
	UGridPartyInventoryComponent* Inventory = TestWorld.MakeInventory();
	if (!TestNotNull(TEXT("Inventory with registered lamp"), Inventory)) return false;
	FGridResolvedItem Lamp;
	if (!TestTrue(TEXT("A registered point-light-only definition exposes its emitter"),
		TestWorld.Scope.Registry->ResolveItem(LampId, 0, Lamp) && Lamp.LightEmitter.HasEmitter())) return false;
	FGridItemInstance ItemInstance;
	ItemInstance.RuntimeObjectId = FGuid::NewGuid();
	ItemInstance.ItemDefinitionId = LampId;
	ItemInstance.Quantity = 1;
	ItemInstance.bLightsEnabled = Lamp.LightEmitter.bDefaultEnabled;
	TestTrue(TEXT("A fresh item instance receives the authored default light state"), ItemInstance.bLightsEnabled);
	TestTrue(TEXT("Applying the item definition metadata succeeds"), Inventory->ApplyItemDefinitionToInstance(ItemInstance));
	TestTrue(TEXT("Applying definition metadata preserves the instance-owned light state"), ItemInstance.bLightsEnabled);

	AGridItemActor* ItemActor = TestWorld.World->SpawnActor<AGridItemActor>();
	TestNotNull(TEXT("The generic item actor is spawned"), ItemActor);
	if (!ItemActor || !ItemActor->LightEmitterComponent)
	{
		return false;
	}

	if (!TestTrue(TEXT("Item actor initializes from registered instance"),
		ItemActor->InitializeFromItemInstance(ItemInstance, TestWorld.Scope.Registry))) return false;
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