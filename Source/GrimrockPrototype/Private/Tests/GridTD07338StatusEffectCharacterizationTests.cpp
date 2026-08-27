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
	TestTrue(TEXT("Character.StatusEffects is durable and non-transient"),
		CharacterStatusProperty && !CharacterStatusProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* DefinitionProperty =
		FindFProperty<FProperty>(FGridStatusEffectRuntimeState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridStatusEffectRuntimeState, DefinitionAsset));
	TestNotNull(TEXT("Runtime DefinitionAsset cache is reflected"), DefinitionProperty);
	TestTrue(TEXT("DefinitionAsset remains transient and reconstructible"),
		DefinitionProperty && DefinitionProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* EffectIdProperty =
		FindFProperty<FProperty>(FGridStatusEffectRuntimeState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridStatusEffectRuntimeState, EffectId));
	TestNotNull(TEXT("Stable EffectId is reflected"), EffectIdProperty);
	TestTrue(TEXT("Stable EffectId remains durable"), EffectIdProperty && !EffectIdProperty->HasAnyPropertyFlags(CPF_Transient));

	UGridStatusEffectDefinitionAsset* Definition = MakeTD07338Definition(TEXT("TD07338_Runtime"));
	FGridCharacterInventoryState Character = MakeTD07338Character(MakeTD07338Id(1));
	Character.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(Definition, MakeTD07338Id(101), 2, 3, 7));

	TestEqual(TEXT("Character durable state owns one active effect"), Character.StatusEffects.Num(), 1);
	TestEqual(TEXT("Stable EffectId is carried directly by character state"), Character.StatusEffects.ActiveEffects[0].EffectId, Definition->EffectId);
	TestTrue(TEXT("DefinitionAsset remains only a runtime cache"), Character.StatusEffects.ActiveEffects[0].DefinitionAsset.Get() == Definition);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338PartySparseSaveMirrorTest,
	"Grimrock.TechnicalDebt.TD07_3_3_8.Characterization.PartySparseSaveMirror",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338PartySparseSaveMirrorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07338Characterization;

	TestNull(TEXT("Separate CharacterStatusEffectStates SaveGame mirror is removed"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("CharacterStatusEffectStates")));

	UGridStatusEffectDefinitionAsset* ActiveDefinition = MakeTD07338Definition(TEXT("TD07338_Active"));
	UGridStatusEffectDefinitionAsset* PoolDefinition =
		MakeTD07338Definition(TEXT("TD07338_Pool"), EGridStatusEffectDurationUnit::Turns, 2, 2);

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Active = MakeTD07338Character(MakeTD07338Id(2));
	Active.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(ActiveDefinition, MakeTD07338Id(102), 2, 2, 5));
	Party.ActiveCharacters.Add(Active);
	Party.ActiveCharacters.Add(MakeTD07338Character(MakeTD07338Id(3)));

	FGridCharacterInventoryState Pool = MakeTD07338Character(MakeTD07338Id(4));
	Pool.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(PoolDefinition, MakeTD07338Id(104), 1, 1, 2));
	Party.CharacterPool.Add(Pool);

	const FGridPartyInventoryState DurableCopy = Party;
	TestEqual(TEXT("Active character carries its status directly"), DurableCopy.ActiveCharacters[0].StatusEffects.Num(), 1);
	TestTrue(TEXT("Empty active character needs no sparse mirror"), DurableCopy.ActiveCharacters[1].StatusEffects.IsEmpty());
	TestEqual(TEXT("Pool character carries its status directly"), DurableCopy.CharacterPool[0].StatusEffects.Num(), 1);

	FString Error;
	TestTrue(TEXT("Direct durable party state validates"), FGridStatusEffectPersistence::ValidatePartyStatusEffects(DurableCopy, Error));
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

	FGridPartyInventoryState SavedParty;
	FGridCharacterInventoryState SavedCharacter = MakeTD07338Character(MakeTD07338Id(5));
	SavedCharacter.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(SavedDefinition, MakeTD07338Id(105), 1, 2, 3));
	SavedCharacter.StatusEffects.ActiveEffects[0].DefinitionAsset = nullptr;
	SavedParty.ActiveCharacters.Add(SavedCharacter);
	SavedParty.ActiveCharacters.Add(MakeTD07338Character(MakeTD07338Id(6)));

	FGridPartyInventoryState RuntimeParty;
	FGridCharacterInventoryState RuntimeCharacter = MakeTD07338Character(MakeTD07338Id(5));
	RuntimeCharacter.StatusEffects.ActiveEffects.Add(MakeTD07338RuntimeState(SentinelDefinition, MakeTD07338Id(106), 1, 3, 1));
	RuntimeParty.ActiveCharacters.Add(RuntimeCharacter);
	RuntimeParty.ActiveCharacters.Add(MakeTD07338Character(MakeTD07338Id(6)));

	RuntimeParty = SavedParty;
	FString Error;
	TestTrue(TEXT("Whole-party restore candidate rehydrates directly"),
		FGridStatusEffectPersistence::RehydratePartyStatusEffects(
			RuntimeParty,
			[SavedDefinition](FName EffectId) -> UGridStatusEffectDefinitionAsset*
			{
				return EffectId == SavedDefinition->EffectId ? SavedDefinition : nullptr;
			},
			Error));
	TestTrue(TEXT("Persisted effect remains"), RuntimeParty.ActiveCharacters[0].StatusEffects.Contains(SavedDefinition->EffectId));
	TestFalse(TEXT("Stale runtime effect is absent after whole-party replacement"),
		RuntimeParty.ActiveCharacters[0].StatusEffects.Contains(SentinelDefinition->EffectId));
	TestTrue(TEXT("Character with no durable status remains empty"), RuntimeParty.ActiveCharacters[1].StatusEffects.IsEmpty());

	FGridPartyInventoryState InvalidCandidate = SavedParty;
	TestFalse(TEXT("Missing canonical definition rejects rehydration atomically"),
		FGridStatusEffectPersistence::RehydratePartyStatusEffects(
			InvalidCandidate,
			[](FName) -> UGridStatusEffectDefinitionAsset*
			{
				return nullptr;
			},
			Error));
	TestNull(TEXT("Failed rehydration does not partially bind DefinitionAsset"),
		InvalidCandidate.ActiveCharacters[0].StatusEffects.ActiveEffects[0].DefinitionAsset.Get());
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
	TestTrue(TEXT("Monster status snapshot remains explicit SaveGame state"),
		MonsterStatusProperty && MonsterStatusProperty->HasAnyPropertyFlags(CPF_SaveGame));

	const FStructProperty* InnerStructProperty =
		MonsterStatusProperty ? CastField<FStructProperty>(MonsterStatusProperty->Inner) : nullptr;
	TestNotNull(TEXT("Monster status array has a struct element type"), InnerStructProperty);
	TestTrue(TEXT("Monster persistence still depends on FGridStatusEffectSaveState"),
		InnerStructProperty && InnerStructProperty->Struct == FGridStatusEffectSaveState::StaticStruct());

	TestNull(TEXT("Monster save snapshot still contains no DefinitionAsset pointer"),
		FindFProperty<FProperty>(FGridStatusEffectSaveState::StaticStruct(), TEXT("DefinitionAsset")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
