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
	Definition->DefaultBehavior.Receptacle.bAcceptAnyItem = false;
	Definition->DefaultBehavior.Receptacle.MaxContainedItems = 4;
	Definition->DefaultBehavior.PressurePlateWeight.RequiredItemWeight = 3.5f;
	Definition->DefaultBehavior.PressurePlateWeight.bUseItemWeight = true;
	Definition->DefaultBehavior.DoorAnimation.bHasChainMechanism = true;
	Definition->DefaultBehavior.DoorAnimation.ChainPullDistance = 31.0f;

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
	ObjectData.Behavior.Receptacle.bAcceptAnyItem = true;
	ObjectData.Behavior.Receptacle.MaxContainedItems = 99;
	ObjectData.Behavior.PressurePlateWeight.RequiredItemWeight = 99.0f;
	ObjectData.Behavior.PressurePlateWeight.bUseItemWeight = false;
	ObjectData.Behavior.DoorAnimation.bHasChainMechanism = false;
	ObjectData.Behavior.DoorAnimation.ChainPullDistance = 99.0f;

	const FGridObjectBehaviorParams Resolved = GridObjectInstanceBehavior::Resolve(ObjectData, Definition);
	TestEqual(TEXT("Definition owns button hold time"), Resolved.ButtonAnimation.ButtonHoldTime, 0.80f);
	TestTrue(TEXT("Definition owns lock consume-key rule"), Resolved.Lock.bConsumeKeyOnUnlock);
	TestFalse(TEXT("Definition owns receptacle accept-any rule"), Resolved.Receptacle.bAcceptAnyItem);
	TestEqual(TEXT("Definition owns receptacle capacity"), Resolved.Receptacle.MaxContainedItems, 4);
	TestTrue(TEXT("Definition owns pressure-plate weight mode"), Resolved.PressurePlateWeight.bUseItemWeight);
	TestEqual(TEXT("Definition owns pressure-plate weight threshold"), Resolved.PressurePlateWeight.RequiredItemWeight, 3.5f);
	TestTrue(TEXT("Definition owns door chain presence"), Resolved.DoorAnimation.bHasChainMechanism);
	TestEqual(TEXT("Definition owns door chain pull distance"), Resolved.DoorAnimation.ChainPullDistance, 31.0f);

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
	FGridWorldObjectMIG09DefinitionAuthorityCutoverTest,
	"Grimrock.WorldObjects.MIG09.DefinitionAuthorityCutover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG09DefinitionAuthorityCutoverTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridObjectArchetypeAsset* Definition = NewObject<UGridObjectArchetypeAsset>();
	Definition->ArchetypeId = TEXT("MIG09_Definition");
	Definition->SupportedType = EGridLevelObjectType::Button;
	Definition->DefaultBehavior.ButtonAnimation.ButtonHoldTime = 0.90f;

	FGridLevelObjectData CompatibilityObject;
	CompatibilityObject.ObjectId = FGuid::NewGuid();
	CompatibilityObject.ArchetypeId = Definition->ArchetypeId;
	CompatibilityObject.Behavior.ButtonAnimation.ButtonHoldTime = 0.25f;

	const FGridObjectBehaviorParams DefinitionResolved = GridObjectInstanceBehavior::Resolve(CompatibilityObject, Definition);
	TestEqual(TEXT("MIG09 always resolves shared behavior from the definition when one is available"),
		DefinitionResolved.ButtonAnimation.ButtonHoldTime, 0.90f);

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	const FGridObjectBehaviorParams LevelResolved = GridObjectInstanceBehavior::Resolve(Level, CompatibilityObject, Definition);
	TestEqual(TEXT("Level migration markers no longer affect definition authority"),
		LevelResolved.ButtonAnimation.ButtonHoldTime, 0.90f);

	const FGridObjectBehaviorParams DirectCallerFallback = GridObjectInstanceBehavior::Resolve(CompatibilityObject, nullptr);
	TestEqual(TEXT("Direct callers without a definition keep temporary compatibility until their initializers are purged"),
		DirectCallerFallback.ButtonAnimation.ButtonHoldTime, 0.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG06SparseStorageContractTest,
	"Grimrock.WorldObjects.MIG06.SparseStorageContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG06SparseStorageContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridItemDefinitionAsset* InitialItem = NewObject<UGridItemDefinitionAsset>();
	InitialItem->ItemDefinitionId = TEXT("MIG06_StorageInitialItem");

	FGridObjectBehaviorParams Source;
	Source.ButtonAnimation.ButtonHoldTime = 4.0f;
	Source.Lock.bConsumeKeyOnUnlock = true;
	Source.Lock.bStartsUnlocked = true;
	Source.Teleporter.TargetCellX = 12;
	Source.PressurePlateWeight.RequiredItemWeight = 6.0f;
	Source.PressurePlateWeight.bUseItemWeight = true;
	Source.DoorAnimation.bHasChainMechanism = true;
	Source.Receptacle.bAcceptAnyItem = false;
	Source.Receptacle.MaxContainedItems = 3;
	FGridReceptacleInitialItemConfig& InitialContent = Source.Receptacle.InitialContent.AddDefaulted_GetRef();
	InitialContent.ItemDefinition = InitialItem;
	InitialContent.Quantity = 2;

	const FGridObjectBehaviorParams Sparse = GridObjectInstanceBehavior::BuildSparseOverrides(Source);
	TestTrue(TEXT("Button definition behavior is not cloned into sparse storage"),
		!FMath::IsNearlyEqual(Sparse.ButtonAnimation.ButtonHoldTime, Source.ButtonAnimation.ButtonHoldTime));
	TestFalse(TEXT("Lock consume-key rule is not cloned into sparse storage"), Sparse.Lock.bConsumeKeyOnUnlock);
	TestTrue(TEXT("Lock initial state is stored as an instance override"), Sparse.Lock.bStartsUnlocked);
	TestEqual(TEXT("Teleporter destination is stored as an instance override"), Sparse.Teleporter.TargetCellX, 12);
	TestFalse(TEXT("Pressure-plate weight mode is not cloned into sparse storage"), Sparse.PressurePlateWeight.bUseItemWeight);
	TestTrue(TEXT("Pressure-plate threshold is not cloned into sparse storage"),
		!FMath::IsNearlyEqual(Sparse.PressurePlateWeight.RequiredItemWeight, Source.PressurePlateWeight.RequiredItemWeight));
	TestFalse(TEXT("Door chain rules are not cloned into sparse storage"), Sparse.DoorAnimation.bHasChainMechanism);
	TestTrue(TEXT("Receptacle accept-any rule is not cloned into sparse storage"), Sparse.Receptacle.bAcceptAnyItem);
	TestTrue(TEXT("Receptacle capacity is not cloned into sparse storage"), Sparse.Receptacle.MaxContainedItems != Source.Receptacle.MaxContainedItems);
	TestEqual(TEXT("Receptacle initial content remains the sparse instance payload"), Sparse.Receptacle.InitialContent.Num(), 1);
	if (Sparse.Receptacle.InitialContent.Num() == 1)
	{
		TestTrue(TEXT("Sparse initial content keeps its definition"), Sparse.Receptacle.InitialContent[0].ItemDefinition == InitialItem);
		TestEqual(TEXT("Sparse initial content keeps quantity"), Sparse.Receptacle.InitialContent[0].Quantity, 2);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
