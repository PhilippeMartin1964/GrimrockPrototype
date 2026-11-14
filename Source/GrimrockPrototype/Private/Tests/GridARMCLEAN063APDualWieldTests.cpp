#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/Combat/GridDualWieldPolicy.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/Combat/GridCanonicalEquippedAttackReadModel.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN063AP
{
	FGridItemInstance MakeItem(FName Id, int32 Level, FGuid CharacterId, EGridEquipmentSlot Slot)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Id;
		Item.ItemLevel = Level;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = Slot;
		Item.Quantity = 1;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063APDualWield,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalDualWieldIndependentLevels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063APDualWield::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Production ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real ItemSystem registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridPartyInventoryComponent> Inventory(NewObject<UGridPartyInventoryComponent>());
	FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters.AddDefaulted_GetRef();
	Character.CharacterId = FGuid::NewGuid();
	Inventory->PartyInventoryState.ActiveEquipment.AddDefaulted();
	FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
	Equipment.MainHand = ARMCLEAN063AP::MakeItem(TEXT("Weapon_Dagger_01"), 3,
		Character.CharacterId, EGridEquipmentSlot::MainHand);
	Equipment.OffHand = ARMCLEAN063AP::MakeItem(TEXT("Weapon_MainGauche_01"), 19,
		Character.CharacterId, EGridEquipmentSlot::OffHand);
	const FGuid MainId = Equipment.MainHand.RuntimeObjectId;
	const FGuid OffId = Equipment.OffHand.RuntimeObjectId;
	TestNull(TEXT("No legacy main definition is required"), Inventory->FindItemDefinition(Equipment.MainHand.ItemDefinitionId));
	TestNull(TEXT("No legacy offhand definition is required"), Inventory->FindItemDefinition(Equipment.OffHand.ItemDefinitionId));
	FGridResolvedItem MainN3, OffN19;
	if (!TestTrue(TEXT("Real ARM001 N3 profile"), Registry->ResolveItem(
			Equipment.MainHand.ItemDefinitionId, 3, MainN3)) ||
		!TestTrue(TEXT("Real ARM013 N19 profile"), Registry->ResolveItem(
			Equipment.OffHand.ItemDefinitionId, 19, OffN19))) return false;
	TestTrue(TEXT("Main is authorized attack source"),
		MainN3.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Off is authorized attack source"),
		OffN19.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
	FGridDualWieldPair Pair;
	if (!TestTrue(TEXT("Two actual canonical sources form a dual pair"),
		FGridDualWieldPolicy::TryResolve(Inventory.Get(), 0, Pair, Registry.Get()))) return false;
	TestTrue(TEXT("Both sources use canonical identity"), Pair.Main.bCanonicalIdentity && Pair.Off.bCanonicalIdentity);
	TestNull(TEXT("No old main weapon asset used"), Pair.Main.Definition);
	TestNull(TEXT("No old offhand weapon asset used"), Pair.Off.Definition);
	TestEqual(TEXT("Main physical GUID"), Pair.Main.Item.RuntimeObjectId, MainId);
	TestEqual(TEXT("Off physical GUID"), Pair.Off.Item.RuntimeObjectId, OffId);
	TestTrue(TEXT("Distinct physical items"), MainId != OffId);
	TestEqual(TEXT("Main N3 unchanged"), Pair.Main.Item.ItemLevel, 3);
	TestEqual(TEXT("Off N19 unchanged"), Pair.Off.Item.ItemLevel, 19);
	TestEqual(TEXT("Primary N3 canonical dice"), Pair.Main.Profile.AttackDefinition.DamageDice.GetMaximumDamage(),
		MainN3.Weapon.DamageDice.GetMaximumDamage());
	TestEqual(TEXT("Secondary N19 canonical dice"), Pair.Off.Profile.AttackDefinition.DamageDice.GetMaximumDamage(),
		OffN19.Weapon.DamageDice.GetMaximumDamage());
	TestTrue(TEXT("Independent attack ActionIds"), Pair.Main.Action.ActionId != Pair.Off.Action.ActionId);
	TestEqual(TEXT("Main action and profile share AttackId"), Pair.Main.Action.ActionId, Pair.Main.Profile.AttackId);
	TestEqual(TEXT("Off action and profile share AttackId"), Pair.Off.Action.ActionId, Pair.Off.Profile.AttackId);
	TestTrue(TEXT("Both transient actions valid"), Pair.Main.Action.IsValid() && Pair.Off.Action.IsValid());
	for (const FName Tag : Pair.Main.SourceTags)
		TestTrue(TEXT("Main tags belong to N3 registry"), MainN3.Tags.Contains(Tag));
	for (const FName Tag : Pair.Off.SourceTags)
		TestTrue(TEXT("Off tags belong to N19 registry"), OffN19.Tags.Contains(Tag));
	TestEqual(TEXT("Composite cost stays 3 AP"), FGridDualWieldPolicy::ActionPointCost, 3);
	TestEqual(TEXT("Offhand coefficient stays 50 percent"), FGridDualWieldPolicy::OffHandDamagePercent, 50);
	FGridCombatActionDefinition Single;
	TestTrue(TEXT("Standalone shares canonical reader"), FGridCanonicalEquippedAttackReadModel::TryResolve(
		Registry.Get(), Equipment.MainHand, EGridEquipmentSlot::MainHand, Pair.Main.Action.ActionId, Single));
	TestEqual(TEXT("Same standalone versus dual main damage"), Single.OffensiveProfile.AttackDefinition.DamageDice.GetMaximumDamage(),
		Pair.Main.Profile.AttackDefinition.DamageDice.GetMaximumDamage());

	Equipment.OffHand.ItemLevel = 0;
	TestFalse(TEXT("Persisted ARM N0 cannot inherit N1"), FGridDualWieldPolicy::TryResolve(
		Inventory.Get(), 0, Pair, Registry.Get()));
	Equipment.OffHand.ItemLevel = 19;
	Equipment.OffHand.ItemDefinitionId = TEXT("Weapon_RoundShield_01");
	TestFalse(TEXT("Passive shield cannot attack twice"), FGridDualWieldPolicy::TryResolve(
		Inventory.Get(), 0, Pair, Registry.Get()));
	Equipment.OffHand.ItemDefinitionId = TEXT("Weapon_MainGauche_01");
	Equipment.OffHand.RuntimeObjectId = MainId;
	TestFalse(TEXT("Same physical GUID cannot dual wield"), FGridDualWieldPolicy::TryResolve(
		Inventory.Get(), 0, Pair, Registry.Get()));
	Equipment.OffHand.RuntimeObjectId = OffId;
	Equipment.MainHand.ItemDefinitionId = TEXT("Weapon_GreatAxe_01");
	TestFalse(TEXT("Two-handed great axe cannot dual wield"), FGridDualWieldPolicy::TryResolve(
		Inventory.Get(), 0, Pair, Registry.Get()));
	Equipment.MainHand.ItemDefinitionId = TEXT("Weapon_Dagger_01");
	Equipment.OffHand.OwnerType = EGridItemOwnerType::CharacterInventory;
	TestFalse(TEXT("OffHand must physically own its attack"), FGridDualWieldPolicy::TryResolve(
		Inventory.Get(), 0, Pair, Registry.Get()));
	Equipment.OffHand.OwnerType = EGridItemOwnerType::EquipmentSlot;
	TestTrue(TEXT("Valid pair restored"), FGridDualWieldPolicy::TryResolve(
		Inventory.Get(), 0, Pair, Registry.Get()));
	return true;
}
#endif
