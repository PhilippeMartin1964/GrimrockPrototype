#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridProjectileVisuals.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063WAmmoFlightMesh,
	"Grimrock.RPG.ARM.CLEAN06_3.UnlevelledAmmoFlightMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063WAmmoFlightMesh::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real staged ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real registry registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	FGridItemInstance Arrow;
	Arrow.RuntimeObjectId = FGuid::NewGuid();
	Arrow.ItemDefinitionId = TEXT("Item_Arrows_01");
	Arrow.ItemLevel = 0;
	Arrow.Quantity = 1;
	FGridResolvedItem Canonical;
	if (!TestTrue(TEXT("Production arrow resolves at real N0"),
		Registry->ResolveItem(Arrow.ItemDefinitionId, Arrow.ItemLevel, Canonical))) return false;
	const FGridProjectileVisualView Flight =
		FGridProjectileVisuals::Resolve(Registry.Get(), Arrow);
	TestTrue(TEXT("Ammo is registered"), Flight.bRegisteredIdentity);
	TestTrue(TEXT("Ammo N0 is a real resolved item"), Flight.bResolved);
	TestTrue(TEXT("Arrow is classified as ammo without synthetic ARM actions"), Flight.bIsAmmunition);
	TestFalse(TEXT("Arrows do not invent a dedicated weapon projectile animation"), Flight.bHasProjectileVisual);
	TestEqual(TEXT("Flight mesh is canonical ammo WorldMesh"),
		Flight.WorldMesh.ToSoftObjectPath().ToString(),
		Canonical.WorldMesh.ToSoftObjectPath().ToString());
	TestTrue(TEXT("Unlevelled arrow remains N0"), Arrow.ItemLevel == 0);
	Arrow.ItemLevel = 1;
	FGridResolvedItem LegacyFamilyRead;
	TestFalse(TEXT("Registry rejects former non-ARM N1 family alias"),
		Registry->ResolveItem(Arrow.ItemDefinitionId, Arrow.ItemLevel, LegacyFamilyRead));
	const FGridProjectileVisualView InvalidPhysical =
		FGridProjectileVisuals::Resolve(Registry.Get(), Arrow);
	TestTrue(TEXT("Non-ARM N1 keeps canonical membership"), InvalidPhysical.bRegisteredIdentity);
	TestFalse(TEXT("Non-ARM N1 is not a physical ammo flight"), InvalidPhysical.bResolved);
	TestFalse(TEXT("Invalid flight cannot classify ammunition"), InvalidPhysical.bIsAmmunition);
	TestTrue(TEXT("Invalid N1 flight has no canonical mesh"), InvalidPhysical.WorldMesh.IsNull());
	Arrow.ItemLevel = -1;
	const FGridProjectileVisualView Invalid =
		FGridProjectileVisuals::Resolve(Registry.Get(), Arrow);
	TestTrue(TEXT("Invalid known arrow retains canonical membership"), Invalid.bRegisteredIdentity);
	TestFalse(TEXT("Invalid known arrow may not use old mesh"), Invalid.bResolved);
	TestTrue(TEXT("Invalid arrow mesh is cleared"), Invalid.WorldMesh.IsNull());
	Arrow.ItemLevel = 0;
	Arrow.RuntimeObjectId = FGuid();
	const FGridProjectileVisualView BadGuid =
		FGridProjectileVisuals::Resolve(Registry.Get(), Arrow);
	TestTrue(TEXT("Missing GUID still classifies known ammo as canonical"), BadGuid.bRegisteredIdentity);
	TestFalse(TEXT("Missing GUID never resolves a mesh"), BadGuid.bResolved);
	TestTrue(TEXT("Missing GUID cannot display an obsolete mesh"), BadGuid.WorldMesh.IsNull());
	FGridItemInstance Unregistered;
	Unregistered.RuntimeObjectId = FGuid::NewGuid();
	Unregistered.ItemDefinitionId = TEXT("UnregisteredArrow_063W");
	Unregistered.Quantity = 1;
	TestFalse(TEXT("Unregistered ammo keeps old flight path"),
		FGridProjectileVisuals::Resolve(Registry.Get(), Unregistered).bRegisteredIdentity);
	return true;
}

#endif
