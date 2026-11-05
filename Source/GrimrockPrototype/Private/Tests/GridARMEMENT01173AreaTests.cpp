#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCatalystAreaResolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01173ASquareArea,
	"Grimrock.RPG.ARMEMENT01.17.3A.SquareArea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01173ASquareArea::RunTest(const FString&)
{
	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("Action_Mage_SurfaceCatalyst_01");
	Action.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	Action.SurfaceInteraction = EGridCombatSurfaceInteraction::Fire;
	// Explicit native action contract: no runtime mutation bridge.
	Action.TargetingPolicy = EGridCombatTargetingPolicy::Area;
	Action.AreaRadiusCells = 1;
	Action.AreaShape = EGridCombatAreaShape::Square;
	Action.bAllowNoSurfaceReaction = true;
	Action.bUsableDuringExploration = true;
	TestTrue(TEXT("Exactly approved item/action pair is recognized"),
		FGridCatalystAreaResolver::IsApprovedItem(TEXT("Item_SurfaceCatalyst_01"), Action.ActionId));
	TestFalse(TEXT("Similar item cannot acquire catalyst abilities"),
		FGridCatalystAreaResolver::IsApprovedItem(TEXT("Item_Flask_Oil"), Action.ActionId));
	TestEqual(TEXT("Default unrelated area shape remains Manhattan"),
		FGridCombatActionDefinition().AreaShape, EGridCombatAreaShape::Manhattan);
	TestEqual(TEXT("Authored catalyst shape is Square"), Action.AreaShape, EGridCombatAreaShape::Square);
	TestEqual(TEXT("Authored catalyst radius one"), Action.AreaRadiusCells, 1);
	TestTrue(TEXT("Native C6 action is approved"), FGridCatalystAreaResolver::IsApprovedAction(Action));

	TArray<FIntPoint> Cells;
	auto Walkable = [](FIntPoint Cell)
	{
		return Cell.X >= 0 && Cell.X < 3 && Cell.Y >= 0 && Cell.Y < 3;
	};
	FGridCatalystAreaResolver::BuildReachableSquare(FIntPoint(1, 1), 1,
		Walkable, [](FIntPoint, EGridEdge) { return true; }, Cells);
	if (!TestEqual(TEXT("Square includes all nine cells"), Cells.Num(), 9)) return false;
	TestTrue(TEXT("Square includes northeast corner"), Cells.Contains(FIntPoint(2, 2)));
	TestTrue(TEXT("Square includes southwest corner"), Cells.Contains(FIntPoint(0, 0)));
	TestEqual(TEXT("Impact cell is first"), Cells[0], FIntPoint(1, 1));
	// Divider wall/door between x=1 and x=2, across all three rows.
	FGridCatalystAreaResolver::BuildReachableSquare(FIntPoint(1, 1), 1,
		Walkable,
		[](FIntPoint From, EGridEdge Direction)
		{
			return !(From.X == 1 && Direction == EGridEdge::East) &&
				!(From.X == 2 && Direction == EGridEdge::West);
		}, Cells);
	TestEqual(TEXT("Impassable divider blocks entire right column"), Cells.Num(), 6);
	TestFalse(TEXT("No catalyst energy through solid wall"), Cells.Contains(FIntPoint(2, 1)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01173AMixedSurfaceSnapshot,
	"Grimrock.RPG.ARMEMENT01.17.3A.MixedSurfaceSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT01173AMixedSurfaceSnapshot::RunTest(const FString&)
{
	TArray<FGridCatalystCellSnapshot> Source;
	const EGridCombatSurfaceType Types[9] = {
		EGridCombatSurfaceType::Oil, EGridCombatSurfaceType::Poison,
		EGridCombatSurfaceType::Ice, EGridCombatSurfaceType::Water,
		EGridCombatSurfaceType::None, EGridCombatSurfaceType::PoisonCloud,
		EGridCombatSurfaceType::Fire, EGridCombatSurfaceType::None,
		EGridCombatSurfaceType::Oil
	};
	for (int32 Index = 0; Index < 9; ++Index)
	{
		FGridCatalystCellSnapshot& Cell = Source.AddDefaulted_GetRef();
		Cell.Cell = FIntPoint(Index % 3, Index / 3);
		Cell.bHasSurface = Types[Index] != EGridCombatSurfaceType::None;
		if (Cell.bHasSurface)
		{
			Cell.Surface.SurfaceType = Types[Index];
			Cell.Surface.RemainingRounds = 4;
		}
	}
	TArray<FGridCatalystCellOutcome> Results;
	const FGridResolvedCombatModifiers Modifiers;
	if (!TestTrue(TEXT("Nine snapshots classified exactly once"),
		FGridCatalystAreaResolver::ResolveSnapshot(
			Source, EGridCombatSurfaceInteraction::Fire, Modifiers, Results))) return false;
	if (!TestEqual(TEXT("Nine outcomes, including neutral and water"), Results.Num(), 9)) return false;
	TestEqual(TEXT("Oil ignites"), Results[0].Reaction.OutputSurfaceType, EGridCombatSurfaceType::Fire);
	TestTrue(TEXT("Poison is explosive"), Results[1].Reaction.bExplosive);
	TestEqual(TEXT("Ice melts"), Results[2].Reaction.OutputSurfaceType, EGridCombatSurfaceType::Water);
	TestFalse(TEXT("Water has no reaction"), Results[3].Reaction.bReacted);
	TestFalse(TEXT("Neutral ground is a valid fizzle"), Results[4].Reaction.bReacted);
	TestTrue(TEXT("Poison cloud is explosive"), Results[5].Reaction.bExplosive);
	TestFalse(TEXT("Already-burning fire has no new reaction"), Results[6].Reaction.bReacted);
	TestTrue(TEXT("Last oil is independently ignited"), Results[8].Reaction.bReacted);
	TestEqual(TEXT("Original snapshots were not mutated"), Source[0].Surface.SurfaceType, EGridCombatSurfaceType::Oil);

	// Copy before Add: TArray forbids aliasing an element of its own storage.
	const FGridCatalystCellSnapshot Duplicate = Source[0];
	Source.Add(Duplicate);
	TestFalse(TEXT("Duplicate cell causes failure before item consumption"),
		FGridCatalystAreaResolver::ResolveSnapshot(
			Source, EGridCombatSurfaceInteraction::Fire, Modifiers, Results));
	TestTrue(TEXT("Failed plan returns no partial outcome"), Results.IsEmpty());
	return true;
}

#endif
