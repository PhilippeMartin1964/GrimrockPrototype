#include "ARMEMENT/GridStartingKitCoherenceAuditCommandlet.h"

#include "ARMEMENT/GridStartingKitCoherenceAudit.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

UGridStartingKitCoherenceAuditCommandlet::UGridStartingKitCoherenceAuditCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridStartingKitCoherenceAuditCommandlet::Main(const FString& Params)
{
    (void)Params;
    FGridStartingKitCoherenceAuditReport Report;
    FString Error;
    if (!FGridStartingKitCoherenceAudit::BuildProductionReport(Report, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.9.4] Audit failed: %s"), *Error);
        return 1;
    }

    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("ARMEMENT"), TEXT("ARMEMENT01.9.4"));
    IFileManager::Get().MakeDirectory(*Directory, true);

    struct FOutput
    {
        const TCHAR* Filename;
        const FString* Csv;
    };
    const FOutput Outputs[] = {
        {TEXT("StartingKitCoherence.csv"), &Report.KitMatrixCsv},
        {TEXT("ClassTalentBranches.csv"), &Report.TalentBranchesCsv},
        {TEXT("ClassSkillAffinities.csv"), &Report.SkillAffinitiesCsv}
    };
    for (const FOutput& Output : Outputs)
    {
        const FString Path = FPaths::Combine(Directory, Output.Filename);
        if (!FFileHelper::SaveStringToFile(
            *Output.Csv, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.9.4] Cannot write %s"), *Path);
            return 1;
        }
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.9.4] CSV: %s"), *Path);
    }

    UE_LOG(LogTemp, Display,
        TEXT("[ARMEMENT01.9.4] ReadOnlyAudit OK Classes=%d Kits=%d Branches=%d Skills=%d AffinityPairs=%d KitsWithoutProposedBranch=%d"),
        Report.ClassCount, Report.KitCount, Report.BranchCount, Report.SkillCount,
        Report.AffinityCount, Report.KitsWithoutProposedBranch);
    UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.9.4] No production assets modified."));
    return 0;
}
