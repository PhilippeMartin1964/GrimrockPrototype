#include "Localization/UILocalizationEnglishSmokeCommandlet.h"

#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"
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
