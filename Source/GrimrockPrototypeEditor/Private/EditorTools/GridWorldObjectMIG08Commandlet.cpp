#include "EditorTools/GridWorldObjectMIG08Commandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridWorldObjectMIG08MigrationService.h"
#include "Engine/DataAsset.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"

namespace
{
	void AppendMigrationResult(const FString& AssetLabel, const FGridWorldObjectMIG08MigrationResult& Result, TArray<FString>& Lines, int32& WarningCount,
		int32& ErrorCount)
	{
		for (const FString& Change : Result.Changes)
		{
			Lines.Add(FString::Printf(TEXT("CHANGE | %s | %s"), *AssetLabel, *Change));
		}
		for (const FString& Warning : Result.Warnings)
		{
			++WarningCount;
			Lines.Add(FString::Printf(TEXT("WARNING | %s | %s"), *AssetLabel, *Warning));
		}
		for (const FString& Error : Result.Errors)
		{
			++ErrorCount;
			Lines.Add(FString::Printf(TEXT("ERROR | %s | %s"), *AssetLabel, *Error));
		}
	}

	void AppendArchetypeValidation(const UGridObjectArchetypeAsset& Archetype, TArray<FString>& Lines, int32& WarningCount, int32& ErrorCount)
	{
		if (Archetype.SupportedType == EGridLevelObjectType::Item)
		{
			++WarningCount;
			Lines.Add(FString::Printf(TEXT("WARNING | %s | legacy item companion archetype remains as a MIG09 deletion candidate."), *Archetype.GetPathName()));
			return;
		}

		TArray<FGridArchetypeValidationMessage> Messages;
		Archetype.ValidateArchetype(Messages);
		for (const FGridArchetypeValidationMessage& Message : Messages)
		{
			if (Message.Severity == EGridArchetypeValidationSeverity::Error)
			{
				++ErrorCount;
				Lines.Add(FString::Printf(TEXT("ERROR | %s | %s"), *Archetype.GetPathName(), *Message.Message));
			}
			else if (Message.Severity == EGridArchetypeValidationSeverity::Warning)
			{
				++WarningCount;
				Lines.Add(FString::Printf(TEXT("WARNING | %s | %s"), *Archetype.GetPathName(), *Message.Message));
			}
		}
	}
}

UGridWorldObjectMIG08Commandlet::UGridWorldObjectMIG08Commandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UGridWorldObjectMIG08Commandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	FString RootPath = TEXT("/Game/GrimrockPrototype");
	FParse::Value(*Params, TEXT("Root="), RootPath);

	FString ReportPath;
	FParse::Value(*Params, TEXT("Report="), ReportPath);
	if (ReportPath.IsEmpty())
	{
		const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
		ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/MIG08"), FString::Printf(TEXT("MIG08-%s.report.txt"), *Timestamp));
	}

	TArray<FString> Lines;
	Lines.Add(TEXT("WORLDOBJ-MIG08 Asset Migration"));
	Lines.Add(FString::Printf(TEXT("Mode     : %s"), bApply ? TEXT("APPLY") : TEXT("DRY-RUN")));
	Lines.Add(FString::Printf(TEXT("Root     : %s"), *RootPath));
	Lines.Add(FString::Printf(TEXT("Report   : %s"), *ReportPath));
	Lines.Add(TEXT(""));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*RootPath));
	Filter.ClassPaths.Add(UDataAsset::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> AssetEntries;
	AssetRegistry.GetAssets(Filter, AssetEntries);
	AssetEntries.Sort(
		[](const FAssetData& A, const FAssetData& B)
		{
			return A.PackageName.LexicalLess(B.PackageName);
		});

	int32 LoadedRelevantAssets = 0;
	int32 ChangedAssets = 0;
	int32 WarningCount = 0;
	int32 ErrorCount = 0;
	TArray<UPackage*> PackagesToSave;

	for (const FAssetData& Entry : AssetEntries)
	{
		UDataAsset* DataAsset = Cast<UDataAsset>(Entry.GetAsset());
		if (!DataAsset)
		{
			++ErrorCount;
			Lines.Add(FString::Printf(TEXT("ERROR | %s | asset failed to load as UDataAsset."), *Entry.PackageName.ToString()));
			continue;
		}

		bool bRelevant = false;
		bool bChanged = false;

		if (UGridLevelAsset* Level = Cast<UGridLevelAsset>(DataAsset))
		{
			bRelevant = true;
			const FGridWorldObjectMIG08MigrationResult Result = FGridWorldObjectMIG08MigrationService::MigrateLevelAsset(*Level);
			bChanged = Result.bChanged;
			AppendMigrationResult(Level->GetPathName(), Result, Lines, WarningCount, ErrorCount);

			TArray<FString> MonsterErrors;
			if (!Level->ValidateMonsterSpawns(MonsterErrors))
			{
				for (const FString& Error : MonsterErrors)
				{
					++ErrorCount;
					Lines.Add(FString::Printf(TEXT("ERROR | %s | %s"), *Level->GetPathName(), *Error));
				}
			}
		}
		else if (UGridObjectPaletteAsset* Palette = Cast<UGridObjectPaletteAsset>(DataAsset))
		{
			bRelevant = true;
			const FGridWorldObjectMIG08MigrationResult Result = FGridWorldObjectMIG08MigrationService::MigratePaletteAsset(*Palette);
			bChanged = Result.bChanged;
			AppendMigrationResult(Palette->GetPathName(), Result, Lines, WarningCount, ErrorCount);

			TArray<FGridArchetypeValidationMessage> PaletteMessages;
			Palette->ValidatePalette(PaletteMessages);
			for (const FGridArchetypeValidationMessage& Message : PaletteMessages)
			{
				if (Message.Severity == EGridArchetypeValidationSeverity::Error)
				{
					++ErrorCount;
					Lines.Add(FString::Printf(TEXT("ERROR | %s | %s"), *Palette->GetPathName(), *Message.Message));
				}
				else if (Message.Severity == EGridArchetypeValidationSeverity::Warning)
				{
					++WarningCount;
					Lines.Add(FString::Printf(TEXT("WARNING | %s | %s"), *Palette->GetPathName(), *Message.Message));
				}
			}
		}
		else if (const UGridObjectArchetypeAsset* Archetype = Cast<UGridObjectArchetypeAsset>(DataAsset))
		{
			bRelevant = true;
			AppendArchetypeValidation(*Archetype, Lines, WarningCount, ErrorCount);
		}
		else if (const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(DataAsset))
		{
			bRelevant = true;
			if (!Item->IsValidDefinition())
			{
				++ErrorCount;
				Lines.Add(FString::Printf(TEXT("ERROR | %s | invalid ItemDefinition."), *Item->GetPathName()));
			}
		}

		if (!bRelevant)
		{
			continue;
		}

		++LoadedRelevantAssets;
		if (bChanged)
		{
			++ChangedAssets;
		}
		if (bApply)
		{
			PackagesToSave.AddUnique(DataAsset->GetOutermost());
		}
	}

	Lines.Add(TEXT(""));
	Lines.Add(TEXT("=== Summary ==="));
	Lines.Add(FString::Printf(TEXT("Relevant assets : %d"), LoadedRelevantAssets));
	Lines.Add(FString::Printf(TEXT("Changed assets  : %d"), ChangedAssets));
	Lines.Add(FString::Printf(TEXT("Warnings        : %d"), WarningCount));
	Lines.Add(FString::Printf(TEXT("Errors          : %d"), ErrorCount));

	int32 ExitCode = 0;
	if (ErrorCount > 0)
	{
		ExitCode = 2;
		Lines.Add(TEXT("Result          : FAILED - resolve migration/content errors before applying."));
	}
	else if (bApply)
	{
		if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, false))
		{
			ExitCode = 3;
			Lines.Add(TEXT("Result          : FAILED - one or more packages could not be saved."));
		}
		else
		{
			Lines.Add(FString::Printf(TEXT("Saved packages  : %d"), PackagesToSave.Num()));
			Lines.Add(TEXT("Result          : OK - migrated assets saved."));
		}
	}
	else
	{
		Lines.Add(TEXT("Result          : OK - dry-run only; no package was saved."));
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ReportPath), true);
	if (!FFileHelper::SaveStringArrayToFile(Lines, *ReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("WORLDOBJ-MIG08: failed to write report %s"), *ReportPath);
		return ExitCode == 0 ? 4 : ExitCode;
	}

	for (const FString& Line : Lines)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
	}

	return ExitCode;
}
