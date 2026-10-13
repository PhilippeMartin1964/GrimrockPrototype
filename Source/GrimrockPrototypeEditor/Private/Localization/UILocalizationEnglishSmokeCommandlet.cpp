#include "Localization/UILocalizationEnglishSmokeCommandlet.h"

#include "HAL/IConsoleManager.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"
#include "RPG/StatusEffects/GridStatusEffectPresentation.h"
#include "UI/UILocalizationText.h"

UUILocalizationEnglishSmokeCommandlet::UUILocalizationEnglishSmokeCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUILocalizationEnglishSmokeCommandlet::Main(const FString& Params)
{
	(void)Params;

	FInternationalization& Internationalization = FInternationalization::Get();
	FTextLocalizationManager& LocalizationManager = FTextLocalizationManager::Get();

	IConsoleVariable* ForceGameLocalizationCVar =
		IConsoleManager::Get().FindConsoleVariable(TEXT("Localization.ForceLoadGameLocalizationInEditor"));
	if (!ForceGameLocalizationCVar)
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-LOCALIZATION01.6] Missing Localization.ForceLoadGameLocalizationInEditor CVar."));
		return 1;
	}

	ForceGameLocalizationCVar->Set(1, ECVF_SetByCode);
	if (!LocalizationManager.ShouldForceLoadGameLocalization())
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-LOCALIZATION01.6] Game localization is still disabled in editor context."));
		return 1;
	}

	FInternationalization::FCultureStateSnapshot Snapshot;
	Internationalization.BackupCultureState(Snapshot);

	bool bSucceeded = true;
	auto CheckText =
		[&](const TCHAR* Culture, const TCHAR* Key, const TCHAR* Expected)
		{
			if (!Internationalization.SetCurrentCulture(Culture))
			{
				UE_LOG(LogTemp, Error, TEXT("[UI-LOCALIZATION01.6] Unable to activate culture '%s'."), Culture);
				bSucceeded = false;
				return;
			}

			// Commandlets run in editor context. Explicitly reload and wait for
			// project localization resources after changing culture so the
			// String Table display strings are evaluated against the new LocRes.
			LocalizationManager.RefreshResources();
			LocalizationManager.WaitForAsyncTasks();

			const FString Actual = FUILocalizationText::Get(Key).ToString();
			if (Actual != Expected)
			{
				UE_LOG(LogTemp, Error, TEXT("[UI-LOCALIZATION01.6] Culture='%s' Key='%s' Expected='%s' Actual='%s'."), Culture, Key, Expected, *Actual);
				bSucceeded = false;
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("[UI-LOCALIZATION01.6] PASS Culture='%s' Key='%s' Text='%s'."), Culture, Key, *Actual);
			}
		};

	CheckText(TEXT("fr"), TEXT("CharacterCreation.Gender.Female"), TEXT("Féminin"));
	CheckText(TEXT("fr"), TEXT("CharacterCreation.Identity.Title"), TEXT("Identité du personnage"));
	CheckText(TEXT("en"), TEXT("CharacterCreation.Gender.Female"), TEXT("Female"));
	CheckText(TEXT("en"), TEXT("CharacterCreation.Gender.Male"), TEXT("Male"));
	CheckText(TEXT("en"), TEXT("CharacterCreation.Identity.Title"), TEXT("Character Identity"));
	CheckText(TEXT("en"), TEXT("CharacterCreation.Wizard.Step.Summary"), TEXT("Summary"));
	CheckText(TEXT("en"), TEXT("CharacterCreation.Summary.State.Ready.Create"), TEXT("Ready to create character."));
	CheckText(TEXT("fr"), TEXT("Status.Tooltip.Duration"), TEXT("Durée : {Duration}"));
	CheckText(TEXT("en"), TEXT("Status.Tooltip.Duration"), TEXT("Duration: {Duration}"));
	CheckText(TEXT("en"), TEXT("Status.Control.BlockTranslation"), TEXT("Grid movement is blocked."));
	CheckText(TEXT("en"), TEXT("Status.Tooltip.PeriodicDamage"), TEXT("Periodic damage active."));

	auto CheckDuration =
		[&](const TCHAR* Culture, EGridStatusEffectDurationUnit Unit, int32 Count, const TCHAR* Expected)
		{
			if (!Internationalization.SetCurrentCulture(Culture))
			{
				UE_LOG(LogTemp, Error, TEXT("[UI-LOCALIZATION01.7] Unable to activate culture '%s'."), Culture);
				bSucceeded = false;
				return;
			}

			LocalizationManager.RefreshResources();
			LocalizationManager.WaitForAsyncTasks();

			const FString Actual = FGridStatusEffectPresentationBuilder::FormatDuration(Unit, Count).ToString();
			if (Actual != Expected)
			{
				UE_LOG(
					LogTemp,
					Error,
					TEXT("[UI-LOCALIZATION01.7] Culture='%s' StatusDuration Expected='%s' Actual='%s'."),
					Culture,
					Expected,
					*Actual);
				bSucceeded = false;
			}
			else
			{
				UE_LOG(
					LogTemp,
					Display,
					TEXT("[UI-LOCALIZATION01.7] PASS Culture='%s' StatusDuration Text='%s'."),
					Culture,
					*Actual);
			}
		};

	CheckDuration(TEXT("fr"), EGridStatusEffectDurationUnit::Rounds, 2, TEXT("2 manches"));
	CheckDuration(TEXT("en"), EGridStatusEffectDurationUnit::Rounds, 2, TEXT("2 rounds"));

	Internationalization.RestoreCultureState(Snapshot);
	LocalizationManager.RefreshResources();
	LocalizationManager.WaitForAsyncTasks();

	if (!bSucceeded)
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-LOCALIZATION01.6] English smoke test failed."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[UI-LOCALIZATION01.6] English smoke test passed."));
	return 0;
}
