#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace GridItemVFX012AuthoringTests
{
    constexpr TCHAR SparklePath[] =
        TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle");
    constexpr TCHAR HealingPath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.DA_Item_Potion_Healing_01");
    constexpr const TCHAR* FlaskPaths[] =
    {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Acid.DA_Item_Flask_Acid"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_CorrosiveCloud.DA_Item_Flask_CorrosiveCloud")
    };

    const FName SparkleId(TEXT("LiquidSparkle"));

    bool IsSparkle(const FGridItemNiagaraEffectConfig& Effect)
    {
        return Effect.EffectId == SparkleId ||
            Effect.NiagaraSystem.ToSoftObjectPath() == FSoftObjectPath(SparklePath);
    }

    const FGridItemNiagaraEffectConfig* FindSparkle(const UGridItemDefinitionAsset& Definition)
    {
        return Definition.VisualEffects.Effects.FindByPredicate(
            [](const FGridItemNiagaraEffectConfig& Effect) { return IsSparkle(Effect); });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012VisualOnlyAuthoringTest,
    "Grimrock.Items.VFX01_2.VisualOnlyAuthoring",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012VisualOnlyAuthoringTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UNiagaraSystem* Sparkle = LoadObject<UNiagaraSystem>(
        nullptr, GridItemVFX012AuthoringTests::SparklePath);
    if (!TestNotNull(TEXT("Production LiquidSparkle Niagara asset loads"), Sparkle))
    {
        return false;
    }

    UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
    Definition->ItemDefinitionId = TEXT("VFX012_VisualOnlyHealingPotion");
    Definition->ItemType = EGridItemType::Potion;
    Definition->LightEmitter.bDefaultEnabled = true;
    TestFalse(TEXT("Default-enabled flag alone cannot create a light emitter"),
        Definition->HasLightEmitter());
    TestFalse(TEXT("A visual-only item does not become a light by default"),
        Definition->IsLightEnabledByDefault());

    FGridItemNiagaraEffectConfig Effect;
    Effect.EffectId = GridItemVFX012AuthoringTests::SparkleId;
    Effect.NiagaraSystem = Sparkle;
    Effect.ActivationMode = EGridItemVfxActivationMode::Always;
    Effect.RelativeTransform = FTransform(FRotator(0.f, 19.f, 0.f), FVector(5.f, 6.f, 7.f));
    Definition->VisualEffects.Effects.Add(Effect);

    TestTrue(TEXT("Healing sparkle is independent from PointLight"),
        Definition->IsValidDefinition() && !Definition->HasLightEmitter());
    TestTrue(TEXT("Niagara transform stays in VisualEffects"),
        Definition->VisualEffects.Effects[0].RelativeTransform.Equals(Effect.RelativeTransform));
    TestTrue(TEXT("LiquidSparkle is permanently visible even when unlit"),
        Definition->VisualEffects.Effects[0].ActivationMode == EGridItemVfxActivationMode::Always);

    Definition->VisualEffects.Effects.Add(Effect);
    TestFalse(TEXT("Duplicate LiquidSparkle IDs are rejected"), Definition->VisualEffects.IsValid());
    Definition->VisualEffects.Effects.Pop();
    TestTrue(TEXT("Removing duplicate restores validity"), Definition->IsValidDefinition());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012ProductionPreflightTest,
    "Grimrock.Items.VFX01_2.ProductionPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012ProductionPreflightTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGridItemDefinitionAsset* Healing = LoadObject<UGridItemDefinitionAsset>(
        nullptr, GridItemVFX012AuthoringTests::HealingPath);
    if (!TestNotNull(TEXT("Healing potion DataAsset loads"), Healing))
    {
        return false;
    }

    TestTrue(TEXT("Healing potion is a valid definition"), Healing->IsValidDefinition());
    const FGridItemNiagaraEffectConfig* Effect = GridItemVFX012AuthoringTests::FindSparkle(*Healing);
    TestNotNull(TEXT("Only healing potion authors LiquidSparkle"), Effect);

    for (const TCHAR* Path : GridItemVFX012AuthoringTests::FlaskPaths)
    {
        UGridItemDefinitionAsset* Flask = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
        if (!TestNotNull(*FString::Printf(TEXT("Flask %s loads"), Path), Flask))
        {
            continue;
        }
        TestTrue(*FString::Printf(TEXT("Flask %s remains valid"), Path), Flask->IsValidDefinition());
        TestNull(*FString::Printf(TEXT("Flask %s does not author LiquidSparkle"), Path),
            GridItemVFX012AuthoringTests::FindSparkle(*Flask));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012ProductionVfxContractTest,
    "Grimrock.Items.VFX01_2.MigratedProductionAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012ProductionVfxContractTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGridItemDefinitionAsset* Healing = LoadObject<UGridItemDefinitionAsset>(
        nullptr, GridItemVFX012AuthoringTests::HealingPath);
    if (!TestNotNull(TEXT("Healing potion loads"), Healing))
    {
        return false;
    }

    const FGridItemNiagaraEffectConfig* Sparkle =
        GridItemVFX012AuthoringTests::FindSparkle(*Healing);
    if (!TestNotNull(TEXT("LiquidSparkle effect exists"), Sparkle))
    {
        return false;
    }

    TestEqual(TEXT("Only canonical effect ID is allowed"), Sparkle->EffectId,
        GridItemVFX012AuthoringTests::SparkleId);
    TestTrue(TEXT("Canonical Niagara system referenced"),
        Sparkle->NiagaraSystem.ToSoftObjectPath() ==
        FSoftObjectPath(GridItemVFX012AuthoringTests::SparklePath));
    TestTrue(TEXT("Visual sparkle ignores instance light state"),
        Sparkle->ActivationMode == EGridItemVfxActivationMode::Always);

    int32 SparkleCount = 0;
    for (const FGridItemNiagaraEffectConfig& Entry : Healing->VisualEffects.Effects)
    {
        if (GridItemVFX012AuthoringTests::IsSparkle(Entry))
        {
            ++SparkleCount;
        }
    }
    TestEqual(TEXT("Healing potion has exactly one canonical sparkle"), SparkleCount, 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
