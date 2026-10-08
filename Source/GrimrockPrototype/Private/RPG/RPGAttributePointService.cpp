#include "RPG/RPGAttributePointService.h"

#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGAttributePointServicePrivate
{
	int32 SumAttributes(const FRPGAttributes& Attributes)
	{
		return Attributes.Strength +
			Attributes.Dexterity +
			Attributes.Constitution +
			Attributes.Intelligence +
			Attributes.Wisdom +
			Attributes.Charisma;
	}

	URPGClassAsset* ResolveClassDefinition(const FGridCharacterInventoryState& Character)
	{
		if (URPGClassAsset* Definition = Character.ClassDefinition.Get();
			IsValid(Definition) && Definition->IsValidDefinition() &&
			(Character.ClassId.IsNone() || Definition->ClassId == Character.ClassId))
		{
			return Definition;
		}
		return FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
	}

	URPGRaceAsset* ResolveRaceDefinition(const FGridCharacterInventoryState& Character)
	{
		return FRPGAuthoringIdentityResolver::ResolveRaceById(Character.RaceId);
	}

	bool TryGetStartingAttributeTotal(const FGridCharacterInventoryState& Character, int32& OutTotal)
	{
		OutTotal = 0;
		URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character);
		URPGRaceAsset* RaceDefinition = ResolveRaceDefinition(Character);
		if (!IsValid(ClassDefinition) || !ClassDefinition->IsValidDefinition() ||
			!IsValid(RaceDefinition) || !RaceDefinition->IsValidDefinition())
		{
			return false;
		}

		OutTotal = SumAttributes(ClassDefinition->BaseAttributes) + SumAttributes(RaceDefinition->AttributeBonuses);
		return true;
	}

	int32& GetMutableAttributeValue(FRPGAttributes& Attributes, ERPGAttributePointTarget Target)
	{
		switch (Target)
		{
			case ERPGAttributePointTarget::Strength:
				return Attributes.Strength;
			case ERPGAttributePointTarget::Dexterity:
				return Attributes.Dexterity;
			case ERPGAttributePointTarget::Constitution:
				return Attributes.Constitution;
			case ERPGAttributePointTarget::Intelligence:
				return Attributes.Intelligence;
			case ERPGAttributePointTarget::Wisdom:
				return Attributes.Wisdom;
			case ERPGAttributePointTarget::Charisma:
			default:
				return Attributes.Charisma;
		}
	}

	int32 PreserveHealthDeficit(
		const FRPGDerivedStats& PreviousStats,
		const FRPGCharacterResources& PreviousResources,
		int32 NewMaximumHealth)
	{
		const int32 SafeNewMaximum = FMath::Max(1, NewMaximumHealth);
		const int32 SafePreviousMaximum = FMath::Max(1, PreviousStats.MaxHealth);
		const int32 SafePreviousCurrent = FMath::Clamp(PreviousResources.CurrentHealth, 0, SafePreviousMaximum);
		if (SafePreviousCurrent <= 0)
		{
			return 0;
		}

		const int32 DamageTaken = SafePreviousMaximum - SafePreviousCurrent;
		return FMath::Clamp(SafeNewMaximum - DamageTaken, 0, SafeNewMaximum);
	}

	int32 PreserveManaDeficit(
		const FRPGDerivedStats& PreviousStats,
		const FRPGCharacterResources& PreviousResources,
		int32 NewMaximumMana)
	{
		const int32 SafeNewMaximum = FMath::Max(0, NewMaximumMana);
		const int32 SafePreviousMaximum = FMath::Max(0, PreviousStats.MaxMana);
		const int32 SafePreviousCurrent = FMath::Clamp(PreviousResources.CurrentMana, 0, SafePreviousMaximum);
		const int32 ManaSpent = SafePreviousMaximum - SafePreviousCurrent;
		return FMath::Clamp(SafeNewMaximum - ManaSpent, 0, SafeNewMaximum);
	}

	bool RebuildDerivedStats(FGridCharacterInventoryState& Character)
	{
		URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character);
		if (!IsValid(ClassDefinition) || !ClassDefinition->IsValidDefinition())
		{
			return false;
		}

		const FRPGDerivedStats PreviousStats = Character.DerivedStats;
		const FRPGCharacterResources PreviousResources = Character.Resources;
		const FRPGDerivedStats NewStats =
			URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, Character.Level);

		Character.DerivedStats = NewStats;
		Character.Resources.CurrentHealth =
			PreserveHealthDeficit(PreviousStats, PreviousResources, NewStats.MaxHealth);
		Character.Resources.CurrentMana =
			PreserveManaDeficit(PreviousStats, PreviousResources, NewStats.MaxMana);
		return true;
	}

	void FillBalanceResult(const FRPGAttributePointBalance& Balance, FRPGAttributePointMutationResult& OutResult)
	{
		OutResult.GrantedPoints = Balance.GrantedPoints;
		OutResult.SpentPoints = Balance.SpentPoints;
		OutResult.RemainingPoints = Balance.RemainingPoints;
	}
}

using namespace RPGAttributePointServicePrivate;

int32 FRPGAttributePointService::GetTotalPointsGranted(int32 CharacterLevel)
{
	if (CharacterLevel < URPGCharacterRulesLibrary::GetMinimumLevel() ||
		CharacterLevel > URPGCharacterRulesLibrary::GetMaximumLevel())
	{
		return 0;
	}

	return CharacterLevel / 4;
}

bool FRPGAttributePointService::TryGetBalance(
	const FGridCharacterInventoryState& CharacterState,
	FRPGAttributePointBalance& OutBalance)
{
	OutBalance = FRPGAttributePointBalance();

	const int32 GrantedPoints = GetTotalPointsGranted(CharacterState.Level);
	if (CharacterState.Level < URPGCharacterRulesLibrary::GetMinimumLevel() ||
		CharacterState.Level > URPGCharacterRulesLibrary::GetMaximumLevel() ||
		!URPGCharacterRulesLibrary::AreAttributesInRange(CharacterState.Attributes))
	{
		return false;
	}

	int32 StartingAttributeTotal = 0;
	if (!TryGetStartingAttributeTotal(CharacterState, StartingAttributeTotal))
	{
		return false;
	}

	const int32 SpentPoints = SumAttributes(CharacterState.Attributes) - StartingAttributeTotal;
	if (SpentPoints < 0 || SpentPoints > GrantedPoints)
	{
		return false;
	}

	OutBalance.GrantedPoints = GrantedPoints;
	OutBalance.SpentPoints = SpentPoints;
	OutBalance.RemainingPoints = GrantedPoints - SpentPoints;
	OutBalance.StartingAttributeTotal = StartingAttributeTotal;
	return true;
}

int32 FRPGAttributePointService::GetAttributeValue(
	const FGridCharacterInventoryState& CharacterState,
	ERPGAttributePointTarget Target)
{
	switch (Target)
	{
		case ERPGAttributePointTarget::Strength:
			return CharacterState.Attributes.Strength;
		case ERPGAttributePointTarget::Dexterity:
			return CharacterState.Attributes.Dexterity;
		case ERPGAttributePointTarget::Constitution:
			return CharacterState.Attributes.Constitution;
		case ERPGAttributePointTarget::Intelligence:
			return CharacterState.Attributes.Intelligence;
		case ERPGAttributePointTarget::Wisdom:
			return CharacterState.Attributes.Wisdom;
		case ERPGAttributePointTarget::Charisma:
		default:
			return CharacterState.Attributes.Charisma;
	}
}

ERPGAttributePointMutationRejectReason FRPGAttributePointService::GetIncreaseAvailability(
	const FGridCharacterInventoryState& CharacterState,
	ERPGAttributePointTarget Target)
{
	if (CharacterState.Level < URPGCharacterRulesLibrary::GetMinimumLevel() ||
		CharacterState.Level > URPGCharacterRulesLibrary::GetMaximumLevel())
	{
		return ERPGAttributePointMutationRejectReason::InvalidLevel;
	}

	if (!URPGCharacterRulesLibrary::AreAttributesInRange(CharacterState.Attributes))
	{
		return ERPGAttributePointMutationRejectReason::InvalidAttributeState;
	}

	int32 StartingAttributeTotal = 0;
	if (!TryGetStartingAttributeTotal(CharacterState, StartingAttributeTotal))
	{
		return ERPGAttributePointMutationRejectReason::InvalidDefinition;
	}

	FRPGAttributePointBalance Balance;
	if (!TryGetBalance(CharacterState, Balance))
	{
		return ERPGAttributePointMutationRejectReason::InvalidPointBalance;
	}

	if (GetAttributeValue(CharacterState, Target) >= MaximumBaseAttributeValue)
	{
		return ERPGAttributePointMutationRejectReason::AttributeCapReached;
	}
	if (Balance.RemainingPoints <= 0)
	{
		return ERPGAttributePointMutationRejectReason::NoAttributePoints;
	}

	return ERPGAttributePointMutationRejectReason::None;
}

bool FRPGAttributePointService::TryPurchasePoint(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 CharacterIndex,
	ERPGAttributePointTarget Target,
	FRPGAttributePointMutationResult& OutResult)
{
	OutResult = FRPGAttributePointMutationResult();
	OutResult.Target = Target;

	if (!IsValid(PartyInventoryComponent))
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidInventory;
		return false;
	}
	if (!PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidCharacter;
		return false;
	}

	FGridCharacterInventoryState& Character =
		PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	OutResult.RejectReason = GetIncreaseAvailability(Character, Target);

	FRPGAttributePointBalance Balance;
	if (TryGetBalance(Character, Balance))
	{
		FillBalanceResult(Balance, OutResult);
	}

	if (OutResult.RejectReason != ERPGAttributePointMutationRejectReason::None)
	{
		return false;
	}

	int32& AttributeValue = GetMutableAttributeValue(Character.Attributes, Target);
	OutResult.PreviousValue = AttributeValue;
	++AttributeValue;

	if (!URPGCharacterRulesLibrary::AreAttributesInRange(Character.Attributes) || !RebuildDerivedStats(Character))
	{
		AttributeValue = OutResult.PreviousValue;
		RebuildDerivedStats(Character);
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::MutationRejected;
		return false;
	}

	OutResult.bCommitted = true;
	OutResult.NewValue = AttributeValue;
	OutResult.SpentPoints = Balance.SpentPoints + 1;
	OutResult.RemainingPoints = Balance.RemainingPoints - 1;
	OutResult.RejectReason = ERPGAttributePointMutationRejectReason::None;

	PartyInventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}

bool FRPGAttributePointService::TryRefundPurchasedPoint(
	UGridPartyInventoryComponent* PartyInventoryComponent,
	int32 CharacterIndex,
	ERPGAttributePointTarget Target,
	int32 SessionFloorValue,
	FRPGAttributePointMutationResult& OutResult)
{
	OutResult = FRPGAttributePointMutationResult();
	OutResult.Target = Target;

	if (!IsValid(PartyInventoryComponent))
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidInventory;
		return false;
	}
	if (!PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidCharacter;
		return false;
	}

	FGridCharacterInventoryState& Character =
		PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
	if (!URPGCharacterRulesLibrary::AreAttributesInRange(Character.Attributes))
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidAttributeState;
		return false;
	}

	int32 StartingAttributeTotal = 0;
	if (!TryGetStartingAttributeTotal(Character, StartingAttributeTotal))
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidDefinition;
		return false;
	}

	FRPGAttributePointBalance Balance;
	if (!TryGetBalance(Character, Balance))
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidPointBalance;
		return false;
	}
	FillBalanceResult(Balance, OutResult);

	int32& AttributeValue = GetMutableAttributeValue(Character.Attributes, Target);
	OutResult.PreviousValue = AttributeValue;
	if (SessionFloorValue < 6 || SessionFloorValue > MaximumBaseAttributeValue ||
		SessionFloorValue > OutResult.PreviousValue)
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::InvalidSessionFloor;
		return false;
	}
	if (OutResult.PreviousValue <= SessionFloorValue)
	{
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::NoSessionPurchaseToUndo;
		return false;
	}

	--AttributeValue;
	if (!URPGCharacterRulesLibrary::AreAttributesInRange(Character.Attributes) || !RebuildDerivedStats(Character))
	{
		AttributeValue = OutResult.PreviousValue;
		RebuildDerivedStats(Character);
		OutResult.RejectReason = ERPGAttributePointMutationRejectReason::MutationRejected;
		return false;
	}

	OutResult.bCommitted = true;
	OutResult.NewValue = AttributeValue;
	OutResult.SpentPoints = Balance.SpentPoints - 1;
	OutResult.RemainingPoints = Balance.RemainingPoints + 1;
	OutResult.RejectReason = ERPGAttributePointMutationRejectReason::None;

	PartyInventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
	return true;
}
