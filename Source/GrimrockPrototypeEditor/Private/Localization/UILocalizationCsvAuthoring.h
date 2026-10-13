#pragma once

#include "CoreMinimal.h"

class UStringTable;

struct FUILocalizationCsvEntry
{
	FString Key;
	FString SourceString;
	FString Comment;
};

struct FUILocalizationCsvAuthoring
{
	static const TCHAR* SourceRelativePath();
	static const TCHAR* PackageName();
	static const TCHAR* AssetName();
	static const TCHAR* ObjectPath();
	static FString SourceFilename();

	static bool ParseAndValidateCsv(
		const FString& CsvContent,
		TArray<FUILocalizationCsvEntry>& OutEntries,
		FString& OutError);

	static bool LoadAndValidateProductionCsv(
		TArray<FUILocalizationCsvEntry>& OutEntries,
		FString& OutError);

	static void ApplyEntries(
		UStringTable& StringTableAsset,
		const TArray<FUILocalizationCsvEntry>& Entries);

	static bool MatchesEntries(
		const UStringTable& StringTableAsset,
		const TArray<FUILocalizationCsvEntry>& Entries);

	static bool MaterializeProductionAsset(
		bool& bOutChanged,
		FString& OutError);
};
