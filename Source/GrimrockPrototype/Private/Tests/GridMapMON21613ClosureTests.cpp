#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapWidget.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMapMON21613CanvasFitTest,
	"Grimrock.Map.MON21_6_13.Layout.CanvasAutoFitUsesAvailableSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21613CanvasFitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FVector2D CanvasSize(1200.0, 700.0);
	const FMargin Padding(48.0f, 72.0f, 48.0f, 96.0f);

	const float Uncapped = UGridMapWidget::ComputeAutoFitCellSize(
		CanvasSize,
		Padding,
		5,
		5,
		0.75f,
		0.0f,
		1.0f);

	const float LegacyCapped = UGridMapWidget::ComputeAutoFitCellSize(
		CanvasSize,
		Padding,
		5,
		5,
		0.75f,
		64.0f,
		1.0f);

	TestTrue(TEXT("Canvas-driven five-cell map is no longer constrained by the former 64 px ceiling"), Uncapped > 64.0f);
	TestTrue(TEXT("Optional designer cap still works when explicitly configured"), FMath::IsNearlyEqual(LegacyCapped, 64.0f, 0.01f));
	TestTrue(TEXT("Auto-fit remains bounded by available canvas height"), Uncapped * 5.0f < 700.0f - Padding.Top - Padding.Bottom);

	const float FullTile = UGridMapWidget::ComputeAutoFitCellSize(
		CanvasSize,
		Padding,
		32,
		32,
		0.75f,
		0.0f,
		1.0f);
	TestTrue(TEXT("A full 32x32 tile still receives a valid readable fit"), FullTile > 0.0f);
	TestTrue(TEXT("A full 32x32 tile remains inside the available canvas height"),
		FullTile * 32.0f < 700.0f - Padding.Top - Padding.Bottom);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMapMON21613BreathingRoomTest,
	"Grimrock.Map.MON21_6_13.Layout.BreathingRoomAndZoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21613BreathingRoomTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FVector2D CanvasSize(1200.0, 700.0);
	const FMargin Padding(48.0f, 72.0f, 48.0f, 96.0f);

	const float OneCell = UGridMapWidget::ComputeAutoFitCellSize(
		CanvasSize,
		Padding,
		1,
		1,
		0.75f,
		0.0f,
		1.0f);
	const float Zoomed = UGridMapWidget::ComputeAutoFitCellSize(
		CanvasSize,
		Padding,
		1,
		1,
		0.75f,
		0.0f,
		2.0f);

	TestTrue(TEXT("Breathing margin prevents a one-cell map from consuming the full available height"),
		OneCell < 700.0f - Padding.Top - Padding.Bottom);
	TestTrue(TEXT("A one-cell map still receives a useful readable footprint"), OneCell > 100.0f);
	TestTrue(TEXT("Zoom is applied after canvas auto-fit"), FMath::IsNearlyEqual(Zoomed, OneCell * 2.0f, 0.01f));

	TestEqual(TEXT("Invalid zero-width content fails closed"),
		UGridMapWidget::ComputeAutoFitCellSize(CanvasSize, Padding, 0, 5, 0.75f, 0.0f, 1.0f), 0.0f);
	TestEqual(TEXT("Invalid canvas fails closed"),
		UGridMapWidget::ComputeAutoFitCellSize(FVector2D(40.0, 40.0), Padding, 5, 5, 0.75f, 0.0f, 1.0f), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMapMON21613PolishDefaultsTest,
	"Grimrock.Map.MON21_6_13.Presentation.FinalPolishDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21613PolishDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMapWidget* Widget = NewObject<UGridMapWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Map widget exists"), Widget))
	{
		return false;
	}

	TestEqual(TEXT("Legacy fixed cell ceiling is disabled by default"), Widget->MaxCellPixels, 0.0f);
	TestTrue(TEXT("Auto-fit leaves breathing room around known geometry"), Widget->AutoFitMarginCells > 0.0f);
	TestTrue(TEXT("Standalone map uses the full allotted surface by default"),
		FMath::IsNearlyZero(Widget->MapDrawPadding.Left) &&
		FMath::IsNearlyZero(Widget->MapDrawPadding.Top) &&
		FMath::IsNearlyZero(Widget->MapDrawPadding.Right) &&
		FMath::IsNearlyZero(Widget->MapDrawPadding.Bottom));
	TestTrue(TEXT("Parchment frame is visible"), Widget->ParchmentEdgeColor.A > 0.0f && Widget->ParchmentEdgeThickness > 0.0f);
	TestTrue(TEXT("Walls have a visible broad underlay"), Widget->WallUnderlayColor.A > 0.0f && Widget->WallUnderlayThicknessScale > 1.0f);
	TestTrue(TEXT("Doors receive graphical jambs"), Widget->DoorJambLengthScale > 1.0f);
	TestTrue(TEXT("Party marker is reduced so underlying map symbols retain visual breathing room"),
		Widget->PartyMarkerScale >= 0.4f && Widget->PartyMarkerScale < 1.0f);
	TestTrue(TEXT("Final hatching is deliberately lighter than the first artistic pass"),
		Widget->CellHatchLineCount <= 2 && Widget->CellHatchColor.A < 0.20f);
	TestTrue(TEXT("Hand-drawn jitter remains present but restrained"),
		Widget->HandDrawnJitterPixels > 0.0f && Widget->HandDrawnJitterPixels < 1.35f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMapMON21613ClosureContractTest,
	"Grimrock.Map.MON21_6_13.Closure.TransientPresentationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21613ClosureContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MapClass = UGridMapWidget::StaticClass();
	for (const TCHAR* PropertyName : {
		TEXT("AutoFitMarginCells"),
		TEXT("MaxCellPixels"),
		TEXT("ParchmentEdgeColor"),
		TEXT("ParchmentEdgeThickness"),
		TEXT("WallUnderlayColor"),
		TEXT("WallUnderlayThicknessScale"),
		TEXT("DoorJambLengthScale"),
		TEXT("PartyMarkerScale") })
	{
		const FProperty* Property = MapClass->FindPropertyByName(PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s exists"), PropertyName), Property);
		TestFalse(*FString::Printf(TEXT("%s remains presentation-only, never SaveGame"), PropertyName),
			Property && Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	const FProperty* FloorViewProperty = MapClass->FindPropertyByName(TEXT("FloorView"));
	TestNotNull(TEXT("FloorView projection remains reflected"), FloorViewProperty);
	TestTrue(TEXT("FloorView remains explicitly transient"),
		FloorViewProperty && FloorViewProperty->HasAnyPropertyFlags(CPF_Transient));
	TestFalse(TEXT("FloorView is never SaveGame"),
		FloorViewProperty && FloorViewProperty->HasAnyPropertyFlags(CPF_SaveGame));

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
	TestEqual(TEXT("World object definitions still opt out of map symbols by default"),
		Definition->MapSymbolStyle, EGridMapSymbolStyle::None);

	TestEqual(TEXT("Map closure does not change exact-match SaveGame v23"),
		UGrimrockPartySaveGame::CurrentSaveVersion, 23);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
