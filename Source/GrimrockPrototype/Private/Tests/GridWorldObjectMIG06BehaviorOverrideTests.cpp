#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectInstanceBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG06SparseBehaviorResolutionTest,
	"Grimrock.WorldObjects.MIG06.SparseBehaviorResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG06SparseBehaviorResolutionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridObjectArchetypeAsset* Definition = NewObject<UGridObjectArchetypeAsset>();
	Definition->ArchetypeId = TEXT("MIG06_Definition");
	Definition->SupportedType = EGridLevelObjectType::Button;
	Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime = 0.80f;
	Definition->DefaultBehavior.Lock.bConsumeKeyOnUnlock = true;
	Definition->DefaultBehavior.Lock.bStartsUnlocked = false;
	Definition->DefaultBehavior.Receptacle.MaxContainedItems = 4;
	Definition->DefaultBehavior.PressurePlateWeight.RequiredItemWeight = 3.5f;

	UGridItemDefinitionAsset* InitialItem = NewObject<UGridItemDefinitionAsset>();
	InitialItem->ItemDefinitionId = TEXT("MIG06_InitialItem");

	FGridObjectBehaviorParams Staged = Definition->DefaultBehavior;
	Staged.Teleporter.TargetCellX = 7;
	Staged.Teleporter.TargetCellY = 8;
	Staged.Transition.bIsTransition = true;
	Staged.Transition.TargetLevelId = TEXT("LowerFloor");
	Staged.Pit.bInitiallyOpen = false;
	Staged.Lock.bStartsUnlocked = true;
	FGridReceptacleInitialItemConfig& InitialContent = Staged.Receptacle.InitialContent.AddDefaulted_GetRef();
	InitialContent.ItemDefinition = InitialItem;
	InitialContent.Quantity = 2;

	FGridLevelObjectData ObjectData;
	ObjectData.ObjectId = FGuid::NewGuid();
	ObjectData.Type = EGridLevelObjectType::Button;
	ObjectData.ArchetypeId = Definition->ArchetypeId;
	ObjectData.Behavior = GridObjectInstanceBehavior::BuildSparseOverrides(Staged);

	// Poison definition-owned fields in the stored sparse container. Resolve()
	// must ignore them and take the authoritative values from the definition.
	ObjectData.Behavior.ButtonAnimation.ButtonHoldTime = 9.0f;
	ObjectData.Behavior.Lock.bConsumeKeyOnUnlock = false;
	ObjectData.Behavior.Receptacle.MaxContainedItems = 99;
	ObjectData.Behavior.PressurePlateWeight.RequiredItemWeight = 99.0f;

	const FGridObjectBehaviorParams Resolved = GridObjectInstanceBehavior::Resolve(ObjectData, Definition, true);
	TestEqual(TEXT("Definition owns button hold time"), Resolved.ButtonAnimation.ButtonHoldTime, 0.80f);
	TestTrue(TEXT("Definition owns lock consume-key rule"), Resolved.Lock.bConsumeKeyOnUnlock);
	TestEqual(TEXT("Definition owns receptacle capacity"), Resolved.Receptacle.MaxContainedItems, 4);
	TestEqual(TEXT("Definition owns pressure-plate weight rule through resolver"), Resolved.PressurePlateWeight.RequiredItemWeight, 3.5f);

	TestEqual(TEXT("Teleporter destination remains instance-owned X"), Resolved.Teleporter.TargetCellX, 7);
	TestEqual(TEXT("Teleporter destination remains instance-owned Y"), Resolved.Teleporter.TargetCellY, 8);
	TestTrue(TEXT("Transition remains instance-owned"), Resolved.Transition.bIsTransition);
	TestEqual(TEXT("Transition target level remains instance-owned"), Resolved.Transition.TargetLevelId, FName(TEXT("LowerFloor")));
	TestFalse(TEXT("Pit initial open state remains instance-owned"), Resolved.Pit.bInitiallyOpen);
	TestTrue(TEXT("Lock initial unlocked state remains instance-owned"), Resolved.Lock.bStartsUnlocked);
	TestEqual(TEXT("Receptacle initial content remains instance-owned"), Resolved.Receptacle.InitialContent.Num(), 1);
	if (Resolved.Receptacle.InitialContent.Num() == 1)
	{
		TestTrue(TEXT("Initial content keeps direct ItemDefinition"), Resolved.Receptacle.InitialContent[0].ItemDefinition == InitialItem);
		TestEqual(TEXT("Initial content keeps quantity"), Resolved.Receptacle.InitialContent[0].Quantity, 2);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG06LegacyBehaviorBridgeTest,
	"Grimrock.WorldObjects.MIG06.LegacyBehaviorBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG06LegacyBehaviorBridgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridObjectArchetypeAsset* Definition = NewObject<UGridObjectArchetypeAsset>();
	Definition->ArchetypeId = TEXT("MIG06_LegacyDefinition");
	Definition->SupportedType = EGridLevelObjectType::Button;
	Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime = 0.90f;

	FGridLevelObjectData LegacyObject;
	LegacyObject.ObjectId = FGuid::NewGuid();
	LegacyObject.ArchetypeId = Definition->ArchetypeId;
	LegacyObject.Behavior.ButtonAnimation.ButtonHoldTime = 0.25f;

	const FGridObjectBehaviorParams LegacyResolved = GridObjectInstanceBehavior::Resolve(LegacyObject, Definition, false);
	TestEqual(TEXT("Pre-MIG06 object keeps its historical full Behavior snapshot"), LegacyResolved.ButtonAnimation.ButtonHoldTime, 0.25f);

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	TestFalse(TEXT("Unmarked object is legacy by default"), Level->UsesSparseBehaviorOverrides(LegacyObject.ObjectId));
	Level->SetSparseBehaviorOverrides(LegacyObject.ObjectId, true);
	TestTrue(TEXT("Level can mark one object as sparse"), Level->UsesSparseBehaviorOverrides(LegacyObject.ObjectId));
	const FGridObjectBehaviorParams SparseResolved = GridObjectInstanceBehavior::Resolve(Level, LegacyObject, Definition);
	TestEqual(TEXT("Marked sparse object resolves shared definition behavior"), SparseResolved.ButtonAnimation.ButtonHoldTime, 0.90f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG06SparseStorageContractTest,
	"Grimrock.WorldObjects.MIG06.SparseStorageContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG06SparseStorageContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridObjectBehaviorParams Source;
	Source.ButtonAnimation.ButtonHoldTime = 4.0f;
	Source.Lock.bConsumeKeyOnUnlock = true;
	Source.Lock.bStartsUnlocked = true;
	Source.Teleporter.TargetCellX = 12;
	Source.PressurePlateWeight.RequiredItemWeight = 6.0f;
	Source.DoorAnimation.bEnableChainMechanism = true;
	Source.Receptacle.MaxContainedItems = 3;

	const FGridObjectBehaviorParams Sparse = GridObjectInstanceBehavior::BuildSparseOverrides(Source);
	TestTrue(TEXT("Button definition behavior is not cloned into sparse storage"),
		!FMath::IsNearlyEqual(Sparse.ButtonAnimation.ButtonHoldTime, Source.ButtonAnimation.ButtonHoldTime));
	TestFalse(TEXT("Lock consume-key rule is not cloned into sparse storage"), Sparse.Lock.bConsumeKeyOnUnlock);
	TestTrue(TEXT("Lock initial state is stored as an instance override"), Sparse.Lock.bStartsUnlocked);
	TestEqual(TEXT("Teleporter destination is stored as an instance override"), Sparse.Teleporter.TargetCellX, 12);

	// Explicit MIG06-A bridges. These assertions make the remaining debt visible
	// and prevent it from being mistaken for the final MIG06 contract.
	TestEqual(TEXT("MIG06-A pressure-plate direct-consumer bridge remains"), Sparse.PressurePlateWeight.RequiredItemWeight, 6.0f);
	TestTrue(TEXT("MIG06-A door-chain direct-consumer bridge remains"), Sparse.DoorAnimation.bEnableChainMechanism);
	TestEqual(TEXT("MIG06-A receptacle validation bridge remains"), Sparse.Receptacle.MaxContainedItems, 3);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
