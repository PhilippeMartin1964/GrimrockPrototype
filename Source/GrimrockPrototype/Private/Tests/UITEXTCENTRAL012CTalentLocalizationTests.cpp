#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/RPGTalentLocalizationText.h"

namespace UITEXTCENTRAL012CTests
{
	FText FormatNamed(FName TextId, const FFormatNamedArguments& Arguments)
	{
		const FText Pattern = FRPGTalentLocalizationText::Get(TextId);
		return Pattern.IsEmpty() ? FText::GetEmpty() : FText::Format(Pattern, Arguments);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUITEXTCENTRAL012CRuntimeResolverTest,
	"Game.UI.TextCentral.CENTRAL01.TalentRuntimeResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITEXTCENTRAL012CRuntimeResolverTest::RunTest(const FString&)
{
	using namespace UITEXTCENTRAL012CTests;

	TestTrue(TEXT("Talent UI vocabulary resolves from ST_UI"),
		FRPGTalentLocalizationText::Get(TEXT("Talent.Type.Active")).EqualTo(FText::FromString(TEXT("ACTIF"))));
	TestTrue(TEXT("Shared Usage resolves from ST_UI"),
		FRPGTalentLocalizationText::Get(TEXT("UI.Section.Usage")).EqualTo(FText::FromString(TEXT("UTILISATION"))));
	TestTrue(TEXT("Shared permanent duration resolves from ST_UI"),
		FRPGTalentLocalizationText::Get(TEXT("Status.Duration.Permanent")).EqualTo(FText::FromString(TEXT("Permanent"))));
	TestTrue(TEXT("Recipe ids are explicitly deferred"),
		FRPGTalentLocalizationText::IsDeferredRecipeTextId(TEXT("Talent.Recipe.Recipe_Bomb_Fire")));
	TestFalse(TEXT("Normal Talent UI ids are not deferred"),
		FRPGTalentLocalizationText::IsDeferredRecipeTextId(TEXT("Talent.Type.Active")));
	TestTrue(TEXT("Deferred recipe still resolves through the compatibility bridge"),
		FRPGTalentLocalizationText::Get(TEXT("Talent.Recipe.Recipe_Bomb_Fire")).EqualTo(FText::FromString(TEXT("Bombe incendiaire"))));

	FFormatNamedArguments Args;
	Args.Add(TEXT("Level"), FText::AsNumber(6));
	TestTrue(TEXT("Named Level format"),
		FormatNamed(TEXT("Talent.Node.LevelFormat"), Args).EqualTo(FText::FromString(TEXT("Niv. 6"))));

	Args.Reset();
	Args.Add(TEXT("Class"), FText::FromString(TEXT("Guerrier")));
	Args.Add(TEXT("Level"), FText::AsNumber(7));
	TestTrue(TEXT("Named Class/Level format"),
		FormatNamed(TEXT("Talent.Screen.ClassLevelFormat"), Args).EqualTo(FText::FromString(TEXT("Guerrier - Niveau 7"))));

	Args.Reset();
	Args.Add(TEXT("TalentPoints"), FText::AsNumber(4));
	TestTrue(TEXT("Named TalentPoints format"),
		FormatNamed(TEXT("Talent.Screen.TalentPointsFormat"), Args).EqualTo(FText::FromString(TEXT("Points de talent : 4"))));

	Args.Reset();
	Args.Add(TEXT("SkillPoints"), FText::AsNumber(3));
	Args.Add(TEXT("RankCap"), FText::AsNumber(2));
	TestTrue(TEXT("Named SkillPoints/RankCap format"),
		FormatNamed(TEXT("Talent.Screen.SkillPointsFormat"), Args).EqualTo(FText::FromString(TEXT("Points de compétence : 3 - Rang max : 2"))));

	Args.Reset();
	Args.Add(TEXT("RemainingPoints"), FText::AsNumber(2));
	TestTrue(TEXT("Named generic success format"),
		FormatNamed(TEXT("Talent.Feedback.Success.Generic"), Args).EqualTo(FText::FromString(TEXT("Acquisition réussie. Points de talent restants : 2."))));

	Args.Reset();
	Args.Add(TEXT("TalentName"), FText::FromString(TEXT("Maîtrise critique")));
	Args.Add(TEXT("RemainingPoints"), FText::AsNumber(1));
	TestTrue(TEXT("Named TalentName/RemainingPoints format"),
		FormatNamed(TEXT("Talent.Feedback.Success.Named"), Args).EqualTo(FText::FromString(TEXT("« Maîtrise critique » a été acquis. Points de talent restants : 1."))));

	return true;
}

#endif
