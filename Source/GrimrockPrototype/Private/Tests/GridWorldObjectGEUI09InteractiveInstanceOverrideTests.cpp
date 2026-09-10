#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridRuntimeWorldObjectData.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectGEUI09InteractiveInstanceOverridesTest,
	"Grimrock.WorldObjects.GEUI09.InteractiveInstanceOverrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectGEUI09InteractiveInstanceOverridesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* ConfigStruct = FGridWorldObjectInstanceConfig::StaticStruct();
	UScriptStruct* InteractionStruct = FGridWorldObjectInteractionOverrides::StaticStruct();
	if (!TestNotNull(TEXT("InstanceConfig exists"), ConfigStruct) ||
		!TestNotNull(TEXT("InteractionOverrides exists"), InteractionStruct))
	{
		return false;
	}

	TestNotNull(TEXT("InstanceConfig exposes sparse InteractionOverrides"),
		ConfigStruct->FindPropertyByName(TEXT("InteractionOverrides")));
	TestNotNull(TEXT("Interaction overrides expose button hold flag"),
		InteractionStruct->FindPropertyByName(TEXT("bOverrideButtonHoldTime")));
	TestNotNull(TEXT("Interaction overrides expose pressure-plate flag"),
		InteractionStruct->FindPropertyByName(TEXT("bOverridePressurePlateWeight")));
	TestNotNull(TEXT("Interaction overrides expose receptacle flag"),
		InteractionStruct->FindPropertyByName(TEXT("bOverrideReceptacleRules")));
	TestNotNull(TEXT("Interaction overrides expose accepted-key flag"),
		InteractionStruct->FindPropertyByName(TEXT("bOverrideAcceptedKeys")));

	// The instance still does not own a second complete Behavior.
	TestNull(TEXT("InstanceConfig does not duplicate DefaultBehavior"),
		ConfigStruct->FindPropertyByName(TEXT("DefaultBehavior")));
	TestNull(TEXT("InstanceConfig does not serialize a full ButtonAnimation block"),
		ConfigStruct->FindPropertyByName(TEXT("ButtonAnimation")));
	TestNull(TEXT("InstanceConfig does not serialize a full Receptacle block"),
		ConfigStruct->FindPropertyByName(TEXT("Receptacle")));

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
	Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime = 0.80f;
	Definition->DefaultBehavior.PressurePlateWeight.bActivateWhenPartyPresent = true;
	Definition->DefaultBehavior.PressurePlateWeight.bUseItemWeight = false;
	Definition->DefaultBehavior.PressurePlateWeight.RequiredItemWeight = 1.0f;
	Definition->DefaultBehavior.PressurePlateWeight.bCountEdgeItems = false;
	Definition->DefaultBehavior.Receptacle.bAcceptAnyItem = true;
	Definition->DefaultBehavior.Receptacle.MaxContainedItems = 1;
	Definition->DefaultBehavior.Receptacle.VisualPlacementMode = EGridReceptacleVisualPlacementMode::PhysicalAtHit;
	Definition->DefaultBehavior.Lock.bConsumeKeyOnUnlock = true;
	Definition->DefaultBehavior.Lock.AcceptedKeyIds = { TEXT("Key_Copper") };
	Definition->DefaultBehavior.DoorAnimation.bHasChainMechanism = false;
	Definition->DefaultBehavior.DoorAnimation.ChainPullDistance = 20.0f;
	Definition->DefaultBehavior.DoorAnimation.ChainPullDuration = 0.25f;

	FGridWorldObjectInstance Instance;
	Instance.InstanceId = FGuid::NewGuid();
	Instance.Type = EGridLevelObjectType::PressurePlate;
	Instance.InstanceConfig.bStartsUnlocked = true;

	// No override -> shared Definition rules survive.
	FGridObjectBehaviorParams Resolved = GridObjectInstanceBehavior::Resolve(Instance, Definition);
	TestEqual(TEXT("Button hold inherits Definition"), Resolved.ButtonAnimation.ButtonHoldTime, 0.80f);
	TestTrue(TEXT("Plate party rule inherits Definition"), Resolved.PressurePlateWeight.bActivateWhenPartyPresent);
	TestFalse(TEXT("Plate item-weight rule inherits Definition"), Resolved.PressurePlateWeight.bUseItemWeight);
	TestTrue(TEXT("Receptacle accept-any inherits Definition"), Resolved.Receptacle.bAcceptAnyItem);
	TestEqual(TEXT("Receptacle capacity inherits Definition"), Resolved.Receptacle.MaxContainedItems, 1);
	TestTrue(TEXT("Lock starts-unlocked remains instance-owned"), Resolved.Lock.bStartsUnlocked);

	// Button.
	Instance.InstanceConfig.InteractionOverrides.bOverrideButtonHoldTime = true;
	Instance.InstanceConfig.InteractionOverrides.ButtonHoldTime = 0.30f;

	// Pressure plate.
	Instance.InstanceConfig.InteractionOverrides.bOverridePressurePlateWeight = true;
	auto& Plate = Instance.InstanceConfig.InteractionOverrides.PressurePlateWeight;
	Plate.bActivateWhenPartyPresent = false;
	Plate.bUseItemWeight = true;
	Plate.RequiredItemWeight = 5.0f;
	Plate.bCountEdgeItems = true;

	// Receptacle puzzle rules. Presentation stays on Definition.
	UGridItemDefinitionAsset* Gem = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	Gem->ItemDefinitionId = TEXT("Gem_Blue");
	FGridReceptacleAcceptedItemConfig AcceptedGem;
	AcceptedGem.ItemDefinition = Gem;
	Instance.InstanceConfig.InteractionOverrides.bOverrideReceptacleRules = true;
	auto& Receptacle = Instance.InstanceConfig.InteractionOverrides.ReceptacleRules;
	Receptacle.bAcceptAnyItem = false;
	Receptacle.AcceptedItems = { AcceptedGem };
	Receptacle.MaxContainedItems = 3;

	// Wall-lock accepted keys only. Consume-key remains Definition-owned because current
	// wall-lock runtime does not expose that flag as an instance-selectable rule.
	UGridItemDefinitionAsset* IronKey = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	IronKey->ItemDefinitionId = TEXT("Key_Iron");
	Instance.InstanceConfig.InteractionOverrides.bOverrideAcceptedKeys = true;
	Instance.InstanceConfig.InteractionOverrides.AcceptedKeys.AcceptedKeyItems = { IronKey };
	Instance.InstanceConfig.InteractionOverrides.AcceptedKeys.AcceptedKeyIds = { TEXT("Key_Iron_Legacy") };

	// Existing door-chain sparse overrides remain independent.
	Instance.InstanceConfig.DoorChainMode = EGridDoorChainMode::Enabled;
	Instance.InstanceConfig.bOverrideChainPullDuration = true;
	Instance.InstanceConfig.ChainPullDuration = 0.60f;

	Resolved = GridObjectInstanceBehavior::Resolve(Instance, Definition);
	TestEqual(TEXT("Button hold resolves instance override"), Resolved.ButtonAnimation.ButtonHoldTime, 0.30f);
	TestFalse(TEXT("Plate can ignore party"), Resolved.PressurePlateWeight.bActivateWhenPartyPresent);
	TestTrue(TEXT("Plate can use item weight"), Resolved.PressurePlateWeight.bUseItemWeight);
	TestEqual(TEXT("Plate required weight resolves instance override"), Resolved.PressurePlateWeight.RequiredItemWeight, 5.0f);
	TestTrue(TEXT("Plate can count edge items"), Resolved.PressurePlateWeight.bCountEdgeItems);

	TestFalse(TEXT("Receptacle accept-any resolves instance override"), Resolved.Receptacle.bAcceptAnyItem);
	TestEqual(TEXT("Receptacle accepted-item count resolves instance override"), Resolved.Receptacle.AcceptedItems.Num(), 1);
	TestEqual(TEXT("Receptacle capacity resolves instance override"), Resolved.Receptacle.MaxContainedItems, 3);
	TestTrue(TEXT("Receptacle visual placement remains Definition-owned"),
		Resolved.Receptacle.VisualPlacementMode == EGridReceptacleVisualPlacementMode::PhysicalAtHit);

	TestEqual(TEXT("Lock accepted asset resolves instance override"), Resolved.Lock.AcceptedKeyItems.Num(), 1);
	TestTrue(TEXT("Lock accepted id resolves instance override"),
		Resolved.Lock.AcceptedKeyIds.Contains(TEXT("Key_Iron_Legacy")));
	TestTrue(TEXT("Lock consume-key remains Definition-owned"), Resolved.Lock.bConsumeKeyOnUnlock);

	TestTrue(TEXT("Door chain can be enabled per instance"), Resolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Door chain distance remains Definition-owned"), Resolved.DoorAnimation.ChainPullDistance, 20.0f);
	TestEqual(TEXT("Door chain duration resolves instance override"), Resolved.DoorAnimation.ChainPullDuration, 0.60f);

	// Runtime payload must preserve the same sparse layer.
	const FGridRuntimeWorldObjectData RuntimeData(Instance);
	TestTrue(TEXT("Runtime payload carries interaction overrides"),
		RuntimeData.InteractionOverrides.bOverridePressurePlateWeight);
	const FGridObjectBehaviorParams RuntimeResolved = GridObjectInstanceBehavior::Resolve(RuntimeData, Definition);
	TestEqual(TEXT("Runtime payload preserves plate required weight"),
		RuntimeResolved.PressurePlateWeight.RequiredItemWeight, 5.0f);
	TestEqual(TEXT("Runtime payload preserves button hold time"),
		RuntimeResolved.ButtonAnimation.ButtonHoldTime, 0.30f);
	TestEqual(TEXT("Runtime payload preserves receptacle capacity"),
		RuntimeResolved.Receptacle.MaxContainedItems, 3);
	TestTrue(TEXT("Runtime payload preserves lock accepted key"),
		RuntimeResolved.Lock.AcceptedKeyIds.Contains(TEXT("Key_Iron_Legacy")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
