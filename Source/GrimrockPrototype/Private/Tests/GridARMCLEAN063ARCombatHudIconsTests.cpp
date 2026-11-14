#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../UI/GridCombatHudCanonicalActionReadModel.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN063AR
{
	static TSoftObjectPtr<UTexture2D> OldInventoryIcon()
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(
			TEXT("/Game/ARMCLEAN063AR/DeprecatedItemIcon.DeprecatedItemIcon")));
	}

	static FGridItemInstance Equipped(FName ItemId, int32 Level, EGridEquipmentSlot Hand)
	{
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemId;
		Item.ItemLevel = Level;
		Item.Quantity = 1;
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = FGuid::NewGuid();
		Item.OwnerCharacterIndex = 0;
		Item.EquipmentSlot = Hand;
		return Item;
	}

	static FGridAvailableCombatAction Attack(const FGridItemInstance& Source, FName ActionId,
		EGridCombatActionType Type = EGridCombatActionType::MeleeAttack)
	{
		FGridAvailableCombatAction Action;
		Action.SourceDefinitionId = Source.ItemDefinitionId;
		Action.SourceRuntimeId = Source.RuntimeObjectId;
		Action.SourceEquipmentSlot = Source.EquipmentSlot;
		Action.CharacterId = Source.OwnerGuid;
		Action.CharacterIndex = Source.OwnerCharacterIndex;
		Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Action.Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.Definition.ActionType = Type;
		Action.Definition.ActionId = ActionId;
		Action.Definition.Icon = OldInventoryIcon();
		return Action;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ARHudActionIcons,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalCombatHudActionIconProvenance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ARHudActionIcons::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Persisted ItemSystem loaded"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Canonical root registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	const FName SwordId(TEXT("Weapon_StraightSword_01"));
	for (const int32 Level : {1, 20})
	{
		FGridResolvedItem Canonical;
		if (!TestTrue(TEXT("Sword level resolves"), Registry->ResolveItem(SwordId, Level, Canonical)) ||
			!TestTrue(TEXT("Sword has a bound action"), !Canonical.AvailableActions.IsEmpty()))
		{
			return false;
		}
		const FGridItemInstance Source = ARMCLEAN063AR::Equipped(
			SwordId, Level, EGridEquipmentSlot::MainHand);
		FGridAvailableCombatAction Action = ARMCLEAN063AR::Attack(
			Source, Canonical.AvailableActions[0].ActionId);
		Action.Definition.DisplayName = FText::FromString(TEXT("Sword Slash"));
		const FText ActionLabel = Action.Definition.DisplayName;
		TestTrue(TEXT("Registered sword bypasses legacy enrichment"),
			FGridCombatHudCanonicalActionReadModel::Apply(
				Registry.Get(), SwordId, true, &Source, Action));
		const auto Expected = Registry->FindEquippedActionIcon(
			EGridEquippedWeaponActionIconKind::MeleeOneHand);
		TestEqual(TEXT("Only global canonical ACTION palette supplies icon"),
			Action.Definition.Icon.ToSoftObjectPath().ToString(),
			Expected.ToSoftObjectPath().ToString());
		TestFalse(TEXT("Old item icon does not contaminate the action"),
			Action.Definition.Icon.ToSoftObjectPath() == ARMCLEAN063AR::OldInventoryIcon().ToSoftObjectPath());
		TestTrue(TEXT("Existing action-specific label is retained"),
			Action.Definition.DisplayName.EqualTo(ActionLabel));

		FGridAvailableCombatAction Stale = ARMCLEAN063AR::Attack(Source,
			Canonical.AvailableActions[0].ActionId);
		FGridItemInstance WrongSource = Source;
		WrongSource.RuntimeObjectId = FGuid::NewGuid();
		TestTrue(TEXT("Known identity remains handled when GUID is stale"),
			FGridCombatHudCanonicalActionReadModel::Apply(
				Registry.Get(), SwordId, true, &WrongSource, Stale));
		TestTrue(TEXT("Stale GUID never borrows an obsolete item icon"), Stale.Definition.Icon.IsNull());

		Stale = ARMCLEAN063AR::Attack(Source, TEXT("Invented_Unbound_Action_AR"));
		TestTrue(TEXT("Unbound action cannot fall back to legacy icon"),
			FGridCombatHudCanonicalActionReadModel::Apply(
				Registry.Get(), SwordId, true, &Source, Stale));
		TestTrue(TEXT("Unbound action has no manufactured image"), Stale.Definition.Icon.IsNull());

		Stale = ARMCLEAN063AR::Attack(Source, Canonical.AvailableActions[0].ActionId);
		TestTrue(TEXT("Missing equipment still marks registered identity"),
			FGridCombatHudCanonicalActionReadModel::Apply(
				Registry.Get(), SwordId, false, nullptr, Stale));
		TestTrue(TEXT("Missing equipment does not receive a weapon icon"), Stale.Definition.Icon.IsNull());
	}

	// A composite has NO per-weapon ActionBinding, but its image is still
	// sourced from the same global ActionIcons palette, not an ItemIcon.
	const FGridItemInstance DualMain = ARMCLEAN063AR::Equipped(
		TEXT("Weapon_StraightSword_01"), 8, EGridEquipmentSlot::MainHand);
	FGridAvailableCombatAction Dual = ARMCLEAN063AR::Attack(
		DualMain, FGridDualWieldPolicy::ActionId());
	TestTrue(TEXT("Existing dual composite is handled as canonical"),
		FGridCombatHudCanonicalActionReadModel::Apply(
			Registry.Get(), DualMain.ItemDefinitionId, true, &DualMain, Dual));
	TestEqual(TEXT("Dual icon has the canonical composite category"),
		Dual.Definition.Icon.ToSoftObjectPath().ToString(),
		Registry->FindEquippedActionIcon(
			EGridEquippedWeaponActionIconKind::MeleeDualWield).ToSoftObjectPath().ToString());

	const FGridItemInstance Shield = ARMCLEAN063AR::Equipped(
		TEXT("Weapon_RoundShield_01"), 20, EGridEquipmentSlot::OffHand);
	FGridAvailableCombatAction ForgedShield = ARMCLEAN063AR::Attack(
		Shield, TEXT("ForgedShieldAttack_AR"));
	TestTrue(TEXT("Passive shield is canonical regardless of fake attack"),
		FGridCombatHudCanonicalActionReadModel::Apply(
			Registry.Get(), Shield.ItemDefinitionId, true, &Shield, ForgedShield));
	TestTrue(TEXT("Passive shield action cannot acquire old or new attack icon"),
		ForgedShield.Definition.Icon.IsNull());

	const FGridItemInstance BadLevel = ARMCLEAN063AR::Equipped(
		SwordId, 0, EGridEquipmentSlot::MainHand);
	FGridAvailableCombatAction Invalid = ARMCLEAN063AR::Attack(
		BadLevel, TEXT("FakeLevelZeroAction_AR"));
	TestTrue(TEXT("Known ARM N0 still bypasses legacy definition"),
		FGridCombatHudCanonicalActionReadModel::Apply(
			Registry.Get(), SwordId, true, &BadLevel, Invalid));
	TestTrue(TEXT("Persisted N0 never manufactures an icon"), Invalid.Definition.Icon.IsNull());

	const FGridItemInstance Unknown = ARMCLEAN063AR::Equipped(
		TEXT("ARMCLEAN063AR_LegacyWeapon"), 0, EGridEquipmentSlot::MainHand);
	FGridAvailableCombatAction Legacy = ARMCLEAN063AR::Attack(Unknown, TEXT("OldAction_AR"));
	const auto PriorIcon = Legacy.Definition.Icon.ToSoftObjectPath();
	TestFalse(TEXT("Unmigrated ID stays in its legacy presentation path"),
		FGridCombatHudCanonicalActionReadModel::Apply(
			Registry.Get(), Unknown.ItemDefinitionId, true, &Unknown, Legacy));
	TestEqual(TEXT("Unknown ID icon is preserved for legacy branch"),
		Legacy.Definition.Icon.ToSoftObjectPath().ToString(), PriorIcon.ToString());

	FGridAvailableCombatAction Quick = ARMCLEAN063AR::Attack(
		DualMain, TEXT("QuickItem_AR"));
	Quick.Definition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	TestFalse(TEXT("Quick-item action never becomes a weapon icon"),
		FGridCombatHudCanonicalActionReadModel::Apply(
			Registry.Get(), DualMain.ItemDefinitionId, true, &DualMain, Quick));
	TestEqual(TEXT("Quick-item icon remains untouched"),
		Quick.Definition.Icon.ToSoftObjectPath().ToString(),
		ARMCLEAN063AR::OldInventoryIcon().ToSoftObjectPath().ToString());
	return true;
}

#endif
