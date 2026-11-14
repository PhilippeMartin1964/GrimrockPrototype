#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "../Runtime/GridCanonicalProjectileVisualReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063RProjectileVisual,
	"Grimrock.RPG.ARM.CLEAN06_3.CanonicalProjectileVisualByActualLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063RProjectileVisual::RunTest(const FString&)
{
	TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
	if (!TestNotNull(TEXT("Staged real ItemSystem"), Root.Get())) return false;
	TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
	TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
	TArray<FString> Errors;
	if (!TestTrue(TEXT("Register canonical catalogue"), Registry->RegisterItemSystem(Root.Get(), Errors)))
	{
		for (const FString& Error : Errors) AddError(Error);
		return false;
	}
	const FName Ids[] = {TEXT("Weapon_ThrowingKnife_01"),
		TEXT("Weapon_Javelin_01"), TEXT("Weapon_Dart_01")};
	const int32 Levels[] = {1, 20};
	for (const FName Id : Ids)
	{
		for (const int32 Level : Levels)
		{
			FGridItemInstance Instance;
			Instance.RuntimeObjectId = FGuid::NewGuid();
			Instance.ItemDefinitionId = Id;
			Instance.ItemLevel = Level;
			Instance.Quantity = 1;
			FGridResolvedItem Expected;
			if (!TestTrue(TEXT("Real N1 or N20 resolves"), Registry->ResolveItem(Id, Level, Expected)))
				return false;
			const FGridCanonicalProjectileVisualReadView Visual =
				FGridCanonicalProjectileVisualReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("Flight visual is a known canonical item"), Visual.bCanonicalIdentity);
			TestTrue(TEXT("Flight visual resolved at real level"), Visual.bResolved);
			TestTrue(TEXT("Throwing ARM uses authored projectile visual"), Visual.bHasProjectileVisual);
			TestEqual(TEXT("Resolved flight visual ID"), Visual.VisualId, Expected.VisualId);
			TestEqual(TEXT("Projectile mesh is taken from global catalogue"),
				Visual.ProjectileMesh.ToSoftObjectPath().ToString(),
				Expected.ProjectileMesh.ToSoftObjectPath().ToString());
			TestTrue(TEXT("Projectile authored rotation preserved"),
				Visual.Appearance.RelativeRotation.Equals(Expected.ProjectileAppearance.RelativeRotation));
			TestTrue(TEXT("Projectile authored scale preserved"),
				Visual.Appearance.RelativeScale.Equals(Expected.ProjectileAppearance.RelativeScale));
			TestEqual(TEXT("Projectile authored rotation mode preserved"),
				static_cast<uint8>(Visual.Appearance.Mode), static_cast<uint8>(Expected.ProjectileAppearance.Mode));
			TestEqual(TEXT("Projectile authored forward axis preserved"),
				static_cast<uint8>(Visual.Appearance.MeshForwardAxis),
				static_cast<uint8>(Expected.ProjectileAppearance.MeshForwardAxis));
			TestTrue(TEXT("Flight policy is enabled only for authored projectile"),
				Visual.FlightPolicy.bEnabled && Expected.Weapon.Projectile.bEnabled);
			TestTrue(TEXT("Landing offset is one N1-N20 canonical value"),
				FMath::IsNearlyEqual(Visual.FlightPolicy.ImpactDropOffset,
					Expected.Weapon.Projectile.ImpactDropOffset));
			TestTrue(TEXT("Lifetime is one N1-N20 canonical value"),
				FMath::IsNearlyEqual(Visual.FlightPolicy.FlightLifetime,
					Expected.Weapon.Projectile.FlightLifetime));
			TestTrue(TEXT("Flight visual projection does not touch item GUID"),
				Instance.RuntimeObjectId.IsValid());
			Instance.ItemLevel = 0;
			const FGridCanonicalProjectileVisualReadView Invalid =
				FGridCanonicalProjectileVisualReadModel::Resolve(Registry.Get(), Instance);
			TestTrue(TEXT("Registered invalid N0 retains canonical classification"), Invalid.bCanonicalIdentity);
			TestFalse(TEXT("Registered invalid N0 does not reuse DA_Weapon flight mesh"), Invalid.bResolved);
			TestTrue(TEXT("Registered invalid N0 has no mesh"), Invalid.ProjectileMesh.IsNull());
			TestFalse(TEXT("Registered invalid N0 cannot inherit flight policy"), Invalid.FlightPolicy.bEnabled);
		}
	}
	FGridItemInstance Unknown;
	Unknown.RuntimeObjectId = FGuid::NewGuid();
	Unknown.ItemDefinitionId = TEXT("Unregistered_063R");
	Unknown.Quantity = 1;
	const FGridCanonicalProjectileVisualReadView Legacy =
		FGridCanonicalProjectileVisualReadModel::Resolve(Registry.Get(), Unknown);
	TestFalse(TEXT("Unregistered instance does not override legacy flight"), Legacy.bCanonicalIdentity);
	return true;
}

#endif
