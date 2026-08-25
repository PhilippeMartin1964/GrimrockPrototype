#include "Save/GrimrockPartySaveGame.h"

#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpNotificationSubsystem.h"
#include "RPG/RPGSaveMigrationService.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Save/GridCombatSavePolicy.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockPartySave, Log, All);

namespace
{
	int32 CountCharacterId(const FGridPartyInventoryState& PartyState, const FGuid& CharacterId)
	{
		if (!CharacterId.IsValid())
		{
			return 0;
		}

		int32 Count = 0;
		for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			Count += Character.CharacterId == CharacterId ? 1 : 0;
		}
		for (const FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			Count += Character.CharacterId == CharacterId ? 1 : 0;
		}
		return Count;
	}

	FGridCharacterInventoryState* FindCharacterById(FGridPartyInventoryState& PartyState, const FGuid& CharacterId)
	{
		if (FGridCharacterInventoryState* Active = PartyState.ActiveCharacters.FindByPredicate(
				[&CharacterId](const FGridCharacterInventoryState& Character)
				{
					return Character.CharacterId == CharacterId;
				}))
		{
			return Active;
		}
		return PartyState.CharacterPool.FindByPredicate(
			[&CharacterId](const FGridCharacterInventoryState& Character)
			{
				return Character.CharacterId == CharacterId;
			});
	}

	void ResetRuntimeStatusEffects(FGridPartyInventoryState& PartyState)
	{
		for (FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			Character.StatusEffects.Reset();
		}
		for (FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			Character.StatusEffects.Reset();
		}
	}
}

bool UGrimrockPartySaveGame::CaptureStatusEffectState(FString& OutError)
{
	TArray<FGridCharacterStatusEffectSaveState> Candidate;

	const auto CaptureCharacters = [this, &Candidate, &OutError](const TArray<FGridCharacterInventoryState>& Characters) -> bool
	{
		for (const FGridCharacterInventoryState& Character : Characters)
		{
			if (Character.StatusEffects.IsEmpty())
			{
				continue;
			}
			if (!Character.CharacterId.IsValid() || CountCharacterId(PartyInventoryState, Character.CharacterId) != 1)
			{
				OutError = TEXT("A status-bearing party character has an invalid or ambiguous CharacterId.");
				return false;
			}

			FGridCharacterStatusEffectSaveState SavedCharacter;
			SavedCharacter.CharacterId = Character.CharacterId;
			if (!FGridStatusEffectPersistence::CaptureCollection(Character.StatusEffects, SavedCharacter.StatusEffects, OutError))
			{
				return false;
			}
			Candidate.Add(MoveTemp(SavedCharacter));
		}
		return true;
	};

	if (!CaptureCharacters(PartyInventoryState.ActiveCharacters) || !CaptureCharacters(PartyInventoryState.CharacterPool))
	{
		return false;
	}

	Candidate.Sort(
		[](const FGridCharacterStatusEffectSaveState& Left, const FGridCharacterStatusEffectSaveState& Right)
		{
			return Left.CharacterId.ToString(EGuidFormats::Digits) < Right.CharacterId.ToString(EGuidFormats::Digits);
		});

	CharacterStatusEffectStates = MoveTemp(Candidate);
	OutError.Reset();
	return true;
}

bool UGrimrockPartySaveGame::RestoreStatusEffectState(TFunctionRef<UGridStatusEffectDefinitionAsset*(FName)> DefinitionResolver, FString& OutError)
{
	FGridPartyInventoryState CandidateParty = PartyInventoryState;
	ResetRuntimeStatusEffects(CandidateParty);

	TSet<FGuid> RestoredCharacterIds;
	for (const FGridCharacterStatusEffectSaveState& SavedCharacter : CharacterStatusEffectStates)
	{
		if (!SavedCharacter.CharacterId.IsValid() || RestoredCharacterIds.Contains(SavedCharacter.CharacterId) ||
			CountCharacterId(CandidateParty, SavedCharacter.CharacterId) != 1)
		{
			OutError = TEXT("A saved status collection references an invalid, duplicated or ambiguous CharacterId.");
			return false;
		}

		FGridCharacterInventoryState* TargetCharacter = FindCharacterById(CandidateParty, SavedCharacter.CharacterId);
		if (!TargetCharacter)
		{
			OutError = TEXT("A saved status collection cannot resolve its party character.");
			return false;
		}

		FGridStatusEffectCollection RestoredCollection;
		if (!FGridStatusEffectPersistence::RestoreCollection(SavedCharacter.StatusEffects, DefinitionResolver, RestoredCollection, OutError))
		{
			return false;
		}

		TargetCharacter->StatusEffects = MoveTemp(RestoredCollection);
		RestoredCharacterIds.Add(SavedCharacter.CharacterId);
	}

	PartyInventoryState = MoveTemp(CandidateParty);
	OutError.Reset();
	return true;
}

bool UGrimrockPartySaveGame::RestoreStatusEffectState(FString& OutError)
{
	return RestoreStatusEffectState(
		[](FName EffectId)
		{
			return FGridStatusEffectPersistence::ResolveDefinitionByEffectId(EffectId);
		},
		OutError);
}

void UGrimrockPartySaveGame::Serialize(FArchive& Ar)
{
	if (Ar.IsSaving())
	{
		// MON18.9.1: the durable SaveGame intentionally excludes turn/round,
		// initiative, pending actions and presentation. Refuse every matching
		// party write while combat is active (and through Defeat) so callers
		// such as inventory-close and EndPlay cannot persist a partial fight.
		if (FGridCombatSavePolicy::IsSaveBlockedForParty(PartyInventoryState))
		{
			UE_LOG(LogGrimrockPartySave, Warning, TEXT("[MON18.9.1] SaveValidation Result=Rejected Reason=CombatStateNotSaveable"));
			Ar.SetError();
			return;
		}

		SaveVersion = CurrentSaveVersion;
		bProgressionLoadValid = true;
		ProgressionLoadError.Reset();

		FString StatusCaptureError;
		if (!CaptureStatusEffectState(StatusCaptureError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridStatusPersistence] SaveCapture Result=Rejected Reason=%s"), *StatusCaptureError);
			Ar.SetError();
			return;
		}

		FText CaptureError;
		if (!FRPGClassProgressionTransactionService::CapturePersistentState(PartyInventoryState, ClassProgressionStates, CaptureError) ||
			!URPGLevelUpNotificationSubsystem::CapturePersistentState(PartyInventoryState, PendingLevelUpNotifications, CaptureError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSaveMigration] SaveCapture Result=Rejected Reason=%s"), *CaptureError.ToString());
			Ar.SetError();
			return;
		}

		FString SkillCaptureError;
		if (!FRPGSkillPersistence::CapturePartySkills(PartyInventoryState, CharacterSkillStates, SkillCaptureError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSkillPersistence] SaveCapture Result=Rejected Reason=%s"), *SkillCaptureError);
			Ar.SetError();
			return;
		}

		UGrimrockPartySaveGame* MutableThis = this;
		if (!FRPGSaveMigrationService::ValidateCurrentSave(MutableThis, CaptureError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSaveMigration] SaveValidation Version=%d Result=Rejected Reason=%s"), SaveVersion,
				*CaptureError.ToString());
			Ar.SetError();
			return;
		}
	}

	Super::Serialize(Ar);

	if (!Ar.IsLoading())
	{
		return;
	}

	FText MigrationError;
	FRPGSaveMigrationReport MigrationReport;
	bProgressionLoadValid = FRPGSaveMigrationService::PrepareLoadedSave(this, MigrationError, &MigrationReport);
	ProgressionLoadError = MigrationError.ToString();

	if (!bProgressionLoadValid)
	{
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSaveMigration] LoadValidation SourceVersion=%d Result=Rejected Reason=%s"),
			MigrationReport.SourceVersion, *ProgressionLoadError);
		return;
	}

	FString StatusRestoreError;
	if (!RestoreStatusEffectState(StatusRestoreError))
	{
		bProgressionLoadValid = false;
		ProgressionLoadError = StatusRestoreError;
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridStatusPersistence] PartyRestore Result=Rejected Reason=%s"), *ProgressionLoadError);
		return;
	}

	if (!FRPGClassProgressionTransactionService::RestorePersistentState(PartyInventoryState, ClassProgressionStates, MigrationError))
	{
		bProgressionLoadValid = false;
		ProgressionLoadError = MigrationError.ToString();
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSaveMigration] ProgressionRestore Result=Rejected Reason=%s"), *ProgressionLoadError);
		return;
	}

	FString SkillRestoreError;
	if (!FRPGSkillPersistence::RestorePartySkills(PartyInventoryState, CharacterSkillStates, SkillRestoreError))
	{
		bProgressionLoadValid = false;
		ProgressionLoadError = SkillRestoreError;
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSkillPersistence] PartyRestore Result=Rejected Reason=%s"), *ProgressionLoadError);
		return;
	}

	URPGLevelUpNotificationSubsystem::RestorePersistentState(PendingLevelUpNotifications);

	UE_LOG(LogGrimrockPartySave, Log,
		TEXT(
			"[GridSaveMigration] Load SourceVersion=%d TargetVersion=%d Migrated=%s Reconciled=%d Choices=%d PendingLevelUps=%d StatusCharacters=%d SpellbookCharacters=%d SkillCharacters=%d Result=Accepted"),
		MigrationReport.SourceVersion, MigrationReport.TargetVersion, MigrationReport.bMigrated ? TEXT("true") : TEXT("false"),
		MigrationReport.ReconciledCharacterCount, ClassProgressionStates.Num(), PendingLevelUpNotifications.Num(), CharacterStatusEffectStates.Num(),
		CharacterSpellbookStates.Num(), CharacterSkillStates.Num());
}
