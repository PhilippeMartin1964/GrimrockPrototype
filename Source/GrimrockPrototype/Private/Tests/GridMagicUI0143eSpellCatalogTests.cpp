#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookUI.h"
#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUI0143eSpellCatalogProjectionTest, "Grimrock.UI.UI01.4.3e.SpellCatalogProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUI0143eSpellCatalogProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeArcaneBolt();
	const FGridCombatActionDefinition Action = UGridSpellbookUILibrary::MakeSpellCombatActionDefinition(Spell);

	TestTrue(TEXT("Projected action is valid"), Action.IsValid());
	TestEqual(TEXT("Action identity is SpellId"), Action.ActionId, Spell.SpellId);
	TestTrue(TEXT("Source policy is Spell"), Action.SourcePolicy == EGridCombatActionSourcePolicy::Spell);
	TestEqual(TEXT("AP cost is preserved"), Action.ActionPointCost, Spell.ActionPointCost);
	TestEqual(TEXT("Mana cost is preserved"), Action.ResourceCosts.ManaCost, Spell.ManaCost);
	TestTrue(TEXT("First axial targeting is preserved"), Action.TargetingPolicy == Spell.TargetingPolicy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUI0143eSpellCatalogBindingIdentityTest, "Grimrock.UI.UI01.4.3e.SpellBindingMatchesProjectedAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUI0143eSpellCatalogBindingIdentityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeLesserHeal();
	const FGridCombatActionDefinition Action = UGridSpellbookUILibrary::MakeSpellCombatActionDefinition(Spell);
	const FGridCombatHotbarBinding Binding = UGridSpellbookUILibrary::MakeSpellHotbarBinding(Spell.SpellId, 2);

	TestEqual(TEXT("Binding ActionId matches projection"), Binding.ActionId, Action.ActionId);
	TestTrue(TEXT("Binding policy matches projection"), Binding.SourcePolicy == Action.SourcePolicy);
	TestEqual(TEXT("Binding definition identity is SpellId"), Binding.SourceDefinitionId, Spell.SpellId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUI0143e2SpellbookCatalogAvailabilityTest, "Grimrock.UI.UI01.4.3e.2.SpellbookCatalogAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUI0143e2SpellbookCatalogAvailabilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridCombatActionCatalogContext Context;
	Context.CharacterIndex = 0;
	Context.CharacterId = FGuid::NewGuid();
	Context.bCombatActive = true;
	Context.bActiveCombatant = true;
	Context.bEnableClassActionExecutors = true;
	Context.RemainingActionPoints = 6;
	Context.CurrentHealth = 20;
	Context.MaximumHealth = 20;
	Context.CurrentMana = 20;
	Context.MaximumMana = 20;

	TArray<FGridSpellDefinition> Spells;
	FGridProductionSpellLibrary::BuildAll(Spells);
	TArray<FGridCombatActionContribution> Contributions;
	for (const FGridSpellDefinition& Spell : Spells)
	{
		FGridCombatActionContribution Contribution;
		Contribution.Definition = UGridSpellbookUILibrary::MakeSpellCombatActionDefinition(Spell);
		Contribution.SourceDefinitionId = Spell.SpellId;
		Contribution.AvailableSourceQuantity = 1;
		Contributions.Add(MoveTemp(Contribution));
	}

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, Contributions, Actions);

	TestEqual(TEXT("All production Spellbook projections reach the catalogue"), Actions.Num(), Spells.Num());
	for (const FGridAvailableCombatAction& Action : Actions)
	{
		TestTrue(*FString::Printf(TEXT("Spellbook action %s is executable"), *Action.Definition.ActionId.ToString()), Action.bEnabled);
		TestTrue(*FString::Printf(TEXT("Spellbook action %s is not blocked as unimplemented"), *Action.Definition.ActionId.ToString()),
			Action.AvailabilityReason != EGridCombatActionAvailabilityReason::ExecutionNotImplemented);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUI0143e2SpellbookCatalogExecutorGateTest, "Grimrock.UI.UI01.4.3e.2.SpellbookCatalogExecutorGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUI0143e2SpellbookCatalogExecutorGateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridSpellDefinition Spell = FGridProductionSpellLibrary::MakeArcaneBolt();
	FGridCombatActionContribution Contribution;
	Contribution.Definition = UGridSpellbookUILibrary::MakeSpellCombatActionDefinition(Spell);
	Contribution.SourceDefinitionId = Spell.SpellId;
	Contribution.AvailableSourceQuantity = 1;

	FGridCombatActionCatalogContext Context;
	Context.CharacterIndex = 0;
	Context.CharacterId = FGuid::NewGuid();
	Context.bCombatActive = true;
	Context.bActiveCombatant = true;
	Context.bEnableClassActionExecutors = false;
	Context.RemainingActionPoints = 6;
	Context.CurrentHealth = 20;
	Context.MaximumHealth = 20;
	Context.CurrentMana = 20;
	Context.MaximumMana = 20;

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);

	TestEqual(TEXT("One Spellbook action is projected"), Actions.Num(), 1);
	if (Actions.Num() != 1)
	{
		return false;
	}
	TestFalse(TEXT("Spellbook execution remains gated when executors are disabled"), Actions[0].bEnabled);
	TestTrue(TEXT("Disabled executor reports ExecutionNotImplemented"),
		Actions[0].AvailabilityReason == EGridCombatActionAvailabilityReason::ExecutionNotImplemented);
	return true;
}

#endif
