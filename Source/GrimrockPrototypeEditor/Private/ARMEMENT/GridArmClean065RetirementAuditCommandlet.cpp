#include "ARMEMENT/GridArmClean065RetirementAuditCommandlet.h"
#include "GridEditorRetirementAudit.h"
#include "GridEditorAlchemySaveCopy.h"
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
    auto Report = MakeShared<FJsonObject>();
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
