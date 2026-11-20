#include "ARMEMENT/GridArmClean065RetirementAuditCommandlet.h"
#include "GridEditorRetirementAudit.h"
#include "GridEditorAlchemySaveCopy.h"
#include "GridEditorWorldSaveProjection.h"
#include "GridEditorInstanceCacheEvidence.h"
#include "Runtime/Items/GridCanonicalWorldSnapshotReadModel.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    FString Normalize(FString Path)
    {
        Path = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeFilename(Path);
        FPaths::CollapseRelativeDirectories(Path);
        return Path;
    }

    bool OutsideProject(const FString& Path)
    {
        if (Path.IsEmpty() || FPaths::IsRelative(Path)) return false;
        FString Project = Normalize(FPaths::ProjectDir());
        FPaths::NormalizeDirectoryName(Project);
        const FString Full = Normalize(Path);
        return !Full.Equals(Project, ESearchCase::IgnoreCase) &&
            !Full.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase);
    }
}

UGridArmClean065RetirementAuditCommandlet::UGridArmClean065RetirementAuditCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean065RetirementAuditCommandlet::Main(const FString& Params)
{
    using namespace GridEditorRetirementAudit;
    FString Copies, Output;
    if (!FParse::Param(*Params, TEXT("ReadOnly")) || FParse::Param(*Params, TEXT("Apply")) ||
        FParse::Param(*Params, TEXT("Publish")) || !FParse::Value(*Params, TEXT("Copies="), Copies) ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !OutsideProject(Copies) ||
        !OutsideProject(Output) || FPaths::GetCleanFilename(Normalize(Copies)) != TEXT("AuditCopies") ||
        !Output.EndsWith(TEXT(".json")) || FPaths::FileExists(Output) ||
        !IFileManager::Get().DirectoryExists(*Copies))
        return 1;
    FNativeAudit Native;
    FString Error;
    if (!CollectNative(Native, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.5A] Native evidence refused: %s"), *Error);
        return 2;
    }
    const bool bWorldPreflight = FParse::Param(*Params, TEXT("WorldPointerPreflight"));
    const bool bInstanceEvidence = FParse::Param(*Params, TEXT("InstanceCacheEvidence"));
    const bool bWeightEvidence = FParse::Param(*Params, TEXT("WeightCacheEvidence"));
    if ((bInstanceEvidence && !bWorldPreflight) || (bWeightEvidence && !bInstanceEvidence)) return 1;
    GridEditorInstanceCacheEvidence::FLegacySources Historical;
    if (bWeightEvidence)
    {
        for (const auto& Value : Native.Report->GetArrayField(TEXT("puzzle_category_sources")))
        {
            const auto Row = Value->AsObject();
            const FName Id(*Row->GetStringField(TEXT("item_id")));
            const auto* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Row->GetStringField(TEXT("source")));
            if (!IsValid(Old) || Old->ItemDefinitionId != Id || Historical.Contains(Id)) return 2;
            Historical.Add(Id, Old);
        }
        if (Historical.Num() != 48) return 2;
    }
    const auto* HistoricalPolicy = bWeightEvidence ? &Historical : nullptr;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    if (bWorldPreflight)
    {
        TArray<FString> Errors;
        UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RootPath());
        if (!Root || !Registry->RegisterItemSystem(Root, Errors)) return 2;
    }
    auto Report = MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("world_pointer_preflight"), bWorldPreflight);
    Report->SetBoolField(TEXT("instance_cache_evidence"), bInstanceEvidence);
    Report->SetBoolField(TEXT("weight_cache_evidence"), bWeightEvidence);
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.5A/retirement-evidence/v1"));
    Report->SetObjectField(TEXT("native"), Native.Report);
    TArray<FString> Filenames;
    IFileManager::Get().FindFiles(Filenames, *(Copies / TEXT("*.sav")), true, false);
    Filenames.Sort();
    TArray<TSharedPtr<FJsonValue>> Saves;
    int32 Failed = 0;
    for (const FString& Filename : Filenames)
    {
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("filename"), Filename);
        TArray<uint8> Bytes;
        if (!FFileHelper::LoadFileToArray(Bytes, *(Copies / Filename)))
        {
            Row->SetBoolField(TEXT("decoded"), false);
            ++Failed;
        }
        else
        {
            Row->SetNumberField(TEXT("bytes"), Bytes.Num());
            Row->SetArrayField(TEXT("raw_path_candidates"), RawPaths(Bytes, Native.CandidatePaths));
            FString LoadError;
            TStrongObjectPtr<UGrimrockPartySaveGame> Save(GridEditorAlchemySaveCopy::LoadExact(Bytes, LoadError));
            Row->SetBoolField(TEXT("decoded"), Save.IsValid());
            if (!Save.IsValid())
            {
                // No player details or raw loader diagnostics in JSON.
                ++Failed;
            }
            else
            {
                FSaveAudit Audit;
                Audit.Collect(*Save.Get(), Native.ItemIds);
                Audit.Write(*Row);
                Row->SetNumberField(TEXT("version"), Save->SaveVersion);
                if (bInstanceEvidence)
                {
                    using namespace GridEditorInstanceCacheEvidence;
                    const FCounts Counts = Inspect(Save->PartyInventoryState, *Registry.Get(), HistoricalPolicy);
                    Row->SetNumberField(TEXT("cache_instances"), Counts.Instances);
                    Row->SetNumberField(TEXT("cache_invalid_identities"), Counts.Invalid);
                    Row->SetNumberField(TEXT("cache_name_conflicts"), Counts.NameConflicts);
                    Row->SetNumberField(TEXT("cache_weight_conflicts"), Counts.WeightConflicts);
                    Row->SetNumberField(TEXT("cache_weight_explained"), Counts.ExplainedWeightConflicts);
                    Row->SetNumberField(TEXT("cache_names"), Counts.CachedNames);
                    Row->SetNumberField(TEXT("cache_weights"), Counts.CachedWeights);
                    bool bNormalizedParity = false;
                    if (Counts.CanClear())
                    {
                        FString CheckError;
                        TStrongObjectPtr<UGrimrockPartySaveGame> Expected(GridEditorAlchemySaveCopy::LoadExact(Bytes, CheckError));
                        TStrongObjectPtr<UGrimrockPartySaveGame> Working(GridEditorAlchemySaveCopy::LoadExact(Bytes, CheckError));
                        TArray<uint8> NormalizedBytes;
                        const bool bEncoded = Expected.IsValid() && Working.IsValid() &&
                            ClearVerifiedCopy(Expected->PartyInventoryState, *Registry.Get(), HistoricalPolicy) &&
                            ClearVerifiedCopy(Working->PartyInventoryState, *Registry.Get(), HistoricalPolicy) &&
                            GridEditorAlchemySaveCopy::SameSaveProperties(*Expected.Get(), *Working.Get(), CheckError) &&
                            GridEditorAlchemySaveCopy::EncodeChecked(*Working.Get(), NormalizedBytes, CheckError);
                        TStrongObjectPtr<UGrimrockPartySaveGame> Reloaded(bEncoded ?
                            GridEditorAlchemySaveCopy::LoadExact(NormalizedBytes, CheckError) : nullptr);
                        if (Reloaded.IsValid())
                        {
                            const FCounts Remaining = Inspect(Reloaded->PartyInventoryState, *Registry.Get());
                            bNormalizedParity = Remaining.CanClear() && Remaining.Instances == Counts.Instances &&
                                Remaining.CachedNames == 0 && Remaining.CachedWeights == 0 &&
                                GridEditorAlchemySaveCopy::SameSaveProperties(*Expected.Get(), *Reloaded.Get(), CheckError);
                        }
                    }
                    Row->SetBoolField(TEXT("cache_normalized_roundtrip_equal"), bNormalizedParity);
                }
                if (bWorldPreflight)
                {
                    FString CheckError;
                    const bool bPhysical = FGridCanonicalWorldSnapshotReadModel::ValidateDungeon(
                        Registry.Get(), Save->DungeonRuntimeState, CheckError);
                    Row->SetBoolField(TEXT("canonical_world_valid"), bPhysical);
                    TArray<TSharedPtr<FJsonValue>> Projection;
                    for (const FString& Line : GridEditorWorldSaveProjection::Build(Save->DungeonRuntimeState))
                        Projection.Add(MakeShared<FJsonValueString>(Line));
                    Row->SetArrayField(TEXT("world_row_projection"), Projection);
                    TArray<uint8> Reencoded;
                    TStrongObjectPtr<UGrimrockPartySaveGame> Expected(
                        GridEditorAlchemySaveCopy::LoadExact(Bytes, CheckError));
                    const bool bEncoded = bPhysical && Expected.IsValid() &&
                        GridEditorAlchemySaveCopy::EncodeChecked(*Save.Get(), Reencoded, CheckError);
                    TStrongObjectPtr<UGrimrockPartySaveGame> Reloaded(bEncoded ?
                        GridEditorAlchemySaveCopy::LoadExact(Reencoded, CheckError) : nullptr);
                    const bool bParity = Reloaded.IsValid() &&
                        GridEditorAlchemySaveCopy::SameSaveProperties(*Expected.Get(), *Reloaded.Get(), CheckError);
                    Row->SetBoolField(TEXT("memory_roundtrip_equal"), bParity);
                    if (!bPhysical || !bParity)
                    {
                        // Private commandlet diagnostics only.
                        UE_LOG(LogTemp, Error, TEXT("[5G85] Save preflight failed: %s"), *CheckError);
                        ++Failed;
                    }
                }
            }
        }
        Saves.Add(MakeShared<FJsonValueObject>(Row));
    }
    Report->SetNumberField(TEXT("save_count"), Saves.Num());
    Report->SetNumberField(TEXT("decode_failures"), Failed);
    Report->SetArrayField(TEXT("saves"), Saves);
    Report->SetBoolField(TEXT("deletion_authorized"), false);
    FString Json;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
        FPaths::FileExists(Output) || !FFileHelper::SaveStringToFile(Json, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        return 3;
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.5A] READ ONLY COMPLETE saves=%d decode_failures=%d"), Saves.Num(), Failed);
    return Failed == 0 ? 0 : 4;
}
