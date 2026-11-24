#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridThrowBallisticsReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063TThrowBallistics,
	"Grimrock.RPG.ARM.CLEAN06_3.ThrowBallisticsByActualLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063TThrowBallistics::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real staged item registry"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real ItemSystem"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FName Ids[] = {
		TEXT("Weapon_ThrowingKnife_01"), TEXT("Weapon_Javelin_01"), TEXT("Weapon_Dart_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
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
			const FGridThrowBallisticsReadView Shot =
				FGridThrowBallisticsReadModel::Resolve(Registry.Get(), Item);
			TestTrue(TEXT("Canonical N1/N20 physical throw has one source"), Shot.bRegisteredIdentity);
			TestTrue(TEXT("Canonical specialized throw is resolved"), Shot.bResolved && Shot.bSpecializedProjectile);
			TestTrue(TEXT("Throw speed ignores contradictory DA_Weapon value"),
				FMath::IsNearlyEqual(Shot.InitialSpeed, Canonical.Weapon.Projectile.InitialSpeed));
			TestTrue(TEXT("Throw arc ignores contradictory DA_Weapon value"),
				FMath::IsNearlyEqual(Shot.Arc, Canonical.Weapon.Projectile.Arc));
			Item.ItemLevel = 0;
			const FGridThrowBallisticsReadView Invalid =
				FGridThrowBallisticsReadModel::Resolve(Registry.Get(), Item);
			TestTrue(TEXT("Known invalid ARM stays classified"), Invalid.bRegisteredIdentity);
			TestFalse(TEXT("Known invalid level never falls back to old speed"), Invalid.bResolved);
			TestEqual(TEXT("No generated launch speed on N0"), Invalid.InitialSpeed, 0.0f);
		}
	}
	FGridItemInstance Legacy;
	Legacy.ItemDefinitionId = TEXT("Unregistered_PhysicalThrow_063T");
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemLevel = 0;
	Legacy.Quantity = 1;
	TestFalse(TEXT("Configured registry rejects unknown flight even with matching old asset"),
		FGridThrowBallisticsReadModel::Resolve(Registry.Get(), Legacy).bResolved);
	const FGridThrowBallisticsReadView Fallback =
		FGridThrowBallisticsReadModel::Resolve(nullptr, Legacy);
	TestFalse(TEXT("Unregistered throw retains old source"), Fallback.bRegisteredIdentity);
	TestFalse(TEXT("Unregistered throw is unresolved"), Fallback.bResolved);
	TestEqual(TEXT("Unregistered throw has no speed"), Fallback.InitialSpeed, 0.0f);
	TestEqual(TEXT("Unregistered throw has no arc"), Fallback.Arc, 0.0f);
	return true;
}

#endif
