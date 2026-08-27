#include "RPG/RPGLevelUpService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridLevelUp, Log, All);

namespace
{
	int32 PreserveHealthDeficit(const FRPGDerivedStats& PreviousStats, const FRPGCharacterResources& PreviousResources, int32 NewMaximumHealth)
	{
		const int32 SafeNewMaximum = FMath::Max(1, NewMaximumHealth);
		const int32 SafePreviousMaximum = FMath::Max(1, PreviousStats.MaxHealth);
		const int32 SafePreviousCurrent = FMath::Clamp(PreviousResources.CurrentHealth, 0, SafePreviousMaximum);

		// A level-up must never resurrect a defeated character.
		if (SafePreviousCurrent <= 0)
		{
			return 0;
		}

		const int32 DamageTaken = SafePreviousMaximum - SafePreviousCurrent;
		return FMath::Clamp(SafeNewMaximum - DamageTaken, 0, SafeNewMaximum);
	}

	int32 PreserveManaDeficit(const FRPGDerivedStats& PreviousStats, const FRPGCharacterResources& PreviousResources, int32 NewMaximumMana)
	{
		const int32 SafeNewMaximum = FMath::Max(0, NewMaximumMana);
		const int32 SafePreviousMaximum = FMath::Max(0, PreviousStats.MaxMana);
		const int32 SafePreviousCurrent = FMath::Clamp(PreviousResources.CurrentMana, 0, SafePreviousMaximum);
		const int32 ManaSpent = SafePreviousMaximum - SafePreviousCurrent;
		return FMath::Clamp(SafeNewMaximum - ManaSpent, 0, SafeNewMaximum);
	}

	URPGClassAsset* ResolveCharacterClassDefinition(FGridCharacterInventoryState& Character)
	{
		URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!ClassDefinition && !Character.ClassDefinition.IsNull())
		{
			ClassDefinition = Character.ClassDefinition.LoadSynchronous();
		}
		return ClassDefinition;
	}
}

FGridCharacterLevelUpAppliedNativeSignature& FRPGLevelUpService::OnCharacterLevelUpApplied()
{
	static FGridCharacterLevelUpAppliedNativeSignature Delegate;
	return Delegate;
}

FGridCharacterLevelUpAppliedWithSourceNativeSignature& FRPGLevelUpService::OnCharacterLevelUpAppliedWithSource()
{
	static FGridCharacterLevelUpAppliedWithSourceNativeSignature Delegate;
	return Delegate;
}

bool FRPGLevelUpService::ApplyPendingLevelUp(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, bool bNotifyPartyInventoryChanged)
{
	if (!IsValid(PartyInventoryComponent) || !PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		return false;
	}

	FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];

	const int32 MinimumLevel = URPGCharacterRulesLibrary::GetMinimumLevel();
	const int32 MaximumLevel = URPGCharacterRulesLibrary::GetMaximumLevel();
	if (Character.Level < MinimumLevel || Character.Level > MaximumLevel ||
		Character.Experience != URPGCharacterRulesLibrary::NormalizeExperience(Character.Experience))
	{
		UE_LOG(LogGridLevelUp, Warning, TEXT("[GridLevelUp] Character=%d Level=%d Experience=%d Result=Rejected Reason=InvalidProgressionState"),
			CharacterIndex, Character.Level, Character.Experience);
		return false;
	}

	const int32 TargetLevel = URPGCharacterRulesLibrary::GetLevelForExperience(Character.Experience);
	if (TargetLevel == Character.Level)
	{
		return false;
	}
	if (TargetLevel < Character.Level)
	{
		UE_LOG(LogGridLevelUp, Warning, TEXT("[GridLevelUp] Character=%d Level=%d Target=%d Experience=%d Result=Rejected Reason=WouldDemote"), CharacterIndex,
			Character.Level, TargetLevel, Character.Experience);
		return false;
	}

	URPGClassAsset* ClassDefinition = ResolveCharacterClassDefinition(Character);
	if (!ClassDefinition || !ClassDefinition->IsValidDefinition() || (!Character.ClassId.IsNone() && ClassDefinition->ClassId != Character.ClassId))
	{
		UE_LOG(LogGridLevelUp, Warning,
			TEXT("[GridLevelUp] Character=%d Level=%d Target=%d Experience=%d ClassId=%s Result=Rejected Reason=InvalidClassDefinition"), CharacterIndex,
			Character.Level, TargetLevel, Character.Experience, *Character.ClassId.ToString());
		return false;
	}

	const int32 PreviousLevel = Character.Level;
	// TD07.3.3.9: LastAcknowledgedLevel deliberately remains unchanged.
	// The resulting gap is the durable signal that a Level-Up modal is pending.
	const FRPGDerivedStats PreviousStats = Character.DerivedStats;
	const FRPGCharacterResources PreviousResources = Character.Resources;

	FRPGDerivedStats NewStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, TargetLevel);
	FRPGCharacterResources NewResources = URPGCharacterRulesLibrary::InitializeCharacterResources(NewStats, ClassDefinition);
	NewResources.CurrentHealth = PreserveHealthDeficit(PreviousStats, PreviousResources, NewStats.MaxHealth);
	NewResources.CurrentMana = PreserveManaDeficit(PreviousStats, PreviousResources, NewStats.MaxMana);

	// Commit the new calculated projection and mutable resources together.
	Character.Level = TargetLevel;
	Character.DerivedStats = NewStats;
	Character.Resources = NewResources;

	// MON15.5 projection is ready before any observer or UI reads the new level.
	FRPGClassProgressionTransactionService::RefreshCharacterProjection(PartyInventoryComponent, CharacterIndex);

	if (bNotifyPartyInventoryChanged)
	{
		PartyInventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
	}

	const int32 LevelsGained = TargetLevel - PreviousLevel;
	OnCharacterLevelUpAppliedWithSource().Broadcast(PartyInventoryComponent, CharacterIndex, PreviousLevel, TargetLevel, LevelsGained);
	OnCharacterLevelUpApplied().Broadcast(CharacterIndex, PreviousLevel, TargetLevel, LevelsGained);

	UE_LOG(LogGridLevelUp, Log, TEXT("[GridLevelUp] Character=%d PreviousLevel=%d NewLevel=%d LevelsGained=%d Experience=%d HP=%d/%d Mana=%d/%d"),
		CharacterIndex, PreviousLevel, TargetLevel, LevelsGained, Character.Experience, Character.Resources.CurrentHealth, Character.DerivedStats.MaxHealth,
		Character.Resources.CurrentMana, Character.DerivedStats.MaxMana);
	return true;
}

int32 FRPGLevelUpService::ApplyPendingLevelUps(UGridPartyInventoryComponent* PartyInventoryComponent, bool bNotifyPartyInventoryChanged)
{
	if (!IsValid(PartyInventoryComponent))
	{
		return 0;
	}

	int32 TotalLevelsGained = 0;
	for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		const int32 PreviousLevel = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].Level;
		if (ApplyPendingLevelUp(PartyInventoryComponent, CharacterIndex, bNotifyPartyInventoryChanged))
		{
			TotalLevelsGained += PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].Level - PreviousLevel;
		}
	}
	return TotalLevelsGained;
}
