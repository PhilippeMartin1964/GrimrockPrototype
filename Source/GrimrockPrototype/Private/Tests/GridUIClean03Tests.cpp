#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "UI/GridInventoryWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIClean03NoLegacyArmorAliasTest, "Grimrock.UI.Clean03.NoLegacyArmorAlias",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIClean03NoLegacyArmorAliasTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* InventoryClass = UGridInventoryWidget::StaticClass();
	if (!TestNotNull(TEXT("Inventory widget class exists"), InventoryClass))
	{
		return false;
	}

	TestNull(TEXT("Legacy Text_CharacterArmor binding is removed"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("Text_CharacterArmor"))));

	TestNotNull(TEXT("Canonical Text_CharacterPhysicalArmor binding remains"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("Text_CharacterPhysicalArmor"))));

	TestNotNull(TEXT("Magical armor binding remains"),
		FindFProperty<FProperty>(InventoryClass, FName(TEXT("Text_CharacterMagicalArmor"))));

	return true;
}

#endif
