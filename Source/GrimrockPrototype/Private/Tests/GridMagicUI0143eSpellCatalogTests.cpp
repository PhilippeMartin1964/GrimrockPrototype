#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookUI.h"
#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridUI0143eSpellCatalogProjectionTest,
    "Grimrock.UI.UI01.4.3e.SpellCatalogProjection",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridUI0143eSpellCatalogProjectionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGridSpellDefinition Spell =
        FGridProductionSpellLibrary::MakeArcaneBolt ();
    const FGridCombatActionDefinition Action =
        UGridSpellbookUILibrary::MakeSpellCombatActionDefinition (Spell);

    TestTrue (TEXT ("Projected action is valid"), Action.IsValid ());
    TestEqual (
        TEXT ("Action identity is SpellId"),
        Action.ActionId,
        Spell.SpellId);
    TestTrue (
        TEXT ("Source policy is Spell"),
        Action.SourcePolicy == EGridCombatActionSourcePolicy::Spell);
    TestEqual (
        TEXT ("AP cost is preserved"),
        Action.ActionPointCost,
        Spell.ActionPointCost);
    TestEqual (
        TEXT ("Mana cost is preserved"),
        Action.ResourceCosts.ManaCost,
        Spell.ManaCost);
    TestTrue (
        TEXT ("First axial targeting is preserved"),
        Action.TargetingPolicy == Spell.TargetingPolicy);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridUI0143eSpellCatalogBindingIdentityTest,
    "Grimrock.UI.UI01.4.3e.SpellBindingMatchesProjectedAction",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridUI0143eSpellCatalogBindingIdentityTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    const FGridSpellDefinition Spell =
        FGridProductionSpellLibrary::MakeLesserHeal ();
    const FGridCombatActionDefinition Action =
        UGridSpellbookUILibrary::MakeSpellCombatActionDefinition (Spell);
    const FGridCombatHotbarBinding Binding =
        UGridSpellbookUILibrary::MakeSpellHotbarBinding (Spell.SpellId, 2);

    TestEqual (TEXT ("Binding ActionId matches projection"),
        Binding.ActionId, Action.ActionId);
    TestTrue (TEXT ("Binding policy matches projection"),
        Binding.SourcePolicy == Action.SourcePolicy);
    TestEqual (TEXT ("Binding definition identity is SpellId"),
        Binding.SourceDefinitionId, Spell.SpellId);
    return true;
}

#endif
