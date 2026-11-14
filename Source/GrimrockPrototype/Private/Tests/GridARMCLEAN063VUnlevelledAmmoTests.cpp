#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridItemWeightReadModel.h"
#include "../Runtime/GridCanonicalItemMeshReadModel.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/GridWeaponVisualUiResolver.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063VUnlevelledAmmo,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalUnlevelledAmmunitionN0",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063VUnlevelledAmmo::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real persisted ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real canonical registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FName ArrowsId(TEXT("Item_Arrows_01"));
	if (!TestTrue(TEXT("Production arrows have a canonical ItemId"), Registry->HasCanonicalItem(ArrowsId)))
		return false;

	FGridResolvedItem N0, N1;
	if (!TestTrue(TEXT("Real arrows resolve as unlevelled item at N0"),
		Registry->ResolveItem(ArrowsId, 0, N0))) return false;
	if (!TestTrue(TEXT("Pre-existing N1 ammo resolutions remain supported"),
		Registry->ResolveItem(ArrowsId, 1, N1))) return false;
	TestFalse(TEXT("Unlevelled arrows do not invent an ARM archetype"), N0.bIsArmament);
	TestEqual(TEXT("Arrows preserve zero item level"), N0.ItemLevel, 0);
	TestEqual(TEXT("Arrows keep a single canonical name"),
		N0.DisplayName.ToString(), N1.DisplayName.ToString());
	TestEqual(TEXT("Arrows retain one authored WeightKg"), N0.WeightKg, N1.WeightKg);
	TestEqual(TEXT("Arrows retain one canonical icon"),
		N0.ItemIcon.ToSoftObjectPath().ToString(), N1.ItemIcon.ToSoftObjectPath().ToString());
	TestEqual(TEXT("Arrows retain one canonical world mesh"),
		N0.WorldMesh.ToSoftObjectPath().ToString(), N1.WorldMesh.ToSoftObjectPath().ToString());

	FGridItemInstance Stack;
	Stack.RuntimeObjectId = FGuid::NewGuid();
	Stack.ItemDefinitionId = ArrowsId;
	Stack.ItemLevel = 0;
	Stack.Quantity = 12;
	Stack.Weight = 99999.0f;
	float UnitMass = 0.0f;
	TestTrue(TEXT("Unlevelled arrow stack resolves weight"),
		FGridItemWeightReadModel::TryResolveUnitWeight(Registry.Get(), Stack, UnitMass));
	TestTrue(TEXT("Arrow unit mass ignores conflicting old snapshot"),
		FMath::IsNearlyEqual(UnitMass, N0.WeightKg));
	TestTrue(TEXT("Stack quantity still multiplies canonical unit mass"),
		FMath::IsNearlyEqual(FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Stack),
			12.0f * N0.WeightKg));
	const FGridInventoryItemMetadata Meta =
		FGridInventoryItemMetadataReadModel::Resolve(Registry.Get(), nullptr, Stack);
	TestTrue(TEXT("Arrow tooltip remains canonical"), Meta.bCanonicalIdentity && Meta.bResolved);
	TestTrue(TEXT("Arrow tooltip uses authored mass"),
		FMath::IsNearlyEqual(Meta.UnitWeightKg, N0.WeightKg));
	const FGridResolvedInventoryItemIcon Icon =
		FGridWeaponVisualUiResolver::ResolveInventoryItemIcon(Registry.Get(), nullptr, Stack);
	TestTrue(TEXT("Arrow icon no longer falls back to old DA"), Icon.bCanonicalIdentity);
	TestEqual(TEXT("Arrow icon from sole canonical source"),
		Icon.Icon.ToSoftObjectPath().ToString(), N0.ItemIcon.ToSoftObjectPath().ToString());
	const FGridCanonicalItemMeshReadView Mesh =
		FGridCanonicalItemMeshReadModel::Resolve(Registry.Get(), Stack);
	TestTrue(TEXT("Arrow world mesh resolves for recovered N0 pickup"), Mesh.bResolved && Mesh.bCanonicalIdentity);
	TestEqual(TEXT("Arrow world mesh from sole canonical source"),
		Mesh.WorldMesh.ToSoftObjectPath().ToString(), N0.WorldMesh.ToSoftObjectPath().ToString());

	FGridResolvedItem Invalid;
	TestFalse(TEXT("Negative levels still rejected"), Registry->ResolveItem(ArrowsId, -1, Invalid));
	TestEqual(TEXT("Negative level reason"), Invalid.Error, EGridCanonicalItemResolveError::InvalidItemLevel);
	TestFalse(TEXT("Unlevelled item above N20 still rejected"), Registry->ResolveItem(ArrowsId, 21, Invalid));
	TestEqual(TEXT("Above N20 reason"), Invalid.Error, EGridCanonicalItemResolveError::InvalidItemLevel);
	TestFalse(TEXT("ARM014 N0 must still fail closed"),
		Registry->ResolveItem(TEXT("Weapon_StraightSword_01"), 0, Invalid));
	TestEqual(TEXT("Weapons cannot silently become N1"),
		Invalid.Error, EGridCanonicalItemResolveError::InvalidItemLevel);
	return true;
}

#endif
