#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBalanceTypes.h"

UGridMonsterDefinitionAsset::UGridMonsterDefinitionAsset()
{
	MonsterActorClass = AGridMonsterActor::StaticClass();
}

FName FGridMonsterLootEntry::GetResolvedItemDefinitionId() const
{
	return ItemDefinitionAsset && !ItemDefinitionAsset->ItemDefinitionId.IsNone() ? ItemDefinitionAsset->ItemDefinitionId : ItemDefinitionId;
}

bool FGridMonsterLootEntry::IsValidDefinition() const
{
	const FName ResolvedId = GetResolvedItemDefinitionId();
	const bool bIdsAgree = !ItemDefinitionAsset || ItemDefinitionId.IsNone() || ItemDefinitionAsset->ItemDefinitionId == ItemDefinitionId;
	return !ResolvedId.IsNone() && bIdsAgree && FMath::IsFinite(DropChance) && DropChance >= 0.0f && DropChance <= 1.0f && MinQuantity > 0 &&
		MaxQuantity >= MinQuantity;
}

void UGridMonsterDefinitionAsset::PostLoad()
{
	Super::PostLoad();

	// An optional presentation list containing only an editor-cleared None
	// entry is equivalent to an empty list. Keep the strict validators for
	// runtime-authored data while canonicalizing serialized assets on load.
	const auto RemoveNullReferences = [](auto& References)
	{
		References.RemoveAll(
			[](const auto& Reference)
			{
				return Reference.IsNull();
			});
	};

	RemoveNullReferences(AlertAudio.Sounds);
	RemoveNullReferences(HurtAudio.Sounds);
	RemoveNullReferences(DeathAudio.Sounds);
	RemoveNullReferences(IdleAudio.Sounds);
	RemoveNullReferences(AlertVFX.Systems);
	RemoveNullReferences(HurtVFX.Systems);
	RemoveNullReferences(DeathVFX.Systems);

	for (FGridMonsterAttackDefinition& Attack : Attacks)
	{
		RemoveNullReferences(Attack.AttackAudio.Sounds);
		RemoveNullReferences(Attack.ImpactHitAudio.Sounds);
		RemoveNullReferences(Attack.ImpactMissAudio.Sounds);
		RemoveNullReferences(Attack.AttackVFXDefinition.Systems);
		RemoveNullReferences(Attack.ImpactHitVFXDefinition.Systems);
		RemoveNullReferences(Attack.ImpactMissVFXDefinition.Systems);
	}
}

FPrimaryAssetId UGridMonsterDefinitionAsset::GetPrimaryAssetId() const
{
	if (MonsterId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}

	return FPrimaryAssetId(FPrimaryAssetType(TEXT("GridMonster")), MonsterId);
}

bool UGridMonsterDefinitionAsset::IsValidDefinition() const
{
	FString Error;
	return ValidateDefinition(Error);
}

bool UGridMonsterDefinitionAsset::ValidateDefinition(FString& OutError) const
{
	TArray<FString> Errors;

	if (MonsterId.IsNone())
	{
		Errors.Add(TEXT("MonsterId must not be None."));
	}

	if (DisplayName.IsEmpty())
	{
		Errors.Add(TEXT("DisplayName must not be empty."));
	}

	if (CategoryId.IsNone())
	{
		Errors.Add(TEXT("CategoryId must not be None."));
	}

	if (!MonsterActorClass)
	{
		Errors.Add(TEXT("MonsterActorClass must be assigned."));
	}

	if (DangerLevel < 1)
	{
		Errors.Add(TEXT("DangerLevel must be at least 1."));
	}

	if (MaxHealth < 1)
	{
		Errors.Add(TEXT("MaxHealth must be at least 1."));
	}

	if (PhysicalArmor < 0 || MagicalArmor < 0)
	{
		Errors.Add(TEXT("Armor values must not be negative."));
	}

	if (ActionPointsPerTurn < 1)
	{
		Errors.Add(TEXT("ActionPointsPerTurn must be at least 1."));
	}

	if (GridFootprint.X < 1 || GridFootprint.Y < 1)
	{
		Errors.Add(TEXT("GridFootprint dimensions must be at least 1."));
	}

	if (!FMath::IsFinite(MoveDuration) || MoveDuration < 0.0f || !FMath::IsFinite(TurnDuration) || TurnDuration < 0.0f)
	{
		Errors.Add(TEXT("Movement durations must be finite and non-negative."));
	}

	if (!FMath::IsFinite(VisualScale.X) || !FMath::IsFinite(VisualScale.Y) || !FMath::IsFinite(VisualScale.Z) || VisualScale.X <= 0.0f ||
		VisualScale.Y <= 0.0f || VisualScale.Z <= 0.0f)
	{
		Errors.Add(TEXT("VisualScale components must be finite and greater than zero."));
	}

	if (SightRangeCells < 0 || HearingRangeCells < 0 || AggroPropagationRange < 0)
	{
		Errors.Add(TEXT("Perception ranges must not be negative."));
	}

	if (PreferredMinDistance < 0 || PreferredMaxDistance < PreferredMinDistance)
	{
		Errors.Add(TEXT("Preferred distance range is invalid."));
	}

	if (!FMath::IsFinite(RetreatChance) || RetreatChance < 0.0f || RetreatChance > 1.0f)
	{
		Errors.Add(TEXT("RetreatChance must be between 0 and 1."));
	}

	if (!FMath::IsFinite(LowHealthThreshold) || LowHealthThreshold < 0.0f || LowHealthThreshold > 1.0f)
	{
		Errors.Add(TEXT("LowHealthThreshold must be between 0 and 1."));
	}

	if (!FMath::IsFinite(DeathExpectedDuration) || DeathExpectedDuration <= 0.0f)
	{
		Errors.Add(TEXT("DeathExpectedDuration must be finite and greater than zero."));
	}

	if (!FMath::IsFinite(DeathDissolveDelay) || DeathDissolveDelay < 0.0f || !FMath::IsFinite(DeathDissolveDuration) || DeathDissolveDuration <= 0.0f)
	{
		Errors.Add(TEXT("Death dissolve delay/duration must be finite; delay must be non-negative and duration greater than zero."));
	}

	if (bEnableDeathDissolve && DeathDissolveParameterName.IsNone())
	{
		Errors.Add(TEXT("DeathDissolveParameterName must not be None when death dissolve is enabled."));
	}

	if (!AlertAudio.IsValidDefinition() || !HurtAudio.IsValidDefinition() || !DeathAudio.IsValidDefinition() || !IdleAudio.IsValidDefinition())
	{
		Errors.Add(TEXT("Monster audio event definitions must be valid."));
	}

	if (!FMath::IsFinite(IdleAudioMinDelay) || !FMath::IsFinite(IdleAudioMaxDelay) || IdleAudioMinDelay <= 0.0f || IdleAudioMaxDelay < IdleAudioMinDelay)
	{
		Errors.Add(TEXT("Idle audio delay range is invalid."));
	}

	if (!AlertVFX.IsValidDefinition() || !HurtVFX.IsValidDefinition() || !DeathVFX.IsValidDefinition())
	{
		Errors.Add(TEXT("Monster VFX event definitions must be valid."));
	}

	if (!FMath::IsFinite(IdleVariationMinDelay) || !FMath::IsFinite(IdleVariationMaxDelay) || IdleVariationMinDelay <= 0.0f ||
		IdleVariationMaxDelay < IdleVariationMinDelay)
	{
		Errors.Add(TEXT("Idle variation delay range is invalid."));
	}

	if (!FMath::IsFinite(IdleVariationBlendInTime) || !FMath::IsFinite(IdleVariationBlendOutTime) || IdleVariationBlendInTime < 0.0f ||
		IdleVariationBlendOutTime < 0.0f)
	{
		Errors.Add(TEXT("Idle variation blend times must be finite and non-negative."));
	}

	if (bEnableIdleVariations && !IdleVariations.IsEmpty() && IdleVariationSlotName.IsNone())
	{
		Errors.Add(TEXT("IdleVariationSlotName must not be None when idle variations are enabled and configured."));
	}

	TSet<FName> IdleVariationIds;
	for (int32 VariationIndex = 0; VariationIndex < IdleVariations.Num(); ++VariationIndex)
	{
		const FGridMonsterIdleVariationDefinition& Variation = IdleVariations[VariationIndex];
		if (!Variation.IsValidDefinition())
		{
			Errors.Add(FString::Printf(TEXT("Idle variation at index %d is invalid."), VariationIndex));
			continue;
		}

		if (IdleVariationIds.Contains(Variation.VariationId))
		{
			Errors.Add(FString::Printf(TEXT("Duplicate idle VariationId: %s."), *Variation.VariationId.ToString()));
			continue;
		}
		IdleVariationIds.Add(Variation.VariationId);
	}

	TSet<FName> AttackIds;
	for (int32 AttackIndex = 0; AttackIndex < Attacks.Num(); ++AttackIndex)
	{
		const FGridMonsterAttackDefinition& Attack = Attacks[AttackIndex];
		FString AttackError;
		if (!Attack.ValidateDefinition(AttackError))
		{
			Errors.Add(FString::Printf(TEXT("Attacks[%d] is invalid: %s"), AttackIndex, *AttackError));
			continue;
		}

		if (AttackIds.Contains(Attack.AttackId))
		{
			Errors.Add(FString::Printf(TEXT("Duplicate AttackId: %s."), *Attack.AttackId.ToString()));
			continue;
		}

		AttackIds.Add(Attack.AttackId);
	}

	TSet<FString> ModifierKeys;
	for (int32 ModifierIndex = 0; ModifierIndex < DamageModifiers.Num(); ++ModifierIndex)
	{
		const FGridMonsterDamageModifier& Modifier = DamageModifiers[ModifierIndex];
		if (!Modifier.IsValidDefinition())
		{
			Errors.Add(FString::Printf(TEXT("Damage modifier at index %d is invalid."), ModifierIndex));
			continue;
		}

		const FString ModifierKey = FString::Printf(TEXT("%d:%d"), static_cast<int32>(Modifier.DamageType), static_cast<int32>(Modifier.PhysicalSubtype));

		if (ModifierKeys.Contains(ModifierKey))
		{
			Errors.Add(FString::Printf(TEXT("Duplicate damage modifier: %s."), *ModifierKey));
			continue;
		}

		ModifierKeys.Add(ModifierKey);
	}

	TSet<FName> LootIds;
	for (int32 LootIndex = 0; LootIndex < LootTable.Num(); ++LootIndex)
	{
		const FGridMonsterLootEntry& LootEntry = LootTable[LootIndex];
		if (!LootEntry.IsValidDefinition())
		{
			Errors.Add(FString::Printf(TEXT("Loot entry at index %d is invalid."), LootIndex));
			continue;
		}

		const FName ResolvedLootId = LootEntry.GetResolvedItemDefinitionId();
		if (LootIds.Contains(ResolvedLootId))
		{
			Errors.Add(FString::Printf(TEXT("Duplicate loot ItemDefinitionId: %s."), *ResolvedLootId.ToString()));
			continue;
		}

		LootIds.Add(ResolvedLootId);
	}

	OutError = FString::Join(Errors, TEXT("\n"));
	return Errors.IsEmpty();
}

bool UGridMonsterDefinitionAsset::HasAIProfile(EGridMonsterAIProfile Profile) const
{
	return PrimaryAIProfile == Profile || AdditionalAIProfiles.Contains(Profile);
}

bool UGridMonsterDefinitionAsset::BuildBalanceSnapshot(FGridMonsterBalanceSnapshot& OutSnapshot) const
{
	return FGridMonsterBalanceAnalyzer::BuildSnapshot(this, OutSnapshot);
}

void UGridMonsterDefinitionAsset::LogBalanceSnapshot() const
{
	FGridMonsterBalanceSnapshot Snapshot;
	if (!BuildBalanceSnapshot(Snapshot))
	{
		UE_LOG(LogGridMonsterBalance, Warning, TEXT("[GridMonsterBalance] Definition=%s Result=Unavailable"), *GetPathName());
		return;
	}

	UE_LOG(LogGridMonsterBalance, Log,
		TEXT(
			"[GridMonsterBalance] Monster=%s Danger=%d HP=%d Armor=%d/%d Initiative=%d Accuracy=%d Evasion=%d AP=%d Sight=%d Hearing=%d Attacks=%d Damage=%d..%d Average=%.2f Cost=%d..%d XP=%d"),
		*Snapshot.MonsterId.ToString(), Snapshot.DangerLevel, Snapshot.MaxHealth, Snapshot.PhysicalArmor, Snapshot.MagicalArmor, Snapshot.Initiative,
		Snapshot.Accuracy, Snapshot.Evasion, Snapshot.ActionPointsPerTurn, Snapshot.SightRangeCells, Snapshot.HearingRangeCells, Snapshot.AttackCount,
		Snapshot.MinimumBaseDamage, Snapshot.MaximumBaseDamage, Snapshot.AverageBaseDamage, Snapshot.MinimumAttackActionPointCost,
		Snapshot.MaximumAttackActionPointCost, Snapshot.ExperienceReward);
}

float UGridMonsterDefinitionAsset::GetDamageMultiplier(EGridDamageType DamageType, EGridPhysicalDamageSubtype PhysicalSubtype) const
{
	float Multiplier = 1.0f;

	for (const FGridMonsterDamageModifier& Modifier : DamageModifiers)
	{
		if (Modifier.Matches(DamageType, PhysicalSubtype))
		{
			Multiplier *= Modifier.DamageMultiplier;
		}
	}

	return Multiplier;
}

bool UGridMonsterDefinitionAsset::GetAttackDefinition(FName AttackId, FGridMonsterAttackDefinition& OutAttack) const
{
	const FGridMonsterAttackDefinition* Attack = FindAttackDefinition(AttackId);
	if (!Attack)
	{
		OutAttack = FGridMonsterAttackDefinition();
		return false;
	}

	OutAttack = *Attack;
	return true;
}

const FGridMonsterAttackDefinition* UGridMonsterDefinitionAsset::FindAttackDefinition(FName AttackId) const
{
	if (AttackId.IsNone())
	{
		return nullptr;
	}

	return Attacks.FindByPredicate(
		[AttackId](const FGridMonsterAttackDefinition& Attack)
		{
			return Attack.AttackId == AttackId;
		});
}
