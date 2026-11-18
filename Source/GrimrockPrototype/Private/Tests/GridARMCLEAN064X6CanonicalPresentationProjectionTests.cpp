#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X6CanonicalPresentation,
    "Grimrock.RPG.ARM.CLEAN06_4X6.CanonicalNonArmPresentationProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X6CanonicalPresentation::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real single ItemSystem"), Root.Get()) ||
        !TestTrue(TEXT("Exact 34/38 registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
    {
        return false;
    }
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(
        NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestNotNull(TEXT("Transient canonical resolver"), Registry.Get()) ||
        !TestTrue(TEXT("Single real ItemSystem registers"),
            Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors)
        {
            AddError(Error);
        }
        return false;
    }

    int32 NonArmCount = 0;
    int32 ArmCount = 0;
    int32 AmmoCount = 0;
    for (const UGridCanonicalItemDefinitionAsset* Definition :
        Root->ItemDefinitions)
    {
        if (!TestNotNull(TEXT("Canonical item definition"), Definition))
        {
            return false;
        }
        const bool bArm = Cast<UGridArmamentDefinitionAsset>(Definition) != nullptr;
        const int32 PhysicalLevel = bArm ? 1 : 0;
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Real physical item resolves"),
            Registry->ResolveItem(Definition->ItemId, PhysicalLevel, Item)) ||
            !TestTrue(TEXT("Resolved view remains valid"), Item.IsValid()) ||
            !TestEqual(TEXT("Exact resolved item identity"),
                Item.ItemId, Definition->ItemId))
        {
            return false;
        }

        if (bArm)
        {
            ++ArmCount;
            if (!TestFalse(TEXT("ARM does not borrow non-ARM point light"),
                Item.LightEmitter.bUsePointLight) ||
                !TestTrue(TEXT("ARM does not borrow non-ARM Niagara"),
                    Item.VisualEffects.Effects.IsEmpty()) ||
                !TestFalse(TEXT("ARM does not borrow non-ARM readable text"),
                    Item.bReadable) ||
                !TestFalse(TEXT("ARM has no non-ARM physical mass policy"),
                    Item.bUseItemWeightAsWorldPhysicsMass))
            {
                return false;
            }
            continue;
        }
        const UGridNonArmamentDefinitionAsset* Source =
            Cast<UGridNonArmamentDefinitionAsset>(Definition);
        if (!TestNotNull(TEXT("Every non-ARM has a concrete definition"), Source))
        {
            return false;
        }
        ++NonArmCount;
        if (Cast<UGridAmmoDefinitionAsset>(Definition)) { ++AmmoCount; }

        if (!TestEqual(TEXT("Default light state"),
                Item.LightEmitter.bDefaultEnabled, Source->LightEmitter.bDefaultEnabled) ||
            !TestEqual(TEXT("Point light enabled"), Item.LightEmitter.bUsePointLight,
                Source->LightEmitter.bUsePointLight) ||
            !TestEqual(TEXT("Point light intensity"), Item.LightEmitter.LightIntensity,
                Source->LightEmitter.LightIntensity) ||
            !TestEqual(TEXT("Point light radius"), Item.LightEmitter.LightRadius,
                Source->LightEmitter.LightRadius) ||
            !TestTrue(TEXT("Point light tint"),
                Item.LightEmitter.LightColor == Source->LightEmitter.LightColor) ||
            !TestEqual(TEXT("Light flicker"), Item.LightEmitter.bEnableLightFlicker,
                Source->LightEmitter.bEnableLightFlicker) ||
            !TestEqual(TEXT("World sparkle enabled"), Item.bEnableWorldSparkle,
                Source->bEnableWorldSparkle) ||
            !TestTrue(TEXT("World sparkle material"),
                Item.WorldSparkleMaterial == Source->WorldSparkleMaterial) ||
            !TestTrue(TEXT("World sparkle tint"),
                Item.WorldSparkleColor == Source->WorldSparkleColor) ||
            !TestEqual(TEXT("World sparkle intensity"), Item.WorldSparkleIntensity,
                Source->WorldSparkleIntensity) ||
            !TestEqual(TEXT("World sparkle speed"), Item.WorldSparkleSpeed,
                Source->WorldSparkleSpeed) ||
            !TestEqual(TEXT("World sparkle variation"), Item.WorldSparkleVariation,
                Source->WorldSparkleVariation) ||
            !TestEqual(TEXT("Readable capability"), Item.bReadable, Source->bReadable) ||
            !TestEqual(TEXT("Readable text"), Item.ReadText.ToString(),
                Source->ReadText.ToString()) ||
            !TestEqual(TEXT("World item mass policy"),
                Item.bUseItemWeightAsWorldPhysicsMass,
                Source->bUseItemWeightAsWorldPhysicsMass) ||
            !TestEqual(TEXT("World physics initial tilt"),
                Item.WorldPhysicsInitialTiltDegrees,
                Source->WorldPhysicsInitialTiltDegrees))
        {
            return false;
        }

        if (!TestEqual(TEXT("All Niagara definitions projected"),
            Item.VisualEffects.Effects.Num(), Source->VisualEffects.Effects.Num()))
        {
            return false;
        }
        for (int32 Index = 0; Index < Item.VisualEffects.Effects.Num(); ++Index)
        {
            const FGridItemNiagaraEffectConfig& Expected =
                Source->VisualEffects.Effects[Index];
            const FGridItemNiagaraEffectConfig& Actual =
                Item.VisualEffects.Effects[Index];
            if (!TestEqual(TEXT("Niagara effect identity"),
                    Actual.EffectId, Expected.EffectId) ||
                !TestTrue(TEXT("Niagara system soft path"),
                    Actual.NiagaraSystem == Expected.NiagaraSystem) ||
                !TestTrue(TEXT("Niagara local transform"),
                    Actual.RelativeTransform.Equals(Expected.RelativeTransform)) ||
                !TestEqual(TEXT("Niagara activation policy"),
                    Actual.ActivationMode, Expected.ActivationMode))
            {
                return false;
            }
        }

        // The returned presentation is a value projection, not an alias
        // to a mutable UObject and never a new serialized source.
        Item.LightEmitter.LightIntensity += 100.f;
        Item.VisualEffects.Effects.Reset();
        FGridResolvedItem Second;
        if (!TestTrue(TEXT("Repeated resolution succeeds"),
                Registry->ResolveItem(Definition->ItemId, 0, Second)) ||
            !TestEqual(TEXT("Transient light mutation does not affect registry"),
                Second.LightEmitter.LightIntensity,
                Source->LightEmitter.LightIntensity) ||
            !TestEqual(TEXT("Transient VFX mutation does not affect registry"),
                Second.VisualEffects.Effects.Num(),
                Source->VisualEffects.Effects.Num()))
        {
            return false;
        }
    }
    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X6] ReadOnly canonical presentation: %d ARM, "
        "%d non-ARM, %d ammo families, all Niagara, light, sparkle, "
        "text and physics values projected; no native assets written."),
        ArmCount, NonArmCount, AmmoCount));
    return TestEqual(TEXT("All real ARM item definitions"), ArmCount, 22) &&
        TestEqual(TEXT("All real non-ARM item definitions"), NonArmCount, Root->ItemDefinitions.Num() - 22) &&
        TestTrue(TEXT("Ammunition early-return path is covered"), AmmoCount > 0);
}
namespace
{
enum class EGridProjectionProbe : uint8
{
    Families,
    Resolve,
    Light,
    Niagara,
    SparkleAndPhysics
};

bool RunX6Probe(FAutomationTestBase& Test, EGridProjectionProbe Mode)
{
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem"));
    if (!Test.TestNotNull(TEXT("Real root"), Root) || !Test.TestTrue(TEXT("Exact 34/38 registry shape"),
        GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root))) { return false; }
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(
        NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Test.TestTrue(TEXT("Configure item resolver"),
        Registry->RegisterItemSystem(Root, Errors))) { return false; }
    int32 Arm = 0, NonArm = 0;
    for (const UGridCanonicalItemDefinitionAsset* Def : Root->ItemDefinitions)
    {
        if (!Test.TestNotNull(TEXT("Concrete definition"), Def)) { return false; }
        const bool bArm = Cast<UGridArmamentDefinitionAsset>(Def) != nullptr;
        if (bArm) {++Arm;} else {++NonArm;}
        if (Mode == EGridProjectionProbe::Families) {continue;}
        FGridResolvedItem Resolved;
        if (!Test.TestTrue(TEXT("Physical family resolves by ItemId"),
            Registry->ResolveItem(Def->ItemId, bArm ? 1 : 0, Resolved)))
        {
            return false;
        }
        if (Mode == EGridProjectionProbe::Resolve || bArm) {continue;}
        const UGridNonArmamentDefinitionAsset* Source =
            Cast<UGridNonArmamentDefinitionAsset>(Def);
        if (!Test.TestNotNull(TEXT("Nonarm concrete owner"), Source)) {return false;}
        if (Mode == EGridProjectionProbe::Light)
        {
            if (!Test.TestEqual(TEXT("Light emitter default"),
                    Resolved.LightEmitter.bDefaultEnabled,
                    Source->LightEmitter.bDefaultEnabled) ||
                !Test.TestEqual(TEXT("Point-light active"),
                    Resolved.LightEmitter.bUsePointLight,
                    Source->LightEmitter.bUsePointLight) ||
                !Test.TestEqual(TEXT("Point-light intensity"),
                    Resolved.LightEmitter.LightIntensity,
                    Source->LightEmitter.LightIntensity) ||
                !Test.TestEqual(TEXT("Point-light radius"),
                    Resolved.LightEmitter.LightRadius,
                    Source->LightEmitter.LightRadius) ||
                !Test.TestTrue(TEXT("Point-light tint"),
                    Resolved.LightEmitter.LightColor ==
                        Source->LightEmitter.LightColor))
            {
                return false;
            }
        }
        if (Mode == EGridProjectionProbe::Niagara)
        {
            if (!Test.TestEqual(TEXT("Niagara count"),
                Resolved.VisualEffects.Effects.Num(), Source->VisualEffects.Effects.Num()))
            {
                return false;
            }
            for (int32 I = 0; I < Source->VisualEffects.Effects.Num(); ++I)
            {
                const FGridItemNiagaraEffectConfig& A = Resolved.VisualEffects.Effects[I];
                const FGridItemNiagaraEffectConfig& B = Source->VisualEffects.Effects[I];
                if (!Test.TestTrue(TEXT("Niagara identity and transform"),
                    A.EffectId == B.EffectId &&
                    A.NiagaraSystem == B.NiagaraSystem &&
                    A.RelativeTransform.Equals(B.RelativeTransform) &&
                    A.ActivationMode == B.ActivationMode))
                {
                    return false;
                }
            }
        }
        if (Mode == EGridProjectionProbe::SparkleAndPhysics)
        {
            if (!Test.TestTrue(TEXT("Sparkle, reading and physics projection"),
                Resolved.bEnableWorldSparkle == Source->bEnableWorldSparkle &&
                Resolved.WorldSparkleMaterial == Source->WorldSparkleMaterial &&
                Resolved.WorldSparkleColor == Source->WorldSparkleColor &&
                Resolved.WorldSparkleIntensity == Source->WorldSparkleIntensity &&
                Resolved.WorldSparkleSpeed == Source->WorldSparkleSpeed &&
                Resolved.WorldSparkleVariation == Source->WorldSparkleVariation &&
                Resolved.ReadText.ToString() == Source->ReadText.ToString() &&
                Resolved.bReadable == Source->bReadable &&
                Resolved.bUseItemWeightAsWorldPhysicsMass ==
                    Source->bUseItemWeightAsWorldPhysicsMass &&
                Resolved.WorldPhysicsInitialTiltDegrees ==
                    Source->WorldPhysicsInitialTiltDegrees))
            {
                return false;
            }
        }
    }
    return Test.TestEqual(TEXT("Expected ARM registry families"), Arm, 22) &&
        Test.TestEqual(TEXT("Expected nonARM registry families"), NonArm, Root->ItemDefinitions.Num() - 22);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X6ProbeFamilies,
    "Grimrock.RPG.ARM.CLEAN06_4X6.ProbeA_Families",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X6ProbeFamilies::RunTest(const FString&)
{
    return RunX6Probe(*this, EGridProjectionProbe::Families);
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X6ProbeResolve,
    "Grimrock.RPG.ARM.CLEAN06_4X6.ProbeB_Resolve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X6ProbeResolve::RunTest(const FString&)
{
    return RunX6Probe(*this, EGridProjectionProbe::Resolve);
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X6ProbeLight,
    "Grimrock.RPG.ARM.CLEAN06_4X6.ProbeC_Light",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X6ProbeLight::RunTest(const FString&)
{
    return RunX6Probe(*this, EGridProjectionProbe::Light);
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X6ProbeNiagara,
    "Grimrock.RPG.ARM.CLEAN06_4X6.ProbeD_Niagara",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X6ProbeNiagara::RunTest(const FString&)
{
    return RunX6Probe(*this, EGridProjectionProbe::Niagara);
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X6ProbeExtra,
    "Grimrock.RPG.ARM.CLEAN06_4X6.ProbeE_SparkleReadingPhysics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X6ProbeExtra::RunTest(const FString&)
{
    return RunX6Probe(*this, EGridProjectionProbe::SparkleAndPhysics);
}

#endif
