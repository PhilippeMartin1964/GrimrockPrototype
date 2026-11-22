#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"

namespace GridItemVFX012AuthoringTests
{
    constexpr TCHAR SparklePath[] =
        TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle");
    const FName HealingId(TEXT("Item_Potion_Healing_01"));
    const FName FlaskIds[] =
    {
        TEXT("Item_Flask_Oil"),
        TEXT("Item_Flask_Acid"),
        TEXT("Item_Flask_CorrosiveCloud")
    };

    const FName SparkleId(TEXT("LiquidSparkle"));

    bool IsSparkle(const FGridItemNiagaraEffectConfig& Effect)
    {
        return Effect.EffectId == SparkleId ||
            Effect.NiagaraSystem.ToSoftObjectPath() == FSoftObjectPath(SparklePath);
    }

    const FGridItemNiagaraEffectConfig* FindSparkle(const FGridItemVisualEffectsConfig& VisualEffects)
    {
        return VisualEffects.Effects.FindByPredicate(
            [](const FGridItemNiagaraEffectConfig& Effect) { return IsSparkle(Effect); });
    }

    struct FCanonicalFixture
    {
        TStrongObjectPtr<UGameInstance> GI;
        TStrongObjectPtr<UGridItemSubsystem> Registry;
        TStrongObjectPtr<UGridItemSystemAsset> Root;

        FCanonicalFixture()
            : GI(NewObject<UGameInstance>()),
              Registry(NewObject<UGridItemSubsystem>(GI.Get())),
              Root(LoadObject<UGridItemSystemAsset>(nullptr,
                  TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")))
        {
        }

        bool Configure()
        {
            TArray<FString> Errors;
            return Registry.IsValid() && Root.IsValid() &&
                Registry->RegisterItemSystem(Root.Get(), Errors);
        }

        bool Resolve(FName ItemId, FGridResolvedItem& OutItem) const
        {
            return Registry.IsValid() && Registry->ResolveItem(ItemId, 0, OutItem);
        }
    };


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
    using namespace GridItemVFX012AuthoringTests;

    FCanonicalFixture Source;
    if (!TestTrue(TEXT("Production ItemSystem registers"), Source.Configure()))
        return false;
    FGridResolvedItem Healing;
    if (!TestTrue(TEXT("Healing potion resolves canonically at N0"),
        Source.Resolve(HealingId, Healing))) return false;
    TestTrue(TEXT("Canonical healing potion has valid identity"), Healing.IsValid());
    TestNotNull(TEXT("Only healing potion authors LiquidSparkle"),
        FindSparkle(Healing.VisualEffects));

    for (FName Id : FlaskIds)
    {
        FGridResolvedItem Flask;
        if (!TestTrue(TEXT("Canonical flask resolves without DA_Item source"),
            Source.Resolve(Id, Flask))) continue;
        TestTrue(TEXT("Flask remains valid"), Flask.IsValid());
        TestNull(TEXT("Flask does not author LiquidSparkle"),
            FindSparkle(Flask.VisualEffects));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012ProductionVfxContractTest,
    "Grimrock.Items.VFX01_2.MigratedProductionAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012ProductionVfxContractTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace GridItemVFX012AuthoringTests;

    FCanonicalFixture Source;
    if (!TestTrue(TEXT("Production ItemSystem registers"), Source.Configure()))
        return false;
    FGridResolvedItem Healing;
    if (!TestTrue(TEXT("Healing N0 is canonical"), Source.Resolve(HealingId, Healing)))
        return false;
    const FGridItemNiagaraEffectConfig* Sparkle = FindSparkle(Healing.VisualEffects);
    if (!TestNotNull(TEXT("Canonical LiquidSparkle effect exists"), Sparkle))
        return false;
    TestEqual(TEXT("Only expected effect ID"), Sparkle->EffectId, SparkleId);
    TestEqual(TEXT("Canonical Niagara system"),
        Sparkle->NiagaraSystem.ToSoftObjectPath(), FSoftObjectPath(SparklePath));
    TestTrue(TEXT("Visual sparkle ignores instance light state"),
        Sparkle->ActivationMode == EGridItemVfxActivationMode::Always);
    int32 SparkleCount = 0;
    for (const FGridItemNiagaraEffectConfig& Entry : Healing.VisualEffects.Effects)
        if (IsSparkle(Entry)) ++SparkleCount;
    TestEqual(TEXT("Only one canonical healing sparkle"), SparkleCount, 1);
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
