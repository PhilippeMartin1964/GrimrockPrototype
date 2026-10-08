#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Engine/Texture2D.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapVisualThemeAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMapFLOOR01ContractTest,
	"Grimrock.UI.MapFloor01.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapFLOOR01ContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ThemeClass = UGridMapVisualThemeAsset::StaticClass();
	if (!TestNotNull(TEXT("Map visual theme class exists"), ThemeClass))
	{
		return false;
	}

	const FObjectPropertyBase* FloorTextureProperty =
		CastField<FObjectPropertyBase>(ThemeClass->FindPropertyByName(TEXT("FloorTexture")));
	TestTrue(
		TEXT("FloorTexture is a UTexture2D reference"),
		FloorTextureProperty && FloorTextureProperty->PropertyClass == UTexture2D::StaticClass());
	TestFalse(
		TEXT("FloorTexture is presentation-only, never SaveGame"),
		FloorTextureProperty && FloorTextureProperty->HasAnyPropertyFlags(CPF_SaveGame));

	const FFloatProperty* FloorOpacityProperty =
		CastField<FFloatProperty>(ThemeClass->FindPropertyByName(TEXT("FloorOpacity")));
	TestNotNull(TEXT("FloorOpacity exists"), FloorOpacityProperty);
	TestFalse(
		TEXT("FloorOpacity is presentation-only, never SaveGame"),
		FloorOpacityProperty && FloorOpacityProperty->HasAnyPropertyFlags(CPF_SaveGame));

	UGridMapVisualThemeAsset* Theme = NewObject<UGridMapVisualThemeAsset>(GetTransientPackage());
	if (!TestNotNull(TEXT("Theme exists"), Theme))
	{
		return false;
	}

	TestNull(TEXT("Floor texture is opt-in"), Theme->FloorTexture.Get());
	TestEqual(TEXT("Floor opacity defaults to 0.35"), Theme->FloorOpacity, 0.35f);
	TestTrue(
		TEXT("Floor opacity default is within the supported range"),
		Theme->FloorOpacity >= 0.0f && Theme->FloorOpacity <= 1.0f);
	TestEqual(
		TEXT("MAP-FLOOR01 does not change exact-match SaveGame version"),
		UGrimrockPartySaveGame::CurrentSaveVersion,
		24);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
