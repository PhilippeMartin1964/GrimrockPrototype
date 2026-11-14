#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridItemWeightReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063NWorldWeight,
	"Grimrock.RPG.ARM.CLEAN06_3.WorldAndReceptacleWeightParity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063NWorldWeight::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real staged ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Registry resolved for both world readers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	const FName Ids[] = {
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Resolved;
			if (!TestTrue(TEXT("Production level exists"), Registry->ResolveItem(Id, Level, Resolved)))
			{
				return false;
			}
			FGridItemInstance Placed;
			Placed.RuntimeObjectId = FGuid::NewGuid();
			Placed.ItemDefinitionId = Id;
			Placed.ItemLevel = Level;
			Placed.Quantity = 3;
			Placed.Weight = 12345.0f; // Conflicting old world-item definition.
			FGridItemInstance Contained = Placed;
			Contained.Weight = -12345.0f; // Conflicting receptacle snapshot.
			TestTrue(TEXT("Placed pickup and contained item must read identical canonical mass"),
				FMath::IsNearlyEqual(
					FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Placed),
					FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Contained)));
			TestTrue(TEXT("Pressure plates now use same quantity-weight contract as carried inventory"),
				FMath::IsNearlyEqual(FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Placed),
					3.0f * Resolved.WeightKg));
			Placed.ItemLevel = 0;
			TestEqual(TEXT("Known invalid level never adds legacy mass to a pressure plate"),
				FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Placed), 0.0f);
		}
	}
	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = TEXT("LegacyWorld_063N");
	Legacy.ItemLevel = 0;
	Legacy.Weight = 2.25f;
	Legacy.Quantity = 4;
	TestEqual(TEXT("Legacy world items still contribute copied weight"),
		FGridItemWeightReadModel::GetTotalWeight(Registry.Get(), Legacy), 9.0f);
	return true;
}

#endif
