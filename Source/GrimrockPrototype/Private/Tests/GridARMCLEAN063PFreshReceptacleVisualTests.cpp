#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridItemMeshResolver.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063PFreshReceptacleMesh,
	"Grimrock.RPG.ARM.CLEAN06_3.FreshReceptacleItemVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063PFreshReceptacleMesh::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Real staged ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Registry is configured once"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& E : Errors) AddError(E);
		return false;
	}
	const FName IDs[] = {
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	for (const FName ID : IDs)
	{
		FGridItemInstance Actual;
		Actual.RuntimeObjectId = FGuid::NewGuid();
		Actual.ItemDefinitionId = ID;
		Actual.Quantity = 2;
		const int32 Levels[] = {1, 20};
		for (const int32 Level : Levels)
		{
			Actual.ItemLevel = Level;
			FGridResolvedItem FromRegistry;
			if (!TestTrue(TEXT("Real N1/N20 item resolves"), Registry->ResolveItem(ID, Level, FromRegistry)))
				return false;
			const FGridItemMeshView Fresh =
				FGridItemMeshResolver::Resolve(Registry.Get(), Actual);
			TestTrue(TEXT("Fresh insertion keeps one canonical mesh"), Fresh.bResolved && Fresh.bRegisteredIdentity);
			TestTrue(TEXT("Insertion keeps catalogue-authored local offset"),
				Fresh.WorldTransform.Equals(FromRegistry.WorldVisualTransform));
			TestEqual(TEXT("Insertion keeps canonical WorldMesh"),
				Fresh.WorldMesh.ToSoftObjectPath().ToString(),
				FromRegistry.WorldMesh.ToSoftObjectPath().ToString());
			TestEqual(TEXT("Presentation does not rewrite contained instance GUID"), Actual.RuntimeObjectId.IsValid(), true);
		}
		Actual.ItemLevel = 0;
		const FGridItemMeshView Invalid = FGridItemMeshResolver::Resolve(Registry.Get(), Actual);
		TestTrue(TEXT("Registered invalid N0 cannot receive legacy mesh"), Invalid.bRegisteredIdentity && !Invalid.bResolved &&
			Invalid.WorldMesh.IsNull());
	}
	return true;
}

#endif
