#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapART01DefaultsTest, "Grimrock.UI.MapART01.DetailDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapART01DefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMapWidget* Widget = NewObject<UGridMapWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Map widget exists"), Widget))
	{
		return false;
	}

	TestTrue(TEXT("Walls use stonework detail by default"), Widget->WallStoneMarkCount >= 2);
	TestTrue(TEXT("Doors use panel detail by default"), Widget->DoorPanelLineCount >= 1);
	TestTrue(TEXT("Stair glyphs use at least four treads"), Widget->StairStepCount >= 4);
	TestTrue(TEXT("Pit glyphs expose depth hatching"), Widget->PitDepthLineCount >= 2);
	TestTrue(TEXT("Symbols have multiple deterministic visual variants"), Widget->SymbolVariantCount >= 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapART01VariantTest, "Grimrock.UI.MapART01.DeterministicSymbolVariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapART01VariantTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FIntPoint Cell(7, -11);
	const int32 VariantCount = 3;
	const int32 First = UGridMapWidget::ComputeDeterministicSymbolVariant(Cell, EGridMapSymbolKind::Pit, VariantCount);
	const int32 Second = UGridMapWidget::ComputeDeterministicSymbolVariant(Cell, EGridMapSymbolKind::Pit, VariantCount);

	TestEqual(TEXT("Same cell and symbol kind always produce the same visual variant"), First, Second);
	TestTrue(TEXT("Variant stays inside configured range"), First >= 0 && First < VariantCount);
	TestEqual(TEXT("A single configured variant always resolves to zero"),
		UGridMapWidget::ComputeDeterministicSymbolVariant(Cell, EGridMapSymbolKind::PointOfInterest, 1), 0);

	TSet<int32> ObservedPitVariants;
	for (int32 X = -8; X <= 8; ++X)
	{
		ObservedPitVariants.Add(
			UGridMapWidget::ComputeDeterministicSymbolVariant(FIntPoint(X, 5), EGridMapSymbolKind::Pit, VariantCount));
	}
	TestTrue(TEXT("Different map cells produce more than one stable pit drawing variant"), ObservedPitVariants.Num() > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapART01PresentationOnlyTest, "Grimrock.UI.MapART01.PresentationOnlyContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapART01PresentationOnlyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MapClass = UGridMapWidget::StaticClass();
	for (const TCHAR* PropertyName : {
		TEXT("WallStoneMarkCount"),
		TEXT("DoorPanelLineCount"),
		TEXT("StairStepCount"),
		TEXT("PitDepthLineCount"),
		TEXT("SymbolVariantCount") })
	{
		const FProperty* Property = MapClass->FindPropertyByName(PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s exists"), PropertyName), Property);
		TestFalse(*FString::Printf(TEXT("%s is presentation-only, never SaveGame"), PropertyName),
			Property && Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	TestEqual(TEXT("MAP-ART01 does not change exact-match SaveGame version"),
		UGrimrockPartySaveGame::CurrentSaveVersion, 24);
	return true;
}

#endif
