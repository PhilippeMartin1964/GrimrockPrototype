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
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real registry registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	const FName Ids[] = {TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
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
			const bool bExpectedOverride = Canonical.bUseItemWeightAsWorldPhysicsMass &&
				FMath::IsFinite(Canonical.WeightKg) && Canonical.WeightKg > KINDA_SMALL_NUMBER;
			const float ExpectedKg = bExpectedOverride ? Canonical.WeightKg : 0.0f;
			float Kg = 900.0f;
			TestEqual(TEXT("No historic asset required for canonical physics"),
				FGridWorldPhysicsMassReadModel::TryResolve(Registry.Get(), Item, Kg),
				bExpectedOverride);
			TestEqual(TEXT("Canonical unit mass"), Kg, ExpectedKg);
			Item.ItemLevel = 0;
			Kg = 900.0f;
			TestFalse(TEXT("Registered N0 cannot fall back to old weight"),
				FGridWorldPhysicsMassReadModel::TryResolve(Registry.Get(), Item, Kg));
			TestEqual(TEXT("Rejected override resets output"), Kg, 0.0f);
		}
	}
	FGridItemInstance Other;
	Other.RuntimeObjectId = FGuid::NewGuid();
	Other.ItemDefinitionId = TEXT("LegacyPhysicsMass_063Q");
	Other.ItemLevel = 0;
	float Kg = 0;
	TestFalse(TEXT("Configured registry refuses unknown physical identity"),
		FGridWorldPhysicsMassReadModel::TryResolve(Registry.Get(), Other, Kg));
	TestEqual(TEXT("Rejected mass is zero"), Kg, 0.0f);
	TestFalse(TEXT("A registry-less item cannot override mass"),
		FGridWorldPhysicsMassReadModel::TryResolve(nullptr, Other, Kg));
	TestEqual(TEXT("Unresolved mass is zero"), Kg, 0.0f);
	TestFalse(TEXT("Opt-out fixture never overrides Chaos mass"),
		FGridWorldPhysicsMassReadModel::TryResolve(nullptr, Other, Kg));
	return true;
}

#endif
