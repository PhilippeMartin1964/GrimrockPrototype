#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridWorldItemLevelReadModel.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AJWorldItemLevels,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldItemLevelBySource",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AJWorldItemLevels::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Canonical ItemSystem registered"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	FName FixtureId;
	const FName Arms[] = {
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01"),
		TEXT("Weapon_ThrowingKnife_01") };
	for (const FName Id : Arms)
	{
		FixtureId = Id;
		for (const int32 Level : {1, 20})
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real N1/N20 ARM profile exists"),
				Registry->ResolveItem(Id, Level, Canonical))) return false;
			const auto Placed = FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
				Registry.Get(), Id, Level);
			const auto Dropped = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
				Registry.Get(), Id, Level);
			TestTrue(TEXT("Authored fixed level accepted"), Placed.bRegisteredIdentity && Placed.bResolved);
			TestTrue(TEXT("Persisted fixed level accepted"), Dropped.bRegisteredIdentity && Dropped.bResolved);
			TestEqual(TEXT("Authoring keeps N-level"), Placed.FixedItemLevel, Level);
			TestEqual(TEXT("World drop keeps N-level"), Dropped.FixedItemLevel, Level);
			TestTrue(TEXT("Canonical world unit mass"), FMath::IsNearlyEqual(Dropped.UnitWeightKg, Canonical.WeightKg));
			TestEqual(TEXT("World item name canonical"), Placed.DisplayName.ToString(),
				Canonical.DisplayName.ToString());
		}
		const auto Initial = FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
			Registry.Get(), Id, 0);
		TestTrue(TEXT("Malformed placement still classified canonical"), Initial.bRegisteredIdentity);
		TestFalse(TEXT("New authored ARM N0 cannot inherit obsolete N7"), Initial.bResolved);
		TestEqual(TEXT("Rejected placement has no fallback mass"), Initial.UnitWeightKg, 0.0f);
		const auto PersistedZero = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), Id, 0);
		TestTrue(TEXT("Persisted invalid N0 remains known canonical"), PersistedZero.bRegisteredIdentity);
		TestFalse(TEXT("Existing ARM N0 never reinherits on drop"), PersistedZero.bResolved);
		TestEqual(TEXT("Rejected instance has no fallback metadata"), PersistedZero.UnitWeightKg, 0.0f);
		TestFalse(TEXT("Authored N0 without definition cannot invent default"),
			FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
				Registry.Get(), Id, 0).bResolved);
		TestTrue(TEXT("Id-only fixed N20 uses canonical"), 
			FGridWorldItemLevelReadModel::ResolvePersistedInstance(
				Registry.Get(), Id, 20).bResolved);
	}
	FixtureId = TEXT("Weapon_RoundShield_01");
	TestFalse(TEXT("Invalid authored default N0 rejected, no forced N1"),
		FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
			Registry.Get(), FixtureId, 0).bResolved);
	TestFalse(TEXT("Known ARM invalid N21 has no legacy fallback"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), FixtureId, 21).bResolved);
	// A registered ItemId does not borrow the obsolete definition ID.
	FGridResolvedItem RealGreatAxe;
	if (!TestTrue(TEXT("Real N7 great axe in canonical catalogue"),
		Registry->ResolveItem(TEXT("Weapon_GreatAxe_01"), 7, RealGreatAxe))) return false;
	const auto RegisteredMismatch = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
		Registry.Get(), TEXT("Weapon_GreatAxe_01"), 7);
	TestTrue(TEXT("World registered N7 cannot be rejected by stale pointer"),
		RegisteredMismatch.bRegisteredIdentity && RegisteredMismatch.bResolved &&
		RegisteredMismatch.FixedItemLevel == 7 &&
		FMath::IsNearlyEqual(RegisteredMismatch.UnitWeightKg, RealGreatAxe.WeightKg));
	TestEqual(TEXT("World registered name remains canonical"),
		RegisteredMismatch.DisplayName.ToString(), RealGreatAxe.DisplayName.ToString());
	TestFalse(TEXT("Unregistered mismatched source still rejected"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), TEXT("AJ_UnregisteredOther"), 0).bResolved);

	FixtureId = TEXT("Item_Arrows_01");
	FGridResolvedItem Arrows;
	if (!TestTrue(TEXT("Real unlevelled arrows N0 resolve"),
		Registry->ResolveItem(FixtureId, 0, Arrows))) return false;
	const auto PlacedAmmo = FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
		Registry.Get(), FixtureId, 0);
	const auto DroppedAmmo = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
		Registry.Get(), FixtureId, 0);
	TestTrue(TEXT("New arrow placement remains N0"), PlacedAmmo.bResolved && PlacedAmmo.FixedItemLevel == 0);
	TestTrue(TEXT("Existing arrow drop remains N0"), DroppedAmmo.bResolved && DroppedAmmo.FixedItemLevel == 0);
	TestTrue(TEXT("Arrow unit mass from ItemSystem"), FMath::IsNearlyEqual(DroppedAmmo.UnitWeightKg, Arrows.WeightKg));
	TestFalse(TEXT("Canonical ammo cannot be assigned N1 in placement"),
		FGridWorldItemLevelReadModel::ResolveAuthoredPlacement(
			Registry.Get(), FixtureId, 1).bResolved);
	TestFalse(TEXT("Canonical ammo cannot be assigned N20 in world drop"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), FixtureId, 20).bResolved);

	FixtureId = TEXT("LegacyWorld_AJ");
	const auto Legacy = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
		Registry.Get(), FixtureId, 0);
	TestFalse(TEXT("Configured registry refuses unknown physical world item"), Legacy.bResolved);
	TestFalse(TEXT("Nonregistered item is not labelled canonical"), Legacy.bRegisteredIdentity);
	TestEqual(TEXT("Rejected world item exposes no legacy weight"), Legacy.UnitWeightKg, 0.f);
	const auto FixtureOnly = FGridWorldItemLevelReadModel::ResolvePersistedInstance(
		nullptr, FixtureId, 0);
	TestTrue(TEXT("Registry-less placement cannot resolve weight"),
		!FixtureOnly.bResolved && !FixtureOnly.bRegisteredIdentity && FixtureOnly.UnitWeightKg == 0.f);
	TestFalse(TEXT("Unknown non-levelled definition does not invent N9"),
		FGridWorldItemLevelReadModel::ResolvePersistedInstance(
			Registry.Get(), FixtureId, -1).bResolved);
	return true;
}

#endif
