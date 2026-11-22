#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "RPG/RPGStartingSurfaceCatalystAuthoring.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01172CatalystContract,
	"Grimrock.RPG.ARMEMENT01.17.2.CatalystDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01172CatalystContract::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	FRPGStartingSurfaceCatalystAuthoring::Configure(*Item);
	if (!TestTrue(TEXT("N1 catalyst has a genuine validated item definition"),
		FRPGStartingSurfaceCatalystAuthoring::Matches(*Item))) return false;
	FGridCombatActionDefinition Action;
	if (!TestTrue(TEXT("Canonical QuickItem action resolves"),
		Item->BuildQuickItemCombatActionDefinition(Action))) return false;
	TestEqual(TEXT("Uses one physical item"), Action.ResourceCosts.SourceItemQuantityCost, 1);
	TestEqual(TEXT("Costs 2 AP in combat"), Action.ActionPointCost, 2);
	TestEqual(TEXT("Range 4"), Action.RangeCells, 4);
	TestTrue(TEXT("Square Area1 radius with neutral-cell acceptance and LOS"),
		Action.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Action.AreaRadiusCells == 1 && Action.AreaShape == EGridCombatAreaShape::Square &&
		Action.bAllowNoSurfaceReaction && Action.bUsableDuringExploration &&
		Action.bRequiresLineOfSight);
	TestTrue(TEXT("Fire interaction, no surface generation or talent action"),
		Action.SurfaceInteraction == EGridCombatSurfaceInteraction::Fire &&
		Action.SurfaceEffects.IsEmpty() && Action.SurfaceConversions.IsEmpty() &&
		Action.ActionId != FName(TEXT("Action_Mage_ElementalConversion")) &&
		Action.ActionId != FName(TEXT("Action_Alchemist_Catalyst")));

	FGridCombatSurfaceState Oil;
	Oil.SurfaceType = EGridCombatSurfaceType::Oil;
	Oil.RemainingRounds = 4;
	FGridCombatSurfaceReactionResult Reaction;
	const FGridResolvedCombatModifiers None;
	TestTrue(TEXT("Catalyst on existing oil triggers canonical Fire reaction"),
		FGridCombatSurfaceResolver::ResolveReaction(Oil, Action.SurfaceInteraction, None, Reaction));
	TestEqual(TEXT("Oil becomes Fire"), Reaction.OutputSurfaceType, EGridCombatSurfaceType::Fire);
	Oil.SurfaceType = EGridCombatSurfaceType::Water;
	TestFalse(TEXT("Catalyst cannot ignite neutral water"),
		FGridCombatSurfaceResolver::ResolveReaction(Oil, Action.SurfaceInteraction, None, Reaction));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01172CatalystProductionAsset,
	"Grimrock.RPG.ARMEMENT01.17.2.ProductionCatalyst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
namespace CanonicalCatalystTest
{
    constexpr TCHAR ItemId[] = TEXT("Item_SurfaceCatalyst_01");
    constexpr TCHAR ActionId[] = TEXT("Action_Mage_SurfaceCatalyst_01");

    bool Resolve(FGridEditorCanonicalStarterTestWorld& Fixture,
        FGridResolvedItem& Item, FGridCombatActionDefinition& Action)
    {
        if (!Fixture.Ready() ||
            !Fixture.Registry->ResolveItem(ItemId, 0, Item) ||
            Item.AvailableActions.Num() != 1 ||
            Item.AvailableActions[0].ActionId != FName(ActionId))
        {
            return false;
        }
        return Fixture.Registry->ResolveDetailedNonArmAction(ItemId, 0, ActionId, Action);
    }
}

bool FARMEMENT01172CatalystProductionAsset::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    FGridResolvedItem Item;
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Canonical N0 catalyst item/action resolves"),
        CanonicalCatalystTest::Resolve(Fixture, Item, Action))) return false;
    TestTrue(TEXT("Valid non-ARM catalyst"), Item.IsValid() && !Item.bIsArmament && Item.ItemLevel == 0);
    TestTrue(TEXT("Catalyst retains surface trigger tag"),
        Item.Tags.Contains(TEXT("QuickItem.SurfaceCatalyst")));
    TestTrue(TEXT("Catalyst has its own physical mesh and icon"),
        !Item.WorldMesh.IsNull() && !Item.ItemIcon.IsNull());
    TestTrue(TEXT("Catalyst still invokes the explicit action"),
        Action.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
        Action.ActionId == FName(CanonicalCatalystTest::ActionId));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01173DSerializedCatalyst,
    "Grimrock.RPG.ARMEMENT01.17.3D.SerializedCatalystContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01173DSerializedCatalyst::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    FGridResolvedItem Item;
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Production catalyst resolved without obsolete authoring DA"),
        CanonicalCatalystTest::Resolve(Fixture, Item, Action))) return false;
    TestEqual(TEXT("Distinct canonical artist mesh"),
        Item.WorldMesh.ToSoftObjectPath().ToString(),
        FString(TEXT("/Game/GrimrockPrototype/Meshes/Items/SM_Catalyst_01.SM_Catalyst_01")));
    TestEqual(TEXT("Distinct canonical artist icon"),
        Item.ItemIcon.ToSoftObjectPath().ToString(),
        FString(TEXT("/Game/GrimrockPrototype/UI/Icons/Items/T_Catalyst_Icon.T_Catalyst_Icon")));
    const bool bLiquidSparkle = Item.VisualEffects.Effects.ContainsByPredicate(
        [](const FGridItemNiagaraEffectConfig& Effect)
        {
            return Effect.EffectId == FName(TEXT("LiquidSparkle"));
        });
    TestFalse(TEXT("Catalyst does not borrow the healing sparkle"), bLiquidSparkle);
    TestTrue(TEXT("Canonical native action is square Area1"),
        Action.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
        Action.AreaRadiusCells == 1 &&
        Action.AreaShape == EGridCombatAreaShape::Square);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01173DNativeActionRuntime,
    "Grimrock.RPG.ARMEMENT01.17.3D.NativeActionWithoutBridge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01173DNativeActionRuntime::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    FGridResolvedItem Item;
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Canonical catalyst action resolves without old source"),
        CanonicalCatalystTest::Resolve(Fixture, Item, Action))) return false;
    TestEqual(TEXT("Area shape still square"), Action.AreaShape, EGridCombatAreaShape::Square);
    TestEqual(TEXT("Area radius stays one cell"), Action.AreaRadiusCells, 1);
    TestTrue(TEXT("No-surface reaction remains permitted"), Action.bAllowNoSurfaceReaction);
    TestTrue(TEXT("Exploration activation remains permitted"), Action.bUsableDuringExploration);
    TestEqual(TEXT("AP cost stays two"), Action.ActionPointCost, 2);
    TestTrue(TEXT("Line of sight stays required"), Action.bRequiresLineOfSight);
    TestEqual(TEXT("Fire surface interaction"), Action.SurfaceInteraction, EGridCombatSurfaceInteraction::Fire);
    TestEqual(TEXT("One catalyst consumed"), Action.ResourceCosts.SourceItemQuantityCost, 1);
    return !HasAnyErrors();
}

#endif
