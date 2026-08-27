#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07338Characterization
{
	FGuid MakeTD07338Id(uint32 Suffix)
	{
		return FGuid(7, 3, 38, Suffix);
	}

	UGridStatusEffectDefinitionAsset* MakeTD07338Definition(
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

	FGridStatusEffectRuntimeState MakeTD07338RuntimeState(
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

	FGridCharacterInventoryState MakeTD07338Character(const FGuid& CharacterId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.Experience = 0;
		Character.Level = 1;
		return Character;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338RuntimeAuthorityBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_8.Characterization.RuntimeAuthorityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338RuntimeAuthorityBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07338Characterization;

	const FProperty* CharacterStatusProperty =
		FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridCharacterInventoryState, StatusEffects));
	TestNotNull(TEXT("Character.StatusEffects is reflected"), CharacterStatusProperty);
	TestTrue(TEXT("Character.StatusEffects is currently transient at the Save boundary"),
		CharacterStatusProperty && CharacterStatusProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* DefinitionProperty =
		FindFProperty<FProperty>(FGridStatusEffectRuntimeState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridStatusEffectRuntimeState, DefinitionAsset));
	TestNotNull(TEXT("Runtime DefinitionAsset cache is reflected"), DefinitionProperty);
	TestTrue(TEXT("DefinitionAsset is transient and reconstructible"),
		DefinitionProperty && DefinitionProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* EffectIdProperty =
		FindFProperty<FProperty>(FGridStatusEffectRuntimeState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridStatusEffectRuntimeState, EffectId));
	TestNotNull(TEXT("Stable EffectId is reflected"), EffectIdProperty);
	TestTrue(TEXT("Stable EffectId is not transient"), EffectIdProperty && !EffectIdProperty->HasAnyPropertyFlags(CPF_Transient));

	UGridStatusEffectDefinitionAsset* Definition = MakeTD07338Definition(TEXT("TD07338_Runtime"));
	FGridCharacterInventoryState Character = MakeTD07338Character(MakeTD07338Id(1));
	Character.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(Definition, MakeTD07338Id(101), 2, 3, 7));

	TestEqual(TEXT("Character runtime state owns one active effect"), Character.StatusEffects.Num(), 1);
	TestEqual(TEXT("Stable runtime EffectId is carried by character state"), Character.StatusEffects.ActiveEffects[0].EffectId, Definition->EffectId);
	TestTrue(TEXT("Runtime state carries only a rehydratable DefinitionAsset cache"),
		Character.StatusEffects.ActiveEffects[0].DefinitionAsset.Get() == Definition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338PartySparseSaveMirrorTest,
	"Grimrock.TechnicalDebt.TD07_3_3_8.Characterization.PartySparseSaveMirror",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338PartySparseSaveMirrorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07338Characterization;

	UGridStatusEffectDefinitionAsset* ActiveDefinition = MakeTD07338Definition(TEXT("TD07338_Active"));
	UGridStatusEffectDefinitionAsset* PoolDefinition =
		MakeTD07338Definition(TEXT("TD07338_Pool"), EGridStatusEffectDurationUnit::Turns, 2, 2);

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();

	FGridCharacterInventoryState Active = MakeTD07338Character(MakeTD07338Id(2));
	Active.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(ActiveDefinition, MakeTD07338Id(102), 2, 2, 5));
	Save->PartyInventoryState.ActiveCharacters.Add(Active);
	Save->PartyInventoryState.ActiveCharacters.Add(MakeTD07338Character(MakeTD07338Id(3)));

	FGridCharacterInventoryState Pool = MakeTD07338Character(MakeTD07338Id(4));
	Pool.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(PoolDefinition, MakeTD07338Id(104), 1, 1, 2));
	Save->PartyInventoryState.CharacterPool.Add(Pool);

	FString Error;
	TestTrue(TEXT("Current party status state captures into a separate Save mirror"), Save->CaptureStatusEffectState(Error));
	TestEqual(TEXT("Sparse mirror omits the empty active character"), Save->CharacterStatusEffectStates.Num(), 2);
	if (Save->CharacterStatusEffectStates.Num() == 2)
	{
		TestTrue(TEXT("Snapshots are deterministically sorted by CharacterId"),
			Save->CharacterStatusEffectStates[0].CharacterId == MakeTD07338Id(2) &&
				Save->CharacterStatusEffectStates[1].CharacterId == MakeTD07338Id(4));
		TestEqual(TEXT("Active snapshot contains one stable effect"), Save->CharacterStatusEffectStates[0].StatusEffects.Num(), 1);
		if (Save->CharacterStatusEffectStates[0].StatusEffects.Num() == 1)
		{
			const FGridStatusEffectSaveState& Snapshot = Save->CharacterStatusEffectStates[0].StatusEffects[0];
			TestEqual(TEXT("Snapshot preserves EffectId"), Snapshot.EffectId, ActiveDefinition->EffectId);
			TestEqual(TEXT("Snapshot preserves StackCount"), Snapshot.StackCount, 2);
			TestEqual(TEXT("Snapshot preserves RemainingDuration"), Snapshot.RemainingDuration, 2);
			TestEqual(TEXT("Snapshot preserves Potency"), Snapshot.Potency, 5);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338PartyRestoreReplacementBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_8.Characterization.PartyRestoreReplacementBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338PartyRestoreReplacementBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07338Characterization;

	UGridStatusEffectDefinitionAsset* SavedDefinition = MakeTD07338Definition(TEXT("TD07338_Saved"));
	UGridStatusEffectDefinitionAsset* SentinelDefinition = MakeTD07338Definition(TEXT("TD07338_Sentinel"));

	UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
	FGridCharacterInventoryState Character = MakeTD07338Character(MakeTD07338Id(5));
	Character.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(SavedDefinition, MakeTD07338Id(105), 1, 2, 3));
	Save->PartyInventoryState.ActiveCharacters.Add(Character);
	Save->PartyInventoryState.ActiveCharacters.Add(MakeTD07338Character(MakeTD07338Id(6)));

	FString Error;
	TestTrue(TEXT("Initial party mirror captures"), Save->CaptureStatusEffectState(Error));

	Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Reset();
	Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.ActiveEffects.Add(
		MakeTD07338RuntimeState(SentinelDefinition, MakeTD07338Id(106), 1, 3, 1));

	TestTrue(TEXT("Restore replaces runtime character status state"),
		Save->RestoreStatusEffectState(
			[SavedDefinition](FName EffectId) -> UGridStatusEffectDefinitionAsset*
			{
				return EffectId == SavedDefinition->EffectId ? SavedDefinition : nullptr;
			},
			Error));
	TestTrue(TEXT("Persisted effect is restored"),
		Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Contains(SavedDefinition->EffectId));
	TestFalse(TEXT("Stale runtime-only effect is removed"),
		Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Contains(SentinelDefinition->EffectId));
	TestTrue(TEXT("Character absent from sparse mirror restores empty"),
		Save->PartyInventoryState.ActiveCharacters[1].StatusEffects.IsEmpty());

	Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Reset();
	Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.ActiveEffects.Add(
		MakeTD07338RuntimeState(SentinelDefinition, MakeTD07338Id(107), 1, 3, 1));

	TestFalse(TEXT("Missing canonical definition rejects whole-party restore atomically"),
		Save->RestoreStatusEffectState(
			[](FName) -> UGridStatusEffectDefinitionAsset*
			{
				return nullptr;
			},
			Error));
	TestTrue(TEXT("Failed restore preserves previous runtime collection"),
		Save->PartyInventoryState.ActiveCharacters[0].StatusEffects.Contains(SentinelDefinition->EffectId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338MonsterSnapshotIsolationTest,
	"Grimrock.TechnicalDebt.TD07_3_3_8.Characterization.MonsterSnapshotIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338MonsterSnapshotIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FArrayProperty* MonsterStatusProperty =
		FindFProperty<FArrayProperty>(FGridRuntimeMonsterState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridRuntimeMonsterState, StatusEffects));
	TestNotNull(TEXT("Monster persistent runtime state keeps a status snapshot array"), MonsterStatusProperty);
	TestTrue(TEXT("Monster status snapshot is explicitly SaveGame state"),
		MonsterStatusProperty && MonsterStatusProperty->HasAnyPropertyFlags(CPF_SaveGame));

	const FStructProperty* InnerStructProperty =
		MonsterStatusProperty ? CastField<FStructProperty>(MonsterStatusProperty->Inner) : nullptr;
	TestNotNull(TEXT("Monster status array has a struct element type"), InnerStructProperty);
	TestTrue(TEXT("Monster persistence intentionally depends on FGridStatusEffectSaveState"),
		InnerStructProperty && InnerStructProperty->Struct == FGridStatusEffectSaveState::StaticStruct());

	TestNull(TEXT("Save snapshot deliberately contains no DefinitionAsset pointer"),
		FindFProperty<FProperty>(FGridStatusEffectSaveState::StaticStruct(), TEXT("DefinitionAsset")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
