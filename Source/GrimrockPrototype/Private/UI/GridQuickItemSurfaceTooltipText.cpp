#include "UI/GridQuickItemSurfaceTooltipText.h"

#include "UI/UILocalizationText.h"

FText FGridQuickItemSurfaceTooltipText::Build(const FGridCombatActionDefinition& Action)
{
	// Project mechanics from the actual action; never grant a talent or
	// duplicate the item's narrative DataAsset description.
	if (Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ||
		Action.ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
		Action.TargetingPolicy != EGridCombatTargetingPolicy::Area ||
		Action.SurfaceEffects.Num() != 1 ||
		Action.SurfaceEffects[0].SurfaceType != EGridCombatSurfaceType::Oil)
	{
		return FText::GetEmpty();
	}

	const FGridCombatSurfaceEffectProfile& Oil = Action.SurfaceEffects[0];
	FFormatNamedArguments Args;
	Args.Add(TEXT("Range"), Action.RangeCells);
	Args.Add(TEXT("Radius"), Action.AreaRadiusCells);
	Args.Add(TEXT("Duration"), Oil.DurationRounds);
	Args.Add(TEXT("TraversalCost"), Oil.TraversalCostModifier);
	Args.Add(TEXT("ActionPoints"), Action.ActionPointCost);
	Args.Add(TEXT("Quantity"), Action.ResourceCosts.SourceItemQuantityCost);
	const FText Pattern = FUILocalizationText::Get(TEXT("Combat.QuickItem.Surface.OilUsage"));
	return Pattern.IsEmpty() ? FText::GetEmpty() : FText::Format(Pattern, Args);
}
