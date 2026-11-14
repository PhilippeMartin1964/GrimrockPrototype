#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridPhysicalThrowStrengthReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063UPhysicalThrowStrength,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalPhysicalThrowStrengthGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063UPhysicalThrowStrength::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Persisted canonical ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real ItemSystem registration"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
	Old->bUseWeaponCatalogue = true;
	Old->Weight = 100000.0f; // Historical copy contradicts canonical data.
	Old->ThrowSpeed = 1500.0f;
	const FName Ids[] = {TEXT("Weapon_ThrowingKnife_01"), TEXT("Weapon_Javelin_01"),
		TEXT("Weapon_Dart_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		Old->ItemDefinitionId = Id;
		for (const int32 Level : Levels)
		{
			FGridItemInstance Item;
			Item.ItemDefinitionId = Id;
			Item.ItemLevel = Level;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.Quantity = 1;
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real thrown ARM N1/N20 exists"), Registry->ResolveItem(Id, Level, Canonical)))
				return false;
			const FGridPhysicalThrowStrengthReadView Gate =
				FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Get(), Old.Get(), Item, 100);
			TestTrue(TEXT("Registered ARM StrengthGate identity"), Gate.bCanonicalIdentity);
			TestTrue(TEXT("Registered ARM level resolves"), Gate.bResolved);
			TestTrue(TEXT("One-handed thrown ARM is one-handed"), Gate.bOneHanded);
			TestTrue(TEXT("Gate mass is canonical despite old contradictory definition"),
				FMath::IsNearlyEqual(Gate.UnitWeightKg, Canonical.WeightKg));
			const float ExpectedMax = Old->GetMaxThrowableWeightForStrength(100);
			TestTrue(TEXT("Weight threshold preserved"), FMath::IsNearlyEqual(Gate.MaxWeightKg, ExpectedMax));
			const bool ExpectedPermission = Canonical.WeightKg <= ExpectedMax + KINDA_SMALL_NUMBER;
			TestEqual(TEXT("Permission uses canonical unit mass"), Gate.bCanThrow, ExpectedPermission);
			if (Gate.bCanThrow)
			{
				TestTrue(TEXT("Allowed canonical throw yields nonzero speed scale"), Gate.SpeedScale > 0.0f);
			}
			Item.ItemLevel = 0;
			const FGridPhysicalThrowStrengthReadView Invalid =
				FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Get(), Old.Get(), Item, 100);
			TestTrue(TEXT("Invalid registered level stays canonical"), Invalid.bCanonicalIdentity);
			TestFalse(TEXT("Invalid N0 refuses thrown item"), Invalid.bCanThrow);
			TestFalse(TEXT("Invalid N0 cannot expose stale mass"), Invalid.bResolved);
		}
	}
	Old->ItemDefinitionId = TEXT("LegacyPhysicalThrow_063U");
	Old->bUseWeaponCatalogue = false;
	Old->Weight = 0.25f;
	FGridItemInstance Other;
	Other.RuntimeObjectId = FGuid::NewGuid();
	Other.ItemDefinitionId = Old->ItemDefinitionId;
	Other.Quantity = 1;
	const FGridPhysicalThrowStrengthReadView Legacy =
		FGridPhysicalThrowStrengthReadModel::Resolve(Registry.Get(), Old.Get(), Other, 10);
	TestFalse(TEXT("Legacy item is never classified as canonical"), Legacy.bCanonicalIdentity);
	TestEqual(TEXT("Legacy speed modifier unchanged"),
		Legacy.SpeedScale, Old->GetThrowSpeedScaleForStrength(10));
	TestEqual(TEXT("Legacy throw permission unchanged"),
		Legacy.bCanThrow, Old->CanBeThrownByStrength(10));
	return true;
}

#endif
