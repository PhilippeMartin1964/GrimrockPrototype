#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT015
{
	const TCHAR* const CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");

	UGridWeaponCatalogueAsset* LoadProductionCatalogue()
	{
		return LoadObject<UGridWeaponCatalogueAsset>(nullptr, CataloguePath);
	}

	FGridCombatActionDefinition MakeLegacyAttack()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("ARM015_Attack");
		Action.DisplayName = FText::FromString(TEXT("Weapon attack"));
		Action.ActionType = EGridCombatActionType::MeleeAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = 2;
		Action.RangeCells = 1;
		Action.PresentationProfileId = Action.ActionId;
		Action.OffensiveProfile.AttackId = Action.ActionId;
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Action.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 1;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 3;
		Action.OffensiveProfile.RangeCells = 1;
		return Action;
	}

	UGridItemDefinitionAsset* MakeDefinition(UGridWeaponCatalogueAsset* Catalogue, FName Archetype, bool bAttack)
	{
		UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
		Item->ItemDefinitionId = TEXT("ARM015_TestWeapon");
		Item->DisplayName = FText::FromString(TEXT("ARM015 test weapon"));
		Item->ItemType = bAttack ? EGridItemType::Weapon : EGridItemType::Shield;
		Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
		Item->bUseWeaponCatalogue = true;
		Item->WeaponArchetypeId = Archetype;
		Item->DefaultWeaponItemLevel = 6;
		Item->WeaponCatalogue = Catalogue;
		if (bAttack)
		{
			Item->CombatActions.Add(MakeLegacyAttack());
		}
		return Item;
	}

	FGridItemInstance MakeInstance(const UGridItemDefinitionAsset* Definition, int32 Level = 0)
	{
		FGridItemInstance Instance;
		Instance.RuntimeObjectId = FGuid::NewGuid();
		Instance.ItemDefinitionId = Definition->ItemDefinitionId;
		Instance.Quantity = 1;
		Instance.ItemLevel = Level;
		return Instance;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015CatalogueBindingTest,
	"Grimrock.RPG.ARMEMENT01.5.CatalogueBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT015CatalogueBindingTest::RunTest(const FString&)
{
	UGridWeaponCatalogueAsset* Catalogue = ARMEMENT015::LoadProductionCatalogue();
	if (!TestNotNull(TEXT("Production weapon catalogue loads"), Catalogue))
	{
		return false;
	}
	TestTrue(TEXT("Production catalogue is complete"), Catalogue->IsValidCatalogue());

	UGridItemDefinitionAsset* Definition = ARMEMENT015::MakeDefinition(Catalogue, TEXT("ARM001"), true);
	TestTrue(TEXT("Linked item authoring is valid"), Definition->IsValidDefinition());
	TestTrue(TEXT("Authored attack remains from CombatActions"), Definition->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
	TestEqual(TEXT("Default instance level is six"), Definition->DefaultWeaponItemLevel, 6);
	const FGridWeaponLevelProfile* Level = nullptr;
	TestTrue(TEXT("Catalogue level six is available"), Definition->TryGetWeaponCatalogueProfile(6, Level));
	return Level != nullptr;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015InstanceLevelTest,
	"Grimrock.RPG.ARMEMENT01.5.FixedLevelAndInstanceMetadata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT015InstanceLevelTest::RunTest(const FString&)
{
	UGridWeaponCatalogueAsset* Catalogue = ARMEMENT015::LoadProductionCatalogue();
	if (!TestNotNull(TEXT("Catalogue available"), Catalogue)) { return false; }
	UGridItemDefinitionAsset* Definition = ARMEMENT015::MakeDefinition(Catalogue, TEXT("ARM001"), true);
	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	if (!TestTrue(TEXT("Definition registers"), Inventory->RegisterItemDefinition(Definition))) { return false; }

	FGridItemInstance NewItem = ARMEMENT015::MakeInstance(Definition);
	const FGuid Identity = NewItem.RuntimeObjectId;
	TestTrue(TEXT("Apply profile to newly created item"), Inventory->ApplyItemDefinitionToInstance(NewItem));
	TestEqual(TEXT("Minted level uses configured default"), NewItem.ItemLevel, 6);
	const FGridWeaponLevelProfile* Profile = Catalogue->FindProfile(TEXT("ARM001"), 6);
	if (!TestNotNull(TEXT("Level 6 source profile"), Profile)) { return false; }
	TestTrue(TEXT("Catalogue weight, not generic legacy weight"), FMath::IsNearlyEqual(NewItem.Weight, Profile->WeightKg));
	TestEqual(TEXT("Identity remains unchanged"), NewItem.RuntimeObjectId, Identity);

	FGridItemInstance SavedCopy = NewItem;
	Definition->DefaultWeaponItemLevel = 2;
	TestTrue(TEXT("Rehydrating old instance succeeds"), Inventory->ApplyItemDefinitionToInstance(SavedCopy));
	TestEqual(TEXT("Saved instance retains immutable level six"), SavedCopy.ItemLevel, 6);
	TestEqual(TEXT("Saved instance retains runtime identity"), SavedCopy.RuntimeObjectId, Identity);
	TestEqual(TEXT("Copied equipment instance retains fixed level"), FGridItemInstance(SavedCopy).ItemLevel, 6);

	// The existing SaveGame stores FGridPartyInventoryState, including every
	// FGridItemInstance in inventory, equipment and cursor. ItemLevel must be
	// a serialized UPROPERTY, not a transient presentation projection.
	const FProperty* LevelProperty = FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
	TestNotNull(TEXT("Fixed ItemLevel is a reflected SaveGame structure field"), LevelProperty);
	TestTrue(TEXT("Fixed ItemLevel is not transient"),
		LevelProperty && !LevelProperty->HasAnyPropertyFlags(CPF_Transient));
	FGridPartyInventoryState SaveState;
	SaveState.ActiveEquipment.SetNum(1);
	SaveState.ActiveEquipment[0].MainHand = SavedCopy;
	SaveState.ActiveEquipment[0].MainHand.EquipmentSlot = EGridEquipmentSlot::MainHand;
	SaveState.CursorItem = SavedCopy;
	SaveState.bHasCursorItem = true;
	FGridPartyInventoryState RestoredCopy = SaveState;
	TestEqual(TEXT("Saved equipment level survives snapshot"), RestoredCopy.ActiveEquipment[0].MainHand.ItemLevel, 6);
	TestEqual(TEXT("Saved cursor level survives snapshot"), RestoredCopy.CursorItem.ItemLevel, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015CombatTest,
	"Grimrock.RPG.ARMEMENT01.5.EquippedCombatProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT015CombatTest::RunTest(const FString&)
{
	UGridWeaponCatalogueAsset* Catalogue = ARMEMENT015::LoadProductionCatalogue();
	if (!TestNotNull(TEXT("Catalogue available"), Catalogue)) { return false; }
	UGridItemDefinitionAsset* Definition = ARMEMENT015::MakeDefinition(Catalogue, TEXT("ARM001"), true);
	FGridItemInstance Instance = ARMEMENT015::MakeInstance(Definition, 7);
	FGridOffensiveEquipmentProfile Offensive;
	TestTrue(TEXT("Resolve equipped level 7 offensive profile"),
		Definition->ResolveEquippedOffensiveProfile(Instance, Definition->CombatActions[0], Offensive));
	const FGridWeaponLevelProfile* Source = Catalogue->FindProfile(TEXT("ARM001"), 7);
	if (!TestNotNull(TEXT("Level 7 available"), Source)) { return false; }
	TestEqual(TEXT("Combat action identity unchanged"), Offensive.AttackId, FName(TEXT("ARM015_Attack")));
	TestEqual(TEXT("Equipped dice enabled"), Offensive.AttackDefinition.DamageDice.bEnabled, true);
	TestEqual(TEXT("Equipped N"), Offensive.AttackDefinition.DamageDice.DiceCount, Source->DamageDice.DiceCount);
	TestEqual(TEXT("Equipped M"), Offensive.AttackDefinition.DamageDice.DiceSides, Source->DamageDice.DiceSides);
	TestEqual(TEXT("Equipped K"), Offensive.AttackDefinition.DamageDice.FlatBonus, Source->DamageDice.FlatBonus);
	TestEqual(TEXT("Min is derived"), Offensive.AttackDefinition.MinDamage, Source->DamageDice.GetMinimumDamage());
	TestEqual(TEXT("Max is derived"), Offensive.AttackDefinition.MaxDamage, Source->DamageDice.GetMaximumDamage());
	TestEqual(TEXT("Range derives from fixed-level profile"), Offensive.RangeCells, Source->RangeCells);
	TestEqual(TEXT("Scaling derives from fixed-level profile"), Offensive.DamageScalingAttribute, Source->ScalingAttribute);
	TestTrue(TEXT("Resulting definition is valid for the original combat resolver"), Offensive.AttackDefinition.IsValid());
	
	FGridItemInstance AnotherInstance = ARMEMENT015::MakeInstance(Definition, 11);
	FGridOffensiveEquipmentProfile AnotherAttack;
	TestTrue(TEXT("Another fixed-level weapon resolves"), Definition->ResolveEquippedOffensiveProfile(
		AnotherInstance, Definition->CombatActions[0], AnotherAttack));
	const FGridWeaponLevelProfile* OtherSource = Catalogue->FindProfile(TEXT("ARM001"), 11);
	if (!TestNotNull(TEXT("Level 11 available"), OtherSource)) { return false; }
	TestEqual(TEXT("Different instance uses its own K"), AnotherAttack.AttackDefinition.DamageDice.FlatBonus, OtherSource->DamageDice.FlatBonus);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015PassiveAndFailureTest,
	"Grimrock.RPG.ARMEMENT01.5.PassiveAndMissingReference",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT015PassiveAndFailureTest::RunTest(const FString&)
{
	UGridWeaponCatalogueAsset* Catalogue = ARMEMENT015::LoadProductionCatalogue();
	if (!TestNotNull(TEXT("Catalogue available"), Catalogue)) { return false; }
	UGridItemDefinitionAsset* Shield = ARMEMENT015::MakeDefinition(Catalogue, TEXT("ARM070"), false);
	TestTrue(TEXT("Passive linked item definition remains valid"), Shield->IsValidDefinition());
	TestFalse(TEXT("Passive item offers no attack"), Shield->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
	FGridOffensiveEquipmentProfile Out;
	TestFalse(TEXT("Passive item refuses fake attacks"), Shield->ResolveEquippedOffensiveProfile(
		ARMEMENT015::MakeInstance(Shield, 6), ARMEMENT015::MakeLegacyAttack(), Out));

	UGridItemDefinitionAsset* Missing = ARMEMENT015::MakeDefinition(Catalogue, TEXT("ARM099"), true);
	TestFalse(TEXT("Unknown archetype is invalid"), Missing->IsValidDefinition());
	FGridItemInstance Bad = ARMEMENT015::MakeInstance(Missing, 6);
	TestFalse(TEXT("Bad archetype refuses fallback"), Missing->ResolveEquippedOffensiveProfile(
		Bad, Missing->CombatActions[0], Out));
	TestFalse(TEXT("Rejected profile clears output"), Out.IsValid());

	UGridItemDefinitionAsset* Unset = ARMEMENT015::MakeDefinition(nullptr, TEXT("ARM001"), true);
	TestFalse(TEXT("Missing catalogue reference invalid"), Unset->IsValidDefinition());
	FGridItemInstance InvalidLevel = ARMEMENT015::MakeInstance(Shield, 21);
	TestFalse(TEXT("ItemLevel above 20 invalid"), InvalidLevel.IsValid());
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015InventoryEquipmentTest,
	"Grimrock.RPG.ARMEMENT01.5.InventoryEquipmentTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT015InventoryEquipmentTest::RunTest(const FString&)
{
	UGridWeaponCatalogueAsset* Catalogue = ARMEMENT015::LoadProductionCatalogue();
	if (!TestNotNull(TEXT("Catalogue available"), Catalogue)) { return false; }
	UGridItemDefinitionAsset* Definition = ARMEMENT015::MakeDefinition(Catalogue, TEXT("ARM001"), true);
	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	Inventory->InitializeDefaultPartyIfNeeded();

	// InitializeDefaultPartyIfNeeded already creates character zero and its
	// matching equipment state. Do not insert a second unmatched character.
	Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(40);
	if (!TestTrue(TEXT("Linked definition registers"), Inventory->RegisterItemDefinition(Definition))) { return false; }

	FGridItemInstance Loot = ARMEMENT015::MakeInstance(Definition, 9);
	if (!TestTrue(TEXT("Fixed level 9 item enters inventory"), Inventory->AddItemToCharacterInventory(0, Loot)))
	{
		return false;
	}
	const FGridItemInstance& InBag = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item;
	TestEqual(TEXT("Level in inventory remains nine"), InBag.ItemLevel, 9);
	if (!TestTrue(TEXT("Equipment accepts fixed-level profile"), Inventory->CanEquipItemToSlot(0, InBag, EGridEquipmentSlot::MainHand)))
	{
		return false;
	}
	if (!TestTrue(TEXT("Equip from inventory succeeds"), Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand)))
	{
		return false;
	}
	FGridItemInstance Equipped;
	TestTrue(TEXT("Equipped item is available"), Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, Equipped));
	TestEqual(TEXT("Equipment preserves level nine"), Equipped.ItemLevel, 9);
	TestEqual(TEXT("Equipment preserves identity"), Equipped.RuntimeObjectId, Loot.RuntimeObjectId);
	FGridOffensiveEquipmentProfile Attack;
	TestTrue(TEXT("Equipped level resolves real dice"), Definition->ResolveEquippedOffensiveProfile(
		Equipped, Definition->CombatActions[0], Attack));
	const FGridWeaponLevelProfile* LevelNine = Catalogue->FindProfile(TEXT("ARM001"), 9);
	if (!TestNotNull(TEXT("Ninth level source"), LevelNine)) { return false; }
	TestEqual(TEXT("Attack uses equipped level's flat K"), Attack.AttackDefinition.DamageDice.FlatBonus, LevelNine->DamageDice.FlatBonus);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT015LegacyTest,
	"Grimrock.RPG.ARMEMENT01.5.LegacyUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT015LegacyTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
	Definition->ItemDefinitionId = TEXT("ARM015_Legacy");
	Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
	Definition->CombatActions.Add(ARMEMENT015::MakeLegacyAttack());
	TestTrue(TEXT("Legacy definition remains valid"), Definition->IsValidDefinition());
	FGridItemInstance Instance = ARMEMENT015::MakeInstance(Definition);
	TestTrue(TEXT("Legacy ItemLevel zero is valid"), Instance.IsValid());
	FGridOffensiveEquipmentProfile Resolved;
	TestTrue(TEXT("Legacy attack resolves"), Definition->ResolveEquippedOffensiveProfile(
		Instance, Definition->CombatActions[0], Resolved));
	TestFalse(TEXT("Legacy attack stays Range mode"), Resolved.AttackDefinition.DamageDice.bEnabled);
	TestEqual(TEXT("Legacy min unchanged"), Resolved.AttackDefinition.MinDamage, 1);
	TestEqual(TEXT("Legacy max unchanged"), Resolved.AttackDefinition.MaxDamage, 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
