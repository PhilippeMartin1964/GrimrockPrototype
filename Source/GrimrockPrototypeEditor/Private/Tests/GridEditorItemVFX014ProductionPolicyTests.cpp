#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"
#include "Runtime/Items/GridItemDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace GridItemVFX014Policy
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

    bool HasSparkle(const FGridItemVisualEffectsConfig& VisualEffects)
    {
        return VisualEffects.Effects.ContainsByPredicate(
            [](const FGridItemNiagaraEffectConfig& Effect)
            {
                return Effect.EffectId == FName(TEXT("LiquidSparkle")) ||
                    Effect.NiagaraSystem.ToSoftObjectPath() == FSoftObjectPath(SparklePath);
            });
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
                  TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")))
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX014FlaskVisualPolicyTest,
    "Grimrock.Items.VFX01_4A.FlaskVisualPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX014FlaskVisualPolicyTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UNiagaraSystem* Sparkle = LoadObject<UNiagaraSystem>(
        nullptr, GridItemVFX014Policy::SparklePath);
    if (!TestNotNull(TEXT("Canonical sparkle Niagara loads"), Sparkle))
    {
        return false;
    }

    UGridNonArmamentDefinitionAsset* CleanFlask = NewObject<UGridNonArmamentDefinitionAsset>();
    CleanFlask->ItemId = TEXT("VFX014_CleanFlask");

    TestTrue(TEXT("An empty flask VFX list is valid"), CleanFlask->VisualEffects.IsValid());
    TestFalse(TEXT("No sparkle is automatically added to a clean flask"),
        GridItemVFX014Policy::HasSparkle(CleanFlask->VisualEffects));

    UNiagaraSystem* Other = NewObject<UNiagaraSystem>(GetTransientPackage());
    FGridItemNiagaraEffectConfig OtherVfx;
    OtherVfx.EffectId = TEXT("OtherFlaskEffect");
    OtherVfx.NiagaraSystem = Other;
    CleanFlask->VisualEffects.Effects.Add(OtherVfx);

    TestTrue(TEXT("An unrelated VFX remains valid without LiquidSparkle"),
        CleanFlask->VisualEffects.IsValid() &&
        !GridItemVFX014Policy::HasSparkle(CleanFlask->VisualEffects));

    FGridItemNiagaraEffectConfig SparkleVfx;
    SparkleVfx.EffectId = TEXT("LiquidSparkle");
    SparkleVfx.NiagaraSystem = Sparkle;
    CleanFlask->VisualEffects.Effects.Add(SparkleVfx);
    TestTrue(TEXT("Test detects unintended sparkle in another item"),
        GridItemVFX014Policy::HasSparkle(CleanFlask->VisualEffects));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX014ProductionPolicyAuditTest,
    "Grimrock.Items.VFX01_4A.ProductionAssetAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX014ProductionPolicyAuditTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace GridItemVFX014Policy;

    FCanonicalFixture Source;
    if (!TestTrue(TEXT("Production ItemSystem registers"), Source.Configure()))
        return false;

    FGridResolvedItem Healing;
    if (!TestTrue(TEXT("Healing potion resolves at canonical N0"),
        Source.Resolve(HealingId, Healing))) return false;
    TestTrue(TEXT("Only healing potion owns authored LiquidSparkle"),
        HasSparkle(Healing.VisualEffects));
    for (FName Id : FlaskIds)
    {
        FGridResolvedItem Flask;
        if (!TestTrue(TEXT("Canonical flask resolves at N0"),
            Source.Resolve(Id, Flask))) continue;
        TestFalse(TEXT("Flask does not borrow healing sparkle"), HasSparkle(Flask.VisualEffects));
    }

    // Inspect all 48 identities from the root, including non-ARM N0 and
    // ARM N1. No AssetRegistry walk of obsolete UGridNonArmamentDefinitionAsset.
    TArray<FName> Ids;
    Source.Registry->GetItemIds(Ids);
    TestEqual(TEXT("All native production items are audited"), Ids.Num(), 48);
    int32 Checked = 0;
    int32 Invalid = 0;
    for (FName Id : Ids)
    {
        FGridResolvedItem Item;
        if (!Source.Registry->ResolveItem(Id, 0, Item) &&
            !Source.Registry->ResolveItem(Id, 1, Item))
        {
            ++Invalid;
            AddError(FString::Printf(TEXT("Canonical item not resolved: %s"), *Id.ToString()));
            continue;
        }
        ++Checked;
        if (!Item.IsValid())
        {
            ++Invalid;
            AddError(FString::Printf(TEXT("Invalid canonical item: %s"), *Id.ToString()));
        }
        if (Id != HealingId && HasSparkle(Item.VisualEffects))
        {
            ++Invalid;
            AddError(FString::Printf(TEXT("LiquidSparkle reserved for healing potion: %s"),
                *Id.ToString()));
        }
    }
    TestEqual(TEXT("Every canonical identity was resolved"), Checked, 48);
    TestEqual(TEXT("No invalid canonical item or misplaced LiquidSparkle"), Invalid, 0);
    TestNull(TEXT("Legacy NiagaraSystem property is absent from LightEmitter"),
        FindFProperty<FProperty>(FGridLightEmitterConfig::StaticStruct(), TEXT("NiagaraSystem")));
    TestNull(TEXT("Legacy NiagaraRelativeLocation property is absent"),
        FindFProperty<FProperty>(FGridLightEmitterConfig::StaticStruct(), TEXT("NiagaraRelativeLocation")));
    TestNull(TEXT("Legacy NiagaraRelativeRotation property is absent"),
        FindFProperty<FProperty>(FGridLightEmitterConfig::StaticStruct(), TEXT("NiagaraRelativeRotation")));
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
