#if WITH_DEV_AUTOMATION_TESTS

#include "Items/GridItemVfx014AuditCommandlet.h"

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace ItemVFX014Tests
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX014CleanupRecipeTest,
    "Grimrock.Items.VFX01_4A.FlaskCleanupRecipe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX014CleanupRecipeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UNiagaraSystem* Sparkle = LoadObject<UNiagaraSystem>(nullptr, ItemVFX014Tests::SparklePath);
    if (!TestNotNull(TEXT("Production sparkle effect loads"), Sparkle))
    {
        return false;
    }

    UGridItemDefinitionAsset* Flask = NewObject<UGridItemDefinitionAsset>();
    Flask->ItemDefinitionId = TEXT("ITEM_VFX014_TestFlask");
    Flask->ItemType = EGridItemType::Potion;

    FGridItemVisualEffectsConfig Cleaned;
    bool bChanged = true;
    FString Error;
    TestTrue(TEXT("A flask already without sparkle is accepted and preserved"),
        FGridItemVfx014Audit::BuildFlaskCleanup(*Flask, Sparkle, Cleaned, bChanged, Error));
    TestFalse(TEXT("User's already-clean oil flask remains unmodified"), bChanged);
    TestTrue(TEXT("No new visual effects added"), Cleaned.Effects.IsEmpty());

    FGridItemNiagaraEffectConfig SparkleEntry;
    SparkleEntry.EffectId = TEXT("LiquidSparkle");
    SparkleEntry.NiagaraSystem = Sparkle;
    SparkleEntry.ActivationMode = EGridItemVfxActivationMode::Always;
    Flask->VisualEffects.Effects.Add(SparkleEntry);

    UNiagaraSystem* OtherSystem = NewObject<UNiagaraSystem>(GetTransientPackage());
    FGridItemNiagaraEffectConfig OtherEffect;
    OtherEffect.EffectId = TEXT("OtherFlaskVfx");
    OtherEffect.NiagaraSystem = OtherSystem;
    OtherEffect.ActivationMode = EGridItemVfxActivationMode::WhenLightEnabled;
    Flask->VisualEffects.Effects.Add(OtherEffect);

    if (!TestTrue(TEXT("Removes only canonical LiquidSparkle"),
        FGridItemVfx014Audit::BuildFlaskCleanup(*Flask, Sparkle, Cleaned, bChanged, Error)))
    {
        AddError(Error);
        return false;
    }

    TestTrue(TEXT("Normalization reports a pending edit"), bChanged);
    TestEqual(TEXT("Unrelated authored VFX remains"), Cleaned.Effects.Num(), 1);
    if (Cleaned.Effects.Num() == 1)
    {
        TestEqual(TEXT("Unrelated effect ID preserved"), Cleaned.Effects[0].EffectId,
            FName(TEXT("OtherFlaskVfx")));
        TestTrue(TEXT("Unrelated VFX system preserved"),
            Cleaned.Effects[0].NiagaraSystem.ToSoftObjectPath() ==
            OtherEffect.NiagaraSystem.ToSoftObjectPath());
    }

    Flask->VisualEffects = Cleaned;
    TestTrue(TEXT("Normalization is idempotent"),
        FGridItemVfx014Audit::BuildFlaskCleanup(*Flask, Sparkle, Cleaned, bChanged, Error));
    TestFalse(TEXT("Second normalization writes nothing"), bChanged);

    // Never silently remove another system under LiquidSparkle's ID.
    Flask->VisualEffects.Effects[0].EffectId = TEXT("LiquidSparkle");
    TestFalse(TEXT("Misnamed unrelated effect is a blocker"),
        FGridItemVfx014Audit::BuildFlaskCleanup(*Flask, Sparkle, Cleaned, bChanged, Error));
    TestFalse(TEXT("Error explains the refused mismatch"), Error.IsEmpty());

    // Nor remove the canonical system under an unrelated user-authored ID.
    Flask->VisualEffects.Effects[0].EffectId = TEXT("OtherFlaskVfx");
    Flask->VisualEffects.Effects[0].NiagaraSystem = Sparkle;
    TestFalse(TEXT("Different ID on sparkle is a blocker"),
        FGridItemVfx014Audit::BuildFlaskCleanup(*Flask, Sparkle, Cleaned, bChanged, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX014ProductionAuditTest,
    "Grimrock.Items.VFX01_4A.ProductionAssetAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX014ProductionAuditTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UGridItemDefinitionAsset* Healing = LoadObject<UGridItemDefinitionAsset>(
        nullptr, ItemVFX014Tests::HealingPath);
    if (!TestNotNull(TEXT("Healing potion is available"), Healing))
    {
        return false;
    }

    const FSoftObjectPath Canonical(ItemVFX014Tests::SparklePath);
    bool bFoundHealingSparkle = false;
    for (const FGridItemNiagaraEffectConfig& Effect : Healing->VisualEffects.Effects)
    {
        if (Effect.EffectId == FName(TEXT("LiquidSparkle")) &&
            Effect.NiagaraSystem.ToSoftObjectPath() == Canonical &&
            Effect.ActivationMode == EGridItemVfxActivationMode::Always)
        {
            bFoundHealingSparkle = true;
        }
    }
    TestTrue(TEXT("Healing potion remains the only canonical sparkle owner"), bFoundHealingSparkle);

    for (const TCHAR* Path : ItemVFX014Tests::FlaskPaths)
    {
        UGridItemDefinitionAsset* Flask = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
        if (!TestNotNull(*FString::Printf(TEXT("Flask %s loads"), Path), Flask))
        {
            continue;
        }

        TestTrue(TEXT("Flask still passes data validation"), Flask->IsValidDefinition());
        TestTrue(TEXT("No legacy Niagara reference on flask"), Flask->LightEmitter.NiagaraSystem.IsNull());
        for (const FGridItemNiagaraEffectConfig& Effect : Flask->VisualEffects.Effects)
        {
            TestTrue(*FString::Printf(TEXT("No accidental sparkle on %s"), Path),
                Effect.EffectId != FName(TEXT("LiquidSparkle")) &&
                Effect.NiagaraSystem.ToSoftObjectPath() != Canonical);
        }
    }

    int32 ScannedCount = 0;
    FString Error;
    if (!TestTrue(TEXT("All item definitions were audited without legacy references"),
        FGridItemVfx014Audit::ScanProduction(false, ScannedCount, Error)))
    {
        AddError(Error);
        return false;
    }
    TestTrue(TEXT("Asset registry scan examined production definitions"), ScannedCount >= 5);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
