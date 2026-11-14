#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridCanonicalItemMeshReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063LHeldVisual,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalHeldItemMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063LHeldVisual::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Canonical ItemSystem package exists"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Real staged ItemSystem registers"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FName ItemIds[] =
	{
		TEXT("Weapon_StraightSword_01"), TEXT("Weapon_GreatAxe_01"),
		TEXT("Weapon_RoundShield_01"), TEXT("Weapon_HolyAegis_01")
	};
	const int32 Levels[] = {1, 20};
	for (const FName Id : ItemIds)
	{
		for (const int32 Level : Levels)
		{
			FGridItemInstance Instance;
			Instance.RuntimeObjectId = FGuid::NewGuid();
			Instance.ItemDefinitionId = Id;
			Instance.ItemLevel = Level;
			Instance.Quantity = 1;
			FGridResolvedItem Canonical;
			if (!TestTrue(TEXT("Real level resolves"), Registry->ResolveItem(Id, Level, Canonical)))
				return false;
			const FGridCanonicalItemMeshReadView Mesh =
				FGridCanonicalItemMeshReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("Resolved mesh view is explicitly canonical"), Mesh.bCanonicalIdentity);
			TestTrue(TEXT("Valid canonical mesh view is resolved"), Mesh.bResolved);
			TestEqual(TEXT("Mesh visual identity preserved"), Mesh.VisualId, Canonical.VisualId);
			TestEqual(TEXT("Canonical world mesh reference preserved"),
				Mesh.WorldMesh.ToSoftObjectPath().ToString(),
				Canonical.WorldMesh.ToSoftObjectPath().ToString());
			TestEqual(TEXT("Equipped mesh uses resolved fallback only"),
				Mesh.EquippedMesh.ToSoftObjectPath().ToString(),
				Canonical.EquippedMesh.ToSoftObjectPath().ToString());

			Instance.ItemLevel = 0;
			const FGridCanonicalItemMeshReadView Invalid =
				FGridCanonicalItemMeshReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("Known canonical ID remains classified when N0 invalid"), Invalid.bCanonicalIdentity);
			TestFalse(TEXT("Invalid level never resolves a legacy mesh"), Invalid.bResolved);
			TestTrue(TEXT("Invalid N0 equipped mesh remains null"), Invalid.EquippedMesh.IsNull());
		}
	}
	FGridItemInstance Other;
	Other.ItemDefinitionId = TEXT("NonCanonical_Held063L");
	Other.RuntimeObjectId = FGuid::NewGuid();
	Other.Quantity = 1;
	const FGridCanonicalItemMeshReadView Unregistered =
		FGridCanonicalItemMeshReadModel::Resolve(Registry.Get(), Other);
	TestFalse(TEXT("Noncanonical item does not override existing held meshes"),
		Unregistered.bCanonicalIdentity);
	TestFalse(TEXT("Noncanonical view has no invented mesh"), Unregistered.bResolved);
	TestTrue(TEXT("Null registry makes no new authority"), !FGridCanonicalItemMeshReadModel::Resolve(nullptr, Other).bResolved);
	return true;
}

#endif
