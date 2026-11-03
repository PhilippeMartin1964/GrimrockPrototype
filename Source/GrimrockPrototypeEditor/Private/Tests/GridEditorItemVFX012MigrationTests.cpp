#if WITH_DEV_AUTOMATION_TESTS

#include "Items/GridItemLiquidVfxMigrationCommandlet.h"

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace GridItemVFX012MigrationTests
{
    UNiagaraSystem* LoadSparkle()
    {
        return LoadObject<UNiagaraSystem>(nullptr,
            TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012MigrationRecipeTest,
    "Grimrock.Items.VFX01_2.MigrationRecipe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012MigrationRecipeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UNiagaraSystem* Sparkle = GridItemVFX012MigrationTests::LoadSparkle();
    if (!TestNotNull(TEXT("Canonical liquid sparkle system loads"), Sparkle))
    {
        return false;
    }

    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>();
    Legacy->ItemDefinitionId = TEXT("VFX012_LegacyPotion");
    Legacy->ItemType = EGridItemType::Potion;
    Legacy->bStackable = true;
    Legacy->MaxStackSize = 10;

    Legacy->LightEmitter.bDefaultEnabled = true;
    Legacy->LightEmitter.bUsePointLight = true;
    Legacy->LightEmitter.LightIntensity = 432.f;
    Legacy->LightEmitter.NiagaraSystem = Sparkle;
    Legacy->LightEmitter.NiagaraRelativeLocation = FVector(5.f, 6.f, 7.f);
    Legacy->LightEmitter.NiagaraRelativeRotation = FRotator(0.f, 27.f, 0.f);

    FGridItemVisualEffectsConfig Visuals;
    FGridLightEmitterConfig Light;
    bool bWouldChange = false;
    FString Error;

    if (!TestTrue(TEXT("Legacy light-embedded sparkle can migrate"),
        FGridItemLiquidVfxMigration::BuildTarget(
            *Legacy, Sparkle, Visuals, Light, bWouldChange, Error)))
    {
        AddError(Error);
        return false;
    }

    TestTrue(TEXT("Migration identifies a change"), bWouldChange);
    TestEqual(TEXT("Exactly one new visual effect"), Visuals.Effects.Num(), 1);
    if (Visuals.Effects.IsEmpty())
    {
        return false;
    }

    TestEqual(TEXT("Named visual effect"), Visuals.Effects[0].EffectId, FName(TEXT("LiquidSparkle")));
    TestTrue(TEXT("Permanent effect is independent of light"),
        Visuals.Effects[0].ActivationMode == EGridItemVfxActivationMode::Always);
    TestTrue(TEXT("Original Niagara asset preserved"),
        Visuals.Effects[0].NiagaraSystem.ToSoftObjectPath() ==
        Legacy->LightEmitter.NiagaraSystem.ToSoftObjectPath());
    TestTrue(TEXT("Legacy position preserved"),
        Visuals.Effects[0].RelativeTransform.GetTranslation().Equals(FVector(5.f, 6.f, 7.f)));
    TestTrue(TEXT("Legacy rotation preserved"),
        Visuals.Effects[0].RelativeTransform.Rotator().Equals(FRotator(0.f, 27.f, 0.f), 0.001f));

    TestTrue(TEXT("Legacy Niagara reference removed from light"),
        Light.NiagaraSystem.IsNull());
    TestTrue(TEXT("Existing authored point light preserved"),
        Light.bUsePointLight && FMath::IsNearlyEqual(Light.LightIntensity, 432.f));
    TestTrue(TEXT("Light default remains a gameplay state"), Light.bDefaultEnabled);

    Legacy->VisualEffects = Visuals;
    Legacy->LightEmitter = Light;

    bWouldChange = true;
    if (!TestTrue(TEXT("Reapplying already migrated asset succeeds"),
        FGridItemLiquidVfxMigration::BuildTarget(
            *Legacy, Sparkle, Visuals, Light, bWouldChange, Error)))
    {
        AddError(Error);
        return false;
    }
    TestFalse(TEXT("Reapplying migration is idempotent"), bWouldChange);
    TestEqual(TEXT("Reapplying migration does not duplicate effect"), Visuals.Effects.Num(), 1);

    UGridItemDefinitionAsset* FreshHealingPotion = NewObject<UGridItemDefinitionAsset>();
    FreshHealingPotion->ItemDefinitionId = TEXT("VFX012_FreshHealingPotion");
    FreshHealingPotion->ItemType = EGridItemType::Potion;
    if (!TestTrue(TEXT("A newly authored healing potion without legacy Niagara obtains permanent bubbles"),
        FGridItemLiquidVfxMigration::BuildTarget(
            *FreshHealingPotion, Sparkle, Visuals, Light, bWouldChange, Error)))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("Fresh healing potion needs an authored effect"), bWouldChange);
    TestEqual(TEXT("Fresh healing potion has one effect"), Visuals.Effects.Num(), 1);
    TestTrue(TEXT("Fresh healing potion has no light emitter"), !Light.HasEmitter());

    // A different system in the legacy light slot must not be silently replaced.
    UNiagaraSystem* Unrelated = NewObject<UNiagaraSystem>(GetTransientPackage());
    FreshHealingPotion->LightEmitter.NiagaraSystem = Unrelated;
    TestFalse(TEXT("Unrelated Niagara systems must be reviewed manually"),
        FGridItemLiquidVfxMigration::BuildTarget(
            *FreshHealingPotion, Sparkle, Visuals, Light, bWouldChange, Error));
    TestFalse(TEXT("Refusal explains the problem"), Error.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012ProductionPreflightTest,
    "Grimrock.Items.VFX01_2.ProductionPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012ProductionPreflightTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FString Error;
    if (!TestTrue(TEXT("Only the healing potion is in the canonical migration allowlist"),
        FGridItemLiquidVfxMigration::Run(false, Error)))
    {
        AddError(Error);
        return false;
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012MigratedProductionTest,
    "Grimrock.Items.VFX01_2.MigratedProductionAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012MigratedProductionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const TCHAR* Paths[] =
    {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.DA_Item_Potion_Healing_01")
    };

    const FSoftObjectPath ExpectedSparkle(
        TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle"));

    for (const TCHAR* Path : Paths)
    {
        UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
        if (!TestNotNull(*FString::Printf(TEXT("Authored item %s"), Path), Item))
        {
            continue;
        }

        TestTrue(*FString::Printf(TEXT("Authored %s remains valid"), Path), Item->IsValidDefinition());
        TestTrue(*FString::Printf(TEXT("Old light Niagara removed for %s"), Path),
            Item->LightEmitter.NiagaraSystem.IsNull());

        const FGridItemNiagaraEffectConfig* Liquid = Item->VisualEffects.Effects.FindByPredicate(
            [](const FGridItemNiagaraEffectConfig& Entry)
            {
                return Entry.EffectId == FName(TEXT("LiquidSparkle"));
            });

        if (!TestNotNull(*FString::Printf(TEXT("Migrated bubbles on %s"), Path), Liquid))
        {
            continue;
        }

        TestTrue(TEXT("Canonical Niagara system referenced"),
            Liquid->NiagaraSystem.ToSoftObjectPath() == ExpectedSparkle);
        TestTrue(TEXT("Liquid particles do not depend on bLightsEnabled"),
            Liquid->ActivationMode == EGridItemVfxActivationMode::Always);
    }
    return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
