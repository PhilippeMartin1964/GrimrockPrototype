#if WITH_DEV_AUTOMATION_TESTS

#include "Core/GridLevelAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Save/GrimrockPartySaveGame.h"

namespace
{
	FGridLevelVariableDefinition MakeBoolVariable(FName VariableId, bool bDefaultValue)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = VariableId;
		Definition.Type = EGridLevelVariableType::Bool;
		Definition.bDefaultBoolValue = bDefaultValue;
		return Definition;
	}

	FGridLevelVariableDefinition MakeIntVariable(FName VariableId, int32 DefaultValue)
	{
		FGridLevelVariableDefinition Definition;
		Definition.VariableId = VariableId;
		Definition.Type = EGridLevelVariableType::Int32;
		Definition.DefaultInt32Value = DefaultValue;
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON192LevelVariableDefaultsTest, "Grimrock.MON19.2.Runtime.LevelVariables.DefaultInitialization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON192LevelVariableDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	Level->LevelVariables = { MakeBoolVariable(TEXT("CryptDoorUnlocked"), false), MakeIntVariable(TEXT("RuneCount"), 3) };

	FGridLevelRuntimeState State;
	FString Error;
	TestTrue(TEXT("Valid declarations initialize"), GridLevelVariableStore::EnsureInitialized(*Level, State, Error));
	TestTrue(TEXT("Runtime snapshot is marked initialized"), State.bLevelVariablesInitialized);
	TestEqual(TEXT("One Bool default is materialized"), State.BoolVariables.Num(), 1);
	TestEqual(TEXT("One Int32 default is materialized"), State.IntVariables.Num(), 1);
	TestFalse(TEXT("Bool default is respected"), State.BoolVariables.FindRef(TEXT("CryptDoorUnlocked")));
	TestEqual(TEXT("Int32 default is respected"), State.IntVariables.FindRef(TEXT("RuneCount")), 3);

	bool bBoolValue = true;
	int32 IntValue = 0;
	TestTrue(TEXT("Declared Bool can be read"), GridLevelVariableStore::TryGetBool(*Level, State, TEXT("CryptDoorUnlocked"), bBoolValue, Error));
	TestFalse(TEXT("Typed Bool read returns current value"), bBoolValue);
	TestTrue(TEXT("Declared Int32 can be read"), GridLevelVariableStore::TryGetInt32(*Level, State, TEXT("RuneCount"), IntValue, Error));
	TestEqual(TEXT("Typed Int32 read returns current value"), IntValue, 3);

	TestFalse(TEXT("Wrong-type read is rejected"), GridLevelVariableStore::TryGetInt32(*Level, State, TEXT("CryptDoorUnlocked"), IntValue, Error));
	TestFalse(TEXT("Undeclared write is rejected"), GridLevelVariableStore::SetBool(*Level, State, TEXT("UnknownVariable"), true, Error));

	UGridLevelAsset* InvalidLevel = NewObject<UGridLevelAsset>();
	InvalidLevel->LevelVariables = { MakeBoolVariable(TEXT("Duplicate"), false), MakeIntVariable(TEXT("Duplicate"), 1) };
	FGridLevelRuntimeState InvalidState;
	TestFalse(TEXT("VariableId is unique across all supported types"), GridLevelVariableStore::EnsureInitialized(*InvalidLevel, InvalidState, Error));
	TestFalse(TEXT("Failed initialization is atomic"), InvalidState.bLevelVariablesInitialized);
	TestTrue(TEXT("Failed initialization leaves Bool state empty"), InvalidState.BoolVariables.IsEmpty());
	TestTrue(TEXT("Failed initialization leaves Int32 state empty"), InvalidState.IntVariables.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON192LevelVariableReconcileTest, "Grimrock.MON19.2.Runtime.LevelVariables.ReconcilePersistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON192LevelVariableReconcileTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridLevelAsset* Level = NewObject<UGridLevelAsset>();
	Level->LevelVariables = { MakeBoolVariable(TEXT("GateOpen"), false), MakeIntVariable(TEXT("Counter"), 2) };

	FGridLevelRuntimeState State;
	FString Error;
	TestTrue(TEXT("Initial store initializes"), GridLevelVariableStore::EnsureInitialized(*Level, State, Error));
	TestTrue(TEXT("Bool mutation succeeds"), GridLevelVariableStore::SetBool(*Level, State, TEXT("GateOpen"), true, Error));
	TestTrue(TEXT("Int32 mutation succeeds"), GridLevelVariableStore::SetInt32(*Level, State, TEXT("Counter"), 9, Error));

	Level->LevelVariables.Add(MakeBoolVariable(TEXT("NewPuzzleFlag"), true));
	TestTrue(TEXT("Adding a declaration reconciles an initialized snapshot"), GridLevelVariableStore::EnsureInitialized(*Level, State, Error));
	TestTrue(TEXT("Existing Bool value survives reconciliation"), State.BoolVariables.FindRef(TEXT("GateOpen")));
	TestEqual(TEXT("Existing Int32 value survives reconciliation"), State.IntVariables.FindRef(TEXT("Counter")), 9);
	TestTrue(TEXT("New declaration receives its current default"), State.BoolVariables.FindRef(TEXT("NewPuzzleFlag")));

	// Simulate a level-authoring change from Int32 to Bool. A type change must
	// not reinterpret the old integer value; it receives the new type default.
	Level->LevelVariables[1] = MakeBoolVariable(TEXT("Counter"), false);
	TestTrue(TEXT("Type change reconciles"), GridLevelVariableStore::EnsureInitialized(*Level, State, Error));
	TestFalse(TEXT("Type-changed variable uses Bool default"), State.BoolVariables.FindRef(TEXT("Counter")));
	TestFalse(TEXT("Stale Int32 representation is removed"), State.IntVariables.Contains(TEXT("Counter")));

	Level->LevelVariables.RemoveAt(0);
	TestTrue(TEXT("Removing a declaration reconciles"), GridLevelVariableStore::EnsureInitialized(*Level, State, Error));
	TestFalse(TEXT("Undeclared stale value is removed"), State.BoolVariables.Contains(TEXT("GateOpen")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON192LevelVariableSnapshotValidationTest, "Grimrock.MON19.2.Save.LevelVariableSnapshotValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON192LevelVariableSnapshotValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FString Error;
	FGridDungeonRuntimeState Dungeon;
	FGridLevelRuntimeState& State = Dungeon.LevelStates.Add(TEXT("Level_A"));
	State.LevelId = TEXT("Level_A");

	TestTrue(TEXT("Empty legacy-style snapshot is structurally valid"), GridLevelVariableStore::ValidateDungeonSnapshots(Dungeon, Error));

	State.BoolVariables.Add(TEXT("Flag"), true);
	TestFalse(TEXT("Values without initialization marker are rejected"), GridLevelVariableStore::ValidateDungeonSnapshots(Dungeon, Error));

	State.bLevelVariablesInitialized = true;
	TestTrue(TEXT("Initialized Bool snapshot is valid"), GridLevelVariableStore::ValidateDungeonSnapshots(Dungeon, Error));

	State.IntVariables.Add(TEXT("Flag"), 1);
	TestFalse(TEXT("Same VariableId cannot be stored as two types"), GridLevelVariableStore::ValidateDungeonSnapshots(Dungeon, Error));

	State.IntVariables.Reset();
	State.BoolVariables.Reset();
	State.IntVariables.Add(NAME_None, 1);
	TestFalse(TEXT("NAME_None runtime VariableId is rejected"), GridLevelVariableStore::ValidateDungeonSnapshots(Dungeon, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMON192LevelVariableSaveRoundTripTest, "Grimrock.MON19.2.Save.LevelVariablesRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMON192LevelVariableSaveRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>();
	FGridLevelRuntimeState& SourceState = Source->DungeonRuntimeState.LevelStates.Add(TEXT("PuzzleLevel"));
	SourceState.LevelId = TEXT("PuzzleLevel");
	SourceState.bLevelVariablesInitialized = true;
	SourceState.BoolVariables.Add(TEXT("Solved"), true);
	SourceState.IntVariables.Add(TEXT("RuneCount"), 4);

	TArray<uint8> SaveBytes;
	TestTrue(TEXT("Current-schema variable snapshot serializes to memory"), UGameplayStatics::SaveGameToMemory(Source, SaveBytes));

	UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
	TestNotNull(TEXT("Variable snapshot deserializes from memory"), Loaded);
	if (!Loaded)
	{
		return false;
	}

	TestEqual(TEXT("Round trip uses current SaveVersion"), Loaded->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	const FGridLevelRuntimeState* LoadedState = Loaded->DungeonRuntimeState.LevelStates.Find(TEXT("PuzzleLevel"));
	TestNotNull(TEXT("Level variable snapshot survives"), LoadedState);
	if (!LoadedState)
	{
		return false;
	}
	TestTrue(TEXT("Loaded variable snapshot stays initialized"), LoadedState->bLevelVariablesInitialized);
	TestTrue(TEXT("Bool value survives SaveGame round trip"), LoadedState->BoolVariables.FindRef(TEXT("Solved")));
	TestEqual(TEXT("Int32 value survives SaveGame round trip"), LoadedState->IntVariables.FindRef(TEXT("RuneCount")), 4);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
