#include "Localization/UILocalizationCsvAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/TextKey.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace UILocalizationCsvAuthoringPrivate
{
	const TCHAR* SourcePath = TEXT("Localization/Source/UI.csv");
	const TCHAR* ProductionPackageName = TEXT("/Game/Localization/StringTables/ST_UI");
	const TCHAR* ProductionAssetName = TEXT("ST_UI");
	const TCHAR* ProductionObjectPath = TEXT("/Game/Localization/StringTables/ST_UI.ST_UI");
	const FName CommentMetaDataId(TEXT("Comment"));

	bool IsBlankRow(const TArray<const TCHAR*>& Row)
	{
		for (const TCHAR* Cell : Row)
		{
			if (Cell && *Cell)
			{
				return false;
			}
		}
		return true;
	}

	/** Decode the C-style escapes promised by the CSV authoring contract. */
	bool DecodeSourceStringEscapes(const FString& Encoded, FString& OutDecoded, FString& OutError)
	{
		OutDecoded.Empty(Encoded.Len());
		for (int32 Index = 0; Index < Encoded.Len(); ++Index)
		{
			const TCHAR Character = Encoded[Index];
			if (Character != TCHAR('\\'))
			{
				OutDecoded.AppendChar(Character);
				continue;
			}
			if (Index + 1 >= Encoded.Len())
			{
				OutError = TEXT("Trailing backslash in CSV source text.");
				return false;
			}
			switch (Encoded[++Index])
			{
				case TCHAR('n'): OutDecoded.AppendChar(TCHAR('\n')); break;
				case TCHAR('r'): OutDecoded.AppendChar(TCHAR('\r')); break;
				case TCHAR('t'): OutDecoded.AppendChar(TCHAR('\t')); break;
				case TCHAR('\\'): OutDecoded.AppendChar(TCHAR('\\')); break;
				default:
					OutError = TEXT("Unsupported CSV source escape (supported: \\n, \\r, \\t, \\\\).");
					return false;
			}
		}
		return true;
	}

	bool ContainsLegacyPrintfToken(const FString& SourceString)
	{
		static const FRegexPattern PrintfPattern(TEXT("%[-+0-9.*]*[diuoxXfFeEgGaAcsp]"));
		FRegexMatcher Matcher(PrintfPattern, SourceString);
		return Matcher.FindNext();
	}

	bool HasBalancedFormatBraces(const FString& SourceString)
	{
		int32 Depth = 0;
		for (const TCHAR Character : SourceString)
		{
			if (Character == TCHAR('{'))
			{
				++Depth;
			}
			else if (Character == TCHAR('}'))
			{
				--Depth;
				if (Depth < 0)
				{
					return false;
				}
			}
		}
		return Depth == 0;
	}

	bool ContainsOrderedPlaceholder(const FString& SourceString)
	{
		static const FRegexPattern OrderedPlaceholderPattern(TEXT("\\{[0-9]+(?:[^}]*)\\}"));
		FRegexMatcher Matcher(OrderedPlaceholderPattern, SourceString);
		return Matcher.FindNext();
	}

	bool ValidateEntry(const FUILocalizationCsvEntry& Entry, int32 CsvRowNumber, FString& OutError)
	{
		if (Entry.Key.IsEmpty())
		{
			OutError = FString::Printf(TEXT("UI.csv row %d has an empty Key."), CsvRowNumber);
			return false;
		}
		if (Entry.SourceString.IsEmpty())
		{
			OutError = FString::Printf(TEXT("UI.csv row %d ('%s') has an empty SourceString."), CsvRowNumber, *Entry.Key);
			return false;
		}
		if (Entry.Key.Contains(TEXT("Grimrock"), ESearchCase::IgnoreCase))
		{
			OutError = FString::Printf(TEXT("UI.csv row %d contains the forbidden localization identifier token."), CsvRowNumber);
			return false;
		}

		static const FRegexPattern KeyPattern(TEXT("^[A-Za-z][A-Za-z0-9]*(\\.[A-Za-z][A-Za-z0-9]*)+$"));
		FRegexMatcher KeyMatcher(KeyPattern, Entry.Key);
		if (!KeyMatcher.FindNext())
		{
			OutError = FString::Printf(TEXT("UI.csv row %d has invalid Key syntax: '%s'."), CsvRowNumber, *Entry.Key);
			return false;
		}
		if (ContainsLegacyPrintfToken(Entry.SourceString))
		{
			OutError = FString::Printf(TEXT("UI.csv row %d ('%s') contains a legacy printf placeholder."), CsvRowNumber, *Entry.Key);
			return false;
		}
		if (!HasBalancedFormatBraces(Entry.SourceString))
		{
			OutError = FString::Printf(TEXT("UI.csv row %d ('%s') has unbalanced format braces."), CsvRowNumber, *Entry.Key);
			return false;
		}
		if (ContainsOrderedPlaceholder(Entry.SourceString))
		{
			OutError = FString::Printf(TEXT("UI.csv row %d ('%s') uses an ordered placeholder; use named placeholders."), CsvRowNumber, *Entry.Key);
			return false;
		}

		return true;
	}

	TMap<FString, FUILocalizationCsvEntry> BuildEntryMap(const TArray<FUILocalizationCsvEntry>& Entries)
	{
		TMap<FString, FUILocalizationCsvEntry> Result;
		Result.Reserve(Entries.Num());
		for (const FUILocalizationCsvEntry& Entry : Entries)
		{
			Result.Add(Entry.Key, Entry);
		}
		return Result;
	}
}

const TCHAR* FUILocalizationCsvAuthoring::SourceRelativePath()
{
	return UILocalizationCsvAuthoringPrivate::SourcePath;
}

const TCHAR* FUILocalizationCsvAuthoring::PackageName()
{
	return UILocalizationCsvAuthoringPrivate::ProductionPackageName;
}

const TCHAR* FUILocalizationCsvAuthoring::AssetName()
{
	return UILocalizationCsvAuthoringPrivate::ProductionAssetName;
}

const TCHAR* FUILocalizationCsvAuthoring::ObjectPath()
{
	return UILocalizationCsvAuthoringPrivate::ProductionObjectPath;
}

FString FUILocalizationCsvAuthoring::SourceFilename()
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), SourceRelativePath()));
}

bool FUILocalizationCsvAuthoring::ParseAndValidateCsv(
	const FString& CsvContent,
	TArray<FUILocalizationCsvEntry>& OutEntries,
	FString& OutError)
{
	OutEntries.Reset();
	OutError.Reset();

	FCsvParser Parser(CsvContent);
	const auto& Rows = Parser.GetRows();
	if (Rows.IsEmpty())
	{
		OutError = TEXT("UI.csv is empty; expected header Key,SourceString,Comment.");
		return false;
	}

	const TArray<const TCHAR*>& Header = Rows[0];
	if (Header.Num() != 3 ||
		FCString::Strcmp(Header[0], TEXT("Key")) != 0 ||
		FCString::Strcmp(Header[1], TEXT("SourceString")) != 0 ||
		FCString::Strcmp(Header[2], TEXT("Comment")) != 0)
	{
		OutError = TEXT("UI.csv header must be exactly Key,SourceString,Comment.");
		return false;
	}

	TSet<FString> SeenKeys;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<const TCHAR*>& Row = Rows[RowIndex];
		if (UILocalizationCsvAuthoringPrivate::IsBlankRow(Row))
		{
			continue;
		}
		if (Row.Num() != 3)
		{
			OutError = FString::Printf(TEXT("UI.csv row %d must contain exactly 3 columns."), RowIndex + 1);
			return false;
		}

		FUILocalizationCsvEntry Entry;
		Entry.Key = Row[0] ? FString(Row[0]).TrimStartAndEnd() : FString();
		const FString EncodedSourceString = Row[1] ? FString(Row[1]) : FString();
		Entry.Comment = Row[2] ? FString(Row[2]) : FString();
		FString DecodeError;
		if (!UILocalizationCsvAuthoringPrivate::DecodeSourceStringEscapes(
			EncodedSourceString, Entry.SourceString, DecodeError))
		{
			OutError = FString::Printf(TEXT("UI.csv row %d: %s"), RowIndex + 1, *DecodeError);
			return false;
		}

		if (!UILocalizationCsvAuthoringPrivate::ValidateEntry(Entry, RowIndex + 1, OutError))
		{
			return false;
		}
		if (SeenKeys.Contains(Entry.Key))
		{
			OutError = FString::Printf(TEXT("UI.csv row %d duplicates Key '%s'."), RowIndex + 1, *Entry.Key);
			return false;
		}
		SeenKeys.Add(Entry.Key);
		OutEntries.Add(MoveTemp(Entry));
	}

	return true;
}

bool FUILocalizationCsvAuthoring::LoadAndValidateProductionCsv(
	TArray<FUILocalizationCsvEntry>& OutEntries,
	FString& OutError)
{
	FString CsvContent;
	const FString Filename = SourceFilename();
	if (!FFileHelper::LoadFileToString(CsvContent, *Filename))
	{
		OutError = FString::Printf(TEXT("Unable to read localization CSV '%s'."), *Filename);
		return false;
	}
	return ParseAndValidateCsv(CsvContent, OutEntries, OutError);
}

void FUILocalizationCsvAuthoring::ApplyEntries(
	UStringTable& StringTableAsset,
	const TArray<FUILocalizationCsvEntry>& Entries)
{
	const FStringTableRef Table = StringTableAsset.GetMutableStringTable();
	Table->ClearSourceStrings(Entries.Num());

	for (const FUILocalizationCsvEntry& Entry : Entries)
	{
		const FTextKey Key(Entry.Key);
		Table->SetSourceString(Key, Entry.SourceString);
		Table->SetMetaData(Key, UILocalizationCsvAuthoringPrivate::CommentMetaDataId, Entry.Comment);
	}
}

bool FUILocalizationCsvAuthoring::MatchesEntries(
	const UStringTable& StringTableAsset,
	const TArray<FUILocalizationCsvEntry>& Entries)
{
	const TMap<FString, FUILocalizationCsvEntry> Expected =
		UILocalizationCsvAuthoringPrivate::BuildEntryMap(Entries);

	const FStringTableConstRef Table = StringTableAsset.GetStringTable();
	TMap<FString, FString> ActualSources;
	Table->EnumerateKeysAndSourceStrings(
		[&ActualSources](const FTextKey& Key, const FString& SourceString)
		{
			ActualSources.Add(Key.ToString(), SourceString);
			return true;
		});

	if (ActualSources.Num() != Expected.Num())
	{
		return false;
	}

	for (const TPair<FString, FUILocalizationCsvEntry>& Pair : Expected)
	{
		const FString* ActualSource = ActualSources.Find(Pair.Key);
		if (!ActualSource || *ActualSource != Pair.Value.SourceString)
		{
			return false;
		}

		const FString ActualComment =
			Table->GetMetaData(FTextKey(Pair.Key), UILocalizationCsvAuthoringPrivate::CommentMetaDataId);
		if (ActualComment != Pair.Value.Comment)
		{
			return false;
		}
	}

	return true;
}

bool FUILocalizationCsvAuthoring::MaterializeProductionAsset(
	bool& bOutChanged,
	FString& OutError)
{
	bOutChanged = false;
	OutError.Reset();

	TArray<FUILocalizationCsvEntry> Entries;
	if (!LoadAndValidateProductionCsv(Entries, OutError))
	{
		return false;
	}

	UStringTable* Asset = LoadObject<UStringTable>(nullptr, ObjectPath());
	UPackage* Package = Asset ? Asset->GetOutermost() : nullptr;
	const bool bCreated = Asset == nullptr;

	if (!Asset)
	{
		Package = CreatePackage(PackageName());
		if (!Package)
		{
			OutError = FString::Printf(TEXT("Unable to create package '%s'."), PackageName());
			return false;
		}

		Asset = NewObject<UStringTable>(
			Package,
			FName(AssetName()),
			RF_Public | RF_Standalone);
		if (!Asset)
		{
			OutError = FString::Printf(TEXT("Unable to create String Table asset '%s'."), ObjectPath());
			return false;
		}
		FAssetRegistryModule::AssetCreated(Asset);
	}

	const bool bMatches = MatchesEntries(*Asset, Entries);
	if (!bCreated && bMatches)
	{
		return true;
	}

	Asset->Modify();
	ApplyEntries(*Asset, Entries);
	if (!MatchesEntries(*Asset, Entries))
	{
		OutError = TEXT("Generated ST_UI does not exactly match UI.csv.");
		return false;
	}

	Package->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		PackageName(),
		FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
	{
		OutError = FString::Printf(TEXT("Unable to save generated String Table '%s'."), *Filename);
		return false;
	}

	bOutChanged = true;
	return true;
}
