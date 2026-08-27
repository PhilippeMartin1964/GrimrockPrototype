#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Save/GrimrockPartySaveGame.h"
#include "RPGMON155TestHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FMON167ProgressionGuard
	{
		FMON167ProgressionGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}

		~FMON167ProgressionGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}
	};

	UGridStatusEffectDefinitionAsset* MakeMON167Definition(
		FName EffectId, EGridStatusEffectDurationUnit DurationUnit = EGridStatusEffectDurationUnit::Rounds, int32 DefaultDuration = 3, int32 MaxStacks = 4)
	{
		UGridStatusEffectDefinitionAsset* Definition = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		Definition->EffectId = EffectId;
		Definition->DisplayName = FText::FromName(EffectId);
		Definition->DurationUnit = DurationUnit;
		Definition->DefaultDuration = DurationUnit == EGridStatusEffectDurationUnit::Permanent ? 0 : DefaultDuration;
		Definition->MaxStacks = MaxStacks;
		Definition->StackPolicy = EGridStatusEffectStackPolicy::AddStacks;
		Definition->DefaultPotency = 0;
		return Definition;
	}

	FGridStatusEffectRuntimeState MakeMON167RuntimeState(
		UGridStatusEffectDefinitionAsset* Definition, const FGuid& SourceId, int32 StackCount, int32 RemainingDuration, int32 Potency)
	{
		FGridStatusEffectRuntimeState State;
		FString Error;
		if (Definition)
		{
			Definition->BuildRuntimeState(SourceId, StackCount, RemainingDuration, Potency, State, Error);
		}
		return State;
	}

	FGridStatusEffectSaveState MakeMON167SaveState(
		FName EffectId, EGridStatusEffectDurationUnit DurationUnit, int32 StackCount, int32 RemainingDuration, int32 Potency)
	{
		FGridStatusEffectSaveState State;
		State.EffectId = EffectId;
		State.SourceId = FGuid(16, 7, StackCount, Potency + 1);
		State.StackCount = StackCount;
		State.DurationUnit = DurationUnit;
		State.RemainingDuration = RemainingDuration;
		State.Potency = Potency;
		return State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON167CollectionCaptureTest, "Grimrock.RPG.MON16.7.CollectionCapture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167CollectionCaptureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Alpha = MakeMON167Definition(TEXT("MON167_Alpha"));
	UGridStatusEffectDefinitionAsset* Beta = MakeMON167Definition(TEXT("MON167_Beta"));

	FGridStatusEffectCollection Runtime;
	Runtime.ActiveEffects.Add(MakeMON167RuntimeState(Beta, FGuid(16, 7, 1, 2), 2, 2, 9));
	Runtime.ActiveEffects.Add(MakeMON167RuntimeState(Alpha, FGuid(16, 7, 1, 1), 1, 3, 4));

	TArray<FGridStatusEffectSaveState> Saved;
	FString Error;
	TestTrue(TEXT("Runtime collection captures"), FGridStatusEffectPersistence::CaptureCollection(Runtime, Saved, Error));
	TestEqual(TEXT("Two effects are captured"), Saved.Num(), 2);
	if (Saved.Num() == 2)
	{
		TestEqual(TEXT("Capture is sorted by EffectId"), Saved[0].EffectId, FName(TEXT("MON167_Alpha")));
		TestEqual(TEXT("Stack count is stable"), Saved[1].StackCount, 2);
		TestEqual(TEXT("Remaining duration is stable"), Saved[1].RemainingDuration, 2);
		TestEqual(TEXT("Potency is stable"), Saved[1].Potency, 9);
		TestTrue(TEXT("SourceId is stable"), Saved[1].SourceId == FGuid(16, 7, 1, 2));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON167CollectionRestoreTest, "Grimrock.RPG.MON16.7.CollectionRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167CollectionRestoreTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Alpha = MakeMON167Definition(TEXT("MON167_Alpha"));
	UGridStatusEffectDefinitionAsset* Beta = MakeMON167Definition(TEXT("MON167_Beta"));

	TArray<FGridStatusEffectSaveState> Saved = { MakeMON167SaveState(TEXT("MON167_Beta"), EGridStatusEffectDurationUnit::Rounds, 2, 2, 8),
		MakeMON167SaveState(TEXT("MON167_Alpha"), EGridStatusEffectDurationUnit::Rounds, 1, 3, 3) };

	FGridStatusEffectCollection Restored;
	FString Error;
	TestTrue(TEXT("Saved collection restores"),
		FGridStatusEffectPersistence::RestoreCollection(
			Saved,
			[Alpha, Beta](FName EffectId) -> UGridStatusEffectDefinitionAsset*
			{
				if (EffectId == Alpha->EffectId)
				{
					return Alpha;
				}
				return EffectId == Beta->EffectId ? Beta : nullptr;
			},
			Restored, Error));
	TestEqual(TEXT("Two runtime effects are restored"), Restored.Num(), 2);
	if (Restored.Num() == 2)
	{
		TestEqual(TEXT("Restore is sorted by EffectId"), Restored.ActiveEffects[0].EffectId, FName(TEXT("MON167_Alpha")));
		TestTrue(TEXT("Definition pointer is rehydrated"), Restored.ActiveEffects[0].DefinitionAsset.Get() == Alpha);
		TestEqual(TEXT("Restored stack count survives"), Restored.ActiveEffects[1].StackCount, 2);
		TestEqual(TEXT("Restored potency survives"), Restored.ActiveEffects[1].Potency, 8);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON167AtomicRestoreFailureTest, "Grimrock.RPG.MON16.7.AtomicRestoreFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167AtomicRestoreFailureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Sentinel = MakeMON167Definition(TEXT("MON167_Sentinel"));
	FGridStatusEffectCollection Runtime;
	Runtime.ActiveEffects.Add(MakeMON167RuntimeState(Sentinel, FGuid(16, 7, 2, 1), 1, 3, 1));

	TArray<FGridStatusEffectSaveState> Saved = { MakeMON167SaveState(TEXT("MON167_Missing"), EGridStatusEffectDurationUnit::Rounds, 1, 2, 0) };
	FString Error;
	TestFalse(TEXT("Missing definition rejects restore"),
		FGridStatusEffectPersistence::RestoreCollection(
			Saved,
			[](FName) -> UGridStatusEffectDefinitionAsset*
			{
				return nullptr;
			},
			Runtime, Error));
	TestEqual(TEXT("Failed restore keeps previous collection"), Runtime.Num(), 1);
	TestEqual(TEXT("Failed restore keeps previous EffectId"), Runtime.ActiveEffects[0].EffectId, FName(TEXT("MON167_Sentinel")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON167DuplicateEffectRejectedTest, "Grimrock.RPG.MON16.7.DuplicateEffectRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167DuplicateEffectRejectedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TArray<FGridStatusEffectSaveState> Saved = { MakeMON167SaveState(TEXT("MON167_Duplicate"), EGridStatusEffectDurationUnit::Turns, 1, 1, 0),
		MakeMON167SaveState(TEXT("MON167_Duplicate"), EGridStatusEffectDurationUnit::Turns, 1, 2, 0) };
	FString Error;
	TestFalse(TEXT("Duplicate EffectId is rejected"), FGridStatusEffectPersistence::ValidateSavedCollection(Saved, Error));
	TestTrue(TEXT("Duplicate rejection reports a reason"), !Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON167DefinitionContractMismatchTest, "Grimrock.RPG.MON16.7.DefinitionContractMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167DefinitionContractMismatchTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* Definition = MakeMON167Definition(TEXT("MON167_Mismatch"), EGridStatusEffectDurationUnit::Rounds);
	TArray<FGridStatusEffectSaveState> Saved = { MakeMON167SaveState(TEXT("MON167_Mismatch"), EGridStatusEffectDurationUnit::Turns, 1, 2, 0) };

	FGridStatusEffectCollection Runtime;
	FString Error;
	TestFalse(TEXT("Duration contract mismatch is rejected"),
		FGridStatusEffectPersistence::RestoreCollection(
			Saved,
			[Definition](FName)
			{
				return Definition;
			},
			Runtime, Error));
	TestTrue(TEXT("Mismatch reports a reason"), !Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON167PartyActivePoolRoundTripTest, "Grimrock.RPG.MON16.7.PartyActiveAndPoolRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167PartyActivePoolRoundTripTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* ActiveDefinition = MakeMON167Definition(TEXT("MON167_Active"));
	UGridStatusEffectDefinitionAsset* PoolDefinition = MakeMON167Definition(TEXT("MON167_Pool"), EGridStatusEffectDurationUnit::Turns);

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	FGridCharacterInventoryState Active;
	Active.CharacterId = FGuid(16, 7, 3, 1);
	Active.StatusEffects.ActiveEffects.Add(MakeMON167RuntimeState(ActiveDefinition, FGuid(16, 7, 3, 11), 2, 2, 5));
	Save->PartyInventoryState.ActiveCharacters.Add(Active);

	FGridCharacterInventoryState Pool;
	Pool.CharacterId = FGuid(16, 7, 3, 2);
	Pool.StatusEffects.ActiveEffects.Add(MakeMON167RuntimeState(PoolDefinition, FGuid(16, 7, 3, 12), 1, 1, 2));
	Save->PartyInventoryState.CharacterPool.Add(Pool);

	FString Error;
	TestTrue(TEXT("Party status snapshots capture"), Save->CaptureStatusEffectState(Error));
	TestEqual(TEXT("Active and pool snapshots are both captured"), Save->CharacterStatusEffectStates.Num(), 2);

	Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Reset();
	Save->PartyInventoryState.CharacterPool[0].StatusEffects.Reset();
	TestTrue(TEXT("Party status snapshots restore"),
		Save->RestoreStatusEffectState(
			[ActiveDefinition, PoolDefinition](FName EffectId) -> UGridStatusEffectDefinitionAsset*
			{
				if (EffectId == ActiveDefinition->EffectId)
				{
					return ActiveDefinition;
				}
				return EffectId == PoolDefinition->EffectId ? PoolDefinition : nullptr;
			},
			Error));
	TestEqual(TEXT("Active character status returns"), Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Num(), 1);
	TestEqual(TEXT("Pool character status returns"), Save->PartyInventoryState.CharacterPool[0].StatusEffects.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON167PartyAtomicFailureTest, "Grimrock.RPG.MON16.7.PartyAtomicFailure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167PartyAtomicFailureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridStatusEffectDefinitionAsset* SavedDefinition = MakeMON167Definition(TEXT("MON167_SavedParty"));
	UGridStatusEffectDefinitionAsset* Sentinel = MakeMON167Definition(TEXT("MON167_PartySentinel"));

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid(16, 7, 4, 1);
	Character.StatusEffects.ActiveEffects.Add(MakeMON167RuntimeState(SavedDefinition, FGuid(16, 7, 4, 2), 1, 2, 0));
	Save->PartyInventoryState.ActiveCharacters.Add(Character);

	FString Error;
	TestTrue(TEXT("Initial party snapshot captures"), Save->CaptureStatusEffectState(Error));

	Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Reset();
	Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.ActiveEffects.Add(MakeMON167RuntimeState(Sentinel, FGuid(16, 7, 4, 3), 1, 3, 0));

	TestFalse(TEXT("Missing definition rejects whole party restore"),
		Save->RestoreStatusEffectState(
			[](FName) -> UGridStatusEffectDefinitionAsset*
			{
				return nullptr;
			},
			Error));
	TestEqual(TEXT("Atomic party failure keeps sentinel status"), Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.ActiveEffects[0].EffectId,
		FName(TEXT("MON167_PartySentinel")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON167MonsterSnapshotContractTest, "Grimrock.RPG.MON16.7.MonsterSnapshotContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167MonsterSnapshotContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FArrayProperty* StatusProperty =
		FindFProperty<FArrayProperty>(FGridRuntimeMonsterState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridRuntimeMonsterState, StatusEffects));
	TestNotNull(TEXT("Monster runtime snapshot exposes status save states"), StatusProperty);
	if (StatusProperty)
	{
		TestTrue(TEXT("Monster status snapshot is marked SaveGame"), StatusProperty->HasAnyPropertyFlags(CPF_SaveGame));
		const FStructProperty* InnerStruct = CastField<FStructProperty>(StatusProperty->Inner);
		TestTrue(
			TEXT("Monster snapshot stores the stable status save struct"), InnerStruct && InnerStruct->Struct == FGridStatusEffectSaveState::StaticStruct());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRPGMON167SaveVersionContractTest, "Grimrock.RPG.MON16.7.SaveVersionContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167SaveVersionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestEqual(TEXT("Current prototype SaveVersion is twelve"), UGrimrockPartySaveGame::CurrentSaveVersion, 12);
	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
	Previous->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion - 1;
	TestFalse(TEXT("Previous prototype SaveVersion is not compatible"), Previous->IsCompatible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPGMON167TransientRuntimeBoundaryTest, "Grimrock.RPG.MON16.7.TransientRuntimeBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPGMON167TransientRuntimeBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FProperty* SaveDefinitionProperty = FGridStatusEffectSaveState::StaticStruct()->FindPropertyByName(TEXT("DefinitionAsset"));
	TestTrue(TEXT("Persistent status snapshot contains no DefinitionAsset"), SaveDefinitionProperty == nullptr);

	const FProperty* RuntimeDefinitionProperty =
		FGridStatusEffectRuntimeState::StaticStruct()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FGridStatusEffectRuntimeState, DefinitionAsset));
	TestNotNull(TEXT("Runtime status still owns DefinitionAsset"), RuntimeDefinitionProperty);
	if (RuntimeDefinitionProperty)
	{
		TestTrue(TEXT("Runtime DefinitionAsset remains transient"), RuntimeDefinitionProperty->HasAnyPropertyFlags(CPF_Transient));
	}

	const FProperty* PartyRuntimeProperty =
		FGridCharacterInventoryState::StaticStruct()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FGridCharacterInventoryState, StatusEffects));
	TestTrue(TEXT("Party runtime status collection remains transient"), PartyRuntimeProperty && PartyRuntimeProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* MonsterRuntimeProperty = AGridMonsterActor::StaticClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(AGridMonsterActor, StatusEffects));
	TestTrue(TEXT("Monster runtime status collection remains transient"), MonsterRuntimeProperty && MonsterRuntimeProperty->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
