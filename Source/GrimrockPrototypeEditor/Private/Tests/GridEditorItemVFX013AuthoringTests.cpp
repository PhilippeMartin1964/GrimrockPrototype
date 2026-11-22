#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/PackageName.h"

namespace GridItemVFX013AuthoringTests
{
    /** Temporary 2D staging selector; removed after Unreal renames the .uasset. */
    const TCHAR* ResolveFlamePath()
    {
        constexpr TCHAR NewPackage[] =
            TEXT("/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch");
        static constexpr TCHAR NewObject[] =
            TEXT("/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch.NS_Flame_8_Torch");
        static constexpr TCHAR OldObject[] =
            TEXT("/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch.NS_Flame_8_Torch");
        return FPackageName::DoesPackageExist(NewPackage) ? NewObject : OldObject;
    }
    const FName FlameId(TEXT("TorchFlame"));

    const FGridItemNiagaraEffectConfig* FindFlame(const FGridItemVisualEffectsConfig& VisualEffects)
    {
        return VisualEffects.Effects.FindByPredicate(
            [](const FGridItemNiagaraEffectConfig& Effect)
            {
                return Effect.EffectId == FlameId;
            });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013TorchAuthoringTest,
    "Grimrock.Items.VFX01_3.TorchAuthoring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013TorchAuthoringTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UNiagaraSystem* FlameSystem = LoadObject<UNiagaraSystem>(
        nullptr, GridItemVFX013AuthoringTests::ResolveFlamePath());
    if (!TestNotNull(TEXT("Canonical torch Niagara system loads"), FlameSystem))
    {
        return false;
    }

    UGridItemDefinitionAsset* Torch = NewObject<UGridItemDefinitionAsset>();
    Torch->ItemDefinitionId = TEXT("VFX013_TorchAuthoring");
    Torch->ItemType = EGridItemType::Torch;
    Torch->LightEmitter.bUsePointLight = true;
    Torch->LightEmitter.bDefaultEnabled = true;
    Torch->LightEmitter.LightIntensity = 769.f;
    Torch->LightEmitter.LightRadius = 386.f;
    Torch->LightEmitter.LightColor = FLinearColor(0.92f, 0.4f, 0.17f);
    Torch->LightEmitter.bEnableLightFlicker = true;
    Torch->LightEmitter.FlickerIntensityAmount = 46.f;

    FGridItemNiagaraEffectConfig Flame;
    Flame.EffectId = GridItemVFX013AuthoringTests::FlameId;
    Flame.NiagaraSystem = FlameSystem;
    Flame.RelativeTransform = FTransform(
        FRotator(9.f, 19.f, 29.f), FVector(8.f, -4.f, 27.f));
    Flame.ActivationMode = EGridItemVfxActivationMode::WhenLightEnabled;
    Torch->VisualEffects.Effects.Add(Flame);

    TestTrue(TEXT("Data-driven torch definition remains valid"), Torch->IsValidDefinition());
    TestTrue(TEXT("Torch uses a PointLight"), Torch->LightEmitter.HasEmitter());
    TestTrue(TEXT("Torch is lit by default"), Torch->IsLightEnabledByDefault());
    TestTrue(TEXT("Light intensity and radius are unchanged"),
        FMath::IsNearlyEqual(Torch->LightEmitter.LightIntensity, 769.f) &&
        FMath::IsNearlyEqual(Torch->LightEmitter.LightRadius, 386.f));
    TestTrue(TEXT("Light flicker remains independent from Niagara"),
        Torch->LightEmitter.bEnableLightFlicker &&
        FMath::IsNearlyEqual(Torch->LightEmitter.FlickerIntensityAmount, 46.f));
    TestTrue(TEXT("Flame offset stays inside VisualEffects"),
        Torch->VisualEffects.Effects[0].RelativeTransform.Equals(Flame.RelativeTransform));
    TestTrue(TEXT("Flame follows the instance light flag"),
        Torch->VisualEffects.Effects[0].ActivationMode == EGridItemVfxActivationMode::WhenLightEnabled);

    Torch->VisualEffects.Effects[0].ActivationMode = EGridItemVfxActivationMode::Always;
    TestTrue(TEXT("Activation policy can be authored independently"), Torch->VisualEffects.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013ProductionPreflightTest,
    "Grimrock.Items.VFX01_3.ProductionPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013ProductionPreflightTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Items(NewObject<UGridItemSubsystem>(GI.Get()));
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
    if (!TestNotNull(TEXT("Canonical ItemSystem root"), Root)) return false;
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical ItemSystem registers"), Items->RegisterItemSystem(Root, Errors)))
        return false;
    FGridResolvedItem Torch;
    if (!TestTrue(TEXT("Canonical N0 torch resolves without old DataAsset"),
        Items->ResolveItem(TEXT("Item_Torch"), 0, Torch))) return false;

    TestTrue(TEXT("Canonical torch identity and level are valid"),
        Torch.IsValid() && !Torch.bIsArmament && Torch.ItemLevel == 0);
    TestTrue(TEXT("Torch has an authored PointLight"), Torch.LightEmitter.bUsePointLight);
    TestTrue(TEXT("Light starts enabled by default"), Torch.LightEmitter.bDefaultEnabled);
    TestTrue(TEXT("Light parameters remain positive"),
        Torch.LightEmitter.LightIntensity > 0.f && Torch.LightEmitter.LightRadius > 0.f);
    TestNotNull(TEXT("TorchFlame is in canonical VisualEffects"),
        GridItemVFX013AuthoringTests::FindFlame(Torch.VisualEffects));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013MigratedProductionTest,
    "Grimrock.Items.VFX01_3.MigratedProductionAsset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013MigratedProductionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Items(NewObject<UGridItemSubsystem>(GI.Get()));
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
    if (!TestNotNull(TEXT("Canonical ItemSystem root"), Root)) return false;
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical ItemSystem registers"), Items->RegisterItemSystem(Root, Errors)))
        return false;
    FGridResolvedItem Torch;
    if (!TestTrue(TEXT("Canonical N0 torch resolves without old DataAsset"),
        Items->ResolveItem(TEXT("Item_Torch"), 0, Torch))) return false;

    const FGridItemNiagaraEffectConfig* Flame =
        GridItemVFX013AuthoringTests::FindFlame(Torch.VisualEffects);
    if (!TestNotNull(TEXT("Canonical TorchFlame exists"), Flame))
        return false;
    TestTrue(TEXT("Niagara still resolves from the authored torch effect"),
        Flame->NiagaraSystem.ToSoftObjectPath() ==
        FSoftObjectPath(GridItemVFX013AuthoringTests::ResolveFlamePath()));
    TestTrue(TEXT("Flame follows the instance light state"),
        Flame->ActivationMode == EGridItemVfxActivationMode::WhenLightEnabled);
    TestFalse(TEXT("Flame local transform contains no invalid values"),
        Flame->RelativeTransform.ContainsNaN());
    int32 Count = 0;
    for (const FGridItemNiagaraEffectConfig& Effect : Torch.VisualEffects.Effects)
        if (Effect.EffectId == GridItemVFX013AuthoringTests::FlameId) ++Count;
    TestEqual(TEXT("Only one canonical torch flame"), Count, 1);
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
