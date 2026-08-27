#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridTD07333Characterization
{
	UGridPartyInventoryComponent* MakeParty()
	{
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->PartyInventoryState = FGridPartyInventoryState();
		Inventory->PartyInventoryState.MaxActiveCharacters = 6;
		Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
		Inventory->PartyInventoryState.bInitialCharacterCreationCompleted = true;

		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid(7, 3, 3, 31);
		Character.DisplayName = FText::FromString(TEXT("TD07333"));
		Character.ClassId = TEXT("Warrior");
		Character.ClassDisplayName = FText::FromString(TEXT("Warrior"));
		Character.RaceId = TEXT("Human");
		Character.RaceDisplayName = FText::FromString(TEXT("Human"));
		Character.Level = 1;
		Character.Experience = 0;
		Character.Attributes = FRPGAttributes(10, 12, 10, 10, 10, 10);
		Character.DerivedStats.MaxHealth = 20;
		Character.Resources.CurrentHealth = 15;
		Character.DerivedStats.MaxMana = 10;
		Character.Resources.CurrentMana = 4;
		Character.Resources.CurrentPhysicalArmor = 2;
		Character.Resources.CurrentMagicalArmor = 1;
		Character.DerivedStats.Initiative = 1;
		Character.DerivedStats.Accuracy = 1;
		Character.DerivedStats.Evasion = 1;
		Character.InventorySlots.SetNum(4);
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < Character.CombatHotbarSlots.Num(); ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}

		Inventory->PartyInventoryState.ActiveCharacters.Add(Character);
		Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Inventory;
	}

	UGridItemDefinitionAsset* EquipProjectionItem(UGridPartyInventoryComponent* Inventory)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
		Definition->ItemDefinitionId = TEXT("TD07333_ProjectionAmulet");
		Definition->DisplayName = FText::FromString(TEXT("Projection Amulet"));
		Definition->ItemType = EGridItemType::Jewelry;
		Definition->Weight = 1.0f;
		Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::Amulet);
		Definition->EquipmentStatBonus.DexterityBonus = 4;
		Definition->EquipmentStatBonus.MaxHealthBonus = 10;
		Definition->EquipmentStatBonus.MaxManaBonus = 6;
		Definition->EquipmentStatBonus.ArmorBonus = 5;
		Inventory->RegisterItemDefinition(Definition);

		FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid(7, 3, 3, 32);
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.DisplayName = Definition->DisplayName;
		Item.Quantity = 1;
		Item.Weight = Definition->Weight;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = Character.CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = EGridEquipmentSlot::Amulet;
		Inventory->PartyInventoryState.ActiveEquipment[0].Amulet = Item;
		return Definition;
	}

	bool LoadProjectSource(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333EquipmentProjectionContractTest,
	"Grimrock.TechnicalDebt.TD07_3_3_3.Characterization.EquipmentProjectionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333EquipmentProjectionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07333Characterization;

	UGridPartyInventoryComponent* Inventory = MakeParty();
	EquipProjectionItem(Inventory);
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];

	FGridInventoryCharacterSummary Summary;
	TestTrue(TEXT("Character summary resolves with equipped projection item"), Inventory->GetCharacterSummary(0, Summary));

	TestEqual(TEXT("Equipment Dexterity bonus changes projected Attributes"), Summary.Attributes.Dexterity, 16);
	TestEqual(TEXT("Equipment Dexterity bonus does not recalculate projected Initiative"), Summary.DerivedStats.Initiative, 1);
	TestEqual(TEXT("Equipment Dexterity bonus does not recalculate projected Accuracy"), Summary.DerivedStats.Accuracy, 1);
	TestEqual(TEXT("Equipment Dexterity bonus does not recalculate projected Evasion"), Summary.DerivedStats.Evasion, 1);

	TestEqual(TEXT("MaxHealth bonus changes projected maximum"), Summary.DerivedStats.MaxHealth, 30);
	TestEqual(TEXT("MaxMana bonus changes projected maximum"), Summary.DerivedStats.MaxMana, 16);
	TestEqual(TEXT("Equipment does not grant current health"), Summary.Resources.CurrentHealth, 15);
	TestEqual(TEXT("Equipment does not grant current mana"), Summary.Resources.CurrentMana, 4);
	TestEqual(TEXT("ArmorBonus changes projected physical armor"), Summary.Resources.CurrentPhysicalArmor, 7);
	TestEqual(TEXT("ArmorBonus does not change projected magical armor"), Summary.Resources.CurrentMagicalArmor, 1);

	TestEqual(TEXT("Stored Dexterity remains base state"), Character.Attributes.Dexterity, 12);
	TestEqual(TEXT("Stored Initiative remains unchanged by equipment"), Character.DerivedStats.Initiative, 1);
	TestEqual(TEXT("Stored Accuracy remains unchanged by equipment"), Character.DerivedStats.Accuracy, 1);
	TestEqual(TEXT("Stored Evasion remains unchanged by equipment"), Character.DerivedStats.Evasion, 1);
	TestEqual(TEXT("Stored MaxHealth remains unchanged by equipment"), Character.DerivedStats.MaxHealth, 20);
	TestEqual(TEXT("Stored MaxMana remains unchanged by equipment"), Character.DerivedStats.MaxMana, 10);
	TestEqual(TEXT("Stored physical armor remains unchanged by equipment"), Character.Resources.CurrentPhysicalArmor, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333ResourceRemovalProjectionTest,
	"Grimrock.TechnicalDebt.TD07_3_3_3.Characterization.ResourceRemovalProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333ResourceRemovalProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07333Characterization;

	UGridPartyInventoryComponent* Inventory = MakeParty();
	EquipProjectionItem(Inventory);
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];

	// Characterize the current projection-only cap behavior. These values are
	// deliberately legal under the equipped summary but exceed the base maxima.
	Character.Resources.CurrentHealth = 28;
	Character.Resources.CurrentMana = 12;

	FGridInventoryCharacterSummary EquippedSummary;
	TestTrue(TEXT("Equipped summary resolves"), Inventory->GetCharacterSummary(0, EquippedSummary));
	TestEqual(TEXT("Equipped projected health accepts the equipment-supported value"), EquippedSummary.Resources.CurrentHealth, 28);
	TestEqual(TEXT("Equipped projected mana accepts the equipment-supported value"), EquippedSummary.Resources.CurrentMana, 12);

	TestTrue(TEXT("Projection item can be unequipped"), Inventory->UnequipItemToInventory(0, EGridEquipmentSlot::Amulet));

	FGridInventoryCharacterSummary UnequippedSummary;
	TestTrue(TEXT("Unequipped summary resolves"), Inventory->GetCharacterSummary(0, UnequippedSummary));
	TestEqual(TEXT("Unequip returns projected MaxHealth to base"), UnequippedSummary.DerivedStats.MaxHealth, 20);
	TestEqual(TEXT("Unequip clamps projected CurrentHealth to base maximum"), UnequippedSummary.Resources.CurrentHealth, 20);
	TestEqual(TEXT("Unequip returns projected MaxMana to base"), UnequippedSummary.DerivedStats.MaxMana, 10);
	TestEqual(TEXT("Unequip clamps projected CurrentMana to base maximum"), UnequippedSummary.Resources.CurrentMana, 10);

	TestEqual(TEXT("Unequip does not normalize stored CurrentHealth"), Character.Resources.CurrentHealth, 28);
	TestEqual(TEXT("Unequip does not normalize stored CurrentMana"), Character.Resources.CurrentMana, 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333CombatConsumerBoundaryTest,
	"Grimrock.TechnicalDebt.TD07_3_3_3.Characterization.CombatConsumerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333CombatConsumerBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07333Characterization;

	FString MonsterCombat;
	TestTrue(TEXT("Monster combat source loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterCombatComponent.cpp"), MonsterCombat));
	TestTrue(TEXT("Incoming attacks currently read stored Evasion directly"),
		MonsterCombat.Contains(TEXT("Target.Evasion = Character.DerivedStats.Evasion;")));
	TestTrue(TEXT("Incoming attacks currently read stored physical armor directly"),
		MonsterCombat.Contains(TEXT("Target.PhysicalArmor = Character.Resources.CurrentPhysicalArmor;")));
	TestTrue(TEXT("Incoming attacks currently read stored magical armor directly"),
		MonsterCombat.Contains(TEXT("Target.MagicalArmor = Character.Resources.CurrentMagicalArmor;")));
	TestTrue(TEXT("Incoming attacks currently mutate stored physical armor directly"),
		MonsterCombat.Contains(TEXT("Character.Resources.CurrentPhysicalArmor = FMath::Max(0, Character.Resources.CurrentPhysicalArmor - OutResult.PhysicalArmorDamage);")));
	TestTrue(TEXT("Incoming attacks currently mutate stored current health directly"),
		MonsterCombat.Contains(TEXT("Character.Resources.CurrentHealth = FMath::Max(0, Character.Resources.CurrentHealth - OutResult.HealthDamage);")));

	FString Initiative;
	TestTrue(TEXT("Initiative source loads"),
		LoadProjectSource(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerInitiative.cpp"), Initiative));
	TestTrue(TEXT("Party initiative remains a calculated DerivedStats consumer"),
		Initiative.Contains(TEXT("Entry.InitiativeBase = 10 + Character.DerivedStats.Initiative;")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07333MixedFactoryContractTest,
	"Grimrock.TechnicalDebt.TD07_3_3_3.Characterization.MixedFactoryContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07333MixedFactoryContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>();
	ClassDefinition->ClassId = TEXT("TD07333_Fighter");
	ClassDefinition->DisplayName = FText::FromString(TEXT("TD07333 Fighter"));
	ClassDefinition->HealthAtLevelOne = 20;
	ClassDefinition->HealthPerLevel = 5;
	ClassDefinition->ManaAtLevelOne = 8;
	ClassDefinition->ManaPerLevel = 2;
	ClassDefinition->BasePhysicalArmor = 3;
	ClassDefinition->BaseMagicalArmor = 2;

	const FRPGAttributes Attributes(10, 14, 12, 10, 10, 10);
	const FRPGDerivedStats Stats = URPGCharacterRulesLibrary::CalculateDerivedStats(Attributes, ClassDefinition, 1);
	const FRPGCharacterResources Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Stats, ClassDefinition);

	TestEqual(TEXT("Factory calculates MaxHealth"), Stats.MaxHealth, 21);
	TestEqual(TEXT("Resource initializer starts CurrentHealth at MaxHealth"), Resources.CurrentHealth, Stats.MaxHealth);
	TestEqual(TEXT("Factory calculates MaxMana"), Stats.MaxMana, 8);
	TestEqual(TEXT("Resource initializer starts CurrentMana at MaxMana"), Resources.CurrentMana, Stats.MaxMana);
	TestEqual(TEXT("Resource initializer starts physical armor from class base"), Resources.CurrentPhysicalArmor, 3);
	TestEqual(TEXT("Resource initializer starts magical armor from class base"), Resources.CurrentMagicalArmor, 2);
	TestEqual(TEXT("Factory calculates Initiative from Dexterity"), Stats.Initiative, 2);
	TestEqual(TEXT("Factory calculates Accuracy from Dexterity"), Stats.Accuracy, 2);
	TestEqual(TEXT("Factory calculates Evasion from Dexterity"), Stats.Evasion, 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
