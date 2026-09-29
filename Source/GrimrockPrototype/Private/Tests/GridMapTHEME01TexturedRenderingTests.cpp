#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Engine/Texture2D.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapVisualThemeAsset.h"
#include "UI/GridMapWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapTHEME01AssetContractTest, "Grimrock.UI.MapTheme01.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapTHEME01AssetContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ThemeClass = UGridMapVisualThemeAsset::StaticClass();
	UClass* WidgetClass = UGridMapWidget::StaticClass();
	if (!TestNotNull(TEXT("Map visual theme class exists"), ThemeClass) ||
		!TestNotNull(TEXT("Map widget class exists"), WidgetClass))
	{
		return false;
	}

	for (const TCHAR* PropertyName : {
		TEXT("ParchmentTexture"),
		TEXT("WallTexture"),
		TEXT("DoorTexture"),
		TEXT("SecretDoorTexture"),
		TEXT("StairsUpTexture"),
		TEXT("StairsDownTexture"),
		TEXT("RelocationTexture"),
		TEXT("PitTexture"),
		TEXT("PointOfInterestTexture"),
		TEXT("PartyMarkerTexture") })
	{
		const FObjectPropertyBase* Property =
			CastField<FObjectPropertyBase>(ThemeClass->FindPropertyByName(PropertyName));
		TestTrue(
			*FString::Printf(TEXT("%s is a UTexture2D reference"), PropertyName),
			Property && Property->PropertyClass == UTexture2D::StaticClass());
		TestFalse(
			*FString::Printf(TEXT("%s is presentation-only, never SaveGame"), PropertyName),
			Property && Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	const FObjectPropertyBase* ThemeProperty =
		CastField<FObjectPropertyBase>(WidgetClass->FindPropertyByName(TEXT("VisualTheme")));
	TestTrue(TEXT("WBP_GridMap exposes the UGridMapVisualThemeAsset contract"),
		ThemeProperty && ThemeProperty->PropertyClass == UGridMapVisualThemeAsset::StaticClass());
	TestFalse(TEXT("VisualTheme is never SaveGame state"),
		ThemeProperty && ThemeProperty->HasAnyPropertyFlags(CPF_SaveGame));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapTHEME01DefaultsTest, "Grimrock.UI.MapTheme01.DefaultsAndFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapTHEME01DefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMapVisualThemeAsset* Theme = NewObject<UGridMapVisualThemeAsset>(GetTransientPackage());
	UGridMapWidget* Widget = NewObject<UGridMapWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Theme exists"), Theme) || !TestNotNull(TEXT("Map widget exists"), Widget))
	{
		return false;
	}

	TestNull(TEXT("Textured theme is opt-in so the validated renderer remains the fallback"), Widget->VisualTheme.Get());
	TestEqual(TEXT("Parchment is fully opaque by default"), Theme->ParchmentOpacity, 1.0f);
	TestTrue(TEXT("Parchment opacity default is within the supported range"),
		Theme->ParchmentOpacity >= 0.0f && Theme->ParchmentOpacity <= 1.0f);
	TestTrue(TEXT("Boundary texture thickness has a usable default"),
		Theme->BoundaryThicknessRatio >= 0.03f && Theme->BoundaryThicknessRatio <= 0.50f);
	TestTrue(TEXT("Symbol texture scale has a usable default"),
		Theme->SymbolScale >= 0.20f && Theme->SymbolScale <= 1.00f);
	TestTrue(TEXT("Party marker texture scale has a usable default"),
		Theme->PartyMarkerScale >= 0.20f && Theme->PartyMarkerScale <= 1.00f);
	TestTrue(TEXT("Symbol visibility threshold is positive"), Theme->SymbolMinCellPixels >= 4.0f);

	Widget->VisualTheme = Theme;
	TestTrue(TEXT("Map widget accepts one data-driven visual theme"), Widget->VisualTheme == Theme);
	TestEqual(TEXT("MAP-THEME01 does not change exact-match SaveGame version"),
		UGrimrockPartySaveGame::CurrentSaveVersion, 23);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
