#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/PlatformProcess.h"
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

	FString RunScript;
	if (LoadProjectFile(TEXT("Scripts/Localization/RunLocalizationSmoke.ps1"), RunScript, *this))
	{
		TestTrue(TEXT("Smoke script enables strict mode"), RunScript.Contains(TEXT("Set-StrictMode -Version Latest")));
		TestTrue(TEXT("Smoke script checks Unreal processes before execution"), RunScript.Contains(TEXT("Assert-NoUnrealProcess")));
		TestTrue(TEXT("Smoke script creates per-run reports"), RunScript.Contains(TEXT("Saved\\Localization\\UI-LOCALIZATION01.6")));
		TestTrue(TEXT("Smoke script captures native output"), RunScript.Contains(TEXT("Tee-Object -FilePath")));
		TestTrue(TEXT("Smoke script requires generated target metadata"), RunScript.Contains(TEXT("Game.locmeta")));
		TestFalse(TEXT("Smoke script does not contain ambiguous LASTEXITCODE interpolation"), RunScript.Contains(TEXT("$LASTEXITCODE:")));
	}

	FString RuntimeSmoke;
	if (LoadProjectFile(TEXT("Source/GrimrockPrototypeEditor/Private/Localization/UILocalizationEnglishSmokeCommandlet.cpp"), RuntimeSmoke, *this))
	{
		TestTrue(TEXT("Runtime smoke forces Game localization in editor context"), RuntimeSmoke.Contains(TEXT("Localization.ForceLoadGameLocalizationInEditor")));
		TestTrue(TEXT("Runtime smoke verifies Game localization is enabled"), RuntimeSmoke.Contains(TEXT("LocalizationManager.ShouldForceLoadGameLocalization()")));
		TestTrue(TEXT("Runtime smoke explicitly refreshes localization resources"), RuntimeSmoke.Contains(TEXT("LocalizationManager.RefreshResources()")));
		TestTrue(TEXT("Runtime smoke waits for localization reload"), RuntimeSmoke.Contains(TEXT("LocalizationManager.WaitForAsyncTasks()")));
	}

	FString TranslationScript;
	if (LoadProjectFile(TEXT("Scripts/Localization/ApplyEnglishSmokeTranslations.ps1"), TranslationScript, *this))
	{
		TestTrue(TEXT("Translation script enables strict mode"), TranslationScript.Contains(TEXT("Set-StrictMode -Version Latest")));
		TestTrue(TEXT("Translation script resolves source text from UI.csv"), TranslationScript.Contains(TEXT("Localization\\Source\\UI.csv")));
		TestTrue(TEXT("Translation script uses stable localization keys"), TranslationScript.Contains(TEXT("CharacterCreation.Gender.Female")));
		TestTrue(TEXT("Translation script reads CSV explicitly as UTF-8"), TranslationScript.Contains(TEXT("ReadAllText($CsvPath, [System.Text.Encoding]::UTF8)")));
		TestTrue(TEXT("Translation script verifies translated PO content"), TranslationScript.Contains(TEXT("English PO validation failed")));
		TestTrue(TEXT("Translation script writes through a temporary file"), TranslationScript.Contains(TEXT(".tmp")));
	}

	FString ScriptValidator;
	if (LoadProjectFile(TEXT("Scripts/Localization/TestLocalizationScripts.ps1"), ScriptValidator, *this))
	{
		TestTrue(TEXT("PowerShell validator enforces ASCII-only script sources"), ScriptValidator.Contains(TEXT("ASCII-only")));
		TestTrue(TEXT("PowerShell validator checks raw script bytes"), ScriptValidator.Contains(TEXT("ReadAllBytes")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION016PowerShellSyntaxTest,
	"Game.UI.Localization.LOCALIZATION01.PowerShellSyntax",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION016PowerShellSyntaxTest::RunTest(const FString&)
{
#if PLATFORM_WINDOWS
	const FString ScriptPath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/Localization/TestLocalizationScripts.ps1"));
	const FString Arguments = FString::Printf(
		TEXT("-NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%s\""),
		*ScriptPath);

	int32 ReturnCode = INDEX_NONE;
	FString StdOut;
	FString StdErr;
	const bool bLaunched = FPlatformProcess::ExecProcess(
		TEXT("powershell.exe"),
		*Arguments,
		&ReturnCode,
		&StdOut,
		&StdErr);

	if (!TestTrue(TEXT("PowerShell syntax validator launches"), bLaunched))
	{
		return false;
	}

	if (ReturnCode != 0)
	{
		AddError(FString::Printf(
			TEXT("Localization PowerShell syntax validation failed. ExitCode=%d\nSTDOUT:\n%s\nSTDERR:\n%s"),
			ReturnCode,
			*StdOut,
			*StdErr));
	}

	TestEqual(TEXT("Localization PowerShell scripts parse successfully"), ReturnCode, 0);
#else
	AddWarning(TEXT("PowerShell syntax validation is only executed on Windows."));
#endif

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
