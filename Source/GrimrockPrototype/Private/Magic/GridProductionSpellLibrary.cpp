#include "Magic/GridProductionSpellLibrary.h"

namespace
{
    FGridSpellEffectDefinition MakeMagnitudeEffect (
        EGridSpellEffectType Type,
        int32 Magnitude)
    {
        FGridSpellEffectDefinition Effect;
        Effect.Type = Type;
        Effect.Magnitude = Magnitude;
        return Effect;
    }

    FGridSpellEffectDefinition MakeStatusEffect (
        EGridSpellEffectType Type,
        FName StatusEffectId)
    {
        FGridSpellEffectDefinition Effect;
        Effect.Type = Type;
        Effect.StatusEffectId = StatusEffectId;
        return Effect;
    }
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeArcaneBolt ()
{
    FGridSpellDefinition Definition;
    Definition.SpellId = ArcaneBoltId ();
    Definition.DisplayName = FText::FromString (TEXT ("Arcane Bolt"));
    Definition.Description = FText::FromString (
        TEXT ("A focused bolt of arcane force that damages the first hostile target in line."));
    Definition.School = EGridSpellSchool::Arcane;
    Definition.ManaCost = 3;
    Definition.ActionPointCost = 2;
    Definition.MinRangeCells = 1;
    Definition.MaxRangeCells = 5;
    Definition.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
    Definition.bRequiresLineOfSight = true;
    Definition.Effects.Add (
        MakeMagnitudeEffect (EGridSpellEffectType::Damage, 4));
    return Definition;
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeLesserHeal ()
{
    FGridSpellDefinition Definition;
    Definition.SpellId = LesserHealId ();
    Definition.DisplayName = FText::FromString (TEXT ("Lesser Heal"));
    Definition.Description = FText::FromString (
        TEXT ("Restores a small amount of health to an allied character."));
    Definition.School = EGridSpellSchool::Life;
    Definition.ManaCost = 4;
    Definition.ActionPointCost = 2;
    Definition.MinRangeCells = 0;
    Definition.MaxRangeCells = 3;
    Definition.TargetingPolicy = EGridCombatTargetingPolicy::Ally;
    Definition.bRequiresLineOfSight = true;
    Definition.Effects.Add (
        MakeMagnitudeEffect (EGridSpellEffectType::Heal, 5));
    return Definition;
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeHaste ()
{
    FGridSpellDefinition Definition;
    Definition.SpellId = HasteId ();
    Definition.DisplayName = FText::FromString (TEXT ("Haste"));
    Definition.Description = FText::FromString (
        TEXT ("Applies the canonical MON16 haste status to an allied character."));
    Definition.School = EGridSpellSchool::Air;
    Definition.ManaCost = 5;
    Definition.ActionPointCost = 2;
    Definition.MinRangeCells = 0;
    Definition.MaxRangeCells = 3;
    Definition.TargetingPolicy = EGridCombatTargetingPolicy::Ally;
    Definition.bRequiresLineOfSight = true;
    Definition.Effects.Add (
        MakeStatusEffect (
            EGridSpellEffectType::ApplyStatusEffect,
            TEXT ("Status_Haste")));
    return Definition;
}

FGridSpellDefinition FGridProductionSpellLibrary::MakeCurePoison ()
{
    FGridSpellDefinition Definition;
    Definition.SpellId = CurePoisonId ();
    Definition.DisplayName = FText::FromString (TEXT ("Cure Poison"));
    Definition.Description = FText::FromString (
        TEXT ("Removes the canonical poison status from an allied character."));
    Definition.School = EGridSpellSchool::Life;
    Definition.ManaCost = 4;
    Definition.ActionPointCost = 2;
    Definition.MinRangeCells = 0;
    Definition.MaxRangeCells = 3;
    Definition.TargetingPolicy = EGridCombatTargetingPolicy::Ally;
    Definition.bRequiresLineOfSight = true;
    Definition.Effects.Add (
        MakeStatusEffect (
            EGridSpellEffectType::RemoveStatusEffect,
            TEXT ("Status_Poison")));
    return Definition;
}

void FGridProductionSpellLibrary::BuildAll (
    TArray<FGridSpellDefinition>& OutDefinitions)
{
    OutDefinitions.Reset ();
    OutDefinitions.Add (MakeArcaneBolt ());
    OutDefinitions.Add (MakeLesserHeal ());
    OutDefinitions.Add (MakeHaste ());
    OutDefinitions.Add (MakeCurePoison ());
}
