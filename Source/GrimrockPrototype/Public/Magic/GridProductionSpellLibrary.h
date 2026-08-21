#pragma once

#include "CoreMinimal.h"
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

    static void BuildAll (TArray<FGridSpellDefinition>& OutDefinitions);
};
