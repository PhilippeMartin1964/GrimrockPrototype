#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION0112
{
	const TCHAR* RequiredKeys[] = {
		TEXT("Combat.Targeting.Invalid.Default"),
		TEXT("Combat.Targeting.SelectDungeonCell"),
		TEXT("Interaction.Throw.CharacterChanged"),
		TEXT("Interaction.Throw.DefinitionOrCharacterUnavailable"),
		TEXT("Interaction.Throw.EmptyMainHand"),
		TEXT("Interaction.Throw.Failed"),
		TEXT("Interaction.Throw.HeldItemChanged"),
		TEXT("Interaction.Throw.ItemMissingInventory"),
		TEXT("Interaction.Throw.NoActiveLevel"),
		TEXT("Interaction.Throw.NoItemSelected"),
		TEXT("Interaction.Throw.NoTargetUnderCursor"),
		TEXT("Interaction.Throw.PlacementFailed"),
		TEXT("Interaction.Throw.RequiresOneHand"),
		TEXT("Interaction.Throw.TargetTooFarOrInvalid"),
		TEXT("Interaction.Throw.TooHeavy"),
		TEXT("Progression.LevelUp.CharacterFallback")
	};

	const TCHAR* PlayerControllerPath =
		TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPlayerController.cpp");
	const TCHAR* LevelUpNotificationPath =
		TEXT("Source/GrimrockPrototype/Private/RPG/RPGLevelUpNotificationSubsystem.cpp");
	const TCHAR* ItemContextActionPath =
		TEXT("Source/GrimrockPrototype/Private/Runtime/GridItemContextActionLibrary.cpp");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION0112FinalP0ClosureTest,
	"Game.UI.Localization.LOCALIZATION01.FinalP0Closure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION0112FinalP0ClosureTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION0112;

	TArray<FUILocalizationCsvEntry> Entries;
	FString Error;
	if (!FUILocalizationCsvAuthoring::LoadAndValidateProductionCsv(Entries, Error))
	{
		AddError(Error);
		return false;
	}

	TSet<FString> CsvKeys;
	for (const FUILocalizationCsvEntry& Entry : Entries)
	{
		CsvKeys.Add(Entry.Key);
	}

	for (const TCHAR* RequiredKey : RequiredKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("UI.csv contains final P0 key '%s'"), RequiredKey),
			CsvKeys.Contains(RequiredKey));
	}

	auto LoadSource =
		[this](const TCHAR* RelativePath, FString& OutSource) -> bool
		{
			const FString Filename = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
			if (!FFileHelper::LoadFileToString(OutSource, *Filename))
			{
				AddError(FString::Printf(TEXT("Unable to read final P0 source '%s'."), *Filename));
				return false;
			}
			return true;
		};

	FString PlayerControllerSource;
	FString LevelUpSource;
	FString ItemContextSource;
	if (!LoadSource(PlayerControllerPath, PlayerControllerSource) ||
		!LoadSource(LevelUpNotificationPath, LevelUpSource) ||
		!LoadSource(ItemContextActionPath, ItemContextSource))
	{
		return false;
	}

	TestFalse(
		TEXT("PlayerController owns no raw FText::FromString(TEXT(...)) player wording"),
		PlayerControllerSource.Contains(TEXT("FText::FromString(TEXT(")));
	TestFalse(
		TEXT("PlayerController owns no raw formatted FText source pattern"),
		PlayerControllerSource.Contains(TEXT("FText::Format(FText::FromString")));
	TestTrue(
		TEXT("Physical throw TooHeavy resolves through shared UI key"),
		PlayerControllerSource.Contains(TEXT("Interaction.Throw.TooHeavy")));
	TestTrue(
		TEXT("Combat targeting missing-cell feedback resolves through UI key"),
		PlayerControllerSource.Contains(TEXT("Combat.Targeting.SelectDungeonCell")));
	TestTrue(
		TEXT("Combat targeting invalid fallback reuses the existing localized key"),
		PlayerControllerSource.Contains(TEXT("Combat.Targeting.Invalid.Default")));

	TestFalse(
		TEXT("Level-up toast no longer owns raw Personnage %d fallback"),
		LevelUpSource.Contains(TEXT("Personnage %d")));
	TestTrue(
		TEXT("Level-up toast resolves character fallback through UI key"),
		LevelUpSource.Contains(TEXT("Progression.LevelUp.CharacterFallback")));

	TestFalse(
		TEXT("Item context action no longer owns a second ThrowTooHeavy NSLOCTEXT authority"),
		ItemContextSource.Contains(TEXT("NSLOCTEXT(\"GridItemActions\", \"ThrowTooHeavy\"")));
	TestTrue(
		TEXT("Item context action reuses physical throw TooHeavy key"),
		ItemContextSource.Contains(TEXT("Interaction.Throw.TooHeavy")));

	FString EnglishFixture;
	const FString EnglishFixturePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/Localization/ApplyEnglishSmokeTranslations.ps1"));
	if (FFileHelper::LoadFileToString(EnglishFixture, *EnglishFixturePath))
	{
		TestTrue(TEXT("English fixture covers physical throw"), EnglishFixture.Contains(TEXT("Interaction.Throw.NoActiveLevel")));
		TestTrue(TEXT("English fixture covers throw formatting"), EnglishFixture.Contains(TEXT("Interaction.Throw.TooHeavy")));
		TestTrue(TEXT("English fixture covers targeting instruction"), EnglishFixture.Contains(TEXT("Combat.Targeting.SelectDungeonCell")));
		TestTrue(TEXT("English fixture covers level-up fallback"), EnglishFixture.Contains(TEXT("Progression.LevelUp.CharacterFallback")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read English smoke fixture '%s'."), *EnglishFixturePath));
	}

	FString SmokeCommandlet;
	const FString SmokePath = FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Source/GrimrockPrototypeEditor/Private/Localization/UILocalizationEnglishSmokeCommandlet.cpp"));
	if (FFileHelper::LoadFileToString(SmokeCommandlet, *SmokePath))
	{
		TestTrue(TEXT("Runtime smoke covers final throw P0"), SmokeCommandlet.Contains(TEXT("Interaction.Throw.NoTargetUnderCursor")));
		TestTrue(TEXT("Runtime smoke covers final targeting P0"), SmokeCommandlet.Contains(TEXT("Combat.Targeting.SelectDungeonCell")));
		TestTrue(TEXT("Runtime smoke covers final progression P0"), SmokeCommandlet.Contains(TEXT("Progression.LevelUp.CharacterFallback")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read runtime smoke commandlet '%s'."), *SmokePath));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
