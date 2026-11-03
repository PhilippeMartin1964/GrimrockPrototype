#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UI/GridQuickItemSurfaceTooltipText.h"

namespace ARMEMENT011713
{
	UGridItemDefinitionAsset* LoadOil()
	{
		return LoadObject<UGridItemDefinitionAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"));
	}

	FGridCombatActionCatalogContext MakeContext()
	{
		FGridCombatActionCatalogContext Context;
		Context.CharacterIndex = 0;
		Context.CharacterId = FGuid::NewGuid();
		Context.bCombatActive = true;
		Context.bActiveCombatant = true;
		Context.bEnableQuickItemExecutors = true;
		Context.RemainingActionPoints = 4;
		Context.CurrentHealth = 20;
		Context.MaximumHealth = 20;
		Context.CurrentMana = 20;
		Context.MaximumMana = 20;
		return Context;
	}

	EGridCombatActionAvailabilityReason Evaluate(
		const FGridCombatActionCatalogContext& Context,
		const FGridCombatActionDefinition& Action,
		int32 Quantity,
		bool& bOutEnabled)
	{
		FGridCombatActionContribution Contribution;
		Contribution.Definition = Action;
		Contribution.SourceDefinitionId = TEXT("Item_Flask_Oil");
		Contribution.AvailableSourceQuantity = Quantity;
		TArray<FGridAvailableCombatAction> Available;
		FGridCombatActionCatalog::Build(Context, { Contribution }, Available);
		bOutEnabled = Available.Num() == 1 && Available[0].bEnabled;
		return Available.Num() == 1
			? Available[0].AvailabilityReason
			: EGridCombatActionAvailabilityReason::InvalidCharacter;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011713OilCombatAvailability,
	"Grimrock.RPG.ARMEMENT01.17.1.3.OilCombatAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011713OilCombatAvailability::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Oil = ARMEMENT011713::LoadOil();
	if (!TestNotNull(TEXT("Actual authored Oil DataAsset"), Oil))
	{
		return false;
	}

	FGridCombatActionDefinition Action;
	if (!TestTrue(TEXT("Oil has a real executable QuickItem definition"),
		Oil->BuildQuickItemCombatActionDefinition(Action)))
	{
		return false;
	}
	if (!TestEqual(TEXT("Oil creates one surface"), Action.SurfaceEffects.Num(), 1))
	{
		return false;
	}
	TestTrue(TEXT("Oil is a targeted environmental effect, not a self-heal"),
		Action.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Action.SurfaceEffects[0].SurfaceType == EGridCombatSurfaceType::Oil);
	TestTrue(TEXT("Oil item costs a physical consumable"),
		Action.ResourceCosts.SourceItemQuantityCost == 1);

	FGridCombatActionCatalogContext Context = ARMEMENT011713::MakeContext();
	bool bEnabled = false;
	TestEqual(TEXT("A surface is useful even with full HP and mana"),
		ARMEMENT011713::Evaluate(Context, Action, 1, bEnabled),
		EGridCombatActionAvailabilityReason::None);
	TestTrue(TEXT("Existing Oil action is enabled in combat"), bEnabled);

	Context.RemainingActionPoints = 1;
	TestEqual(TEXT("No bypass of the two-action-point cost"),
		ARMEMENT011713::Evaluate(Context, Action, 1, bEnabled),
		EGridCombatActionAvailabilityReason::InsufficientActionPoints);
	TestFalse(TEXT("Insufficient PA disables oil"), bEnabled);

	Context.RemainingActionPoints = 4;
	TestEqual(TEXT("No inventory flask means no use"),
		ARMEMENT011713::Evaluate(Context, Action, 0, bEnabled),
		EGridCombatActionAvailabilityReason::InsufficientSourceItems);
	TestFalse(TEXT("Zero flasks disables action"), bEnabled);

	Context.bEnableQuickItemExecutors = false;
	TestEqual(TEXT("Executor opt-in is still enforced"),
		ARMEMENT011713::Evaluate(Context, Action, 1, bEnabled),
		EGridCombatActionAvailabilityReason::ExecutionNotImplemented);
	TestFalse(TEXT("Unavailable executor disables action"), bEnabled);

	Context.bEnableQuickItemExecutors = true;
	Context.CurrentHealth = 20;
	Context.MaximumHealth = 20;
	TestEqual(TEXT("Restoring executors re-enables world utility"),
		ARMEMENT011713::Evaluate(Context, Action, 1, bEnabled),
		EGridCombatActionAvailabilityReason::None);
	TestTrue(TEXT("Oil is available again"), bEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011713OilTooltip,
	"Grimrock.RPG.ARMEMENT01.17.1.3.OilTooltipPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011713OilTooltip::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Oil = ARMEMENT011713::LoadOil();
	if (!TestNotNull(TEXT("Production Oil item"), Oil))
	{
		return false;
	}
	FGridCombatActionDefinition Action;
	if (!TestTrue(TEXT("Same action source used by inventory and hotbar"),
		Oil->BuildQuickItemCombatActionDefinition(Action)))
	{
		return false;
	}
	const FText Summary = FGridQuickItemSurfaceTooltipText::Build(Action);
	if (!TestFalse(TEXT("Localized mechanical summary must be materialized in ST_UI"),
		Summary.IsEmpty()))
	{
		return false;
	}
	TestTrue(TEXT("Summary includes canonical range and duration"),
		Summary.ToString().Contains(TEXT("4")));
	TestTrue(TEXT("Summary includes AP cost"),
		Summary.ToString().Contains(TEXT("2")));

	// This text is derived from mechanics and should respond to real changes,
	// rather than being an out-of-date, hardcoded oil-flask description.
	FGridCombatActionDefinition Changed = Action;
	Changed.RangeCells = 3;
	const FText Reprojected = FGridQuickItemSurfaceTooltipText::Build(Changed);
	TestFalse(TEXT("Mechanical tooltip updates when action data changes"),
		Reprojected.EqualTo(Summary));

	Changed = Action;
	Changed.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	TestTrue(TEXT("Unrelated spells never inherit the oil QuickItem tooltip"),
		FGridQuickItemSurfaceTooltipText::Build(Changed).IsEmpty());

	Changed = Action;
	Changed.SurfaceEffects.Reset();
	TestTrue(TEXT("No surface payload means no invented tooltip"),
		FGridQuickItemSurfaceTooltipText::Build(Changed).IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
