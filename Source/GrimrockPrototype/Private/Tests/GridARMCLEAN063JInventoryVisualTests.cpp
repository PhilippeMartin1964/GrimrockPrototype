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
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
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
					Registry.Get(), ConflictingLegacy.Get(), Instance);
			TestTrue(TEXT("Registered ARM cannot consult legacy icon"), Icon.bCanonicalIdentity);
			TestEqual(TEXT("Visible item icon uses canonical WeaponVisualCatalogue"),
				Icon.Icon.ToSoftObjectPath().ToString(), Canonical.ItemIcon.ToSoftObjectPath().ToString());
			TestEqual(TEXT("Inventory tooltip visual identity uses canonical VisualId"),
				Icon.VisualId, Canonical.VisualId);
			TestTrue(TEXT("Canonical profile does not use the conflicting legacy icon"),
				Icon.Icon.ToSoftObjectPath() != ConflictingLegacy->Icon.ToSoftObjectPath());

			Instance.ItemLevel = 0;
			const FGridResolvedInventoryItemIcon Invalid =
				FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
					Registry.Get(), ConflictingLegacy.Get(), Instance);
			TestTrue(TEXT("N0 retains known canonical identity"), Invalid.bCanonicalIdentity);
			TestTrue(TEXT("N0 cannot fall back to old icon"), Invalid.Icon.IsNull());
			TestTrue(TEXT("N0 visual identity is reset"), Invalid.VisualId.IsNone());
		}
	}

	// Item IDs outside the registered system retain their authored icon during 06.3.
	ConflictingLegacy->ItemDefinitionId = TEXT("LegacyOther_063J");
	FGridItemInstance LegacyInstance;
	LegacyInstance.ItemDefinitionId = ConflictingLegacy->ItemDefinitionId;
	LegacyInstance.RuntimeObjectId = FGuid::NewGuid();
	LegacyInstance.ItemLevel = 1;
	LegacyInstance.Quantity = 1;
	const FGridResolvedInventoryItemIcon Legacy =
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
			Registry.Get(), ConflictingLegacy.Get(), LegacyInstance);
	TestFalse(TEXT("Unknown ID uses temporary old presentation"), Legacy.bCanonicalIdentity);
	TestEqual(TEXT("Unknown item keeps existing icon"),
		Legacy.Icon.ToSoftObjectPath().ToString(), ConflictingLegacy->Icon.ToSoftObjectPath().ToString());
	const FGridResolvedInventoryItemIcon Missing =
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(
			Registry.Get(), nullptr, LegacyInstance);
	TestTrue(TEXT("Missing legacy definition produces no fabricated icon"), Missing.Icon.IsNull());
	return true;
}

#endif
