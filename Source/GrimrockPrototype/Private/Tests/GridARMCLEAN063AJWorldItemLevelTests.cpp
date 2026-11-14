#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridWorldItemLevelReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AJWorldItemLevels,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldItemLevelBySource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AJWorldItemLevels::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Canonical ItemSystem registered"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
	Old->DefaultWeaponItemLevel = 7;
	Old->DisplayName = FText::FromString(TEXT("OUTDATED WORLD ITEM"));
	Old->Weight = 4321.0f;
	Old->bUseWeaponCatalogue = false; // Intentionally disagree with registry.
	const FName Arms[] = {
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01"),
		TEXT("Weapon_ThrowingKnife_01") };
	for (const FName Id : Arms)
	{
		Old->ItemDefinitionId = Id;
		for (const int32 Level : {1, 20})
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real N1/N20 ARM profile exists"),
				Registry->ResolveItem(Id, Level, Canonical))) return false;
			const auto Placed = FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
				Registry.Get(), Old.Get(), Id, Level);
			const auto Dropped = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
				Registry.Get(), Old.Get(), Id, Level);
			TestTrue(TEXT("Authored fixed level accepted"), Placed.bCanonicalIdentity && Placed.bResolved);
			TestTrue(TEXT("Persisted fixed level accepted"), Dropped.bCanonicalIdentity && Dropped.bResolved);
			TestEqual(TEXT("Authoring keeps N-level"), Placed.FixedItemLevel, Level);
			TestEqual(TEXT("World drop keeps N-level"), Dropped.FixedItemLevel, Level);
			TestTrue(TEXT("Canonical world unit mass"), FMath::IsNearlyEqual(Dropped.UnitWeightKg, Canonical.WeightKg));
			TestEqual(TEXT("World item name canonical"), Placed.DisplayName.ToString(),
				Canonical.DisplayName.ToString());
		}
		FGridResolvedItem Default;
		if (!TestTrue(TEXT("Real canonical N7 exists"), Registry->ResolveItem(Id, 7, Default)))
			return false;
		const auto Initial = FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
			Registry.Get(), Old.Get(), Id, 0);
		TestTrue(TEXT("New authored ARM inherits once"), Initial.bResolved);
		TestEqual(TEXT("New authored ARM inherits N7 not N1"), Initial.FixedItemLevel, 7);
		TestTrue(TEXT("Inherited world item mass canonical"), FMath::IsNearlyEqual(Initial.UnitWeightKg, Default.WeightKg));
		const auto PersistedZero = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), Old.Get(), Id, 0);
		TestTrue(TEXT("Persisted invalid N0 remains known canonical"), PersistedZero.bCanonicalIdentity);
		TestFalse(TEXT("Existing ARM N0 never reinherits on drop"), PersistedZero.bResolved);
		TestEqual(TEXT("Rejected instance has no fallback metadata"), PersistedZero.UnitWeightKg, 0.0f);
		TestFalse(TEXT("Authored N0 without definition cannot invent default"),
			FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
				Registry.Get(), nullptr, Id, 0).bResolved);
		TestTrue(TEXT("Id-only fixed N20 uses canonical"), 
			FGridWorldItemLevelReadModel::ResolvePersistedInstance(
				Registry.Get(), nullptr, Id, 20).bResolved);
	}
	Old->ItemDefinitionId = TEXT("Weapon_RoundShield_01");
	Old->DefaultWeaponItemLevel = 0;
	TestFalse(TEXT("Invalid authored default N0 rejected, no forced N1"),
		FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
			Registry.Get(), Old.Get(), Old->ItemDefinitionId, 0).bResolved);
	Old->DefaultWeaponItemLevel = 7;
	TestFalse(TEXT("Known ARM invalid N21 has no legacy fallback"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), Old.Get(), Old->ItemDefinitionId, 21).bResolved);
	TestFalse(TEXT("Mismatched item definition ID rejected"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), Old.Get(), TEXT("Weapon_GreatAxe_01"), 7).bResolved);

	Old->ItemDefinitionId = TEXT("Item_Arrows_01");
	Old->bUseWeaponCatalogue = true; // Misleading historic flag must be ignored.
	FGridResolvedItem Arrows;
	if (!TestTrue(TEXT("Real unlevelled arrows N0 resolve"),
		Registry->ResolveItem(Old->ItemDefinitionId, 0, Arrows))) return false;
	const auto PlacedAmmo = FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
		Registry.Get(), Old.Get(), Old->ItemDefinitionId, 0);
	const auto DroppedAmmo = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
		Registry.Get(), Old.Get(), Old->ItemDefinitionId, 0);
	TestTrue(TEXT("New arrow placement remains N0"), PlacedAmmo.bResolved && PlacedAmmo.FixedItemLevel == 0);
	TestTrue(TEXT("Existing arrow drop remains N0"), DroppedAmmo.bResolved && DroppedAmmo.FixedItemLevel == 0);
	TestTrue(TEXT("Arrow unit mass from ItemSystem"), FMath::IsNearlyEqual(DroppedAmmo.UnitWeightKg, Arrows.WeightKg));
	TestFalse(TEXT("Canonical ammo cannot be assigned N1 in placement"),
		FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
			Registry.Get(), Old.Get(), Old->ItemDefinitionId, 1).bResolved);
	TestFalse(TEXT("Canonical ammo cannot be assigned N20 in world drop"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), Old.Get(), Old->ItemDefinitionId, 20).bResolved);

	Old->ItemDefinitionId = TEXT("LegacyWorld_AJ");
	Old->bUseWeaponCatalogue = false;
	Old->Weight = 2.5f;
	const auto Legacy = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
		Registry.Get(), Old.Get(), Old->ItemDefinitionId, 0);
	TestTrue(TEXT("Nonregistered legacy item still accepted"), Legacy.bResolved);
	TestFalse(TEXT("Nonregistered item is not labelled canonical"), Legacy.bCanonicalIdentity);
	TestEqual(TEXT("Legacy world weight retained"), Legacy.UnitWeightKg, 2.5f);
	TestFalse(TEXT("Unknown non-levelled definition does not invent N9"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), Old.Get(), Old->ItemDefinitionId, -1).bResolved);
	TestEqual(TEXT("Default authoring level never mutated"), Old->DefaultWeaponItemLevel, 7);
	return true;
}

#endif
