#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridPartyHud01Tests
{
	FGuid MakeId(uint32 Value)
	{
		return FGuid(0x50485544, 0x30310000, 0, Value);
	}

	FGridCharacterInventoryState MakeCharacter(
		const FGuid& CharacterId,
		URPGClassAsset* ClassDefinition,
		int32 CurrentHealth,
		int32 MaximumHealth,
		int32 CurrentMana,
		int32 MaximumMana,
		int32 CurrentPhysicalArmor,
		int32 CurrentMagicalArmor)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = CharacterId;
		Character.DisplayName = FText::FromString(FString::Printf(TEXT("Portrait_%u"), CharacterId.D));
		Character.ClassId = ClassDefinition ? ClassDefinition->ClassId : NAME_None;
		Character.ClassDefinition = ClassDefinition;
		Character.DerivedStats.MaxHealth = MaximumHealth;
		Character.DerivedStats.MaxMana = MaximumMana;
		Character.Resources.CurrentHealth = CurrentHealth;
		Character.Resources.CurrentMana = CurrentMana;
		Character.Resources.CurrentPhysicalArmor = CurrentPhysicalArmor;
		Character.Resources.CurrentMagicalArmor = CurrentMagicalArmor;
		return Character;
	}

	URPGClassAsset* MakeClass(UObject* Outer, FName ClassId, int32 PhysicalArmor, int32 MagicalArmor)
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(Outer);
		ClassDefinition->ClassId = ClassId;
		ClassDefinition->DisplayName = FText::FromName(ClassId);
		ClassDefinition->HealthAtLevelOne = 1;
		ClassDefinition->BasePhysicalArmor = PhysicalArmor;
		ClassDefinition->BaseMagicalArmor = MagicalArmor;
		return ClassDefinition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01ViewProjectionTest, "Grimrock.UI.PartyHud01.ViewProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01ViewProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* MageClass = MakeClass(Inventory, TEXT("PartyHudMage"), 10, 4);
	URPGClassAsset* WarriorClass = MakeClass(Inventory, TEXT("PartyHudWarrior"), 6, 0);

	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(MakeId(1), MageClass, 75, 100, 25, 50, 8, 2));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(MakeId(2), WarriorClass, 40, 80, 0, 0, 3, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 0;

	TArray<FGridPartyPortraitView> Views;
	TestTrue(TEXT("Portrait projection is complete"), Inventory->GetPartyPortraitViews(Views));
	TestEqual(TEXT("Two portraits are projected"), Views.Num(), 2);
	if (Views.Num() != 2)
	{
		return false;
	}

	TestEqual(TEXT("Mage CharacterId remains stable"), Views[0].CharacterId, MakeId(1));
	TestEqual(TEXT("Mage runtime index is resolved"), Views[0].CharacterIndex, 0);
	TestTrue(TEXT("Mage is selected"), Views[0].bIsSelected);
	TestEqual(TEXT("Health current"), Views[0].CurrentHealth, 75);
	TestEqual(TEXT("Health maximum"), Views[0].MaximumHealth, 100);
	TestTrue(TEXT("Health ratio"), FMath::IsNearlyEqual(Views[0].HealthRatio, 0.75f));
	TestTrue(TEXT("Mage exposes mana"), Views[0].bHasMana);
	TestTrue(TEXT("Mana ratio"), FMath::IsNearlyEqual(Views[0].ManaRatio, 0.5f));
	TestEqual(TEXT("Physical armor reference uses class authority"), Views[0].ReferencePhysicalArmor, 10);
	TestTrue(TEXT("Physical armor ratio"), FMath::IsNearlyEqual(Views[0].PhysicalArmorRatio, 0.8f));
	TestEqual(TEXT("Magical armor reference uses class authority"), Views[0].ReferenceMagicalArmor, 4);
	TestTrue(TEXT("Magical armor ratio"), FMath::IsNearlyEqual(Views[0].MagicalArmorRatio, 0.5f));

	TestFalse(TEXT("Zero-mana warrior hides mana bar"), Views[1].bHasMana);
	TestEqual(TEXT("Zero-mana warrior ratio is zero"), Views[1].ManaRatio, 0.0f);
	TestEqual(TEXT("Zero magical armor reference remains zero"), Views[1].ReferenceMagicalArmor, 0);
	TestEqual(TEXT("Zero magical armor ratio remains zero"), Views[1].MagicalArmorRatio, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01OrderSanitizationTest, "Grimrock.UI.PartyHud01.OrderSanitization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01OrderSanitizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudOrder"), 0, 0);
	const FGuid A = MakeId(10);
	const FGuid B = MakeId(11);
	const FGuid C = MakeId(12);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(C, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(3);

	Inventory->PartyPortraitOrder = { C, FGuid(), C, A, MakeId(99) };
	Inventory->SanitizePartyPortraitOrder();

	TestEqual(TEXT("Sanitized order contains every active character once"), Inventory->PartyPortraitOrder.Num(), 3);
	TestEqual(TEXT("Existing valid first entry is preserved"), Inventory->PartyPortraitOrder[0], C);
	TestEqual(TEXT("Existing valid second entry is preserved"), Inventory->PartyPortraitOrder[1], A);
	TestEqual(TEXT("Missing active character is appended"), Inventory->PartyPortraitOrder[2], B);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01ReorderPreservesRuntimeTest, "Grimrock.UI.PartyHud01.ReorderPreservesRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01ReorderPreservesRuntimeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudReorder"), 0, 0);
	const FGuid A = MakeId(20);
	const FGuid B = MakeId(21);
	const FGuid C = MakeId(22);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(C, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(3);
	Inventory->PartyInventoryState.SelectedCharacterIndex = 1;
	Inventory->SanitizePartyPortraitOrder();

	TestTrue(TEXT("C can be inserted before A"), Inventory->ReorderPartyPortrait(C, A, false));
	TestEqual(TEXT("Visual order C"), Inventory->PartyPortraitOrder[0], C);
	TestEqual(TEXT("Visual order A"), Inventory->PartyPortraitOrder[1], A);
	TestEqual(TEXT("Visual order B"), Inventory->PartyPortraitOrder[2], B);

	TestEqual(TEXT("Runtime A index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, A);
	TestEqual(TEXT("Runtime B index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[1].CharacterId, B);
	TestEqual(TEXT("Runtime C index is unchanged"), Inventory->PartyInventoryState.ActiveCharacters[2].CharacterId, C);
	TestEqual(TEXT("SelectedCharacterIndex is unchanged"), Inventory->PartyInventoryState.SelectedCharacterIndex, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPartyHud01RosterChangeTest, "Grimrock.UI.PartyHud01.RosterChangeSanitization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPartyHud01RosterChangeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPartyHud01Tests;

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	URPGClassAsset* ClassDefinition = MakeClass(Inventory, TEXT("PartyHudRoster"), 0, 0);
	const FGuid A = MakeId(30);
	const FGuid B = MakeId(31);
	const FGuid C = MakeId(32);
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(A, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(B, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.SetNum(2);
	Inventory->NotifyPartyInventoryChanged(INDEX_NONE);

	TestEqual(TEXT("Initial roster creates two portrait ids"), Inventory->PartyPortraitOrder.Num(), 2);

	Inventory->PartyInventoryState.ActiveCharacters.Add(MakeCharacter(C, ClassDefinition, 1, 1, 0, 0, 0, 0));
	Inventory->PartyInventoryState.ActiveEquipment.AddDefaulted();
	Inventory->NotifyPartyInventoryChanged(2);
	TestEqual(TEXT("New recruit is appended"), Inventory->PartyPortraitOrder.Last(), C);

	Inventory->PartyInventoryState.ActiveCharacters.RemoveAt(1);
	Inventory->PartyInventoryState.ActiveEquipment.RemoveAt(1);
	Inventory->NotifyPartyInventoryChanged(INDEX_NONE);
	TestEqual(TEXT("Removed character disappears from portrait order"), Inventory->PartyPortraitOrder.Num(), 2);
	TestFalse(TEXT("Removed id is absent"), Inventory->PartyPortraitOrder.Contains(B));
	TestTrue(TEXT("Remaining A is present"), Inventory->PartyPortraitOrder.Contains(A));
	TestTrue(TEXT("Remaining C is present"), Inventory->PartyPortraitOrder.Contains(C));
	return true;
}

#endif
