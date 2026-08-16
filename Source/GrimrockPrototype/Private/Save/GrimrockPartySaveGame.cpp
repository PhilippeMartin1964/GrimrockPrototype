#include "Save/GrimrockPartySaveGame.h"

#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpNotificationSubsystem.h"
#include "RPG/RPGSaveMigrationService.h"

DEFINE_LOG_CATEGORY_STATIC (LogGrimrockPartySave, Log, All);

void UGrimrockPartySaveGame::Serialize (FArchive& Ar)
{
    if (Ar.IsSaving ())
    {
        SaveVersion = CurrentSaveVersion;
        bProgressionLoadValid = true;
        ProgressionLoadError.Reset ();

        FText CaptureError;
        if (!FRPGClassProgressionTransactionService::CapturePersistentState (
                PartyInventoryState,
                ClassProgressionStates,
                CaptureError) ||
            !URPGLevelUpNotificationSubsystem::CapturePersistentState (
                PartyInventoryState,
                PendingLevelUpNotifications,
                CaptureError))
        {
            UE_LOG (
                LogGrimrockPartySave,
                Error,
                TEXT ("[GridSaveMigration] SaveCapture Result=Rejected Reason=%s"),
                *CaptureError.ToString ());
            Ar.SetError ();
            return;
        }

        UGrimrockPartySaveGame* MutableThis = this;
        if (!FRPGSaveMigrationService::ValidateCurrentSave (
                MutableThis,
                CaptureError))
        {
            UE_LOG (
                LogGrimrockPartySave,
                Error,
                TEXT ("[GridSaveMigration] SaveValidation Version=%d Result=Rejected Reason=%s"),
                SaveVersion,
                *CaptureError.ToString ());
            Ar.SetError ();
            return;
        }
    }

    Super::Serialize (Ar);

    if (!Ar.IsLoading ())
    {
        return;
    }

    FText MigrationError;
    FRPGSaveMigrationReport MigrationReport;
    bProgressionLoadValid = FRPGSaveMigrationService::PrepareLoadedSave (
        this,
        MigrationError,
        &MigrationReport);
    ProgressionLoadError = MigrationError.ToString ();

    if (!bProgressionLoadValid)
    {
        UE_LOG (
            LogGrimrockPartySave,
            Error,
            TEXT ("[GridSaveMigration] LoadValidation SourceVersion=%d Result=Rejected Reason=%s"),
            MigrationReport.SourceVersion,
            *ProgressionLoadError);
        return;
    }

    if (!FRPGClassProgressionTransactionService::RestorePersistentState (
            PartyInventoryState,
            ClassProgressionStates,
            MigrationError))
    {
        bProgressionLoadValid = false;
        ProgressionLoadError = MigrationError.ToString ();
        UE_LOG (
            LogGrimrockPartySave,
            Error,
            TEXT ("[GridSaveMigration] ProgressionRestore Result=Rejected Reason=%s"),
            *ProgressionLoadError);
        return;
    }

    URPGLevelUpNotificationSubsystem::RestorePersistentState (
        PendingLevelUpNotifications);

    UE_LOG (
        LogGrimrockPartySave,
        Log,
        TEXT ("[GridSaveMigration] Load SourceVersion=%d TargetVersion=%d Migrated=%s Reconciled=%d Choices=%d PendingLevelUps=%d Result=Accepted"),
        MigrationReport.SourceVersion,
        MigrationReport.TargetVersion,
        MigrationReport.bMigrated ? TEXT ("true") : TEXT ("false"),
        MigrationReport.ReconciledCharacterCount,
        ClassProgressionStates.Num (),
        PendingLevelUpNotifications.Num ());
}
