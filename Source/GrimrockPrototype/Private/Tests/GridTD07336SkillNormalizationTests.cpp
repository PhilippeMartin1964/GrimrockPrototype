#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridTD07336Normalization
{
	URPGSkillAsset* MakeSkill(FName SkillId, int32 MaxRank = 5)
	{
		URPGSkillAsset* Skill = NewObject<URPGSkillAsset>();
		Skill->SkillId = SkillId;
		Skill->DisplayName = FText::FromName(SkillId);
		Skill->MaxRank = MaxRank;
		return Skill;
	}

	FGridCharacterInventoryState MakeCharacter(uint32 Suffix)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid(7, 3, 36, Suffix);
		Character.Experience = 0;
		Character.Level = 1;
		return Character;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336SchemaAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UScriptStruct* CharacterStruct = FGridCharacterInventoryState::StaticStruct();
	FProperty* SkillRanksProperty = FindFProperty<FProperty>(CharacterStruct, TEXT("SkillRanks"));
	TestNotNull(TEXT("SkillRanks remains reflected"), SkillRanksProperty);
	TestTrue(TEXT("SkillRanks is durable and no longer transient"), SkillRanksProperty && !SkillRanksProperty->HasAnyPropertyFlags(CPF_Transient));
	TestNull(TEXT("Separate CharacterSkillStates SaveGame property is removed"),
		FindFProperty<FProperty>(UGrimrockPartySaveGame::StaticClass(), TEXT("CharacterSkillStates")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336DirectValidationTest, "Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.DirectValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336DirectValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07336Normalization;

	URPGSkillAsset* Skill = MakeSkill(TEXT("Skill_A"), 3);
	const auto Resolver = [Skill](FName SkillId) -> const URPGSkillAsset*
	{
		return SkillId == Skill->SkillId ? Skill : nullptr;
	};

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Active = MakeCharacter(1);
	FRPGSkillMutationResult Mutation;
	TestTrue(TEXT("Active rank mutation succeeds"), FRPGSkillService::TrySetSkillRank(Active, Skill, 2, Mutation));
	Party.ActiveCharacters.Add(Active);
	FGridCharacterInventoryState Pool = MakeCharacter(2);
	TestTrue(TEXT("Pool rank mutation succeeds"), FRPGSkillService::TrySetSkillRank(Pool, Skill, 1, Mutation));
	Party.CharacterPool.Add(Pool);

	FString Error;
	TestTrue(TEXT("Direct durable Active + Pool state validates"), FRPGSkillPersistence::ValidatePartySkills(Party, Resolver, Error));

	FGridPartyInventoryState Invalid = Party;
	Invalid.CharacterPool[0].SkillRanks[0].Rank = 4;
	TestFalse(TEXT("Direct durable over-rank is rejected"), FRPGSkillPersistence::ValidatePartySkills(Invalid, Resolver, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336DeterministicMutationTest, "Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.DeterministicMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336DeterministicMutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07336Normalization;
	FGridCharacterInventoryState Character = MakeCharacter(3);
	URPGSkillAsset* SkillC = MakeSkill(TEXT("Skill_C"));
	URPGSkillAsset* SkillA = MakeSkill(TEXT("Skill_A"));
	URPGSkillAsset* SkillB = MakeSkill(TEXT("Skill_B"));
	FRPGSkillMutationResult Mutation;
	TestTrue(TEXT("C mutation"), FRPGSkillService::TrySetSkillRank(Character, SkillC, 1, Mutation));
	TestTrue(TEXT("A mutation"), FRPGSkillService::TrySetSkillRank(Character, SkillA, 2, Mutation));
	TestTrue(TEXT("B mutation"), FRPGSkillService::TrySetSkillRank(Character, SkillB, 3, Mutation));
	TestEqual(TEXT("Three durable ranks"), Character.SkillRanks.Num(), 3);
	if (Character.SkillRanks.Num() == 3)
	{
		TestEqual(TEXT("A first"), Character.SkillRanks[0].SkillId, FName(TEXT("Skill_A")));
		TestEqual(TEXT("B second"), Character.SkillRanks[1].SkillId, FName(TEXT("Skill_B")));
		TestEqual(TEXT("C third"), Character.SkillRanks[2].SkillId, FName(TEXT("Skill_C")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07336SaveSchemaVersionTest, "Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.SaveSchemaVersion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07336SaveSchemaVersionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestTrue(TEXT("TD07.3.3.6 established SaveGame v16 or later"), UGrimrockPartySaveGame::CurrentSaveVersion >= 16);
	UGrimrockPartySaveGame* Current = NewObject<UGrimrockPartySaveGame>();
	TestEqual(TEXT("New SaveGame starts on current schema"), Current->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
	TestTrue(TEXT("Current schema is compatible"), Current->IsCompatible());

	UGrimrockPartySaveGame* Previous = NewObject<UGrimrockPartySaveGame>();
	Previous->SaveVersion = 15;
	FText Error;
	TestFalse(TEXT("Previous v15 is rejected without migration"), Previous->ValidateCurrentState(Error));
	TestFalse(TEXT("Previous v15 is incompatible"), Previous->IsCompatible());
	TestEqual(TEXT("Validation does not rewrite v15"), Previous->SaveVersion, 15);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
