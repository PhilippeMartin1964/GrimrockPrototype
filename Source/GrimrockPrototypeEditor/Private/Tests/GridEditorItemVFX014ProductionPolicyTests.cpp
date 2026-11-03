#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace GridItemVFX014Policy
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

    bool HasSparkle(const UGridItemDefinitionAsset& Item)
    {
        return Item.VisualEffects.Effects.ContainsByPredicate(
            [](const FGridItemNiagaraEffectConfig& Effect)
            {
                return Effect.EffectId == FName(TEXT("LiquidSparkle")) ||
                    Effect.NiagaraSystem.ToSoftObjectPath() == FSoftObjectPath(SparklePath);
            });
    }
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

    UGridItemDefinitionAsset* CleanFlask = NewObject<UGridItemDefinitionAsset>();
    CleanFlask->ItemDefinitionId = TEXT("VFX014_CleanFlask");
    CleanFlask->ItemType = EGridItemType::Potion;

    TestTrue(TEXT("An empty flask VFX list is valid"), CleanFlask->VisualEffects.IsValid());
    TestFalse(TEXT("No sparkle is automatically added to a clean flask"),
        GridItemVFX014Policy::HasSparkle(*CleanFlask));

    UNiagaraSystem* Other = NewObject<UNiagaraSystem>(GetTransientPackage());
    FGridItemNiagaraEffectConfig OtherVfx;
    OtherVfx.EffectId = TEXT("OtherFlaskEffect");
    OtherVfx.NiagaraSystem = Other;
    CleanFlask->VisualEffects.Effects.Add(OtherVfx);

    TestTrue(TEXT("An unrelated VFX remains valid without LiquidSparkle"),
        CleanFlask->VisualEffects.IsValid() &&
        !GridItemVFX014Policy::HasSparkle(*CleanFlask));

    FGridItemNiagaraEffectConfig SparkleVfx;
    SparkleVfx.EffectId = TEXT("LiquidSparkle");
    SparkleVfx.NiagaraSystem = Sparkle;
    CleanFlask->VisualEffects.Effects.Add(SparkleVfx);
    TestTrue(TEXT("Test detects unintended sparkle in another item"),
        GridItemVFX014Policy::HasSparkle(*CleanFlask));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX014ProductionPolicyAuditTest,
    "Grimrock.Items.VFX01_4A.ProductionAssetAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX014ProductionPolicyAuditTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGridItemDefinitionAsset* Healing = LoadObject<UGridItemDefinitionAsset>(
        nullptr, GridItemVFX014Policy::HealingPath);
    if (!TestNotNull(TEXT("Healing potion loads"), Healing))
    {
        return false;
    }
    TestTrue(TEXT("Healing potion is the canonical LiquidSparkle owner"),
        GridItemVFX014Policy::HasSparkle(*Healing));

    for (const TCHAR* Path : GridItemVFX014Policy::FlaskPaths)
    {
        UGridItemDefinitionAsset* Flask = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
        if (!TestNotNull(*FString::Printf(TEXT("Flask %s loads"), Path), Flask))
        {
            continue;
        }
        TestTrue(TEXT("Flask definition remains valid"), Flask->IsValidDefinition());
        TestFalse(*FString::Printf(TEXT("No LiquidSparkle on %s"), Path),
            GridItemVFX014Policy::HasSparkle(*Flask));
    }

    // Re-scan production assets rather than trusting Git LFS pointer texts.
    FAssetRegistryModule& Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& Registry = Module.Get();
    Registry.SearchAllAssets(true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByClass(UGridItemDefinitionAsset::StaticClass()->GetClassPathName(),
        Assets, true);
    if (!TestTrue(TEXT("Production registry contains item definitions"), !Assets.IsEmpty()))
    {
        return false;
    }

    int32 Checked = 0;
    int32 Invalid = 0;
    for (const FAssetData& Data : Assets)
    {
        UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(Data.GetAsset());
        if (!IsValid(Item))
        {
            ++Invalid;
            AddError(FString::Printf(TEXT("Could not load item asset: %s"),
                *Data.PackageName.ToString()));
            continue;
        }
        ++Checked;
        if (!Item->IsValidDefinition())
        {
            ++Invalid;
            AddError(FString::Printf(TEXT("Invalid item definition: %s"),
                *Item->GetPathName()));
        }
        if (Item != Healing && GridItemVFX014Policy::HasSparkle(*Item))
        {
            ++Invalid;
            AddError(FString::Printf(TEXT("LiquidSparkle is reserved for healing potion: %s"),
                *Item->GetPathName()));
        }
    }

    TestTrue(TEXT("At least the 5 canonical assets were audited"), Checked >= 5);
    TestEqual(TEXT("No invalid production item or misplaced LiquidSparkle"), Invalid, 0);
    TestNull(TEXT("Legacy NiagaraSystem UPROPERTY is absent from LightEmitter"),
        FindFProperty<FProperty>(FGridLightEmitterConfig::StaticStruct(), TEXT("NiagaraSystem")));
    TestNull(TEXT("Legacy NiagaraRelativeLocation UPROPERTY is absent"),
        FindFProperty<FProperty>(FGridLightEmitterConfig::StaticStruct(), TEXT("NiagaraRelativeLocation")));
    TestNull(TEXT("Legacy NiagaraRelativeRotation UPROPERTY is absent"),
        FindFProperty<FProperty>(FGridLightEmitterConfig::StaticStruct(), TEXT("NiagaraRelativeRotation")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
