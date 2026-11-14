#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridCanonicalThrowBallisticsReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063TThrowBallistics,
	"Grimrock.RPG.ARM.CLEAN06_3.ThrowBallisticsByActualLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063TThrowBallistics::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real staged item registry"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
	Old->bUseWeaponCatalogue = true;
	Old->ThrowSpeed = 12.0f;
	Old->ThrowArc = 33.0f;
	const FName Ids[] = {
		TEXT("Weapon_ThrowingKnife_01"), TEXT("Weapon_Javelin_01"), TEXT("Weapon_Dart_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		Old->ItemDefinitionId = Id;
		for (const int32 Level : Levels)
		{
			FGridItemInstance Item;
			Item.ItemDefinitionId = Id;
			Item.RuntimeObjectId = FGuid::NewGuid();
			Item.ItemLevel = Level;
			Item.Quantity = 1;
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real ranged/throwing ARM exists"), Registry->ResolveItem(Id, Level, Canonical)))
				return false;
			const FGridCanonicalThrowBallisticsReadView Shot =
				FGridCanonicalThrowBallisticsReadModel::Resolve(Registry.Get(), Old.Get(), Item);
			TestTrue(TEXT("Canonical N1/N20 physical throw has one source"), Shot.bCanonicalIdentity);
			TestTrue(TEXT("Canonical specialized throw is resolved"), Shot.bResolved && Shot.bSpecializedProjectile);
			TestTrue(TEXT("Throw speed ignores contradictory DA_Weapon value"),
				FMath::IsNearlyEqual(Shot.InitialSpeed, Canonical.Weapon.Projectile.InitialSpeed));
			TestTrue(TEXT("Throw arc ignores contradictory DA_Weapon value"),
				FMath::IsNearlyEqual(Shot.Arc, Canonical.Weapon.Projectile.Arc));
			Item.ItemLevel = 0;
			const FGridCanonicalThrowBallisticsReadView Invalid =
				FGridCanonicalThrowBallisticsReadModel::Resolve(Registry.Get(), Old.Get(), Item);
			TestTrue(TEXT("Known invalid ARM stays classified"), Invalid.bCanonicalIdentity);
			TestFalse(TEXT("Known invalid level never falls back to old speed"), Invalid.bResolved);
			TestEqual(TEXT("No generated launch speed on N0"), Invalid.InitialSpeed, 0.0f);
		}
	}
	Old->bUseWeaponCatalogue = false;
	Old->ItemDefinitionId = TEXT("Unregistered_PhysicalThrow_063T");
	FGridItemInstance Legacy;
	Legacy.ItemDefinitionId = Old->ItemDefinitionId;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemLevel = 0;
	Legacy.Quantity = 1;
	const FGridCanonicalThrowBallisticsReadView Fallback =
		FGridCanonicalThrowBallisticsReadModel::Resolve(Registry.Get(), Old.Get(), Legacy);
	TestFalse(TEXT("Unregistered throw retains old source"), Fallback.bCanonicalIdentity);
	TestTrue(TEXT("Unregistered throw resolves"), Fallback.bResolved);
	TestEqual(TEXT("Unregistered throw speed unchanged"), Fallback.InitialSpeed, 12.0f);
	TestEqual(TEXT("Unregistered throw arc unchanged"), Fallback.Arc, 33.0f);
	return true;
}

#endif
