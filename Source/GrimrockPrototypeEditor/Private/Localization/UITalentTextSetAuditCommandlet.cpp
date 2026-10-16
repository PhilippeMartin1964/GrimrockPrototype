#include "Localization/UITalentTextSetAuditCommandlet.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/RPGTalentDetailTextSet.h"

namespace
{
	const TCHAR* TalentTextSetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet.DA_RPGTalentDetailTextSet");

	struct FTalentTextAuditRow
	{
		int32 SourceIndex = INDEX_NONE;
		FName TextId = NAME_None;
		FString SourceText;
		FString Category;
		FString Disposition;
		bool bAlreadyInUiCsv = false;
	};

	FString CsvEscape(const FString& Value)
	{
		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""), ESearchCase::CaseSensitive);
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	void ClassifyTextId(const FName TextId, FString& OutCategory, FString& OutDisposition)
	{
		const FString Key = TextId.ToString();

		if (Key.StartsWith(TEXT("Talent.Recipe."), ESearchCase::CaseSensitive))
		{
			OutCategory = TEXT("Content.Recipe");
			OutDisposition = TEXT("DEFER_CONTENT_AUTHORITY");
			return;
		}

		const TCHAR* UiPrefixes[] =
		{
			TEXT("Talent.Section."),
			TEXT("Talent.Action."),
			TEXT("Talent.Type."),
			TEXT("Talent.Status."),
			TEXT("Talent.Node."),
			TEXT("Talent.Screen."),
			TEXT("Talent.Feedback."),
			TEXT("Talent.Detail."),
			TEXT("Talent.Effect."),
			TEXT("Talent.Context."),
			TEXT("Talent.Skill."),
			TEXT("Talent.Party."),
			TEXT("Talent.Reaction."),
			TEXT("Talent.Acquisition.")
		};

		for (const TCHAR* Prefix : UiPrefixes)
		{
			if (Key.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				OutCategory = TEXT("UI.Talent");
				OutDisposition = TEXT("MIGRATE_ST_UI");
				return;
			}
		}

		OutCategory = TEXT("Unclassified");
		OutDisposition = TEXT("REVIEW");
	}
}

UUITalentTextSetAuditCommandlet::UUITalentTextSetAuditCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UUITalentTextSetAuditCommandlet::Main(const FString& Params)
{
	(void)Params;

	const URPGTalentDetailTextSet* TextSet =
		LoadObject<URPGTalentDetailTextSet>(nullptr, TalentTextSetPath);
	if (!TextSet)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[UI-TEXT-CENTRAL01.2A] Unable to load production Talent TextSet: %s"),
			TalentTextSetPath);
		return 1;
	}

	if (TextSet->Entries.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[UI-TEXT-CENTRAL01.2A] Production Talent TextSet has no entries."));
		return 1;
	}

	const FString UiCsvFilename =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Localization/Source/UI.csv"));

	FString UiCsv;
	if (!FFileHelper::LoadFileToString(UiCsv, *UiCsvFilename))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[UI-TEXT-CENTRAL01.2A] Unable to read central UI CSV: %s"),
			*UiCsvFilename);
		return 1;
	}

	TArray<FTalentTextAuditRow> Rows;
	Rows.Reserve(TextSet->Entries.Num());

	int32 MigrateCount = 0;
	int32 DeferredContentCount = 0;
	int32 ReviewCount = 0;
	int32 AlreadyCentralCount = 0;
	int32 InvalidCount = 0;

	TSet<FName> SeenIds;
	int32 DuplicateCount = 0;

	for (int32 Index = 0; Index < TextSet->Entries.Num(); ++Index)
	{
		const FRPGTalentDetailTextEntry& Entry = TextSet->Entries[Index];

		FTalentTextAuditRow Row;
		Row.SourceIndex = Index;
		Row.TextId = Entry.TextId;
		Row.SourceText = Entry.Text.ToString();

		if (Entry.TextId.IsNone() || Entry.Text.IsEmpty())
		{
			++InvalidCount;
		}

		if (!Entry.TextId.IsNone())
		{
			if (SeenIds.Contains(Entry.TextId))
			{
				++DuplicateCount;
			}
			SeenIds.Add(Entry.TextId);
		}

		ClassifyTextId(Entry.TextId, Row.Category, Row.Disposition);

		const FString ExactCsvKey =
			FString::Printf(TEXT("\"%s\""), *Entry.TextId.ToString());
		Row.bAlreadyInUiCsv = !Entry.TextId.IsNone() && UiCsv.Contains(ExactCsvKey);
		if (Row.bAlreadyInUiCsv)
		{
			++AlreadyCentralCount;
		}

		if (Row.Disposition == TEXT("MIGRATE_ST_UI"))
		{
			++MigrateCount;
		}
		else if (Row.Disposition == TEXT("DEFER_CONTENT_AUTHORITY"))
		{
			++DeferredContentCount;
		}
		else
		{
			++ReviewCount;
		}

		Rows.Add(MoveTemp(Row));
	}

	Rows.Sort(
		[](const FTalentTextAuditRow& A, const FTalentTextAuditRow& B)
		{
			return A.TextId.ToString() < B.TextId.ToString();
		});

	const FString OutputDir =
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Localization/UI-TEXT-CENTRAL01.2A"));
	IFileManager::Get().MakeDirectory(*OutputDir, true);

	const FString CsvFilename =
		FPaths::Combine(OutputDir, TEXT("TalentTextSetAudit.csv"));
	const FString SummaryFilename =
		FPaths::Combine(OutputDir, TEXT("TalentTextSetAudit_summary.txt"));

	FString Csv;
	Csv += TEXT("SourceIndex,TextId,SourceText,Category,Disposition,AlreadyInUICsv\n");

	for (const FTalentTextAuditRow& Row : Rows)
	{
		Csv += FString::Printf(
			TEXT("%d,%s,%s,%s,%s,%s\n"),
			Row.SourceIndex,
			*CsvEscape(Row.TextId.ToString()),
			*CsvEscape(Row.SourceText),
			*CsvEscape(Row.Category),
			*CsvEscape(Row.Disposition),
			Row.bAlreadyInUiCsv ? TEXT("true") : TEXT("false"));
	}

	if (!FFileHelper::SaveStringToFile(
		Csv,
		*CsvFilename,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[UI-TEXT-CENTRAL01.2A] Failed to write audit CSV: %s"),
			*CsvFilename);
		return 1;
	}

	const FString Summary = FString::Printf(
		TEXT("UI-TEXT-CENTRAL01.2A - Talent TextSet read-only audit\n")
		TEXT("=====================================================\n\n")
		TEXT("Production asset          : %s\n")
		TEXT("Entries                   : %d\n")
		TEXT("MIGRATE_ST_UI             : %d\n")
		TEXT("DEFER_CONTENT_AUTHORITY   : %d\n")
		TEXT("REVIEW                     : %d\n")
		TEXT("Already present in UI.csv : %d\n")
		TEXT("Invalid entries           : %d\n")
		TEXT("Duplicate TextIds         : %d\n\n")
		TEXT("CSV                        : %s\n"),
		TalentTextSetPath,
		Rows.Num(),
		MigrateCount,
		DeferredContentCount,
		ReviewCount,
		AlreadyCentralCount,
		InvalidCount,
		DuplicateCount,
		*CsvFilename);

	if (!FFileHelper::SaveStringToFile(
		Summary,
		*SummaryFilename,
		FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[UI-TEXT-CENTRAL01.2A] Failed to write audit summary: %s"),
			*SummaryFilename);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[UI-TEXT-CENTRAL01.2A] Read-only audit completed."));
	UE_LOG(LogTemp, Display, TEXT("[UI-TEXT-CENTRAL01.2A] Entries=%d MIGRATE_ST_UI=%d DEFER_CONTENT_AUTHORITY=%d REVIEW=%d AlreadyInUICsv=%d Invalid=%d Duplicates=%d"),
		Rows.Num(),
		MigrateCount,
		DeferredContentCount,
		ReviewCount,
		AlreadyCentralCount,
		InvalidCount,
		DuplicateCount);
	UE_LOG(LogTemp, Display, TEXT("[UI-TEXT-CENTRAL01.2A] CSV=%s"), *CsvFilename);
	UE_LOG(LogTemp, Display, TEXT("[UI-TEXT-CENTRAL01.2A] Summary=%s"), *SummaryFilename);

	return InvalidCount == 0 && DuplicateCount == 0 ? 0 : 2;
}
