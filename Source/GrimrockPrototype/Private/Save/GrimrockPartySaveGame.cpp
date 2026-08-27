#include "Save/GrimrockPartySaveGame.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpNotificationSubsystem.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Save/GridCombatSavePolicy.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockPartySave, Log, All);

namespace GridPartySaveValidationPrivate
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

	void RebuildTransientCharacterLevel(FGridCharacterInventoryState& Character)
	{
		Character.Level = URPGCharacterRulesLibrary::GetLevelForExperience(Character.Experience);
	}

	void RebuildTransientPartyLevels(FGridPartyInventoryState& PartyState)
	{
		for (FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			RebuildTransientCharacterLevel(Character);
		}
		for (FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			RebuildTransientCharacterLevel(Character);
		}
	}

	URPGClassAsset* ResolveClassDefinition(const FGridCharacterInventoryState& Character)
	{
		URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!ClassDefinition && !Character.ClassDefinition.IsNull())
		{
			ClassDefinition = Character.ClassDefinition.LoadSynchronous();
		}
		if (!IsValid(ClassDefinition) || !ClassDefinition->IsValidDefinition() ||
			(!Character.ClassId.IsNone() && ClassDefinition->ClassId != Character.ClassId))
		{
			return nullptr;
		}
		return ClassDefinition;
	}

	bool BuildSelectionSet(const TArray<FName>& ChoiceIds, TSet<FName>& OutChoiceIds)
	{
		OutChoiceIds.Reset();
		for (const FName ChoiceId : ChoiceIds)
		{
			if (ChoiceId.IsNone() || OutChoiceIds.Contains(ChoiceId))
			{
				OutChoiceIds.Reset();
				return false;
			}
			OutChoiceIds.Add(ChoiceId);
		}
		return true;
	}

	bool ValidateCharacterProgression(const FGridCharacterInventoryState& Character, const TCHAR* Location, FText& OutError)
	{
		if (Character.Experience != URPGCharacterRulesLibrary::NormalizeExperience(Character.Experience))
		{
			OutError = FText::FromString(
				FString::Printf(TEXT("%s possède une valeur Experience invalide : %d."), Location, Character.Experience));
			return false;
		}

		const int32 ExpectedLevel = URPGCharacterRulesLibrary::GetLevelForExperience(Character.Experience);
		if (Character.Level != ExpectedLevel)
		{
			OutError = FText::FromString(FString::Printf(
				TEXT("%s possède un cache Level incohérent : Level=%d Expected=%d Experience=%d."), Location, Character.Level, ExpectedLevel, Character.Experience));
			return false;
		}

		TSet<FName> SelectedChoiceIds;
		if (!BuildSelectionSet(Character.SelectedClassProgressionChoiceIds, SelectedChoiceIds))
		{
			OutError = FText::FromString(FString::Printf(TEXT("%s contient un ChoiceId de progression vide ou dupliqué."), Location));
			return false;
		}

		if (SelectedChoiceIds.IsEmpty())
		{
			return true;
		}

		URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character);
		if (!ClassDefinition)
		{
			OutError = FText::FromString(FString::Printf(TEXT("%s possède des choix de progression sans définition de classe valide."), Location));
			return false;
		}

		int32 GrantedPoints = 0;
		int32 SpentPoints = 0;
		int32 RemainingPoints = 0;
		if (!FRPGClassProgressionService::TryGetChoicePointBalance(
				ClassDefinition, Character.Level, SelectedChoiceIds, GrantedPoints, SpentPoints, RemainingPoints))
		{
			OutError = FText::FromString(
				FString::Printf(TEXT("%s possède des choix de progression incompatibles avec son niveau, son budget ou ses prérequis."), Location));
			return false;
		}
		return true;
	}

	bool ValidateProgressionState(
		const FGridPartyInventoryState& PartyState, const TArray<FRPGPendingLevelUpSaveState>& PendingNotifications, FText& OutError)
	{
		OutError = FText::GetEmpty();

		TSet<FGuid> ActiveCharacterIds;
		for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
		{
			const FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[CharacterIndex];
			if (!Character.CharacterId.IsValid() || ActiveCharacterIds.Contains(Character.CharacterId))
			{
				OutError = FText::FromString(TEXT("Les CharacterId des personnages actifs sont invalides ou dupliqués."));
				return false;
			}
			ActiveCharacterIds.Add(Character.CharacterId);

			const FString Location = FString::Printf(TEXT("ActiveCharacter[%d]"), CharacterIndex);
			if (!ValidateCharacterProgression(Character, *Location, OutError))
			{
				return false;
			}
		}

		for (int32 PoolIndex = 0; PoolIndex < PartyState.CharacterPool.Num(); ++PoolIndex)
		{
			const FString Location = FString::Printf(TEXT("CharacterPool[%d]"), PoolIndex);
			if (!ValidateCharacterProgression(PartyState.CharacterPool[PoolIndex], *Location, OutError))
			{
				return false;
			}
		}

		TSet<FGuid> PendingCharacterIds;
		for (const FRPGPendingLevelUpSaveState& Pending : PendingNotifications)
		{
			if (!Pending.CharacterId.IsValid() || PendingCharacterIds.Contains(Pending.CharacterId) || !ActiveCharacterIds.Contains(Pending.CharacterId))
			{
				OutError = FText::FromString(TEXT("Une notification Level Up persistante référence un CharacterId invalide, dupliqué ou non actif."));
				return false;
			}
			PendingCharacterIds.Add(Pending.CharacterId);

			const FGridCharacterInventoryState* Character = PartyState.ActiveCharacters.FindByPredicate(
				[&Pending](const FGridCharacterInventoryState& Candidate)
				{
					return Candidate.CharacterId == Pending.CharacterId;
				});
			if (!Character || Pending.PreviousLevel < URPGCharacterRulesLibrary::GetMinimumLevel() ||
				Pending.NewLevel > URPGCharacterRulesLibrary::GetMaximumLevel() || Pending.PreviousLevel >= Pending.NewLevel ||
				Pending.LevelsGained != Pending.NewLevel - Pending.PreviousLevel || Pending.NewLevel != Character->Level)
			{
				OutError = FText::FromString(TEXT("Une notification Level Up persistante est incohérente avec le niveau actuel du personnage."));
				return false;
			}
		}
		return true;
	}

	bool ValidateSpellbooks(const UGrimrockPartySaveGame& SaveGame, FText& OutError)
	{
		FString SpellbookError;
		if (FGridSpellbookPersistence::ValidateSavedPartySpellbooks(SaveGame.PartyInventoryState, SaveGame.CharacterSpellbookStates, SpellbookError))
		{
			return true;
		}
		OutError = FText::FromString(SpellbookError);
		return false;
	}

	bool ValidateSkills(const UGrimrockPartySaveGame& SaveGame, FText& OutError)
	{
		FString SkillError;
		if (FRPGSkillPersistence::ValidateSavedPartySkills(SaveGame.PartyInventoryState, SaveGame.CharacterSkillStates, SkillError))
		{
			return true;
		}
		OutError = FText::FromString(SkillError);
		return false;
	}

	bool ValidateLevelVariables(const UGrimrockPartySaveGame& SaveGame, FText& OutError)
	{
		FString VariableError;
		if (GridLevelVariableStore::ValidateDungeonSnapshots(SaveGame.DungeonRuntimeState, VariableError))
		{
			return true;
		}
		OutError = FText::FromString(VariableError);
		return false;
	}

	int32 CountSelectedClassChoices(const FGridPartyInventoryState& PartyState)
	{
		int32 Count = 0;
		for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			Count += Character.SelectedClassProgressionChoiceIds.Num();
		}
		for (const FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			Count += Character.SelectedClassProgressionChoiceIds.Num();
		}
		return Count;
	}
}

using namespace GridPartySaveValidationPrivate;

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
	Candidate.Sort([](const FGridCharacterStatusEffectSaveState& Left, const FGridCharacterStatusEffectSaveState& Right)
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
		if (!SavedCharacter.CharacterId.IsValid() || RestoredCharacterIds.Contains(SavedCharacter.CharacterId) || CountCharacterId(CandidateParty, SavedCharacter.CharacterId) != 1)
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
	return RestoreStatusEffectState([](FName EffectId)
	{
		return FGridStatusEffectPersistence::ResolveDefinitionByEffectId(EffectId);
	}, OutError);
}

bool UGrimrockPartySaveGame::ValidateCurrentState(FText& OutError) const
{
	OutError = FText::GetEmpty();
	if (SaveVersion != CurrentSaveVersion)
	{
		OutError = FText::FromString(FString::Printf(TEXT("Version de sauvegarde %d incompatible avec le schéma prototype courant %d."), SaveVersion, CurrentSaveVersion));
		return false;
	}
	if (!ValidateProgressionState(PartyInventoryState, PendingLevelUpNotifications, OutError) || !ValidateSpellbooks(*this, OutError) ||
		!ValidateSkills(*this, OutError) || !ValidateLevelVariables(*this, OutError))
	{
		return false;
	}
	return true;
}

void UGrimrockPartySaveGame::Serialize(FArchive& Ar)
{
	if (Ar.IsSaving())
	{
		if (FGridCombatSavePolicy::IsSaveBlockedForParty(PartyInventoryState))
		{
			UE_LOG(LogGrimrockPartySave, Warning, TEXT("[GridSave] SaveValidation Result=Rejected Reason=CombatStateNotSaveable"));
			Ar.SetError();
			return;
		}
		SaveVersion = CurrentSaveVersion;
		bLoadValid = true;
		LoadError.Reset();
		ClassProgressionStates.Reset();
		FString StatusCaptureError;
		if (!CaptureStatusEffectState(StatusCaptureError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridStatusPersistence] SaveCapture Result=Rejected Reason=%s"), *StatusCaptureError);
			Ar.SetError();
			return;
		}
		FText CaptureError;
		if (!URPGLevelUpNotificationSubsystem::CapturePersistentState(PartyInventoryState, PendingLevelUpNotifications, CaptureError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSave] SaveCapture Result=Rejected Reason=%s"), *CaptureError.ToString());
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
		if (!ValidateCurrentState(CaptureError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSave] SaveValidation Version=%d Result=Rejected Reason=%s"), SaveVersion, *CaptureError.ToString());
			Ar.SetError();
			return;
		}
	}

	Super::Serialize(Ar);
	if (!Ar.IsLoading())
	{
		return;
	}
	bLoadValid = true;
	LoadError.Reset();
	RebuildTransientPartyLevels(PartyInventoryState);
	FText ValidationError;
	if (!ValidateCurrentState(ValidationError))
	{
		bLoadValid = false;
		LoadError = ValidationError.ToString();
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSave] LoadValidation Version=%d Result=Rejected Reason=%s"), SaveVersion, *LoadError);
		return;
	}
	FString StatusRestoreError;
	if (!RestoreStatusEffectState(StatusRestoreError))
	{
		bLoadValid = false;
		LoadError = StatusRestoreError;
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridStatusPersistence] PartyRestore Result=Rejected Reason=%s"), *LoadError);
		return;
	}
	if (!FRPGClassProgressionTransactionService::RebuildRuntimeProjection(PartyInventoryState, ValidationError))
	{
		bLoadValid = false;
		LoadError = ValidationError.ToString();
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSave] ProgressionProjection Result=Rejected Reason=%s"), *LoadError);
		return;
	}
	FString SkillRestoreError;
	if (!FRPGSkillPersistence::RestorePartySkills(PartyInventoryState, CharacterSkillStates, SkillRestoreError))
	{
		bLoadValid = false;
		LoadError = SkillRestoreError;
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSkillPersistence] PartyRestore Result=Rejected Reason=%s"), *LoadError);
		return;
	}
	URPGLevelUpNotificationSubsystem::RestorePersistentState(PendingLevelUpNotifications);
	UE_LOG(LogGrimrockPartySave, Log,
		TEXT("[GridSave] Load Version=%d ClassChoices=%d PendingLevelUps=%d StatusCharacters=%d SpellbookCharacters=%d SkillCharacters=%d Result=Accepted"),
		SaveVersion, CountSelectedClassChoices(PartyInventoryState), PendingLevelUpNotifications.Num(), CharacterStatusEffectStates.Num(),
		CharacterSpellbookStates.Num(), CharacterSkillStates.Num());
}
