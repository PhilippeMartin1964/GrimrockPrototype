#include "Localization/UITalentTextSetCentralizeCommandlet.h"

#include "Localization/UILocalizationCsvAuthoring.h"
#include "Misc/FileHelper.h"
#include "UI/RPGTalentDetailTextSet.h"

namespace
{
	const TCHAR* TalentTextSetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet.DA_RPGTalentDetailTextSet");

	FString CsvEscape(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""), ESearchCase::CaseSensitive);
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	FString SerializeCsv(const TArray<FUILocalizationCsvEntry>& Entries)
	{
		FString Csv = TEXT("Key,SourceString,Comment\n");
		for (const FUILocalizationCsvEntry& Entry : Entries)
		{
			Csv += FString::Printf(
				TEXT("%s,%s,%s\n"),
				*CsvEscape(Entry.Key),
				*CsvEscape(Entry.SourceString),
				*CsvEscape(Entry.Comment));
		}
		return Csv;
	}

	bool IsDeferredRecipe(const FString& TextId)
	{
		return TextId.StartsWith(TEXT("Talent.Recipe."), ESearchCase::CaseSensitive);
	}

	FString ResolveCentralKey(const FString& LegacyTextId)
	{
		if (LegacyTextId == TEXT("Talent.Section.Usage"))
		{
			return TEXT("UI.Section.Usage");
		}
		if (LegacyTextId == TEXT("Talent.Value.Permanent"))
		{
			return TEXT("Status.Duration.Permanent");
		}
		return LegacyTextId;
	}
}

UUITalentTextSetCentralizeCommandlet::UUITalentTextSetCentralizeCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUITalentTextSetCentralizeCommandlet::Main(const FString& Params)
{
	(void)Params;

	const URPGTalentDetailTextSet* TextSet =
		LoadObject<URPGTalentDetailTextSet>(nullptr, TalentTextSetPath);
	if (!TextSet)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-TEXT-CENTRAL01.2B] Unable to load production Talent TextSet: %s"),
			TalentTextSetPath);
		return 1;
	}

	TArray<FUILocalizationCsvEntry> UiEntries;
	FString Error;
	if (!FUILocalizationCsvAuthoring::LoadAndValidateProductionCsv(UiEntries, Error))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-TEXT-CENTRAL01.2B] Unable to load UI.csv: %s"), *Error);
		return 1;
	}

	TMap<FString, int32> ExistingIndexByKey;
	for (int32 Index = 0; Index < UiEntries.Num(); ++Index)
	{
		ExistingIndexByKey.Add(UiEntries[Index].Key, Index);
	}

	TArray<FUILocalizationCsvEntry> Additions;
	int32 SharedReuseCount = 0;
	int32 DeferredRecipeCount = 0;
	int32 AlreadyPresentCount = 0;
	int32 ConflictCount = 0;

	for (const FRPGTalentDetailTextEntry& Entry : TextSet->Entries)
	{
		if (Entry.TextId.IsNone() || Entry.Text.IsEmpty())
		{
			++ConflictCount;
			UE_LOG(LogTemp, Error,
				TEXT("[UI-TEXT-CENTRAL01.2B] Invalid legacy TextSet entry."));
			continue;
		}

		const FString LegacyKey = Entry.TextId.ToString();
		const FString LegacySource = Entry.Text.ToString();

		if (IsDeferredRecipe(LegacyKey))
		{
			++DeferredRecipeCount;
			continue;
		}

		const FString CentralKey = ResolveCentralKey(LegacyKey);
		const bool bSharedReuse = CentralKey != LegacyKey;
		if (bSharedReuse)
		{
			++SharedReuseCount;
		}

		if (const int32* ExistingIndex = ExistingIndexByKey.Find(CentralKey))
		{
			const FUILocalizationCsvEntry& Existing = UiEntries[*ExistingIndex];
			if (Existing.SourceString != LegacySource)
			{
				++ConflictCount;
				UE_LOG(LogTemp, Error,
					TEXT("[UI-TEXT-CENTRAL01.2B] Source conflict: legacy '%s' -> central '%s'."),
					*LegacyKey, *CentralKey);
			}
			else if (!bSharedReuse)
			{
				++AlreadyPresentCount;
			}
			continue;
		}

		if (bSharedReuse)
		{
			++ConflictCount;
			UE_LOG(LogTemp, Error,
				TEXT("[UI-TEXT-CENTRAL01.2B] Required shared key is missing: %s -> %s"),
				*LegacyKey, *CentralKey);
			continue;
		}

		FUILocalizationCsvEntry NewEntry;
		NewEntry.Key = CentralKey;
		NewEntry.SourceString = LegacySource;
		NewEntry.Comment =
			CentralKey.StartsWith(TEXT("Talent.Target."), ESearchCase::CaseSensitive)
				? TEXT("Vocabulaire de ciblage présenté dans l'interface Talent")
				: CentralKey.StartsWith(TEXT("Talent.Value."), ESearchCase::CaseSensitive)
					? TEXT("Valeur textuelle présentée dans l'interface Talent")
					: TEXT("Vocabulaire UI Talent centralisé depuis DA_RPGTalentDetailTextSet");
		Additions.Add(MoveTemp(NewEntry));
	}

	if (ConflictCount > 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-TEXT-CENTRAL01.2B] Migration aborted. Conflicts=%d. UI.csv was not modified."),
			ConflictCount);
		return 2;
	}

	Additions.Sort(
		[](const FUILocalizationCsvEntry& A, const FUILocalizationCsvEntry& B)
		{
			return A.Key < B.Key;
		});

	UiEntries.Append(Additions);
	const FString NewCsv = SerializeCsv(UiEntries);

	TArray<FUILocalizationCsvEntry> ValidationEntries;
	if (!FUILocalizationCsvAuthoring::ParseAndValidateCsv(NewCsv, ValidationEntries, Error))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-TEXT-CENTRAL01.2B] Generated UI.csv failed validation: %s"),
			*Error);
		return 3;
	}

	const FString Filename = FUILocalizationCsvAuthoring::SourceFilename();
	if (!FFileHelper::SaveStringToFile(
		NewCsv,
		*Filename,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-TEXT-CENTRAL01.2B] Failed to write UI.csv: %s"),
			*Filename);
		return 4;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[UI-TEXT-CENTRAL01.2B] Talent UI vocabulary centralization succeeded."));
	UE_LOG(LogTemp, Display,
		TEXT("[UI-TEXT-CENTRAL01.2B] Added=%d SharedReuse=%d DeferredRecipes=%d AlreadyPresent=%d Conflicts=%d"),
		Additions.Num(),
		SharedReuseCount,
		DeferredRecipeCount,
		AlreadyPresentCount,
		ConflictCount);
	UE_LOG(LogTemp, Display,
		TEXT("[UI-TEXT-CENTRAL01.2B] Source=%s"),
		*Filename);

	return 0;
}
