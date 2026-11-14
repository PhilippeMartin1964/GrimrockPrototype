#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridWorldPhysicsMassReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063QWorldMass,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPhysicsMass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063QWorldMass::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real registry registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	TStrongObjectPtr<UGridItemDefinitionAsset> Legacy(NewObject<UGridItemDefinitionAsset>());
	Legacy->Weight = 10000.0f;
	Legacy->bUseItemWeightAsWorldPhysicsMass = true;
	const FName Ids[] = {TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		Legacy->ItemDefinitionId = Id;
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real N1/N20 has unit mass"), Registry->ResolveItem(Id, Level, Canonical)))
				return false;
			FGridItemInstance Item;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemDefinitionId = Id;
			Item.ItemLevel = Level;
			Item.Quantity = 4;
			Item.Weight = 90000.0f;
			float Kg = 0.0f;
			TestTrue(TEXT("Opt-in canonical mass resolves"), FGridWorldPhysicsMassReadModel::TryResolve(
				Registry.Get(), Legacy.Get(), Item, Kg));
			TestTrue(TEXT("Chaos unit mass never copies stack mass or stale DA_Weapon weight"),
				FMath::IsNearlyEqual(Kg, Canonical.WeightKg));
			Item.ItemLevel = 0;
			Kg = 900.0f;
			TestFalse(TEXT("Registered N0 cannot fall back to old weight"),
				FGridWorldPhysicsMassReadModel::TryResolve(Registry.Get(), Legacy.Get(), Item, Kg));
			TestEqual(TEXT("Rejected override resets output"), Kg, 0.0f);
		}
	}
	Legacy->ItemDefinitionId = TEXT("LegacyPhysicsMass_063Q");
	FGridItemInstance Other;
	Other.RuntimeObjectId = FGuid::NewGuid();
	Other.ItemDefinitionId = Legacy->ItemDefinitionId;
	Other.ItemLevel = 0;
	float Kg = 0;
	TestTrue(TEXT("Unregistered opt-in mass keeps old policy"),
		FGridWorldPhysicsMassReadModel::TryResolve(Registry.Get(), Legacy.Get(), Other, Kg));
	TestEqual(TEXT("Old unit mass remains unchanged"), Kg, 10000.0f);
	Legacy->bUseItemWeightAsWorldPhysicsMass = false;
	TestFalse(TEXT("Opt-out definition never overrides Chaos mass"),
		FGridWorldPhysicsMassReadModel::TryResolve(Registry.Get(), Legacy.Get(), Other, Kg));
	return true;
}

#endif
