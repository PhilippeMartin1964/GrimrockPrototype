#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Magic/GridSpellTypes.h"

namespace
{
	FGridSpellDefinition MakeValidDamageSpell()
	{
		FGridSpellDefinition Definition;
		Definition.SpellId = TEXT("Spell_TestBolt");
		Definition.DisplayName = FText::FromString(TEXT("Test Bolt"));
		Definition.Description = FText::FromString(TEXT("MON18.1 structural fixture."));
		Definition.School = EGridSpellSchool::Arcane;
		Definition.ManaCost = 3;
		Definition.ActionPointCost = 2;
		Definition.MinRangeCells = 1;
		Definition.MaxRangeCells = 6;
		Definition.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Definition.bRequiresLineOfSight = true;
		Definition.CooldownRounds = 0;

		FGridSpellEffectDefinition Effect;
		Effect.Type = EGridSpellEffectType::Damage;
		Effect.Magnitude = 4;
		Definition.Effects.Add(Effect);
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON181DefinitionValidationTest, "Grimrock.Magic.MON18.1.DefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON181DefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridSpellDefinition Definition = MakeValidDamageSpell();
	TestEqual(TEXT("A complete definition is structurally valid"), FGridSpellContract::ValidateDefinition(Definition), EGridSpellValidationError::None);

	Definition.SpellId = NAME_None;
	TestEqual(TEXT("SpellId is mandatory"), FGridSpellContract::ValidateDefinition(Definition), EGridSpellValidationError::MissingSpellId);

	Definition = MakeValidDamageSpell();
	Definition.MaxRangeCells = 0;
	TestEqual(TEXT("Max range cannot be lower than min range"), FGridSpellContract::ValidateDefinition(Definition), EGridSpellValidationError::InvalidRange);

	Definition = MakeValidDamageSpell();
	Definition.TargetingPolicy = EGridCombatTargetingPolicy::None;
	TestEqual(TEXT("Targeting policy is mandatory"), FGridSpellContract::ValidateDefinition(Definition), EGridSpellValidationError::MissingTargetPolicy);

	Definition = MakeValidDamageSpell();
	Definition.Effects.Reset();
	TestEqual(
		TEXT("At least one declarative effect is mandatory"), FGridSpellContract::ValidateDefinition(Definition), EGridSpellValidationError::MissingEffects);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON181StatusEffectBridgeTest, "Grimrock.Magic.MON18.1.StatusEffectBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON181StatusEffectBridgeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridSpellDefinition Definition = MakeValidDamageSpell();
	Definition.Effects.Reset();

	FGridSpellEffectDefinition ApplyEffect;
	ApplyEffect.Type = EGridSpellEffectType::ApplyStatusEffect;
	ApplyEffect.StatusEffectId = TEXT("Status_Haste");
	Definition.Effects.Add(ApplyEffect);

	TestEqual(TEXT("A spell may reference MON16 through a stable StatusEffectId"), FGridSpellContract::ValidateDefinition(Definition),
		EGridSpellValidationError::None);

	Definition.Effects[0].StatusEffectId = NAME_None;
	TestEqual(TEXT("Status-effect spell entries require a stable effect id"), FGridSpellContract::ValidateDefinition(Definition),
		EGridSpellValidationError::InvalidEffect);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMagicMON181CastRequestValidationTest, "Grimrock.Magic.MON18.1.CastRequestValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON181CastRequestValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridSpellDefinition Definition = MakeValidDamageSpell();

	FGridSpellCastRequest Request;
	Request.CasterCharacterId = FGuid(18, 1, 0, 1);
	Request.SpellId = Definition.SpellId;
	Request.Target.TargetId = FGuid(18, 1, 0, 2);

	TestEqual(TEXT("Stable caster and target identities form a valid axial-target request"), FGridSpellContract::ValidateRequest(Definition, Request),
		EGridSpellValidationError::None);

	Request.Target.TargetId.Invalidate();
	TestEqual(TEXT("Axial-target spells require a stable target identity"), FGridSpellContract::ValidateRequest(Definition, Request),
		EGridSpellValidationError::MissingTarget);

	FGridSpellDefinition CellDefinition = MakeValidDamageSpell();
	CellDefinition.TargetingPolicy = EGridCombatTargetingPolicy::Cell;
	CellDefinition.MinRangeCells = 0;

	Request.SpellId = CellDefinition.SpellId;
	Request.Target.bHasGridCell = true;
	Request.Target.GridCell = FIntPoint(7, 12);

	TestEqual(TEXT("Cell-target spells use serializable grid coordinates"), FGridSpellContract::ValidateRequest(CellDefinition, Request),
		EGridSpellValidationError::None);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMagicMON181NoRuntimePaymentTest, "Grimrock.Magic.MON18.1.ContractIsPure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMagicMON181NoRuntimePaymentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridSpellDefinition Definition = MakeValidDamageSpell();
	Definition.ManaCost = 7;
	Definition.ActionPointCost = 3;

	const FGridSpellDefinition Before = Definition;
	const EGridSpellValidationError Result = FGridSpellContract::ValidateDefinition(Definition);

	TestEqual(TEXT("Validation succeeds"), Result, EGridSpellValidationError::None);
	TestEqual(TEXT("Mana cost remains declarative"), Definition.ManaCost, Before.ManaCost);
	TestEqual(TEXT("AP cost remains declarative"), Definition.ActionPointCost, Before.ActionPointCost);
	TestEqual(TEXT("Effects remain declarative"), Definition.Effects.Num(), Before.Effects.Num());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
