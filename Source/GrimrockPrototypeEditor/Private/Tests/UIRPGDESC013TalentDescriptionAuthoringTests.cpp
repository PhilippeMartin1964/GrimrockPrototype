#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/RPGRangerAuthoring.h"
#include "RPG/RPGRogueAuthoring.h"
#include "RPG/RPGWarriorAuthoring.h"

namespace UIRPGDESC013Authoring
{
	void AddClass(TArray<URPGClassAsset*>& OutClasses, FName ClassId)
	{
		URPGClassAsset* Class = NewObject<URPGClassAsset>(GetTransientPackage());
		Class->ClassId = ClassId;
		Class->HealthAtLevelOne = 10;

		if (ClassId == TEXT("Warrior")) FRPGWarriorAuthoring::ConfigureClass(*Class);
		else if (ClassId == TEXT("Rogue")) FRPGRogueAuthoring::ConfigureClass(*Class);
		else if (ClassId == TEXT("Ranger")) FRPGRangerAuthoring::ConfigureClass(*Class, { TEXT("Goblin"), TEXT("Vermin") });
		else if (ClassId == TEXT("Mage")) FRPGMageAuthoring::ConfigureClass(*Class);
		else if (ClassId == TEXT("Priest")) FRPGPriestAuthoring::ConfigureClass(*Class);
		else if (ClassId == TEXT("Alchemist")) FRPGAlchemistAuthoring::ConfigureClass(*Class);

		OutClasses.Add(Class);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC013TalentDescriptionsAuthoringTest,
	"Grimrock.UI.RPG.DESC01.Authoring.CompleteTalentDescriptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC013TalentDescriptionsAuthoringTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace UIRPGDESC013Authoring;

	TArray<URPGClassAsset*> Classes;
	for (const FName ClassId : { FName(TEXT("Warrior")), FName(TEXT("Rogue")), FName(TEXT("Ranger")),
		FName(TEXT("Mage")), FName(TEXT("Priest")), FName(TEXT("Alchemist")) })
	{
		AddClass(Classes, ClassId);
	}

	TSet<FString> ConceptualNodes;
	for (const URPGClassAsset* Class : Classes)
	{
		TestNotNull(TEXT("Transient class is authored"), Class);
		if (!Class) continue;

		for (const FRPGClassProgressionChoiceDefinition& Choice : Class->ProgressionChoices)
		{
			ConceptualNodes.Add(Class->ClassId.ToString() + TEXT(":") + Choice.TalentNodeId.ToString());
			const FString Description = Choice.Description.ToString();
			TestFalse(
				*FString::Printf(TEXT("%s / %s no longer uses a bare 'Débloque ...' description"),
					*Class->ClassId.ToString(), *Choice.ChoiceId.ToString()),
				Description.StartsWith(TEXT("Débloque "), ESearchCase::IgnoreCase));
			TestTrue(
				*FString::Printf(TEXT("%s / %s has a meaningful description"),
					*Class->ClassId.ToString(), *Choice.ChoiceId.ToString()),
				Description.Len() >= 30);
		}
	}

	TestEqual(TEXT("Six classes still expose exactly 90 conceptual Talent nodes"), ConceptualNodes.Num(), 90);
	return true;
}

#endif
