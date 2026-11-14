#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridItemWeightReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063IWeightRuntime,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalCarryWeightReadModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063IWeightRuntime::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Authored canonical ItemSystem"), Root.Get()))
	{
		return false;
	}
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real ItemSystem registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) { AddError(Error); }
		return false;
	}

	const FName ArmIds[] =
	{
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName ItemId : ArmIds)
	{
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Source;
			if (!TestTrue(TEXT("Real ARM resolves at N1 and N20"),
				Registry->ResolveItem(ItemId, Level, Source))) return false;
			FGridItemInstance Item;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemDefinitionId = ItemId;
			Item.ItemLevel = Level;
			Item.Quantity = 3;
			Item.Weight = 99999.0f; // Deliberately corrupt legacy snapshot.
			float UnitMass = 0.0f;
			TestTrue(TEXT("Canonical item mass resolves despite stale snapshot"),
				FGridItemWeightReadModel::TryResolveUnitWeight(Registry.Get(), Item, UnitMass));
			TestTrue(TEXT("Unit mass belongs to the canonical definition"),
				FMath::IsNearlyEqual(UnitMass, Source.WeightKg));
			TestTrue(TEXT("Stack multiplies canonical unit mass, not stale snapshot"),
				FMath::IsNearlyEqual(FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Item),
				3.0f * Source.WeightKg));
			Item.ItemLevel = 0;
			UnitMass = 42.0f;
			TestFalse(TEXT("Canonical N0 fails closed without legacy weight fallback"),
				FGridItemWeightReadModel::TryResolveUnitWeight(Registry.Get(), Item, UnitMass));
			TestEqual(TEXT("Failed mass read resets its output"), UnitMass, 0.0f);
			TestEqual(TEXT("Invalid canonical item contributes no mass from copied snapshot"),
				FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Item), 0.0f);
		}
	}

	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = TEXT("NonCanonical_063I");
	Legacy.Quantity = 4;
	Legacy.Weight = 1.25f;
	float UnitMass = 0.0f;
	TestTrue(TEXT("Unregistered identity keeps temporary legacy unit mass"),
		FGridItemWeightReadModel::TryResolveUnitWeight(Registry.Get(), Legacy, UnitMass));
	TestEqual(TEXT("Legacy unit mass unchanged"), UnitMass, 1.25f);
	TestEqual(TEXT("Legacy quantity unchanged"), FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Legacy), 5.0f);
	TestFalse(TEXT("Invalid empty instance cannot contribute weight"),
		FGridItemWeightReadModel::TryResolveUnitWeight(Registry.Get(), FGridItemInstance(), UnitMass));
	TestEqual(TEXT("Failed invalid instance resets unit weight"), UnitMass, 0.0f);
	return true;
}

#endif
