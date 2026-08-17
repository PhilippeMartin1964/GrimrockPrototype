#include "RPG/RPGSaveMigrationService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "Save/GrimrockPartySaveGame.h"

namespace MON156SaveMigrationPrivate
{
    int32 PreserveHealthDeficit (
        const FRPGDerivedStats& PreviousStats,
        int32 NewMaximumHealth)
    {
        const int32 SafeNewMaximum = FMath::Max (1, NewMaximumHealth);
        const int32 SafePreviousMaximum = FMath::Max (1, PreviousStats.MaxHealth);
        const int32 SafePreviousCurrent = FMath::Clamp (
            PreviousStats.CurrentHealth,
            0,
            SafePreviousMaximum);
        if (SafePreviousCurrent <= 0)
        {
            return 0;
        }
        const int32 DamageTaken = SafePreviousMaximum - SafePreviousCurrent;
        return FMath::Clamp (
            SafeNewMaximum - DamageTaken,
            0,
            SafeNewMaximum);
    }

    int32 PreserveManaDeficit (
        const FRPGDerivedStats& PreviousStats,
        int32 NewMaximumMana)
    {
        const int32 SafeNewMaximum = FMath::Max (0, NewMaximumMana);
        const int32 SafePreviousMaximum = FMath::Max (0, PreviousStats.MaxMana);
        const int32 SafePreviousCurrent = FMath::Clamp (
            PreviousStats.CurrentMana,
            0,
            SafePreviousMaximum);
        const int32 ManaSpent = SafePreviousMaximum - SafePreviousCurrent;
        return FMath::Clamp (
            SafeNewMaximum - ManaSpent,
            0,
            SafeNewMaximum);
    }

    URPGClassAsset* ResolveClassDefinition (
        const FGridCharacterInventoryState& Character)
    {
        URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get ();
        if (!ClassDefinition && !Character.ClassDefinition.IsNull ())
        {
            ClassDefinition = Character.ClassDefinition.LoadSynchronous ();
        }
        if (!IsValid (ClassDefinition) ||
            !ClassDefinition->IsValidDefinition () ||
            (!Character.ClassId.IsNone () &&
                ClassDefinition->ClassId != Character.ClassId))
        {
            return nullptr;
        }
        return ClassDefinition;
    }

    bool BuildSelectionSet (
        const TArray<FName>& ChoiceIds,
        TSet<FName>& OutChoiceIds)
    {
        OutChoiceIds.Reset ();
        for (const FName ChoiceId : ChoiceIds)
        {
            if (ChoiceId.IsNone () || OutChoiceIds.Contains (ChoiceId))
            {
                OutChoiceIds.Reset ();
                return false;
            }
            OutChoiceIds.Add (ChoiceId);
        }
        return true;
    }

    void ResetLegacyPartyStatusEffects (
        FGridPartyInventoryState& PartyState)
    {
        for (FGridCharacterInventoryState& Character :
            PartyState.ActiveCharacters)
        {
            Character.StatusEffects.Reset ();
        }
        for (FGridCharacterInventoryState& Character :
            PartyState.CharacterPool)
        {
            Character.StatusEffects.Reset ();
        }
    }

    void ResetLegacyMonsterStatusEffects (
        FGridDungeonRuntimeState& DungeonState)
    {
        for (TPair<FName, FGridLevelRuntimeState>& LevelPair :
            DungeonState.LevelStates)
        {
            for (TPair<FGuid, FGridRuntimeMonsterState>& MonsterPair :
                LevelPair.Value.Monsters)
            {
                MonsterPair.Value.StatusEffects.Reset ();
            }
            for (TPair<FGuid, FGridRuntimeMonsterPlacementState>& PlacementPair :
                LevelPair.Value.MonsterPlacements)
            {
                PlacementPair.Value.MonsterState.StatusEffects.Reset ();
            }
        }
    }

    bool MigrateLegacyCharacter (
        FGridCharacterInventoryState& Character,
        int32& InOutReconciledCount,
        FText& OutError)
    {
        const int32 OriginalLevel = Character.Level;
        const int32 OriginalExperience = Character.Experience;
        const FRPGDerivedStats OriginalStats = Character.DerivedStats;

        const int32 SafeStoredLevel = FMath::Clamp (
            OriginalLevel,
            URPGCharacterRulesLibrary::GetMinimumLevel (),
            URPGCharacterRulesLibrary::GetMaximumLevel ());
        const int32 NormalizedExperience =
            URPGCharacterRulesLibrary::NormalizeExperience (OriginalExperience);
        const int32 ExperienceFloorForStoredLevel =
            URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (
                SafeStoredLevel);
        const int32 ReconciledExperience = FMath::Max (
            NormalizedExperience,
            ExperienceFloorForStoredLevel);
        const int32 ReconciledLevel =
            URPGCharacterRulesLibrary::GetLevelForExperience (
                ReconciledExperience);

        const bool bNeedsReconciliation =
            OriginalLevel != ReconciledLevel ||
            OriginalExperience != ReconciledExperience;
        if (!bNeedsReconciliation)
        {
            return true;
        }

        if (OriginalLevel != ReconciledLevel)
        {
            URPGClassAsset* ClassDefinition = ResolveClassDefinition (Character);
            if (!ClassDefinition)
            {
                OutError = FText::FromString (
                    FString::Printf (
                        TEXT ("La sauvegarde legacy du personnage %s nécessite un recalcul de niveau mais sa définition de classe est invalide."),
                        Character.CharacterId.IsValid ()
                            ? *Character.CharacterId.ToString (EGuidFormats::Digits)
                            : TEXT ("Unknown")));
                return false;
            }

            FRPGDerivedStats RecalculatedStats =
                URPGCharacterRulesLibrary::CalculateDerivedStats (
                    Character.Attributes,
                    ClassDefinition,
                    ReconciledLevel);
            RecalculatedStats.CurrentHealth = PreserveHealthDeficit (
                OriginalStats,
                RecalculatedStats.MaxHealth);
            RecalculatedStats.CurrentMana = PreserveManaDeficit (
                OriginalStats,
                RecalculatedStats.MaxMana);
            Character.DerivedStats = RecalculatedStats;
        }

        Character.Level = ReconciledLevel;
        Character.Experience = ReconciledExperience;
        ++InOutReconciledCount;
        return true;
    }

    bool ValidateCharacterProgression (
        const FGridCharacterInventoryState& Character,
        const TCHAR* Location,
        FText& OutError)
    {
        if (!URPGCharacterRulesLibrary::IsLevelExperienceConsistent (
                Character.Level,
                Character.Experience))
        {
            OutError = FText::FromString (
                FString::Printf (
                    TEXT ("%s possède un couple Level/Experience incohérent : Level=%d Experience=%d."),
                    Location,
                    Character.Level,
                    Character.Experience));
            return false;
        }
        return true;
    }

    bool ValidateSnapshot (
        const FGridPartyInventoryState& PartyState,
        const TArray<FRPGCharacterProgressionSaveState>& ProgressionStates,
        const TArray<FRPGPendingLevelUpSaveState>& PendingNotifications,
        FText& OutError)
    {
        OutError = FText::GetEmpty ();

        TSet<FGuid> ActiveCharacterIds;
        for (int32 CharacterIndex = 0;
            CharacterIndex < PartyState.ActiveCharacters.Num ();
            ++CharacterIndex)
        {
            const FGridCharacterInventoryState& Character =
                PartyState.ActiveCharacters[CharacterIndex];
            if (!Character.CharacterId.IsValid () ||
                ActiveCharacterIds.Contains (Character.CharacterId))
            {
                OutError = FText::FromString (
                    TEXT ("Les CharacterId des personnages actifs sont invalides ou dupliqués."));
                return false;
            }
            ActiveCharacterIds.Add (Character.CharacterId);

            const FString Location = FString::Printf (
                TEXT ("ActiveCharacter[%d]"),
                CharacterIndex);
            if (!ValidateCharacterProgression (
                    Character,
                    *Location,
                    OutError))
            {
                return false;
            }
        }

        for (int32 PoolIndex = 0;
            PoolIndex < PartyState.CharacterPool.Num ();
            ++PoolIndex)
        {
            const FString Location = FString::Printf (
                TEXT ("CharacterPool[%d]"),
                PoolIndex);
            if (!ValidateCharacterProgression (
                    PartyState.CharacterPool[PoolIndex],
                    *Location,
                    OutError))
            {
                return false;
            }
        }

        if (ProgressionStates.Num () != PartyState.ActiveCharacters.Num ())
        {
            OutError = FText::FromString (
                FString::Printf (
                    TEXT ("Le snapshot contient %d états de progression pour %d personnages actifs."),
                    ProgressionStates.Num (),
                    PartyState.ActiveCharacters.Num ()));
            return false;
        }

        TMap<FGuid, const FRPGCharacterProgressionSaveState*> ProgressionById;
        for (const FRPGCharacterProgressionSaveState& SavedState : ProgressionStates)
        {
            if (!SavedState.CharacterId.IsValid () ||
                ProgressionById.Contains (SavedState.CharacterId) ||
                !ActiveCharacterIds.Contains (SavedState.CharacterId))
            {
                OutError = FText::FromString (
                    TEXT ("Un état de progression référence un CharacterId invalide, dupliqué ou non actif."));
                return false;
            }
            ProgressionById.Add (SavedState.CharacterId, &SavedState);
        }

        for (const FGridCharacterInventoryState& Character :
            PartyState.ActiveCharacters)
        {
            const FRPGCharacterProgressionSaveState* const* SavedStatePtr =
                ProgressionById.Find (Character.CharacterId);
            if (!SavedStatePtr || !*SavedStatePtr)
            {
                OutError = FText::FromString (
                    TEXT ("Un personnage actif ne possède aucun état de progression sauvegardé."));
                return false;
            }

            TSet<FName> SelectedChoiceIds;
            if (!BuildSelectionSet (
                    (*SavedStatePtr)->SelectedChoiceIds,
                    SelectedChoiceIds))
            {
                OutError = FText::FromString (
                    TEXT ("Un état de progression contient un ChoiceId vide ou dupliqué."));
                return false;
            }

            if (SelectedChoiceIds.IsEmpty ())
            {
                continue;
            }

            URPGClassAsset* ClassDefinition = ResolveClassDefinition (Character);
            if (!ClassDefinition)
            {
                OutError = FText::FromString (
                    TEXT ("Une progression non vide ne peut pas être validée sans définition de classe."));
                return false;
            }

            int32 GrantedPoints = 0;
            int32 SpentPoints = 0;
            int32 RemainingPoints = 0;
            if (!FRPGClassProgressionService::TryGetChoicePointBalance (
                    ClassDefinition,
                    Character.Level,
                    SelectedChoiceIds,
                    GrantedPoints,
                    SpentPoints,
                    RemainingPoints))
            {
                OutError = FText::FromString (
                    TEXT ("Les choix de progression sauvegardés ne respectent plus le niveau, le budget ou les prérequis de la classe."));
                return false;
            }
        }

        TSet<FGuid> PendingCharacterIds;
        for (const FRPGPendingLevelUpSaveState& Pending : PendingNotifications)
        {
            if (!Pending.CharacterId.IsValid () ||
                PendingCharacterIds.Contains (Pending.CharacterId) ||
                !ActiveCharacterIds.Contains (Pending.CharacterId))
            {
                OutError = FText::FromString (
                    TEXT ("Une notification Level Up persistante référence un CharacterId invalide, dupliqué ou non actif."));
                return false;
            }
            PendingCharacterIds.Add (Pending.CharacterId);

            const FGridCharacterInventoryState* Character =
                PartyState.ActiveCharacters.FindByPredicate (
                    [&Pending] (const FGridCharacterInventoryState& Candidate)
                    {
                        return Candidate.CharacterId == Pending.CharacterId;
                    });
            if (!Character ||
                Pending.PreviousLevel <
                    URPGCharacterRulesLibrary::GetMinimumLevel () ||
                Pending.NewLevel >
                    URPGCharacterRulesLibrary::GetMaximumLevel () ||
                Pending.PreviousLevel >= Pending.NewLevel ||
                Pending.LevelsGained !=
                    Pending.NewLevel - Pending.PreviousLevel ||
                Pending.NewLevel != Character->Level)
            {
                OutError = FText::FromString (
                    TEXT ("Une notification Level Up persistante est incohérente avec le niveau actuel du personnage."));
                return false;
            }
        }
        return true;
    }
}

using namespace MON156SaveMigrationPrivate;

bool FRPGSaveMigrationService::PrepareLoadedSave (
    UGrimrockPartySaveGame* SaveGame,
    FText& OutError,
    FRPGSaveMigrationReport* OutReport)
{
    OutError = FText::GetEmpty ();
    if (OutReport)
    {
        *OutReport = FRPGSaveMigrationReport ();
    }
    if (!SaveGame)
    {
        OutError = FText::FromString (TEXT ("Le SaveGame est nul."));
        return false;
    }

    const int32 SourceVersion = SaveGame->SaveVersion;
    if (OutReport)
    {
        OutReport->SourceVersion = SourceVersion;
        OutReport->TargetVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
    }

    if (SourceVersion < UGrimrockPartySaveGame::MinimumCompatibleSaveVersion ||
        SourceVersion > UGrimrockPartySaveGame::CurrentSaveVersion)
    {
        OutError = FText::FromString (
            FString::Printf (
                TEXT ("Version de sauvegarde %d hors de la plage compatible %d-%d."),
                SourceVersion,
                UGrimrockPartySaveGame::MinimumCompatibleSaveVersion,
                UGrimrockPartySaveGame::CurrentSaveVersion));
        return false;
    }

    if (SourceVersion == UGrimrockPartySaveGame::CurrentSaveVersion)
    {
        return ValidateCurrentSave (SaveGame, OutError);
    }

    // MON16.7: v4 is already authoritative for MON15 progression. Do not run
    // the v1-v3 reconstruction path, which would erase confirmed class choices.
    if (SourceVersion == 4)
    {
        if (!ValidateSnapshot (
                SaveGame->PartyInventoryState,
                SaveGame->ClassProgressionStates,
                SaveGame->PendingLevelUpNotifications,
                OutError))
        {
            return false;
        }

        ResetLegacyPartyStatusEffects (SaveGame->PartyInventoryState);
        ResetLegacyMonsterStatusEffects (SaveGame->DungeonRuntimeState);
        SaveGame->CharacterStatusEffectStates.Reset ();
        SaveGame->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;

        if (OutReport)
        {
            OutReport->ReconciledCharacterCount = 0;
            OutReport->bMigrated = true;
        }
        return true;
    }

    FGridPartyInventoryState MigratedPartyState = SaveGame->PartyInventoryState;
    ResetLegacyPartyStatusEffects (MigratedPartyState);
    int32 ReconciledCharacterCount = 0;
    for (FGridCharacterInventoryState& Character :
        MigratedPartyState.ActiveCharacters)
    {
        if (!MigrateLegacyCharacter (
                Character,
                ReconciledCharacterCount,
                OutError))
        {
            return false;
        }
    }
    for (FGridCharacterInventoryState& Character :
        MigratedPartyState.CharacterPool)
    {
        if (!MigrateLegacyCharacter (
                Character,
                ReconciledCharacterCount,
                OutError))
        {
            return false;
        }
    }

    TArray<FRPGCharacterProgressionSaveState> MigratedProgressionStates;
    MigratedProgressionStates.Reserve (
        MigratedPartyState.ActiveCharacters.Num ());
    for (const FGridCharacterInventoryState& Character :
        MigratedPartyState.ActiveCharacters)
    {
        FRPGCharacterProgressionSaveState ProgressionState;
        ProgressionState.CharacterId = Character.CharacterId;
        MigratedProgressionStates.Add (MoveTemp (ProgressionState));
    }

    TArray<FRPGPendingLevelUpSaveState> MigratedPendingNotifications;
    if (!ValidateSnapshot (
            MigratedPartyState,
            MigratedProgressionStates,
            MigratedPendingNotifications,
            OutError))
    {
        return false;
    }

    FGridDungeonRuntimeState MigratedDungeonState =
        SaveGame->DungeonRuntimeState;
    ResetLegacyMonsterStatusEffects (MigratedDungeonState);

    SaveGame->PartyInventoryState = MoveTemp (MigratedPartyState);
    SaveGame->ClassProgressionStates = MoveTemp (MigratedProgressionStates);
    SaveGame->PendingLevelUpNotifications = MoveTemp (MigratedPendingNotifications);
    SaveGame->CharacterStatusEffectStates.Reset ();
    SaveGame->DungeonRuntimeState = MoveTemp (MigratedDungeonState);
    SaveGame->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;

    if (OutReport)
    {
        OutReport->ReconciledCharacterCount = ReconciledCharacterCount;
        OutReport->bMigrated = true;
    }
    return true;
}

bool FRPGSaveMigrationService::ValidateCurrentSave (
    UGrimrockPartySaveGame* SaveGame,
    FText& OutError)
{
    OutError = FText::GetEmpty ();
    if (!SaveGame)
    {
        OutError = FText::FromString (TEXT ("Le SaveGame est nul."));
        return false;
    }
    if (SaveGame->SaveVersion != UGrimrockPartySaveGame::CurrentSaveVersion)
    {
        OutError = FText::FromString (
            FString::Printf (
                TEXT ("La validation stricte attend la version %d et reçoit la version %d."),
                UGrimrockPartySaveGame::CurrentSaveVersion,
                SaveGame->SaveVersion));
        return false;
    }

    return ValidateSnapshot (
        SaveGame->PartyInventoryState,
        SaveGame->ClassProgressionStates,
        SaveGame->PendingLevelUpNotifications,
        OutError);
}
