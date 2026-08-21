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

    FGridSpellPresentationProfile MakeInstantPresentation ()
    {
        FGridSpellPresentationProfile Profile;
        Profile.Projectile.bEnabled = false;
        Profile.FeedbackDurationSeconds = 1.25f;
        return Profile;
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

FGridSpellPresentationProfile
FGridProductionSpellLibrary::MakeArcaneBoltPresentation ()
{
    FGridSpellPresentationProfile Profile;
    Profile.Projectile.bEnabled = true;
    Profile.Projectile.TravelDurationSeconds = 0.20f;
    Profile.Projectile.VisualScale = FVector (0.25f);
    Profile.FeedbackDurationSeconds = 1.25f;
    return Profile;
}

FGridSpellPresentationProfile
FGridProductionSpellLibrary::MakeLesserHealPresentation ()
{
    return MakeInstantPresentation ();
}

FGridSpellPresentationProfile
FGridProductionSpellLibrary::MakeHastePresentation ()
{
    return MakeInstantPresentation ();
}

FGridSpellPresentationProfile
FGridProductionSpellLibrary::MakeCurePoisonPresentation ()
{
    return MakeInstantPresentation ();
}

bool FGridProductionSpellLibrary::TryBuildPresentationProfile (
    FName SpellId,
    FGridSpellPresentationProfile& OutProfile)
{
    OutProfile = FGridSpellPresentationProfile ();
    if (SpellId == ArcaneBoltId ())
    {
        OutProfile = MakeArcaneBoltPresentation ();
        return true;
    }
    if (SpellId == LesserHealId ())
    {
        OutProfile = MakeLesserHealPresentation ();
        return true;
    }
    if (SpellId == HasteId ())
    {
        OutProfile = MakeHastePresentation ();
        return true;
    }
    if (SpellId == CurePoisonId ())
    {
        OutProfile = MakeCurePoisonPresentation ();
        return true;
    }
    return false;
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
