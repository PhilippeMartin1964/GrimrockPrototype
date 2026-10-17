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
			*FString::Printf(TEXT("%s keeps a neutral manifest identifier"), Label),
			Content.Contains(TEXT("ManifestName=Grimrock"), ESearchCase::IgnoreCase));
		Test.TestFalse(
			*FString::Printf(TEXT("%s keeps a neutral archive identifier"), Label),
			Content.Contains(TEXT("ArchiveName=Grimrock"), ESearchCase::IgnoreCase));
		Test.TestFalse(
			*FString::Printf(TEXT("%s keeps a neutral resource identifier"), Label),
			Content.Contains(TEXT("ResourceName=Grimrock"), ESearchCase::IgnoreCase));
		Test.TestFalse(
			*FString::Printf(TEXT("%s keeps a neutral PO identifier"), Label),
			Content.Contains(TEXT("PortableObjectName=Grimrock"), ESearchCase::IgnoreCase));
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
			TEXT("Gather target still includes generated UI String Tables"),
			Gather.Contains(TEXT("IncludePathFilters=Content/Localization/StringTables/*")));
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
		TestTrue(TEXT("Smoke script uses report-local GatherText config copies"), RunScript.Contains(TEXT("WorkingConfigRoot")));
		TestTrue(TEXT("Smoke script copies source-controlled configs before Unreal uses them"), RunScript.Contains(TEXT("Copy-Item -LiteralPath $GatherConfigSource")));
		TestTrue(TEXT("Smoke script hashes source-controlled configs"), RunScript.Contains(TEXT("Get-FileHash -LiteralPath $SourceConfigPath -Algorithm SHA256")));
		TestTrue(TEXT("Smoke script rejects source config mutation"), RunScript.Contains(TEXT("Source-controlled localization config was modified during smoke execution")));
		TestTrue(TEXT("Smoke script requires generated target metadata"), RunScript.Contains(TEXT("Game.locmeta")));
		TestTrue(TEXT("Smoke script validates fresh Gather output"), RunScript.Contains(TEXT("Assert-GatherIntegrity")));
		TestTrue(TEXT("Smoke script supports explicit generated-output materialization"), RunScript.Contains(TEXT("KeepGeneratedOutputs")));
		TestTrue(TEXT("Smoke script snapshots volatile generated outputs"), RunScript.Contains(TEXT("New-FileSnapshot")) && RunScript.Contains(TEXT("Game.csv")));
		TestTrue(TEXT("Smoke script restores volatile generated outputs after validation"), RunScript.Contains(TEXT("Restore-FileSnapshot")) && RunScript.Contains(TEXT("Volatile generated localization outputs restored after smoke validation")));
		TestTrue(TEXT("Smoke script rejects UMG TextBlock placeholders"), RunScript.Contains(TEXT("TextBlockDefaultValue")));
		TestTrue(TEXT("Smoke script rejects test fixtures in the player manifest"), RunScript.Contains(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Tests/")));
		TestTrue(TEXT("Smoke script rejects reflected enum metadata"), RunScript.Contains(TEXT("EGridSpellSchool")) && RunScript.Contains(TEXT("ERPGSkillGoverningAttribute")));
		TestTrue(TEXT("Smoke script requires an empty localization conflict report"), RunScript.Contains(TEXT("Game_Conflicts.txt")) && RunScript.Contains(TEXT("Localization conflict report is not empty")));
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
		TestTrue(TEXT("Translation script targets PO entries by msgctxt"), TranslationScript.Contains(TEXT("msgctxt")));
		TestTrue(TEXT("Translation script binds smoke keys to the ST_UI namespace"), TranslationScript.Contains(TEXT("ST_UI,{0}")));
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
	FUILOCALIZATION022FullGameGatherContractTest,
	"Game.UI.Localization.LOCALIZATION02.FullGameGatherContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION022FullGameGatherContractTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION016;

	FString Gather;
	if (!LoadProjectFile(TEXT("Config/Localization/Game_Gather.ini"), Gather, *this))
	{
		return false;
	}

	TestTrue(TEXT("Full Game gather reads player-facing C++ source"), Gather.Contains(TEXT("CommandletClass=GatherTextFromSource")));
	TestTrue(TEXT("Source gather is limited to the runtime module"), Gather.Contains(TEXT("SearchDirectoryPaths=Source/GrimrockPrototype")));
	TestTrue(TEXT("Source gather includes headers"), Gather.Contains(TEXT("FileNameFilters=*.h")));
	TestTrue(TEXT("Source gather includes implementation files"), Gather.Contains(TEXT("FileNameFilters=*.cpp")));
	TestTrue(TEXT("Source gather excludes runtime Automation tests"), Gather.Contains(TEXT("ExcludePathFilters=Source/GrimrockPrototype/Private/Tests/*")));
	TestTrue(TEXT("Source gather excludes the PIE progression simulator"), Gather.Contains(TEXT("ExcludePathFilters=Source/GrimrockPrototype/Private/RPG/RPGProgressionPIESimulator.*")));
	TestFalse(TEXT("Full Game gather does not search the Editor module"), Gather.Contains(TEXT("SearchDirectoryPaths=Source/GrimrockPrototypeEditor")));
	TestFalse(TEXT("Full Game gather does not enable broad metadata gathering"), Gather.Contains(TEXT("CommandletClass=GatherTextFromMetaData")));

	const TCHAR* RequiredAssetScopes[] = {
		TEXT("IncludePathFilters=Content/Localization/StringTables/*"),
		TEXT("IncludePathFilters=Content/GrimrockPrototype/Core/DataAssets/*"),
		TEXT("IncludePathFilters=Content/GrimrockPrototype/Monsters/Categories/*"),
		TEXT("IncludePathFilters=Content/GrimrockPrototype/Monsters/*/Data/*"),
		TEXT("IncludePathFilters=Content/GrimrockPrototype/Blueprints/UI/*"),
		TEXT("IncludePathFilters=Content/GrimrockPrototype/Blueprints/Runtime/*"),
		TEXT("IncludePathFilters=Content/GrimrockPrototype/Maps/*")
	};
	for (const TCHAR* RequiredScope : RequiredAssetScopes)
	{
		TestTrue(*FString::Printf(TEXT("Full Game asset gather contains '%s'"), RequiredScope), Gather.Contains(RequiredScope));
	}

	TestTrue(TEXT("Asset gather excludes the dedicated test fixture subtree"), Gather.Contains(TEXT("ExcludePathFilters=Content/GrimrockPrototype/Core/DataAssets/Tests/*")));
	TestFalse(TEXT("Asset gather no longer relies on the test-note filename"), Gather.Contains(TEXT("DA_Item_TestNote*")));
	TestFalse(TEXT("Asset gather no longer relies on readable filename wildcards"), Gather.Contains(TEXT("Readables/*_Test*")));
	TestTrue(TEXT("Asset gather includes uasset packages"), Gather.Contains(TEXT("PackageFileNameFilters=*.uasset")));
	TestTrue(TEXT("Asset gather includes runtime maps"), Gather.Contains(TEXT("PackageFileNameFilters=*.umap")));
	TestTrue(TEXT("Editor-only serialized data stays excluded"), Gather.Contains(TEXT("ShouldGatherFromEditorOnlyData=false")));
	TestFalse(TEXT("Editor Blueprints stay outside the player target"), Gather.Contains(TEXT("IncludePathFilters=Content/GrimrockPrototype/Blueprints/Editor/*")));
	TestFalse(TEXT("Dungeon editor icons stay outside the player target"), Gather.Contains(TEXT("IncludePathFilters=Content/GrimrockPrototype/Icons/DungeonEditorIcons/*")));

	FString UiCsv;
	if (LoadProjectFile(TEXT("Localization/Source/UI.csv"), UiCsv, *this))
	{
		const TCHAR* RequiredSpellSchoolKeys[] = {
			TEXT("Spellbook.School.None"),
			TEXT("Spellbook.School.Arcane"),
			TEXT("Spellbook.School.Fire"),
			TEXT("Spellbook.School.Frost"),
			TEXT("Spellbook.School.Air"),
			TEXT("Spellbook.School.Earth"),
			TEXT("Spellbook.School.Life"),
			TEXT("Spellbook.School.Death"),
			TEXT("Spellbook.School.Protection")
		};
		for (const TCHAR* RequiredKey : RequiredSpellSchoolKeys)
		{
			TestTrue(*FString::Printf(TEXT("UI.csv contains '%s'"), RequiredKey), UiCsv.Contains(RequiredKey));
		}

		const TCHAR* RequiredSkillDetailKeys[] = {
			TEXT("UI.Section.Description"),
			TEXT("UI.Section.Usage"),
			TEXT("UI.Section.Progression"),
			TEXT("Skill.Section.Training"),
			TEXT("Skill.Section.BuildAdvice"),
			TEXT("Skill.Section.Check"),
			TEXT("Skill.Section.Affinities"),
			TEXT("Skill.Section.Synergies"),
			TEXT("Skill.Check.Rank"),
			TEXT("Skill.Check.AttributeModifier"),
			TEXT("Skill.Check.BaseBonus"),
			TEXT("Skill.Affinity.RecommendedClasses"),
			TEXT("Skill.Affinity.UsefulClasses"),
			TEXT("Skill.Affinity.DiscouragedClasses"),
			TEXT("Skill.Progression.CurrentRankCap"),
			TEXT("Skill.Progression.RemainingSkillPoints"),
			TEXT("Skill.Progression.NextRank"),
			TEXT("Skill.Progression.NextRankCost")
		};
		for (const TCHAR* RequiredKey : RequiredSkillDetailKeys)
		{
			TestTrue(*FString::Printf(TEXT("UI.csv contains Skill detail key '%s'"), RequiredKey), UiCsv.Contains(RequiredKey));
		}
	}

	FString SpellbookEntrySource;
	if (LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/UI/GridSpellbookEntryWidget.cpp"), SpellbookEntrySource, *this))
	{
		TestTrue(TEXT("Spell school presentation reads ST_UI vocabulary"), SpellbookEntrySource.Contains(TEXT("FUILocalizationText::Get")));
		TestTrue(TEXT("Spell school presentation references the Fire key"), SpellbookEntrySource.Contains(TEXT("Spellbook.School.Fire")));
		TestFalse(TEXT("Spell school presentation no longer depends on enum metadata"), SpellbookEntrySource.Contains(TEXT("GetDisplayNameTextByValue")));
	}

	FString SkillEntrySource;
	if (LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillEntryWidget.cpp"), SkillEntrySource, *this))
	{
		TestTrue(TEXT("Skill attribute presentation reads ST_UI vocabulary"), SkillEntrySource.Contains(TEXT("FUILocalizationText::Get")));
		TestTrue(TEXT("Skill attribute presentation reuses the Dexterity key"), SkillEntrySource.Contains(TEXT("CharacterCreation.Attribute.Dexterity")));
		TestFalse(TEXT("Skill attribute presentation no longer depends on enum metadata"), SkillEntrySource.Contains(TEXT("GetDisplayNameTextByValue")));
	}

	FString SkillDetailSource;
	if (LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/UI/GridSkillDetailWidget.cpp"), SkillDetailSource, *this))
	{
		TestTrue(TEXT("Skill detail static labels read ST_UI vocabulary"), SkillDetailSource.Contains(TEXT("FUILocalizationText::Get")));
		TestTrue(TEXT("Skill detail uses shared UI section vocabulary"), SkillDetailSource.Contains(TEXT("UI.Section.Usage")));
		TestTrue(TEXT("Skill detail references the centralized rank label"), SkillDetailSource.Contains(TEXT("Skill.Check.Rank")));
		TestTrue(TEXT("Skill detail references the centralized attribute modifier label"), SkillDetailSource.Contains(TEXT("Skill.Check.AttributeModifier")));
	}

	FString MaterializedManifest;
	if (LoadProjectFile(TEXT("Content/Localization/Game/Game.manifest"), MaterializedManifest, *this))
	{
		TestFalse(TEXT("Materialized manifest excludes UMG TextBlock placeholder keys"), MaterializedManifest.Contains(TEXT("TextBlockDefaultValue")));
		TestFalse(TEXT("Materialized manifest excludes UMG TextBlock placeholder source text"), MaterializedManifest.Contains(TEXT("\"Text\": \"Text Block\"")));
		TestFalse(TEXT("Materialized manifest excludes the test fixture subtree"), MaterializedManifest.Contains(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Tests/")));
		TestFalse(TEXT("Materialized manifest excludes the test-note fixture"), MaterializedManifest.Contains(TEXT("DA_Item_TestNote")));
		TestFalse(TEXT("Materialized manifest excludes the test book-page fixture"), MaterializedManifest.Contains(TEXT("DA_BookPage_Test")));
		TestFalse(TEXT("Materialized manifest excludes the test scroll fixture"), MaterializedManifest.Contains(TEXT("DA_Scroll_Test")));
		TestFalse(TEXT("Materialized manifest excludes reflected spell-school metadata"), MaterializedManifest.Contains(TEXT("EGridSpellSchool")));
		TestFalse(TEXT("Materialized manifest excludes reflected skill-attribute metadata"), MaterializedManifest.Contains(TEXT("ERPGSkillGoverningAttribute")));
		TestTrue(TEXT("Materialized manifest contains the stable Dexterity UI key"), MaterializedManifest.Contains(TEXT("CharacterCreation.Attribute.Dexterity")));
		TestTrue(TEXT("Materialized manifest contains the stable spell-school UI key"), MaterializedManifest.Contains(TEXT("Spellbook.School.Fire")));
	}

	FString ConflictReport;
	if (LoadProjectFile(TEXT("Content/Localization/Game/Game_Conflicts.txt"), ConflictReport, *this))
	{
		ConflictReport.TrimStartAndEndInline();
		TestTrue(TEXT("Materialized localization conflict report remains empty"), ConflictReport.IsEmpty());
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
