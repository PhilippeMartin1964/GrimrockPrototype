#include "Save/GrimrockPartySaveGame.h"

#include "Algo/Count.h"

#include "Magic/GridSpellbookPersistence.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionService.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/StatusEffects/GridStatusEffectPersistence.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Save/GridCombatSavePolicy.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimrockPartySave, Log, All);

namespace GridPartySaveValidationPrivate
{
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

	void RebuildTransientCharacterDerivedStats(FGridCharacterInventoryState& Character)
	{
		Character.DerivedStats =
			URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ResolveClassDefinition(Character), Character.Level);
	}

	void RebuildTransientPartyDerivedStats(FGridPartyInventoryState& PartyState)
	{
		for (FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			RebuildTransientCharacterDerivedStats(Character);
		}
		for (FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			RebuildTransientCharacterDerivedStats(Character);
		}
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

		const int32 MinimumLevel = URPGCharacterRulesLibrary::GetMinimumLevel();
		if (Character.LastAcknowledgedLevel < MinimumLevel || Character.LastAcknowledgedLevel > Character.Level)
		{
			OutError = FText::FromString(FString::Printf(
				TEXT("%s possède un LastAcknowledgedLevel invalide : Acknowledged=%d Level=%d."), Location, Character.LastAcknowledgedLevel, Character.Level));
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

	bool ValidateProgressionState(const FGridPartyInventoryState& PartyState, FText& OutError)
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

		return true;
	}

	bool ValidateSpellbooks(const UGrimrockPartySaveGame& SaveGame, FText& OutError)
	{
		FString SpellbookError;
		if (FGridSpellbookPersistence::ValidatePartySpellbooks(SaveGame.PartyInventoryState, SpellbookError))
		{
			return true;
		}
		OutError = FText::FromString(SpellbookError);
		return false;
	}

	bool ValidateSkills(const UGrimrockPartySaveGame& SaveGame, FText& OutError)
	{
		FString SkillError;
		if (FRPGSkillPersistence::ValidatePartySkills(SaveGame.PartyInventoryState, SkillError))
		{
			return true;
		}
		OutError = FText::FromString(SkillError);
		return false;
	}

	bool ValidateStatusEffects(const UGrimrockPartySaveGame& SaveGame, FText& OutError)
	{
		FString StatusError;
		if (FGridStatusEffectPersistence::ValidatePartyStatusEffects(SaveGame.PartyInventoryState, StatusError))
		{
			return true;
		}
		OutError = FText::FromString(StatusError);
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

	int32 CountCharactersWithKnownSpells(const FGridPartyInventoryState& PartyState)
	{
		int32 Count = 0;
		for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			Count += Character.KnownSpellIds.IsEmpty() ? 0 : 1;
		}
		for (const FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			Count += Character.KnownSpellIds.IsEmpty() ? 0 : 1;
		}
		return Count;
	}

	int32 CountCharactersWithStatusEffects(const FGridPartyInventoryState& PartyState)
	{
		int32 Count = 0;
		for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
		{
			Count += Character.StatusEffects.IsEmpty() ? 0 : 1;
		}
		for (const FGridCharacterInventoryState& Character : PartyState.CharacterPool)
		{
			Count += Character.StatusEffects.IsEmpty() ? 0 : 1;
		}
		return Count;
	}
}

using namespace GridPartySaveValidationPrivate;

bool UGrimrockPartySaveGame::ValidateCurrentState(FText& OutError) const
{
	OutError = FText::GetEmpty();
	if (SaveVersion != CurrentSaveVersion)
	{
		OutError = FText::FromString(FString::Printf(TEXT("Version de sauvegarde %d incompatible avec le schéma prototype courant %d."), SaveVersion, CurrentSaveVersion));
		return false;
	}
	if (!ValidateProgressionState(PartyInventoryState, OutError) || !ValidateSpellbooks(*this, OutError) ||
		!ValidateSkills(*this, OutError) || !ValidateStatusEffects(*this, OutError) || !ValidateLevelVariables(*this, OutError))
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
		FString StatusValidationError;
		if (!FGridStatusEffectPersistence::ValidateRuntimePartyStatusEffects(PartyInventoryState, StatusValidationError))
		{
			UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridStatusPersistence] SaveValidation Result=Rejected Reason=%s"), *StatusValidationError);
			Ar.SetError();
			return;
		}
		FText CaptureError;
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
	RebuildTransientPartyDerivedStats(PartyInventoryState);
	FText ValidationError;
	if (!ValidateCurrentState(ValidationError))
	{
		bLoadValid = false;
		LoadError = ValidationError.ToString();
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSave] LoadValidation Version=%d Result=Rejected Reason=%s"), SaveVersion, *LoadError);
		return;
	}
	FString StatusRehydrateError;
	if (!FGridStatusEffectPersistence::RehydratePartyStatusEffects(PartyInventoryState, StatusRehydrateError))
	{
		bLoadValid = false;
		LoadError = StatusRehydrateError;
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridStatusPersistence] PartyRehydrate Result=Rejected Reason=%s"), *LoadError);
		return;
	}
	if (!FRPGClassProgressionTransactionService::RebuildRuntimeProjection(PartyInventoryState, ValidationError))
	{
		bLoadValid = false;
		LoadError = ValidationError.ToString();
		UE_LOG(LogGrimrockPartySave, Error, TEXT("[GridSave] ProgressionProjection Result=Rejected Reason=%s"), *LoadError);
		return;
	}
	const int32 PendingLevelUpAcknowledgements = Algo::CountIf(
		PartyInventoryState.ActiveCharacters,
		[](const FGridCharacterInventoryState& Character)
		{
			return Character.LastAcknowledgedLevel < Character.Level;
		});
	UE_LOG(LogGrimrockPartySave, Log,
		TEXT("[GridSave] Load Version=%d ClassChoices=%d PendingLevelUps=%d StatusCharacters=%d KnownSpellCharacters=%d Result=Accepted"),
		SaveVersion, CountSelectedClassChoices(PartyInventoryState), PendingLevelUpAcknowledgements,
		CountCharactersWithStatusEffects(PartyInventoryState), CountCharactersWithKnownSpells(PartyInventoryState));
}
