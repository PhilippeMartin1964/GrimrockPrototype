#if WITH_DEV_AUTOMATION_TESTS

#include "Items/GridItemTorchVfxMigrationCommandlet.h"

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace GridItemVFX013Tests
{
    constexpr TCHAR TorchPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.DA_Item_Torch");
    constexpr TCHAR FlamePath[] =
        TEXT("/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch.NS_Flame_8_Torch");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013MigrationRecipeTest,
    "Grimrock.Items.VFX01_3.MigrationRecipe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013MigrationRecipeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UNiagaraSystem* Flame = LoadObject<UNiagaraSystem>(nullptr, GridItemVFX013Tests::FlamePath);
    if (!TestNotNull(TEXT("Canonical torch flame Niagara loads"), Flame))
    {
        return false;
    }

    UGridItemDefinitionAsset* Torch = NewObject<UGridItemDefinitionAsset>();
    Torch->ItemDefinitionId = TEXT("VFX013_TestTorch");
    Torch->ItemType = EGridItemType::Torch;
    Torch->LightEmitter.bUsePointLight = true;
    Torch->LightEmitter.bDefaultEnabled = true;
    Torch->LightEmitter.LightIntensity = 769.f;
    Torch->LightEmitter.LightRadius = 386.f;
    Torch->LightEmitter.LightColor = FLinearColor(0.92f, 0.4f, 0.17f);
    Torch->LightEmitter.bEnableLightFlicker = true;
    Torch->LightEmitter.FlickerIntensityAmount = 46.f;
    Torch->LightEmitter.NiagaraSystem = Flame;
    Torch->LightEmitter.NiagaraRelativeLocation = FVector(8.f, -4.f, 27.f);
    Torch->LightEmitter.NiagaraRelativeRotation = FRotator(9.f, 19.f, 29.f);

    FGridItemVisualEffectsConfig NewVisuals;
    FGridLightEmitterConfig NewLight;
    bool bWouldChange = false;
    FString Error;
    if (!TestTrue(TEXT("Legacy torch can migrate without modifying the input"),
        FGridItemTorchVfxMigration::BuildTarget(
            *Torch, Flame, NewVisuals, NewLight, bWouldChange, Error)))
    {
        AddError(Error);
        return false;
    }

    TestTrue(TEXT("Migration reports changes"), bWouldChange);
    TestEqual(TEXT("Exactly one torch flame effect"), NewVisuals.Effects.Num(), 1);
    if (NewVisuals.Effects.Num() != 1)
    {
        return false;
    }

    const FGridItemNiagaraEffectConfig& Vfx = NewVisuals.Effects[0];
    TestEqual(TEXT("Canonical torch effect ID"), Vfx.EffectId, FName(TEXT("TorchFlame")));
    TestTrue(TEXT("Torch flame follows light state"),
        Vfx.ActivationMode == EGridItemVfxActivationMode::WhenLightEnabled);
    TestTrue(TEXT("Torch Niagara reference preserved"),
        Vfx.NiagaraSystem.ToSoftObjectPath() ==
        Torch->LightEmitter.NiagaraSystem.ToSoftObjectPath());
    TestTrue(TEXT("Flame relative location retained"),
        Vfx.RelativeTransform.GetLocation().Equals(FVector(8.f, -4.f, 27.f)));
    TestTrue(TEXT("Flame rotation retained"),
        Vfx.RelativeTransform.Rotator().Equals(FRotator(9.f, 19.f, 29.f), 0.001f));

    TestTrue(TEXT("Legacy Niagara removed from light executor"), NewLight.NiagaraSystem.IsNull());
    TestTrue(TEXT("Torch remains a point-light emitter"), NewLight.HasEmitter() && NewLight.bUsePointLight);
    TestTrue(TEXT("Default-on state preserved"), NewLight.bDefaultEnabled);
    TestTrue(TEXT("Point-light intensity preserved"), FMath::IsNearlyEqual(NewLight.LightIntensity, 769.f));
    TestTrue(TEXT("Point-light radius preserved"), FMath::IsNearlyEqual(NewLight.LightRadius, 386.f));
    TestTrue(TEXT("Point-light color preserved"),
        NewLight.LightColor.Equals(FLinearColor(0.92f, 0.4f, 0.17f)));
    TestTrue(TEXT("Flicker configuration preserved"),
        NewLight.bEnableLightFlicker && FMath::IsNearlyEqual(NewLight.FlickerIntensityAmount, 46.f));

    // Repeated authoring never adds a second flame or modifies the already
    // migrated definition.
    Torch->VisualEffects = NewVisuals;
    Torch->LightEmitter = NewLight;
    bWouldChange = true;
    if (!TestTrue(TEXT("A migrated torch passes an idempotent second migration"),
        FGridItemTorchVfxMigration::BuildTarget(
            *Torch, Flame, NewVisuals, NewLight, bWouldChange, Error)))
    {
        AddError(Error);
        return false;
    }
    TestFalse(TEXT("Second migration does not write"), bWouldChange);
    TestEqual(TEXT("No duplicate flame"), NewVisuals.Effects.Num(), 1);

    // Fail closed: never synthesize an arbitrary flame offset or override
    // an unrelated system or incompatible authored visual.
    Torch->VisualEffects.Effects.Reset();
    TestFalse(TEXT("Missing old and new flame is refused"),
        FGridItemTorchVfxMigration::BuildTarget(
            *Torch, Flame, NewVisuals, NewLight, bWouldChange, Error));

    Torch->LightEmitter.NiagaraSystem = NewObject<UNiagaraSystem>(GetTransientPackage());
    TestFalse(TEXT("Unrelated legacy Niagara is refused"),
        FGridItemTorchVfxMigration::BuildTarget(
            *Torch, Flame, NewVisuals, NewLight, bWouldChange, Error));

    Torch->LightEmitter.NiagaraSystem = Flame;
    Torch->LightEmitter.bUsePointLight = false;
    TestFalse(TEXT("A torch without a point light is refused"),
        FGridItemTorchVfxMigration::BuildTarget(
            *Torch, Flame, NewVisuals, NewLight, bWouldChange, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013ProductionPreflightTest,
    "Grimrock.Items.VFX01_3.ProductionPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013ProductionPreflightTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FString Error;
    if (!TestTrue(TEXT("Production torch migrates or is already migrated without package writes"),
        FGridItemTorchVfxMigration::Run(false, Error)))
    {
        AddError(Error);
        return false;
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013MigratedProductionTest,
    "Grimrock.Items.VFX01_3.MigratedProductionAsset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013MigratedProductionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGridItemDefinitionAsset* Torch = LoadObject<UGridItemDefinitionAsset>(
        nullptr, GridItemVFX013Tests::TorchPath);
    if (!TestNotNull(TEXT("Production torch is present"), Torch))
    {
        return false;
    }

    TestTrue(TEXT("Production torch definition stays valid"), Torch->IsValidDefinition());
    TestTrue(TEXT("Torch still owns its point light"), Torch->LightEmitter.bUsePointLight);
    TestTrue(TEXT("Torch retains its default lit state"), Torch->IsLightEnabledByDefault());
    TestTrue(TEXT("Production point light has a positive intensity and radius"),
        Torch->LightEmitter.LightIntensity > 0.f && Torch->LightEmitter.LightRadius > 0.f);
    TestTrue(TEXT("Legacy flame is absent from the light emitter"),
        Torch->LightEmitter.NiagaraSystem.IsNull());

    const FGridItemNiagaraEffectConfig* MigratedFlame = Torch->VisualEffects.Effects.FindByPredicate(
        [](const FGridItemNiagaraEffectConfig& Effect)
        {
            return Effect.EffectId == FName(TEXT("TorchFlame"));
        });
    if (!TestNotNull(TEXT("TorchFlame is authored in the independent VFX list"), MigratedFlame))
    {
        return false;
    }

    TestTrue(TEXT("Production VFX references NS_Flame_8_Torch"),
        MigratedFlame->NiagaraSystem.ToSoftObjectPath() ==
        FSoftObjectPath(GridItemVFX013Tests::FlamePath));
    TestTrue(TEXT("The torch flame is conditional on light state"),
        MigratedFlame->ActivationMode == EGridItemVfxActivationMode::WhenLightEnabled);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
