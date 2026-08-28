#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07338Normalization
{
	FGuid MakeTD07338NId(uint32 Suffix)
	{
		return FGuid(7, 3, 138, Suffix);
	}

	UGridStatusEffectDefinitionAsset* MakeTD07338NDefinition(
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

	FGridStatusEffectRuntimeState MakeTD07338NState(
		UGridStatusEffectDefinitionAsset* Definition, const FGuid& SourceId, int32 StackCount = 1, int32 RemainingDuration = 2, int32 Potency = 0)
	{
		FGridStatusEffectRuntimeState State;
		FString Error;
		Definition->BuildRuntimeState(SourceId, StackCount, RemainingDuration, Potency, State, Error);
		return State;
	}

	FGridCharacterInventoryState MakeTD07338NCharacter(uint32 Suffix)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = MakeTD07338NId(Suffix);
		Character.Experience = 0;
		Character.Level = 1;
		return Character;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338SchemaAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_3_8.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FProperty* StatusProperty =
		FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridCharacterInventoryState, StatusEffects));
	TestNotNull(TEXT("Character.StatusEffects exists"), StatusProperty);
	TestTrue(TEXT("Character.StatusEffects is durable"), StatusProperty && !StatusProperty->HasAnyPropertyFlags(CPF_Transient));

	const FProperty* DefinitionProperty =
		FindFProperty<FProperty>(FGridStatusEffectRuntimeState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridStatusEffectRuntimeState, DefinitionAsset));
	TestTrue(TEXT("DefinitionAsset remains transient"), DefinitionProperty && DefinitionProperty->HasAnyPropertyFlags(CPF_Transient));

	TestNull(TEXT("Separate party status SaveGame mirror is removed"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("CharacterStatusEffectStates")));

	const FArrayProperty* MonsterStatusProperty =
		FindFProperty<FArrayProperty>(FGridRuntimeMonsterState::StaticStruct(), GET_MEMBER_NAME_CHECKED(FGridRuntimeMonsterState, StatusEffects));
	const FStructProperty* MonsterInner = MonsterStatusProperty ? CastField<FStructProperty>(MonsterStatusProperty->Inner) : nullptr;
	TestTrue(TEXT("Monster status snapshot contract is preserved"),
		MonsterStatusProperty && MonsterStatusProperty->HasAnyPropertyFlags(CPF_SaveGame) && MonsterInner &&
			MonsterInner->Struct == FGridStatusEffectSaveState::StaticStruct());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338DirectDurableValidationTest, "Grimrock.TechnicalDebt.TD07_3_3_8.Normalization.DirectDurableValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338DirectDurableValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07338Normalization;
	UGridStatusEffectDefinitionAsset* ActiveDefinition = MakeTD07338NDefinition(TEXT("TD07338N_Active"));
	UGridStatusEffectDefinitionAsset* PoolDefinition = MakeTD07338NDefinition(TEXT("TD07338N_Pool"), EGridStatusEffectDurationUnit::Turns, 2, 2);

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Active = MakeTD07338NCharacter(2);
	Active.StatusEffects.ActiveEffects.Add(MakeTD07338NState(ActiveDefinition, MakeTD07338NId(102), 2, 2, 5));
	Party.ActiveCharacters.Add(Active);
	FGridCharacterInventoryState Pool = MakeTD07338NCharacter(3);
	Pool.StatusEffects.ActiveEffects.Add(MakeTD07338NState(PoolDefinition, MakeTD07338NId(103), 1, 1, 2));
	Party.CharacterPool.Add(Pool);

	FString Error;
	TestTrue(TEXT("Live runtime party validates with DefinitionAsset caches"), FGridStatusEffectPersistence::ValidateRuntimePartyStatusEffects(Party, Error));

	Party.ActiveCharacters[0].StatusEffects.ActiveEffects[0].DefinitionAsset = nullptr;
	Party.CharacterPool[0].StatusEffects.ActiveEffects[0].DefinitionAsset = nullptr;
	TestTrue(TEXT("Deserialized durable party validates without transient caches"), FGridStatusEffectPersistence::ValidatePartyStatusEffects(Party, Error));
	TestFalse(TEXT("Runtime validation distinguishes missing caches"), FGridStatusEffectPersistence::ValidateRuntimePartyStatusEffects(Party, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338AtomicRehydrationTest, "Grimrock.TechnicalDebt.TD07_3_3_8.Normalization.AtomicRehydration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338AtomicRehydrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07338Normalization;
	UGridStatusEffectDefinitionAsset* A = MakeTD07338NDefinition(TEXT("TD07338N_A"));
	UGridStatusEffectDefinitionAsset* B = MakeTD07338NDefinition(TEXT("TD07338N_B"));

	FGridPartyInventoryState Loaded;
	FGridCharacterInventoryState Active = MakeTD07338NCharacter(4);
	Active.StatusEffects.ActiveEffects.Add(MakeTD07338NState(A, MakeTD07338NId(104), 1, 2, 1));
	Active.StatusEffects.ActiveEffects[0].DefinitionAsset = nullptr;
	Loaded.ActiveCharacters.Add(Active);
	FGridCharacterInventoryState Pool = MakeTD07338NCharacter(5);
	Pool.StatusEffects.ActiveEffects.Add(MakeTD07338NState(B, MakeTD07338NId(105), 1, 3, 2));
	Pool.StatusEffects.ActiveEffects[0].DefinitionAsset = nullptr;
	Loaded.CharacterPool.Add(Pool);

	FString Error;
	TestTrue(TEXT("Direct durable state rehydrates Active + Pool"),
		FGridStatusEffectPersistence::RehydratePartyStatusEffects(
			Loaded,
			[A, B](FName EffectId) -> UGridStatusEffectDefinitionAsset*
			{
				if (EffectId == A->EffectId)
				{
					return A;
				}
				return EffectId == B->EffectId ? B : nullptr;
			},
			Error));
	TestTrue(TEXT("Active cache rebound"), Loaded.ActiveCharacters[0].StatusEffects.ActiveEffects[0].DefinitionAsset.Get() == A);
	TestTrue(TEXT("Pool cache rebound"), Loaded.CharacterPool[0].StatusEffects.ActiveEffects[0].DefinitionAsset.Get() == B);

	FGridPartyInventoryState Invalid = Loaded;
	Invalid.ActiveCharacters[0].StatusEffects.ActiveEffects[0].DefinitionAsset = nullptr;
	Invalid.CharacterPool[0].StatusEffects.ActiveEffects[0].DefinitionAsset = nullptr;
	TestFalse(TEXT("Missing definition rejects whole party atomically"),
		FGridStatusEffectPersistence::RehydratePartyStatusEffects(
			Invalid,
			[A](FName EffectId) -> UGridStatusEffectDefinitionAsset*
			{
				return EffectId == A->EffectId ? A : nullptr;
			},
			Error));
	TestNull(TEXT("Atomic failure leaves Active cache unbound"), Invalid.ActiveCharacters[0].StatusEffects.ActiveEffects[0].DefinitionAsset.Get());
	TestNull(TEXT("Atomic failure leaves Pool cache unbound"), Invalid.CharacterPool[0].StatusEffects.ActiveEffects[0].DefinitionAsset.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07338SaveSchemaVersionTest, "Grimrock.TechnicalDebt.TD07_3_3_8.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07338SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestTrue(TEXT("TD07.3.3.8 established SaveGame v18 or later"), UGrimrockPartySaveGame::CurrentSaveVersion >= 18);
	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on the current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Current schema is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 17;
	FText Error;
	TestFalse(TEXT("Previous v17 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v17 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation does not rewrite v17"), Previous->SaveVersion, 17);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
