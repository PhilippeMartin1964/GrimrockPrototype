#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

FPrimaryAssetId UGridMonsterDefinitionAsset::GetPrimaryAssetId () const
{
    if (MonsterId.IsNone ())
    {
        return Super::GetPrimaryAssetId ();
    }

    return FPrimaryAssetId (FPrimaryAssetType (TEXT ("GridMonster")), MonsterId);
}

bool UGridMonsterDefinitionAsset::IsValidDefinition () const
{
    FString Error;
    return ValidateDefinition (Error);
}

bool UGridMonsterDefinitionAsset::ValidateDefinition (FString& OutError) const
{
    TArray<FString> Errors;

    if (MonsterId.IsNone ())
    {
        Errors.Add (TEXT ("MonsterId must not be None."));
    }

    if (DisplayName.IsEmpty ())
    {
        Errors.Add (TEXT ("DisplayName must not be empty."));
    }

    if (CategoryId.IsNone ())
    {
        Errors.Add (TEXT ("CategoryId must not be None."));
    }

    if (DangerLevel < 1)
    {
        Errors.Add (TEXT ("DangerLevel must be at least 1."));
    }

    if (MaxHealth < 1)
    {
        Errors.Add (TEXT ("MaxHealth must be at least 1."));
    }

    if (PhysicalArmor < 0 || MagicalArmor < 0)
    {
        Errors.Add (TEXT ("Armor values must not be negative."));
    }

    if (ActionPointsPerTurn < 1)
    {
        Errors.Add (TEXT ("ActionPointsPerTurn must be at least 1."));
    }

    if (GridFootprint.X < 1 || GridFootprint.Y < 1)
    {
        Errors.Add (TEXT ("GridFootprint dimensions must be at least 1."));
    }

    if (!FMath::IsFinite (MoveDuration) || MoveDuration < 0.0f ||
        !FMath::IsFinite (TurnDuration) || TurnDuration < 0.0f)
    {
        Errors.Add (TEXT ("Movement durations must be finite and non-negative."));
    }

    if (VisualScale.X <= 0.0f || VisualScale.Y <= 0.0f || VisualScale.Z <= 0.0f ||
        !VisualScale.IsFinite ())
    {
        Errors.Add (TEXT ("VisualScale components must be finite and greater than zero."));
    }

    if (SightRangeCells < 0 || HearingRangeCells < 0 || AggroPropagationRange < 0)
    {
        Errors.Add (TEXT ("Perception ranges must not be negative."));
    }

    if (PreferredMinDistance < 0 || PreferredMaxDistance < PreferredMinDistance)
    {
        Errors.Add (TEXT ("Preferred distance range is invalid."));
    }

    if (!FMath::IsFinite (RetreatChance) || RetreatChance < 0.0f || RetreatChance > 1.0f)
    {
        Errors.Add (TEXT ("RetreatChance must be between 0 and 1."));
    }

    if (!FMath::IsFinite (LowHealthThreshold) || LowHealthThreshold < 0.0f || LowHealthThreshold > 1.0f)
    {
        Errors.Add (TEXT ("LowHealthThreshold must be between 0 and 1."));
    }

    TSet<FName> AttackIds;
    for (int32 AttackIndex = 0; AttackIndex < Attacks.Num (); ++AttackIndex)
    {
        const FGridMonsterAttackDefinition& Attack = Attacks[AttackIndex];
        if (!Attack.IsValidDefinition ())
        {
            Errors.Add (FString::Printf (TEXT ("Attack at index %d is invalid."), AttackIndex));
            continue;
        }

        if (AttackIds.Contains (Attack.AttackId))
        {
            Errors.Add (FString::Printf (TEXT ("Duplicate AttackId: %s."), *Attack.AttackId.ToString ()));
            continue;
        }

        AttackIds.Add (Attack.AttackId);
    }

    TSet<FString> ModifierKeys;
    for (int32 ModifierIndex = 0; ModifierIndex < DamageModifiers.Num (); ++ModifierIndex)
    {
        const FGridMonsterDamageModifier& Modifier = DamageModifiers[ModifierIndex];
        if (!Modifier.IsValidDefinition ())
        {
            Errors.Add (FString::Printf (TEXT ("Damage modifier at index %d is invalid."), ModifierIndex));
            continue;
        }

        const FString ModifierKey = FString::Printf (
            TEXT ("%d:%d"),
            static_cast<int32> (Modifier.DamageType),
            static_cast<int32> (Modifier.PhysicalSubtype));

        if (ModifierKeys.Contains (ModifierKey))
        {
            Errors.Add (FString::Printf (TEXT ("Duplicate damage modifier: %s."), *ModifierKey));
            continue;
        }

        ModifierKeys.Add (ModifierKey);
    }

    float TotalDropChance = 0.0f;
    TSet<FName> LootIds;
    for (int32 LootIndex = 0; LootIndex < LootTable.Num (); ++LootIndex)
    {
        const FGridMonsterLootEntry& LootEntry = LootTable[LootIndex];
        if (!LootEntry.IsValidDefinition ())
        {
            Errors.Add (FString::Printf (TEXT ("Loot entry at index %d is invalid."), LootIndex));
            continue;
        }

        if (LootIds.Contains (LootEntry.ItemDefinitionId))
        {
            Errors.Add (FString::Printf (
                TEXT ("Duplicate loot ItemDefinitionId: %s."),
                *LootEntry.ItemDefinitionId.ToString ()));
            continue;
        }

        LootIds.Add (LootEntry.ItemDefinitionId);
        TotalDropChance += LootEntry.DropChance;
    }

    if (TotalDropChance > 1.0f + KINDA_SMALL_NUMBER)
    {
        Errors.Add (TEXT ("The sum of LootTable DropChance values must not exceed 1."));
    }

    OutError = FString::Join (Errors, TEXT ("\n"));
    return Errors.IsEmpty ();
}

bool UGridMonsterDefinitionAsset::HasAIProfile (EGridMonsterAIProfile Profile) const
{
    return PrimaryAIProfile == Profile || AdditionalAIProfiles.Contains (Profile);
}

float UGridMonsterDefinitionAsset::GetDamageMultiplier (
    EGridDamageType DamageType,
    EGridPhysicalDamageSubtype PhysicalSubtype) const
{
    float Multiplier = 1.0f;

    for (const FGridMonsterDamageModifier& Modifier : DamageModifiers)
    {
        if (Modifier.Matches (DamageType, PhysicalSubtype))
        {
            Multiplier *= Modifier.DamageMultiplier;
        }
    }

    return Multiplier;
}

bool UGridMonsterDefinitionAsset::GetAttackDefinition (
    FName AttackId,
    FGridMonsterAttackDefinition& OutAttack) const
{
    const FGridMonsterAttackDefinition* Attack = FindAttackDefinition (AttackId);
    if (!Attack)
    {
        OutAttack = FGridMonsterAttackDefinition ();
        return false;
    }

    OutAttack = *Attack;
    return true;
}

const FGridMonsterAttackDefinition* UGridMonsterDefinitionAsset::FindAttackDefinition (FName AttackId) const
{
    if (AttackId.IsNone ())
    {
        return nullptr;
    }

    return Attacks.FindByPredicate ([AttackId] (const FGridMonsterAttackDefinition& Attack)
    {
        return Attack.AttackId == AttackId;
    });
}
