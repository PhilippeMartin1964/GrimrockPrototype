#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/SkillAffinityProductionAuthoring.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSAFFINITY013CanonicalMatrixTest,
	"Grimrock.UI.Skills02.Affinity.AuthoringMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSAFFINITY013CanonicalMatrixTest::RunTest(const FString&)
{
	struct FCase
	{
		FName SkillId;
		TArray<FName> Expected;
	};

	const FCase Cases[] =
	{
		{ TEXT("Skill_Lockpicking"), { TEXT("Priest") } },
		{ TEXT("Skill_Traps"), { TEXT("Warrior"), TEXT("Priest") } },
		{ TEXT("Skill_Alchemy"), { TEXT("Warrior"), TEXT("Ranger"), TEXT("Priest") } },
		{ TEXT("Skill_Arcana"), { TEXT("Warrior"), TEXT("Ranger"), TEXT("Priest") } },
		{ TEXT("Skill_Runes"), { TEXT("Warrior"), TEXT("Ranger"), TEXT("Priest") } },
		{ TEXT("Skill_Religion"), { TEXT("Warrior"), TEXT("Rogue"), TEXT("Alchemist") } },
		{ TEXT("Skill_Perception"), {} }
	};

	for (const FCase& Case : Cases)
	{
		TArray<FName> Actual;
		FSkillAffinityProductionAuthoring::GetCanonicalDiscouragedClassIds(Case.SkillId, Actual);
		TestTrue(
			*FString::Printf(TEXT("%s discouraged matrix is canonical"), *Case.SkillId.ToString()),
			Actual == Case.Expected);
	}

	return true;
}

#endif
