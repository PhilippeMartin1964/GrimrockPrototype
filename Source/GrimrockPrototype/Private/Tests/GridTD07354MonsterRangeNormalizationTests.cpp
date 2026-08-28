#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace GridTD07354Normalization
{
	const TCHAR* RatAssetPath =
		TEXT("/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant.DA_MON_RatGiant");
	const TCHAR* GoblinAssetPath =
		TEXT("/Game/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.DA_MON_GoblinThrower");

	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	bool ValidateRanges(FAutomationTestBase& Test, const TCHAR* Label, const UGridMonsterDefinitionAsset& Definition)
	{
		bool bValid = true;
		for (const FGridMonsterAttackDefinition& Attack : Definition.Attacks)
		{
			bValid &= Test.TestTrue(
				*FString::Printf(TEXT("%s attack %s minimum range is positive"), Label, *Attack.AttackId.ToString()),
				Attack.MinRangeCells > 0);
			bValid &= Test.TestTrue(
				*FString::Printf(TEXT("%s attack %s maximum range covers minimum"), Label, *Attack.AttackId.ToString()),
				Attack.MaxRangeCells >= Attack.MinRangeCells);
		}
		return bValid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07354SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_5_4.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07354SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UScriptStruct* AttackStruct = FGridMonsterAttackDefinition::StaticStruct();
	TestNotNull(TEXT("Monster attack struct exists"), AttackStruct);
	if (!AttackStruct)
	{
		return false;
	}

	TestNull(TEXT("Legacy RangeCells is physically removed"), AttackStruct->FindPropertyByName(TEXT("RangeCells")));
	TestNotNull(TEXT("MinRangeCells is authoritative"), AttackStruct->FindPropertyByName(TEXT("MinRangeCells")));
	TestNotNull(TEXT("MaxRangeCells is authoritative"), AttackStruct->FindPropertyByName(TEXT("MaxRangeCells")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07354RatGiantRangeAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_5_4.Normalization.RatGiantRangeAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07354RatGiantRangeAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07354Normalization;

	UGridMonsterDefinitionAsset* Rat = LoadObject<UGridMonsterDefinitionAsset>(nullptr, RatAssetPath);
	TestNotNull(TEXT("RatGiant definition loads"), Rat);
	if (!Rat)
	{
		return false;
	}

	TestTrue(TEXT("RatGiant ranges use current schema"), ValidateRanges(*this, TEXT("RatGiant"), *Rat));
	TestTrue(TEXT("RatGiant definition remains valid"), Rat->IsValidDefinition());

	FGridMonsterAttackDefinition Bite;
	TestTrue(TEXT("RatGiant Attack_Bite resolves"), Rat->GetAttackDefinition(FName(TEXT("Attack_Bite")), Bite));
	TestEqual(TEXT("RatGiant bite minimum range remains one"), Bite.MinRangeCells, 1);
	TestEqual(TEXT("RatGiant bite maximum range remains one"), Bite.MaxRangeCells, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07354GoblinThrowerRangeAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_5_4.Normalization.GoblinThrowerRangeAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07354GoblinThrowerRangeAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07354Normalization;

	UGridMonsterDefinitionAsset* Goblin = LoadObject<UGridMonsterDefinitionAsset>(nullptr, GoblinAssetPath);
	TestNotNull(TEXT("GoblinThrower definition loads"), Goblin);
	if (!Goblin)
	{
		return false;
	}

	TestTrue(TEXT("GoblinThrower ranges use current schema"), ValidateRanges(*this, TEXT("GoblinThrower"), *Goblin));
	TestTrue(TEXT("GoblinThrower definition remains valid"), Goblin->IsValidDefinition());

	FGridMonsterAttackDefinition ThrowKnife;
	TestTrue(TEXT("GoblinThrower Attack_ThrowKnife resolves"),
		Goblin->GetAttackDefinition(FName(TEXT("Attack_ThrowKnife")), ThrowKnife));
	TestEqual(TEXT("ThrowKnife minimum range remains two"), ThrowKnife.MinRangeCells, 2);
	TestEqual(TEXT("ThrowKnife maximum range remains six"), ThrowKnife.MaxRangeCells, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07354RuntimeConsumersNormalizedTest,
	"Grimrock.TechnicalDebt.TD07_3_5_4.Normalization.RuntimeConsumersNormalized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07354RuntimeConsumersNormalizedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07354Normalization;

	FString TypesSource;
	FString PhasesSource;
	FString ActionsSource;
	FString AuditSource;
	TestTrue(TEXT("Monster types source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterTypes.h"), TypesSource));
	TestTrue(TEXT("Turn phases source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPhases.cpp"), PhasesSource));
	TestTrue(TEXT("Turn actions source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerActions.cpp"), ActionsSource));
	TestTrue(TEXT("TD07.3.1 audit source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/Tests/GridEditorTD0731CurrentSchemaAssetAuditTests.cpp"), AuditSource));

	TestFalse(TEXT("Legacy RangeCells declaration is gone"),
		TypesSource.Contains(TEXT("int32 RangeCells =")));
	TestTrue(TEXT("Range helpers read MaxRangeCells"),
		TypesSource.Contains(TEXT("DistanceCells <= MaxRangeCells")) && TypesSource.Contains(TEXT("MaxRangeCells > 1")));
	TestFalse(TEXT("Ranged planner no longer reads RangeCells"),
		PhasesSource.Contains(TEXT("Attack.RangeCells")) || PhasesSource.Contains(TEXT("RangedAttack.RangeCells")));
	TestFalse(TEXT("Monster action execution no longer reads RangeCells"),
		ActionsSource.Contains(TEXT("Attack->RangeCells")));
	TestFalse(TEXT("TD07.3.1 no longer reports the resolved range rename"),
		AuditSource.Contains(TEXT("MONSTER.RANGE_FIELD_RENAME")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
