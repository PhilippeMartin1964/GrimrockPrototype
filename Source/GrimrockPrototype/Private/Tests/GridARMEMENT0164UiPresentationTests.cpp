#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GridWeaponVisualUiResolver.h"
#include "Engine/Texture2D.h"

namespace ARMEMENT0164
{
	const TCHAR* CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");

	UGridItemDefinitionAsset* MakeDefinition(UGridWeaponVisualCatalogueAsset* Catalogue,
		FName ItemId, FName ArchetypeId)
	{
		UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
		Definition->ItemDefinitionId = ItemId;
		Definition->ItemType = ArchetypeId == FName(TEXT("ARM075")) ? EGridItemType::Shield : EGridItemType::Weapon;
		Definition->DisplayName = FText::FromName(ItemId);
		Definition->WeaponArchetypeId = ArchetypeId;
		Definition->bUseWeaponVisualCatalogue = true;
		Definition->WeaponVisualCatalogue = Catalogue;
		return Definition;
	}

	FGridItemInstance MakeInstance(const UGridItemDefinitionAsset* Definition, int32 Level)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.ItemLevel = Level;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::CharacterInventory;
		Item.OwnerCharacterIndex = 0;
		return Item;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0164InventoryIconTest,
	"Grimrock.RPG.ARMEMENT01.6.UI.InventoryTooltipIconAndRealLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0164InventoryIconTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0164::CataloguePath);
	if (!TestNotNull(TEXT("Realistic catalogue loads"), Catalogue)) { return false; }

	UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
	UGridInventoryWidget* Owner = NewObject<UGridInventoryWidget>();
	Owner->InventoryComponent = Inventory;
	UGridInventorySlotWidget* Slot = NewObject<UGridInventorySlotWidget>();
	Slot->SetOwnerInventoryWidget(Owner);
	Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);

	struct FPilot { const TCHAR* ArchetypeId; const TCHAR* VisualId; };
	const FPilot Pilots[] =
	{
		{TEXT("ARM001"), TEXT("VIS_Dagger_01")},
		{TEXT("ARM045"), TEXT("VIS_ShortBow_01")},
		{TEXT("ARM075"), TEXT("VIS_RoundShield_01")}
	};
	for (const FPilot& Pilot : Pilots)
	{
		const FName ItemId(*FString::Printf(TEXT("ARM0164_Test_%s"), Pilot.ArchetypeId));
		UGridItemDefinitionAsset* Definition =
			ARMEMENT0164::MakeDefinition(Catalogue, ItemId, FName(Pilot.ArchetypeId));
		TestTrue(TEXT("Pilot definition registered for UI"), Inventory->RegisterItemDefinition(Definition));
		const FGridWeaponVisualProfile* Profile =
			Catalogue->FindProfileForArchetype(FName(Pilot.ArchetypeId));
		if (!TestNotNull(TEXT("Approved visual profile exists"), Profile)) { continue; }
		UTexture2D* ExpectedIcon = Profile->BaseIcon.LoadSynchronous();
		if (!TestNotNull(TEXT("Approved icon loads"), ExpectedIcon)) { continue; }

		const FGridItemInstance Instance = ARMEMENT0164::MakeInstance(Definition, 12);
		Slot->SetItem(Instance);
		TestEqual(TEXT("Inventory slot uses the mapped RGBA icon"), Slot->GetIconTexture(), ExpectedIcon);
		const FGridItemTooltipView Tooltip = Slot->GetTooltipView();
		TestTrue(TEXT("Tooltip remains valid"), Tooltip.bValid);
		TestEqual(TEXT("Tooltip and inventory use identical resolved icon"), Tooltip.Icon.Get(), ExpectedIcon);
		TestEqual(TEXT("Tooltip exposes the canonical numeric VisualId"), Tooltip.VisualId, FName(Pilot.VisualId));
		TestTrue(TEXT("Level is present only from the real instance"), Tooltip.bHasItemLevel);
		TestEqual(TEXT("Tooltip uses the exact instance level"), Tooltip.ItemLevel, 12);
		TestEqual(TEXT("Presentation never mutates instance level"), Slot->GetCachedItem().ItemLevel, 12);

		const FGridItemInstance LegacyLevel = ARMEMENT0164::MakeInstance(Definition, 0);
		Slot->SetItem(LegacyLevel);
		const FGridItemTooltipView NoLevel = Slot->GetTooltipView();
		TestFalse(TEXT("Level 0 never produces a fabricated level badge"), NoLevel.bHasItemLevel);
		TestEqual(TEXT("Level 0 projects as zero"), NoLevel.ItemLevel, 0);
		TestEqual(TEXT("One silhouette for all 20 levels and Legacy"), Slot->GetIconTexture(), ExpectedIcon);
	}
	Slot->ClearItem();
	TestNull(TEXT("Cleared slot removes resolved icon"), Slot->GetIconTexture());
	TestFalse(TEXT("Cleared slot tooltip is invalid"), Slot->GetTooltipView().bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0164HotbarIconTest,
	"Grimrock.RPG.ARMEMENT01.6.UI.HotbarCatalogueIconAndLegacySafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0164HotbarIconTest::RunTest(const FString&)
{
	UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0164::CataloguePath);
	if (!TestNotNull(TEXT("Realistic catalogue loads"), Catalogue)) { return false; }
	UGridItemDefinitionAsset* Definition =
		ARMEMENT0164::MakeDefinition(Catalogue, TEXT("ARM0164_HotbarDagger"), TEXT("ARM001"));

	TSoftObjectPtr<UTexture2D> ActionIcon = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/T_ActionPlaceholder.T_ActionPlaceholder")));
	FGridWeaponVisualUiResolver::ApplyCatalogueIcon(Definition, nullptr, ActionIcon);
	const FGridWeaponVisualProfile* Profile = Catalogue->FindProfileForArchetype(TEXT("ARM001"));
	if (!TestNotNull(TEXT("Dagger profile exists"), Profile)) { return false; }
	TestEqual(TEXT("Equipment action uses dagger silhouette"), ActionIcon.ToSoftObjectPath(),
		Profile->BaseIcon.ToSoftObjectPath());

	// A passive shield must never change combat actions: this method only reads an icon.
	UGridItemDefinitionAsset* Shield =
		ARMEMENT0164::MakeDefinition(Catalogue, TEXT("ARM0164_Shield"), TEXT("ARM075"));
	TestFalse(TEXT("Shield visual opt-in does not enable RPG catalogue"), Shield->bUseWeaponCatalogue);
	TestTrue(TEXT("Shield visual opt-in adds no combat actions"), Shield->CombatActions.IsEmpty());

	// The system PrimaryAttack slot has no item source when unarmed.
	const TSoftObjectPtr<UTexture2D> UnarmedIcon = ActionIcon;
	FGridWeaponVisualUiResolver::ApplyCatalogueIcon(nullptr, nullptr, ActionIcon);
	TestEqual(TEXT("Unarmed action icon is never overwritten"), ActionIcon.ToSoftObjectPath(),
		UnarmedIcon.ToSoftObjectPath());

	Definition->bUseWeaponVisualCatalogue = false;
	Definition->Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
		TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/T_LegacyTest.T_LegacyTest")));
	ActionIcon = UnarmedIcon;
	FGridWeaponVisualUiResolver::ApplyCatalogueIcon(Definition, nullptr, ActionIcon);
	TestEqual(TEXT("Legacy action-specific icon remains untouched"), ActionIcon.ToSoftObjectPath(),
		UnarmedIcon.ToSoftObjectPath());

	Definition->bUseWeaponVisualCatalogue = true;
	Definition->WeaponArchetypeId = TEXT("ARM082"); // unmapped in this three-asset pilot
	FGridWeaponVisualUiResolver::ApplyCatalogueIcon(Definition, nullptr, ActionIcon);
	TestEqual(TEXT("Missing mapping cannot steal another weapon icon"), ActionIcon.ToSoftObjectPath(),
		UnarmedIcon.ToSoftObjectPath());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
