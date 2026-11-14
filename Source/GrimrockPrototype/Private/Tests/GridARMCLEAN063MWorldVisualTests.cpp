#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridCanonicalItemMeshReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063MCanonicalWorldMesh,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupMeshAndTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063MCanonicalWorldMesh::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Persisted canonical ItemSystem loads"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register real staged registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}

	const FName IDs[] = {TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : IDs)
	{
		for (const int32 Level : Levels)
		{
			FGridResolvedItem Source;
			if (!TestTrue(TEXT("Real ARM N1/N20 resolves"), Registry->ResolveItem(Id, Level, Source)))
			{
				return false;
			}
			FGridItemInstance Instance;
			Instance.RuntimeObjectId = FGuid::NewGuid();
			Instance.ItemDefinitionId = Id;
			Instance.ItemLevel = Level;
			Instance.Quantity = 1;
			FGridCanonicalItemMeshReadView View = FGridCanonicalItemMeshReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("World mesh source is canonical"), View.bCanonicalIdentity);
			TestTrue(TEXT("World mesh resolves at instance level"), View.bResolved);
			TestEqual(TEXT("World visual profile identity is stable"), View.VisualId, Source.VisualId);
			TestEqual(TEXT("Canonical world mesh path is not an old item copy"),
				View.WorldMesh.ToSoftObjectPath().ToString(), Source.WorldMesh.ToSoftObjectPath().ToString());
			TestTrue(TEXT("Pickup visual offset is exactly authored catalogue transform"),
				View.WorldTransform.Equals(Source.WorldVisualTransform));
			TestTrue(TEXT("Pickup resolution is read-only on real instance"),
				Instance.ItemLevel == Level && Instance.RuntimeObjectId.IsValid());
			Instance.ItemLevel = 0;
			View = FGridCanonicalItemMeshReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("Invalid N0 is still a known canonical identity"), View.bCanonicalIdentity);
			TestFalse(TEXT("Invalid N0 must not get a legacy mesh"), View.bResolved);
			TestTrue(TEXT("Invalid N0 world mesh is empty"), View.WorldMesh.IsNull());
			TestTrue(TEXT("Invalid N0 cannot reuse world visual transform"),
				View.WorldTransform.Equals(FTransform::Identity));
		}
	}

	FGridItemInstance Legacy;
	Legacy.RuntimeObjectId = FGuid::NewGuid();
	Legacy.ItemDefinitionId = TEXT("Unregistered_063M");
	Legacy.Quantity = 1;
	const FGridCanonicalItemMeshReadView Old = FGridCanonicalItemMeshReadModel::Resolve(Registry.Get(), Legacy);
	TestFalse(TEXT("Unknown item does not replace its legacy world mesh"), Old.bCanonicalIdentity);
	TestFalse(TEXT("Unknown item has no fabricated canonical visual"), Old.bResolved);
	return true;
}

#endif
