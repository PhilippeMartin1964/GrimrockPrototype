#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellPresentation.h"
#include "Magic/GridSpellTypes.h"

/** First canonical production spell definitions introduced by MON18.5. */
struct GRIMROCKPROTOTYPE_API FGridProductionSpellLibrary
{
    static FName ArcaneBoltId () { return TEXT ("Spell_ArcaneBolt"); }
    static FName LesserHealId () { return TEXT ("Spell_LesserHeal"); }
    static FName HasteId () { return TEXT ("Spell_Haste"); }
    static FName CurePoisonId () { return TEXT ("Spell_CurePoison"); }

    static FGridSpellDefinition MakeArcaneBolt ();
    static FGridSpellDefinition MakeLesserHeal ();
    static FGridSpellDefinition MakeHaste ();
    static FGridSpellDefinition MakeCurePoison ();

    static FGridSpellPresentationProfile MakeArcaneBoltPresentation ();
    static FGridSpellPresentationProfile MakeLesserHealPresentation ();
    static FGridSpellPresentationProfile MakeHastePresentation ();
    static FGridSpellPresentationProfile MakeCurePoisonPresentation ();
    static bool TryBuildPresentationProfile (
        FName SpellId,
        FGridSpellPresentationProfile& OutProfile);

    static void BuildAll (TArray<FGridSpellDefinition>& OutDefinitions);
};
