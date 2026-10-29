#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Engine/Texture2D.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorPaletteIcon01,
	"Grimrock.Editor.Palette.ICON01.CanonicalIconAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorPaletteIcon01::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// The Details panel must hide the palette icon for a direct collectible.
	const FProperty* IconProperty = FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(
		GET_MEMBER_NAME_CHECKED(FGridObjectPaletteEntry, Icon));
	if (!TestNotNull(TEXT("Palette entry Icon is reflected"), IconProperty))
	{
		return false;
	}
	TestEqual(TEXT("Icon is editable only without a direct ItemDefinition"),
		IconProperty->GetMetaData(TEXT("EditCondition")), FString(TEXT("DefaultItemDefinition == nullptr")));
	TestTrue(TEXT("Inapplicable icon is hidden in Details"), IconProperty->HasMetaData(TEXT("EditConditionHides")));

	UTexture2D* WorldIcon = NewObject<UTexture2D>();
	UTexture2D* ItemIcon = NewObject<UTexture2D>();
	UGridWorldObjectDefinitionAsset* Door = NewObject<UGridWorldObjectDefinitionAsset>();
	Door->DefinitionId = TEXT("Door_Icon01");
	Door->SupportedType = EGridLevelObjectType::Door;
	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
	Item->ItemDefinitionId = TEXT("Item_Icon01");
	Item->Icon = ItemIcon;

	FGridObjectPaletteEntry WorldEntry;
	WorldEntry.EntryId = Door->DefinitionId;
	WorldEntry.PaletteCategory = TEXT("Doors");
	WorldEntry.DefaultWorldObjectDefinition = Door;
	WorldEntry.Icon = WorldIcon;
	TestFalse(TEXT("World-object icon must not be cleared"), WorldEntry.ClearRedundantIconForDirectItem());
	TestTrue(TEXT("World-object icon preserved"), WorldEntry.Icon.Get() == WorldIcon);

	FGridObjectPaletteEntry DirectItemEntry;
	DirectItemEntry.EntryId = Item->ItemDefinitionId;
	DirectItemEntry.PaletteCategory = TEXT("Items");
	DirectItemEntry.DefaultItemDefinition = Item;
	DirectItemEntry.Icon = WorldIcon; // Simulate setting Icon before choosing the ItemDefinition.
	TestFalse(TEXT("Duplicate icon is invalid before normalizing"), DirectItemEntry.IsValidEntry());
	TestTrue(TEXT("Switch to direct item clears its stale palette icon"), DirectItemEntry.ClearRedundantIconForDirectItem());
	TestNull(TEXT("Palette icon cleared for the direct item"), DirectItemEntry.Icon.Get());
	TestTrue(TEXT("Direct item valid after normalizing"), DirectItemEntry.IsValidEntry());
	TestFalse(TEXT("Normalization is idempotent"), DirectItemEntry.ClearRedundantIconForDirectItem());
	TestTrue(TEXT("ItemDefinition's canonical icon remains intact"), Item->Icon.Get() == ItemIcon);

	UGridObjectPaletteAsset* Palette = NewObject<UGridObjectPaletteAsset>();
	Palette->Entries.Add(WorldEntry);
	Palette->Entries.Add(DirectItemEntry);
	TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
	TestTrue(TEXT("Mixed door and collectible palette remains valid"), Palette->ValidatePalette(Messages));
	TestEqual(TEXT("No palette validation diagnostics"), Messages.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
