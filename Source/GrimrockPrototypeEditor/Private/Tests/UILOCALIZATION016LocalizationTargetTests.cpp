#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION016
{
	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutContent, FAutomationTestBase& Test)
	{
		const FString Filename = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		if (!FFileHelper::LoadFileToString(OutContent, *Filename))
		{
			Test.AddError(FString::Printf(TEXT("Unable to read '%s'."), *Filename));
			return false;
		}
		return true;
	}

	void CheckCommonCultureContract(const FString& Content, const TCHAR* Label, FAutomationTestBase& Test)
	{
		Test.TestTrue(*FString::Printf(TEXT("%s declares French as native culture"), Label), Content.Contains(TEXT("NativeCulture=fr")));
		Test.TestTrue(*FString::Printf(TEXT("%s generates French"), Label), Content.Contains(TEXT("CulturesToGenerate=fr")));
		Test.TestTrue(*FString::Printf(TEXT("%s generates English"), Label), Content.Contains(TEXT("CulturesToGenerate=en")));
		Test.TestFalse(
			*FString::Printf(TEXT("%s uses neutral localization identifiers"), Label),
			Content.Contains(TEXT("Grimrock"), ESearchCase::IgnoreCase));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION016TargetContractTest,
	"Game.UI.Localization.LOCALIZATION01.LocalizationTargetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION016TargetContractTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION016;

	const TCHAR* TargetConfigs[] = {
		TEXT("Config/Localization/Game_Gather.ini"),
		TEXT("Config/Localization/Game_Export.ini"),
		TEXT("Config/Localization/Game_Import.ini"),
		TEXT("Config/Localization/Game_Compile.ini")
	};

	for (const TCHAR* RelativePath : TargetConfigs)
	{
		FString Content;
		if (!LoadProjectFile(RelativePath, Content, *this))
		{
			continue;
		}

		CheckCommonCultureContract(Content, RelativePath, *this);
		TestTrue(*FString::Printf(TEXT("%s targets Game.manifest"), RelativePath), Content.Contains(TEXT("ManifestName=Game.manifest")));
		TestTrue(*FString::Printf(TEXT("%s targets Game.archive"), RelativePath), Content.Contains(TEXT("ArchiveName=Game.archive")));
	}

	FString Gather;
	if (LoadProjectFile(TEXT("Config/Localization/Game_Gather.ini"), Gather, *this))
	{
		TestTrue(
			TEXT("Gather target is intentionally scoped to generated UI String Tables"),
			Gather.Contains(TEXT("IncludePathFilters=Content/Localization/StringTables/*")));
		TestFalse(
			TEXT("01.6 smoke target does not gather legacy source namespaces"),
			Gather.Contains(TEXT("CommandletClass=GatherTextFromSource")));
	}

	FString Export;
	if (LoadProjectFile(TEXT("Config/Localization/Game_Export.ini"), Export, *this))
	{
		TestTrue(TEXT("Export produces Game.po"), Export.Contains(TEXT("PortableObjectName=Game.po")));
		TestTrue(TEXT("Export step is enabled"), Export.Contains(TEXT("bExportLoc=true")));
	}

	FString Import;
	if (LoadProjectFile(TEXT("Config/Localization/Game_Import.ini"), Import, *this))
	{
		TestTrue(TEXT("Import consumes Game.po"), Import.Contains(TEXT("PortableObjectName=Game.po")));
		TestTrue(TEXT("Import step is enabled"), Import.Contains(TEXT("bImportLoc=true")));
	}

	FString Compile;
	if (LoadProjectFile(TEXT("Config/Localization/Game_Compile.ini"), Compile, *this))
	{
		TestTrue(TEXT("Compile produces Game.locres"), Compile.Contains(TEXT("ResourceName=Game.locres")));
		TestTrue(TEXT("Compile validates format patterns"), Compile.Contains(TEXT("bValidateFormatPatterns=true")));
	}

	FString DefaultGame;
	if (LoadProjectFile(TEXT("Config/DefaultGame.ini"), DefaultGame, *this))
	{
		TestTrue(
			TEXT("Runtime loads Game localization target"),
			DefaultGame.Contains(TEXT("+LocalizationPaths=%GAMEDIR%Content/Localization/Game")));
		TestTrue(TEXT("Packaging stages French localization"), DefaultGame.Contains(TEXT("+CulturesToStage=fr")));
		TestTrue(TEXT("Packaging stages English localization"), DefaultGame.Contains(TEXT("+CulturesToStage=en")));
		TestTrue(TEXT("Packaging includes French internationalization data"), DefaultGame.Contains(TEXT("InternationalizationPreset=EFIGS")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
