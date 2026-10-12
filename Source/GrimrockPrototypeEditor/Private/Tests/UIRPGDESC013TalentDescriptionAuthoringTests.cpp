#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"

namespace UIRPGDESC013Authoring
{
	struct FClassSpec
	{
		FName ClassId = NAME_None;
		const TCHAR* ObjectPath = nullptr;
	};

	const FClassSpec ClassSpecs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior") },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue") },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger") },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage") },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest") },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist") }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC013TalentDescriptionsAuthoringTest,
	"Grimrock.UI.RPG.DESC01.Authoring.CompleteTalentDescriptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC013TalentDescriptionsAuthoringTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace UIRPGDESC013Authoring;

	TSet<FString> ConceptualNodes;
	for (const FClassSpec& Spec : ClassSpecs)
	{
		const URPGClassAsset* Class = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		if (!TestNotNull(
				*FString::Printf(TEXT("%s serialized DA_Class text authority loads"), *Spec.ClassId.ToString()),
				Class))
		{
			continue;
		}

		TestEqual(
			*FString::Printf(TEXT("%s keeps canonical ClassId"), *Spec.ClassId.ToString()),
			Class->ClassId,
			Spec.ClassId);

		for (const FRPGClassProgressionChoiceDefinition& Choice : Class->ProgressionChoices)
		{
			ConceptualNodes.Add(Class->ClassId.ToString() + TEXT(":") + Choice.TalentNodeId.ToString());
			const FString Description = Choice.Description.ToString();
			TestFalse(
				*FString::Printf(TEXT("%s / %s no longer uses a bare 'Débloque ...' description"),
					*Class->ClassId.ToString(), *Choice.ChoiceId.ToString()),
				Description.StartsWith(TEXT("Débloque "), ESearchCase::IgnoreCase));
			TestTrue(
				*FString::Printf(TEXT("%s / %s has a meaningful serialized description"),
					*Class->ClassId.ToString(), *Choice.ChoiceId.ToString()),
				Description.Len() >= 30);
		}
	}

	TestEqual(TEXT("Six serialized classes still expose exactly 90 conceptual Talent nodes"), ConceptualNodes.Num(), 90);
	return true;
}

#endif
