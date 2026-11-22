#if WITH_DEV_AUTOMATION_TESTS

#include "Effects/GridEffectsOrgTorchMoveCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AssetRegistryInterface.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorEffectsOrg012DTorchPreflightTest,
    "Grimrock.Editor.EffectsOrg01.TorchMigrationPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorEffectsOrg012DTorchPreflightTest::RunTest(const FString&)
{
    // Temporary migration test. Before migration, execute the REAL commandlet
    // in read-only mode. Afterwards, verify the new asset dependency chain.
    // No content package is modified in either path.
    static const FString OldPackage(TEXT("/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch"));
    static const FString NewPackage(TEXT("/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch"));
    static const FName FlameMaterial(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Flame/MI_Flame_8"));
    static const FName DistortionMaterial(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Flame/MI_Distortion_1"));

    const bool bOld = FPackageName::DoesPackageExist(OldPackage);
    const bool bNew = FPackageName::DoesPackageExist(NewPackage);
    if (!TestTrue(TEXT("Exactly one torch Niagara location exists"), bOld != bNew))
    {
        return false;
    }

    if (bOld)
    {
        UGridEffectsOrgTorchMoveCommandlet* Commandlet =
            NewObject<UGridEffectsOrgTorchMoveCommandlet>(GetTransientPackage());
        if (!TestNotNull(TEXT("Torch migration commandlet can be constructed"), Commandlet))
        {
            return false;
        }

        const int32 Result = Commandlet->Main(TEXT(""));
        const bool bReady = TestEqual(TEXT("Actual Unreal commandlet dry-run succeeded"), Result, 0);
        const bool bSourceUntouched = TestTrue(TEXT("Dry-run preserved original Niagara"),
            FPackageName::DoesPackageExist(OldPackage));
        const bool bTargetAbsent = TestFalse(TEXT("Dry-run did not create target Niagara"),
            FPackageName::DoesPackageExist(NewPackage));
        return bReady && bSourceUntouched && bTargetAbsent;
    }

    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    Registry.SearchAllAssets(true);
        TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
        TStrongObjectPtr<UGridItemSubsystem> Items(NewObject<UGridItemSubsystem>(GI.Get()));
        UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem"));
        if (!TestNotNull(TEXT("Canonical ItemSystem root"), Root)) return false;
        TArray<FString> Errors;
        if (!TestTrue(TEXT("Canonical ItemSystem registers"), Items->RegisterItemSystem(Root, Errors)))
            return false;
        FGridResolvedItem Torch;
        if (!TestTrue(TEXT("Canonical N0 torch resolves without old DataAsset"),
            Items->ResolveItem(TEXT("Item_Torch"), 0, Torch))) return false;
    
    const FSoftObjectPath ExpectedNiagara(
        TEXT("/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch.NS_Flame_8_Torch"));
    const bool bLinked = Torch.VisualEffects.Effects.ContainsByPredicate(
        [&ExpectedNiagara](const FGridItemNiagaraEffectConfig& Effect)
        {
            return Effect.EffectId == FName(TEXT("TorchFlame")) &&
                Effect.NiagaraSystem.ToSoftObjectPath() == ExpectedNiagara;
        });
    TArray<FName> NiagaraDependencies;
    const bool bNiagaraOK = Registry.GetDependencies(FName(*NewPackage), NiagaraDependencies,
        UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery());
    return TestTrue(TEXT("Canonical torch points to relocated Niagara"), bLinked) &&
        TestTrue(TEXT("Relocated Niagara dependency graph is readable"), bNiagaraOK) &&
        TestTrue(TEXT("Flame material remains intact"), NiagaraDependencies.Contains(FlameMaterial)) &&
        TestTrue(TEXT("Distortion material remains intact"), NiagaraDependencies.Contains(DistortionMaterial));
}

#endif // WITH_DEV_AUTOMATION_TESTS
