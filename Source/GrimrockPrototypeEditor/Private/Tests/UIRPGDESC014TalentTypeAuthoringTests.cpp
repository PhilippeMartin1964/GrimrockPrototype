#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/RPGRangerAuthoring.h"
#include "RPG/RPGRogueAuthoring.h"
#include "RPG/RPGWarriorAuthoring.h"
#include "RPG/RPGClassAsset.h"

namespace UIRPGDESC014
{
	URPGClassAsset* BuildClass(FName ClassId)
	{
		URPGClassAsset* Asset = NewObject<URPGClassAsset>();
		if (ClassId == TEXT("Warrior")) FRPGWarriorAuthoring::ConfigureClass(*Asset);
		else if (ClassId == TEXT("Rogue")) FRPGRogueAuthoring::ConfigureClass(*Asset);
		else if (ClassId == TEXT("Ranger")) FRPGRangerAuthoring::ConfigureClass(*Asset);
		else if (ClassId == TEXT("Mage")) FRPGMageAuthoring::ConfigureClass(*Asset);
		else if (ClassId == TEXT("Priest")) FRPGPriestAuthoring::ConfigureClass(*Asset);
		else if (ClassId == TEXT("Alchemist")) FRPGAlchemistAuthoring::ConfigureClass(*Asset);
		return Asset;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC014NinetyTalentTypesTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.NinetyTalentTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC014NinetyTalentTypesTest::RunTest(const FString&)
{
	using namespace UIRPGDESC014;
	TMap<ERPGTalentPresentationType, int32> Counts;
	int32 TotalNodes = 0;

	for (const FName ClassId : {
		FName(TEXT("Warrior")), FName(TEXT("Rogue")), FName(TEXT("Ranger")),
		FName(TEXT("Mage")), FName(TEXT("Priest")), FName(TEXT("Alchemist")) })
	{
		URPGClassAsset* Asset = BuildClass(ClassId);
		TestNotNull(*FString::Printf(TEXT("%s transient class builds"), *ClassId.ToString()), Asset);
		if (!Asset) continue;

		TMap<FName, ERPGTalentPresentationType> TypeByNode;
		for (const FRPGClassProgressionChoiceDefinition& Choice : Asset->ProgressionChoices)
		{
			TestTrue(*FString::Printf(TEXT("%s/%s has explicit Talent TYPE"),
				*ClassId.ToString(), *Choice.ChoiceId.ToString()),
				Choice.PresentationType != ERPGTalentPresentationType::None);
			if (ERPGTalentPresentationType* Existing = TypeByNode.Find(Choice.TalentNodeId))
			{
				TestEqual(TEXT("All concrete variants share one TYPE"), *Existing, Choice.PresentationType);
			}
			else
			{
				TypeByNode.Add(Choice.TalentNodeId, Choice.PresentationType);
			}
		}

		TestEqual(*FString::Printf(TEXT("%s exposes fifteen conceptual nodes"), *ClassId.ToString()), TypeByNode.Num(), 15);
		TotalNodes += TypeByNode.Num();
		for (const TPair<FName, ERPGTalentPresentationType>& Pair : TypeByNode)
		{
			Counts.FindOrAdd(Pair.Value)++;
		}
	}

	TestEqual(TEXT("Exactly ninety conceptual Talents are typed"), TotalNodes, 90);
	TestEqual(TEXT("ACTIF count"), Counts.FindRef(ERPGTalentPresentationType::Active), 32);
	TestEqual(TEXT("SORT ACTIF count"), Counts.FindRef(ERPGTalentPresentationType::ActiveSpell), 22);
	TestEqual(TEXT("PASSIF count"), Counts.FindRef(ERPGTalentPresentationType::Passive), 22);
	TestEqual(TEXT("RÉACTION AUTOMATIQUE count"), Counts.FindRef(ERPGTalentPresentationType::AutomaticReaction), 5);
	TestEqual(TEXT("RECETTE + OBJET RAPIDE count"), Counts.FindRef(ERPGTalentPresentationType::RecipeQuickItem), 8);
	TestEqual(TEXT("RECETTE + ACTIF count"), Counts.FindRef(ERPGTalentPresentationType::RecipeActive), 1);
	return true;
}

#endif
