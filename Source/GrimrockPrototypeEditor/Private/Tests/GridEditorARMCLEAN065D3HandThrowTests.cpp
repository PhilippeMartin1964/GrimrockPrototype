#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorArmHandThrowPlan.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065D3NativeProjection,
    "Grimrock.RPG.ARM.CLEAN06_5D3.RealHandThrowProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorARMCLEAN065D3NativeProjection::RunTest(const FString&)
{
    namespace Plan = GridEditorArmHandThrowPlan;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real native registry"), Root.Get())) return false;
    const FString Baseline = Plan::Export(*Root->WeaponCatalogue) + Plan::Export(*Root->WeaponVisualCatalogue);
    Plan::FPlan Projection;
    FString Error;
    if (!TestTrue(TEXT("Exactly 22 real sources and no conflicting physics"), Plan::Build(*Root.Get(), Projection, Error)))
    { AddError(Error); return false; }
    TestEqual(TEXT("Ordinary policies"), Projection.Ordinary, 19);
    TestEqual(TEXT("Specialized projectiles preserved"), Projection.Specialized, 3);
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    if (!TestTrue(TEXT("Projected root validates"), Plan::AssignPreview(*Root.Get(), Projection, *Preview.Get()))) return false;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Runtime accepts projected root"), Registry->RegisterItemSystem(Preview.Get(), Errors))) return false;
    TArray<FGridWeaponPipelineRow> Rows;
    if (!FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)) return false;
    for (const auto& Row : Rows)
    {
        const auto* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Row.ItemPath);
        if (!TestNotNull(TEXT("Exact old source"), Old)) return false;
        for (int32 N : {1,20})
        {
            FGridResolvedItem Item;
            if (!TestTrue(TEXT("Runtime resolves physical level"), Registry->ResolveItem(FName(*Row.ItemDefinitionId), N, Item))) return false;
            const auto& P = Item.Weapon.Projectile;
            const auto& V = Item.ProjectileAppearance;
            TestEqual(TEXT("Specialized combat authorization unchanged"), P.bEnabled, Row.IsThrowable());
            TestEqual(TEXT("Ordinary policy explicit"), P.bGenericThrowAuthored, !Row.IsThrowable());
            TestEqual(TEXT("Exact speed"), P.InitialSpeed, Old->ThrowSpeed);
            TestEqual(TEXT("Exact signed arc"), P.Arc, Old->ThrowArc);
            TestEqual(TEXT("Exact lifetime"), P.FlightLifetime, Old->ThrowLifeSeconds);
            TestEqual(TEXT("Exact impact offset"), P.ImpactDropOffset, Old->ThrowImpactDropOffset);
            TestTrue(TEXT("Flight rotation"), V.RelativeRotation.Equals(Old->ThrowVisualRelativeRotation));
            TestTrue(TEXT("Flight scale"), V.RelativeScale.Equals(Old->ThrowVisualRelativeScale));
            TestTrue(TEXT("Flight angular axis"), V.TumbleAxis.Equals(Old->ThrowVisualTumbleAxis));
            TestTrue(TEXT("Flight forward axis"), V.MeshForwardAxis == Old->ProjectileMeshForwardAxis);
            TestEqual(TEXT("Tumble"), V.TumbleDegreesPerSecond, Old->ThrowVisualTumbleDegreesPerSecond);
            TestEqual(TEXT("Spin"), V.SpinDegreesPerSecond, Old->ThrowVisualSpinDegreesPerSecond);
            TestTrue(TEXT("No dedicated projectile mesh for ordinary ARM"), Row.IsThrowable() || Item.ProjectileMesh.IsNull());
        }
    }
    TestEqual(TEXT("Original catalogues untouched"), Plan::Export(*Root->WeaponCatalogue) + Plan::Export(*Root->WeaponVisualCatalogue), Baseline);
    return !HasAnyErrors();
}
#endif
