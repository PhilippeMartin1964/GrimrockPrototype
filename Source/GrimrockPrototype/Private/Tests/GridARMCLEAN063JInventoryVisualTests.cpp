#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/GridWeaponVisualUiResolver.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063JInventoryIconTest,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryItemIcons",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063JInventoryIconTest::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real staged canonical ItemSystem"), Root.Get()))
	{
		return false;
	}
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real persisted registry"),
		Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) { AddError(Error); }
		return false;
	}

	const FName IDs[] =
	{
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	const int32 Levels[] = {1, 20};
	TStrongObjectPtr<UGridItemDefinitionAsset> ConflictingLegacy(NewObject<UGridItemDefinitionAsset>());
	ConflictingLegacy->Icon = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(TEXT("/Game/Tests/ARM063J/LegacyOnly.LegacyOnly")));
	for (const FName ItemId : IDs)
	{
		ConflictingLegacy->ItemDefinitionId = ItemId;
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real authored canonical item resolves"),
				Registry->ResolveItem(ItemId, Level, Canonical)))
			{
				return false;
			}
			FGridItemInstance Instance;
			Instance.ItemDefinitionId = ItemId;
			Instance.RuntimeObjectId = FGuid::NewGuid();
			Instance.ItemLevel = Level;
			Instance.Quantity = 1;

			const FGridResolvedInventoryItemIcon Icon =
				FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
					Registry.Get(), Instance);
			TestTrue(TEXT("Registered ARM cannot consult legacy icon"), Icon.bRegisteredIdentity);
			TestEqual(TEXT("Visible item icon uses canonical WeaponVisualCatalogue"),
				Icon.Icon.ToSoftObjectPath().ToString(), Canonical.ItemIcon.ToSoftObjectPath().ToString());
			TestEqual(TEXT("Inventory tooltip visual identity uses canonical VisualId"),
				Icon.VisualId, Canonical.VisualId);
			TestTrue(TEXT("Canonical profile does not use the conflicting legacy icon"),
				Icon.Icon.ToSoftObjectPath() != ConflictingLegacy->Icon.ToSoftObjectPath());

			Instance.ItemLevel = 0;
			const FGridResolvedInventoryItemIcon Invalid =
				FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
					Registry.Get(), Instance);
			TestTrue(TEXT("N0 retains known canonical identity"), Invalid.bRegisteredIdentity);
			TestTrue(TEXT("N0 cannot fall back to old icon"), Invalid.Icon.IsNull());
			TestTrue(TEXT("N0 visual identity is reset"), Invalid.VisualId.IsNone());
		}
	}

	// H.68: once ItemSystem exists, an unknown ID cannot borrow old UI
	// fields even if an obsolete asset pointer is still in memory.
	ConflictingLegacy->ItemDefinitionId = TEXT("LegacyOther_063J");
	FGridItemInstance LegacyInstance;
	LegacyInstance.ItemDefinitionId = ConflictingLegacy->ItemDefinitionId;
	LegacyInstance.RuntimeObjectId = FGuid::NewGuid();
	LegacyInstance.ItemLevel = 1;
	LegacyInstance.Quantity = 1;
	const FGridResolvedInventoryItemIcon Unknown =
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
			Registry.Get(), LegacyInstance);
	TestFalse(TEXT("Unknown ID is not canonical"), Unknown.bRegisteredIdentity);
	TestTrue(TEXT("Configured registry fails closed on unregistered icon"), Unknown.Icon.IsNull());
	TestTrue(TEXT("Configured registry cannot fabricate a VisualId"), Unknown.VisualId.IsNone());
	const FGridResolvedInventoryItemIcon Missing =
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
			Registry.Get(), LegacyInstance);
	TestTrue(TEXT("Missing definition also produces no fabricated icon"), Missing.Icon.IsNull());

	const FGridResolvedInventoryItemIcon WorldlessFixture =
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
			nullptr, LegacyInstance);
	TestFalse(TEXT("Worldless test identity remains noncanonical"), WorldlessFixture.bRegisteredIdentity);
	TestTrue(TEXT("Worldless item has no icon authority"), WorldlessFixture.Icon.IsNull());
	return true;
}

#endif
