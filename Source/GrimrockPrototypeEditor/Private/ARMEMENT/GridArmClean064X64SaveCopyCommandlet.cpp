#include "ARMEMENT/GridArmClean064X64SaveCopyCommandlet.h"
#include "GridEditorAlchemySaveCopy.h"
#include "GridEditorReadableSaveRepair.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

namespace
{
    FString NormalizedDirectory(FString Path)
    {
        Path = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeDirectoryName(Path);
        FPaths::CollapseRelativeDirectories(Path);
        return Path;
    }

    bool IsUnder(const FString& Path, const FString& Root)
    {
        return Path.Equals(Root, ESearchCase::IgnoreCase) ||
            Path.StartsWith(Root + TEXT("/"), ESearchCase::IgnoreCase);
    }

    bool ValidFilename(const FString& Name)
    {
        if (Name.Len() < 5 || !Name.EndsWith(TEXT(".sav"), ESearchCase::IgnoreCase)) return false;
        // A deliberately narrow regular filename, not a path, device name,
        // alternate data stream, hidden path or UNC path.
        const FString Stem = Name.LeftChop(4);
        for (TCHAR Ch : Stem)
            if (!(Ch >= TEXT('A') && Ch <= TEXT('Z')) &&
                !(Ch >= TEXT('a') && Ch <= TEXT('z')) &&
                !(Ch >= TEXT('0') && Ch <= TEXT('9')) &&
                Ch != TEXT('_') && Ch != TEXT('-'))
                return false;
        const FString Upper = Stem.ToUpper();
        if (Upper == TEXT("CON") || Upper == TEXT("PRN") || Upper == TEXT("AUX") || Upper == TEXT("NUL") ||
            (Upper.Len() == 4 && (Upper.StartsWith(TEXT("COM")) || Upper.StartsWith(TEXT("LPT"))) &&
                FChar::IsDigit(Upper[3]))) return false;
        return true;
    }
}

UGridArmClean064X64SaveCopyCommandlet::UGridArmClean064X64SaveCopyCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X64SaveCopyCommandlet::Main(const FString& Params)
{
    FString Root, Slot;
    const bool bRepairReadables = FParse::Param(*Params, TEXT("RepairReadables"));
    const auto Convert = bRepairReadables ? &GridEditorReadableSaveRepair::Convert : &GridEditorAlchemySaveCopy::Convert;
    if (!FParse::Param(*Params, TEXT("ConvertCopy")) ||
        FParse::Param(*Params, TEXT("Apply")) || FParse::Param(*Params, TEXT("Publish")) ||
        !FParse::Value(*Params, TEXT("SnapshotRoot="), Root) || FPaths::IsRelative(Root) ||
        !FParse::Value(*Params, TEXT("File="), Slot) || !ValidFilename(Slot)) return 1;
    Root = NormalizedDirectory(Root);
    const FString Project = NormalizedDirectory(FPaths::ProjectDir());
    const FString ProjectSaved = NormalizedDirectory(FPaths::ProjectSavedDir());
    // A snapshot must be distinct from the project and its live Saved dir.
    // The operator must use real directories, not junctions/symlinks.
    if (Root.IsEmpty() || IsUnder(Root, Project) || IsUnder(Root, ProjectSaved) ||
        IsUnder(Project, Root) || IsUnder(ProjectSaved, Root)) return 1;
    const FString InputDir = FPaths::Combine(Root, TEXT("Original"));
    const FString OutputDir = FPaths::Combine(Root, TEXT("Converted"));
    const FString Input = FPaths::Combine(InputDir, Slot);
    const FString Output = FPaths::Combine(OutputDir, Slot);
    if (!FPaths::DirectoryExists(InputDir) || !FPaths::DirectoryExists(OutputDir) ||
        !FPaths::FileExists(Input) || FPaths::FileExists(Output)) return 2;
    TArray<uint8> Before, After;
    if (!FFileHelper::LoadFileToArray(Before, *Input)) return 2;
    int32 Changed = INDEX_NONE;
    FString Error;
    if (!Convert(Before, After, Changed, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X64] Copy refused: %s"), *Error);
        return 3;
    }
    TArray<uint8> Current;
    if (!FFileHelper::LoadFileToArray(Current, *Input) || Current != Before) return 4;
    // NoReplaceExisting is a second safeguard against racing output writes.
    TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Output, FILEWRITE_NoReplaceExisting));
    if (!Writer) return 5;
    Writer->Serialize(After.GetData(), After.Num());
    const bool bClosed = Writer->Close();
    const bool bWriteError = Writer->IsError();
    Writer.Reset();
    if (!bClosed || bWriteError) return 5;
    TArray<uint8> Readback;
    if (!FFileHelper::LoadFileToArray(Readback, *Output) || Readback != After ||
        !FFileHelper::LoadFileToArray(Current, *Input) || Current != Before) return 6;
    TArray<uint8> Verified;
    int32 Residual = INDEX_NONE;
    if (!Convert(Readback, Verified, Residual, Error) ||
        Residual != 0 || Verified != Readback) return 6;
    UE_LOG(LogTemp, Display, TEXT("[%s] VERIFIED COPY changes=%d bytes=%d file=%s"),
        bRepairReadables ? TEXT("ARM-CLEAN06.5B") : TEXT("ARM-CLEAN06.4X64"), Changed, Readback.Num(), *Slot);
    return 0;
}
