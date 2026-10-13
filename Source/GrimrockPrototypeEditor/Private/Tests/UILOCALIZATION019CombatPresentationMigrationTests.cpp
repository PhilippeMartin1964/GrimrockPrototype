#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Internationalization/Regex.h"
#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace UILOCALIZATION019
{
	const TCHAR* MigratedSources[] = {
		TEXT("Source/GrimrockPrototype/Private/UI/GridCombatHudWidget.cpp"),
		TEXT("Source/GrimrockPrototype/Private/UI/GridCombatActionPanelWidget.cpp"),
		TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp")
	};

	const TCHAR* RequiredKeys[] = {
		TEXT("Combat.Action.ThrowName"),
		TEXT("Combat.Action.Unavailable.Character"),
		TEXT("Combat.Action.Unavailable.EquipmentMissing"),
		TEXT("Combat.Action.Unavailable.Generic"),
		TEXT("Combat.Action.Unavailable.ItemDefinition"),
		TEXT("Combat.Action.Unavailable.NoInventoryCopies"),
		TEXT("Combat.Action.Unavailable.QuickItemMissingAction"),
		TEXT("Combat.Action.Unavailable.Stale"),
		TEXT("Combat.Action.Unavailable.StatusBlocksSpells"),
		TEXT("Combat.Action.Unavailable.ThrowRequirement"),
		TEXT("Combat.Cost.ActionPoints"),
		TEXT("Combat.Cost.ActionPointsMana"),
		TEXT("Combat.Cost.WithQuantity"),
		TEXT("Combat.Cost.WithQuantityConsumed"),
		TEXT("Combat.EndTurn.EnemyTurn"),
		TEXT("Combat.EndTurn.InactiveCombat"),
		TEXT("Combat.EndTurn.PartyMotion"),
		TEXT("Combat.EndTurn.Resolution"),
		TEXT("Combat.EndTurn.Tooltip"),
		TEXT("Combat.Hotbar.EmptyTooltip"),
		TEXT("Combat.Hotbar.Tooltip.Cost"),
		TEXT("Combat.Hotbar.Tooltip.KeepsItem"),
		TEXT("Combat.Hotbar.Tooltip.RemoveShortcut"),
		TEXT("Combat.Hotbar.Tooltip.Unavailable"),
		TEXT("Combat.Initiative.Round"),
		TEXT("Combat.Resource.ActionPoints"),
		TEXT("Combat.Resource.Health"),
		TEXT("Combat.Resource.MobilityActionPoints"),
		TEXT("Combat.State.Active"),
		TEXT("Combat.State.Defeated"),
		TEXT("Combat.State.Incapacitated"),
		TEXT("Combat.Targeting.Invalid.ActionNoLongerAvailable"),
		TEXT("Combat.Targeting.Invalid.ActionUnavailable"),
		TEXT("Combat.Targeting.Invalid.AreaNoEligibleTarget"),
		TEXT("Combat.Targeting.Invalid.CellNoEligibleTarget"),
		TEXT("Combat.Targeting.Invalid.CellOutsideLevel"),
		TEXT("Combat.Targeting.Invalid.Default"),
		TEXT("Combat.Targeting.Invalid.DestinationRequiresWalkableCell"),
		TEXT("Combat.Targeting.Invalid.InvalidPartyFacing"),
		TEXT("Combat.Targeting.Invalid.LevelOrPartyUnavailable"),
		TEXT("Combat.Targeting.Invalid.LineOfSightBlocked"),
		TEXT("Combat.Targeting.Invalid.NoSurfaceReaction"),
		TEXT("Combat.Targeting.Invalid.OccupancyUnavailable"),
		TEXT("Combat.Targeting.Invalid.OutOfRange"),
		TEXT("Combat.Targeting.Invalid.RelocationBlocked"),
		TEXT("Combat.Targeting.Invalid.TrapRequiresWalkableCell"),
		TEXT("Combat.Targeting.Invalid.UnsupportedTargetingPolicy")
	};

	const TCHAR* ForbiddenPlayerFacingLiterals[] = {
		TEXT("FText::FromString(TEXT(\"Personnage indisponible.\"))"),
		TEXT("FText::FromString(TEXT(\"Définition d'objet indisponible.\"))"),
		TEXT("FText::FromString(TEXT(\"Aucun exemplaire dans l'inventaire.\"))"),
		TEXT("FText::FromString(TEXT(\"Cet objet est trop lourd ou ne peut pas être lancé d'une seule main.\"))"),
		TEXT("FText::FromString(TEXT(\"Déposez ici un objet de l'inventaire, un sort ou une capacité.\"))"),
		TEXT("FText::FromString(TEXT(\"ACTIF\"))"),
		TEXT("FText::FromString(TEXT(\"INCAPACITÉ\"))"),
		TEXT("FText::FromString(TEXT(\"VAINCU\"))"),
		TEXT("FText::FromString(TEXT(\"Combat inactif\"))"),
		TEXT("FText::FromString(TEXT(\"Tour ennemi\"))"),
		TEXT("FText::FromString(TEXT(\"Déplacement en cours\"))"),
		TEXT("FText::FromString(TEXT(\"Résolution en cours\"))"),
		TEXT("FText::FromString(TEXT(\"Terminer le tour actif\"))"),
		TEXT("TEXT(\"ROUND %d\")"),
		TEXT("FText::FromString(TEXT(\"Un effet de statut empêche l'utilisation des sorts.\"))"),
		TEXT("TEXT(\"Cible invalide.\")")
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION019CombatPresentationMigrationTest,
	"Game.UI.Localization.LOCALIZATION01.CombatPresentationCsvCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION019CombatPresentationMigrationTest::RunTest(const FString&)
{
	using namespace UILOCALIZATION019;

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
			*FString::Printf(TEXT("UI.csv contains Combat key '%s'"), RequiredKey),
			CsvKeys.Contains(RequiredKey));
	}

	const FRegexPattern KeyPattern(TEXT("TEXT\\(\\\"(Combat\\.[A-Za-z0-9.]+)\\\"\\)"));
	TSet<FString> ReferencedKeys;
	FString HudSource;

	for (const TCHAR* RelativePath : MigratedSources)
	{
		FString Source;
		const FString Filename = FPaths::Combine(FPaths::ProjectDir(), RelativePath);
		if (!FFileHelper::LoadFileToString(Source, *Filename))
		{
			AddError(FString::Printf(TEXT("Unable to read migrated Combat source '%s'."), *Filename));
			continue;
		}

		if (FString(RelativePath).EndsWith(TEXT("GridCombatHudWidget.cpp")))
		{
			HudSource = Source;
		}

		for (const TCHAR* ForbiddenLiteral : ForbiddenPlayerFacingLiterals)
		{
			TestFalse(
				*FString::Printf(TEXT("%s no longer owns migrated player-facing wording"), RelativePath),
				Source.Contains(ForbiddenLiteral));
		}

		FRegexMatcher Matcher(KeyPattern, Source);
		while (Matcher.FindNext())
		{
			ReferencedKeys.Add(Matcher.GetCaptureGroup(1));
		}
	}

	TestEqual(
		TEXT("Combat sources reference exactly the authored Combat keys"),
		ReferencedKeys.Num(),
		static_cast<int32>(UE_ARRAY_COUNT(RequiredKeys)));

	for (const TCHAR* RequiredKey : RequiredKeys)
	{
		TestTrue(
			*FString::Printf(TEXT("Migrated Combat sources reference key '%s'"), RequiredKey),
			ReferencedKeys.Contains(RequiredKey));
	}

	TestTrue(
		TEXT("Assigned hotbar tooltip composes localized sections as FText"),
		HudSource.Contains(TEXT("TArray<FText> Sections")));
	TestFalse(
		TEXT("Assigned hotbar tooltip no longer flattens authored DisplayName through FString"),
		HudSource.Contains(TEXT("Definition.DisplayName.ToString()")));

	FString EnglishFixture;
	const FString EnglishFixturePath =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Scripts/Localization/ApplyEnglishSmokeTranslations.ps1"));
	if (FFileHelper::LoadFileToString(EnglishFixture, *EnglishFixturePath))
	{
		TestTrue(TEXT("English fixture covers Combat active state"), EnglishFixture.Contains(TEXT("Combat.State.Active")));
		TestTrue(TEXT("English fixture covers Combat round"), EnglishFixture.Contains(TEXT("Combat.Initiative.Round")));
		TestTrue(TEXT("English fixture covers Combat targeting"), EnglishFixture.Contains(TEXT("Combat.Targeting.Invalid.OutOfRange")));
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
		TestTrue(TEXT("Runtime smoke covers Combat HUD"), SmokeCommandlet.Contains(TEXT("Combat.Hotbar.EmptyTooltip")));
		TestTrue(TEXT("Runtime smoke covers Combat action points"), SmokeCommandlet.Contains(TEXT("Combat.Resource.ActionPoints")));
		TestTrue(TEXT("Runtime smoke checks English active state"), SmokeCommandlet.Contains(TEXT("TEXT(\"ACTIVE\")")));
	}
	else
	{
		AddError(FString::Printf(TEXT("Unable to read runtime smoke commandlet '%s'."), *SmokePath));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
