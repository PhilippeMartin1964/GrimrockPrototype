#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemVisualEffectsComponent.h"

namespace GridItemVFX011Tests
{
    struct FTestWorld
    {
        UWorld* World = nullptr;

        FTestWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false)
                .RequiresHitProxies(false)
                .CreatePhysicsScene(false)
                .CreateNavigation(false)
                .CreateAISystem(false)
                .ShouldSimulatePhysics(false)
                .SetTransactional(false);

            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ITEM_VFX011_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
                World->InitializeActorsForPlay(FURL());
            }
        }

        ~FTestWorld()
        {
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine)
                {
                    GEngine->DestroyWorldContext(World);
                }
            }
        }
    };

    UNiagaraSystem* LoadProductionNiagara()
    {
        return LoadObject<UNiagaraSystem>(nullptr,
            TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX011ConfigurationTest,
    "Grimrock.Items.VFX01_1.Configuration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX011ConfigurationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
    Definition->ItemDefinitionId = TEXT("VFX011_VisualOnlyPotion");
    TestFalse(TEXT("A visual-only item has no light emitter"), Definition->HasLightEmitter());
    TestTrue(TEXT("Empty visual effect configuration is valid"), Definition->VisualEffects.IsValid());

    UNiagaraSystem* System = GridItemVFX011Tests::LoadProductionNiagara();
    if (!TestNotNull(TEXT("Production NS_LiquideSparkle asset exists"), System))
    {
        return false;
    }

    FGridItemNiagaraEffectConfig Effect;
    Effect.EffectId = TEXT("Bubbles");
    Effect.NiagaraSystem = System;
    Effect.RelativeTransform = FTransform(FRotator(0.f, 12.f, 0.f), FVector(2.f, -3.f, 7.f));
    Definition->VisualEffects.Effects.Add(Effect);

    TestTrue(TEXT("A Niagara-only definition is valid"), Definition->IsValidDefinition());
    TestFalse(TEXT("Visual effects do not implicitly provide light"), Definition->HasLightEmitter());

    Definition->VisualEffects.Effects.Add(FGridItemNiagaraEffectConfig());
    TestFalse(TEXT("An absent Niagara asset invalidates the definition"), Definition->IsValidDefinition());
    Definition->VisualEffects.Effects.Pop();

    Definition->VisualEffects.Effects.Add(Effect);
    TestFalse(TEXT("Duplicate named effect IDs are rejected"), Definition->VisualEffects.IsValid());
    Definition->VisualEffects.Effects.Last().EffectId = TEXT("OtherEffect");
    TestTrue(TEXT("Multiple differently named effects are accepted"), Definition->VisualEffects.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX011RuntimeTest,
    "Grimrock.Items.VFX01_1.RuntimeLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX011RuntimeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UNiagaraSystem* System = GridItemVFX011Tests::LoadProductionNiagara();
    if (!TestNotNull(TEXT("Production NS_LiquideSparkle asset exists"), System))
    {
        return false;
    }

    GridItemVFX011Tests::FTestWorld TestWorld;
    if (!TestNotNull(TEXT("Transient world exists"), TestWorld.World))
    {
        return false;
    }

    UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>();
    Definition->ItemDefinitionId = TEXT("VFX011_ConditionalAndAlways");
    Definition->LightEmitter.bUsePointLight = true;
    Definition->LightEmitter.bDefaultEnabled = false;
    Definition->LightEmitter.bEnableLightFlicker = false;
    Definition->LightEmitter.bEnableLightPositionFlicker = false;
    Definition->LightEmitter.bEnableLightColorFlicker = false;

    FGridItemNiagaraEffectConfig Permanent;
    Permanent.EffectId = TEXT("Permanent");
    Permanent.NiagaraSystem = System;
    Permanent.RelativeTransform = FTransform(FRotator(0.f, 15.f, 0.f), FVector(3.f, 4.f, 5.f));
    Definition->VisualEffects.Effects.Add(Permanent);

    FGridItemNiagaraEffectConfig Conditional;
    Conditional.EffectId = TEXT("FlameLike");
    Conditional.NiagaraSystem = System;
    Conditional.ActivationMode = EGridItemVfxActivationMode::WhenLightEnabled;
    Conditional.RelativeTransform = FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 13.f));
    Definition->VisualEffects.Effects.Add(Conditional);

    AGridItemActor* Item = TestWorld.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Generic item actor spawned"), Item) ||
        !TestNotNull(TEXT("Visual-effects component exists"), Item ? Item->VisualEffectsComponent.Get() : nullptr))
    {
        return false;
    }

    Item->InitializeFromItemDefinition(Definition, FGuid::NewGuid());
    UGridItemVisualEffectsComponent* Visuals = Item->VisualEffectsComponent;
    TestTrue(TEXT("Effects follow the mesh, not the actor root"),
        Visuals->GetAttachParent() == Item->MeshComponent.Get());
    TestFalse(TEXT("Item begins unlit"), Item->AreItemLightsEnabled());
    TestEqual(TEXT("Two effects configured"), Visuals->GetConfiguredEffectCount(), 2);

    UNiagaraComponent* PermanentComponent = Visuals->GetEffectComponent(0);
    TestNotNull(TEXT("Always effect exists even when light is off"), PermanentComponent);
    TestNull(TEXT("Conditional effect is not created while unlit"), Visuals->GetEffectComponent(1));
    if (!PermanentComponent)
    {
        return false;
    }

    TestTrue(TEXT("Correct Niagara system"), PermanentComponent->GetAsset() == System);
    TestTrue(TEXT("Niagara attaches to visual executor"), PermanentComponent->GetAttachParent() == Visuals);
    TestTrue(TEXT("Authored transform preserved"),
        PermanentComponent->GetRelativeTransform().Equals(Permanent.RelativeTransform, 0.001f));
    TestTrue(TEXT("Permanent effect visible while unlit"), PermanentComponent->IsVisible());
    TestFalse(TEXT("No component tick"), Visuals->IsComponentTickEnabled());

    Item->SetItemLightsEnabled(true);
    UNiagaraComponent* ConditionalComponent = Visuals->GetEffectComponent(1);
    TestTrue(TEXT("Existing point-light state turns on"), Item->AreItemLightsEnabled());
    TestNotNull(TEXT("Conditional effect appears"), ConditionalComponent);
    if (!ConditionalComponent)
    {
        return false;
    }

    TestTrue(TEXT("Conditional effect visible when lit"), ConditionalComponent->IsVisible());
    Item->SetItemLightsEnabled(false);
    TestTrue(TEXT("Permanent effect survives extinguishing"), PermanentComponent->IsVisible());
    TestFalse(TEXT("Conditional effect hides when extinguished"), ConditionalComponent->IsVisible());
    TestFalse(TEXT("Point light turns off"), Item->AreItemLightsEnabled());

    Item->InitializeFromItemDefinition(Definition, Item->GetRuntimeObjectId());
    TestTrue(TEXT("Permanent effect reused after reinitialization"),
        Visuals->GetEffectComponent(0) == PermanentComponent);
    TestTrue(TEXT("Conditional effect reused after reinitialization"),
        Visuals->GetEffectComponent(1) == ConditionalComponent);
    TArray<UNiagaraComponent*> SpawnedNiagara;
    Item->GetComponents<UNiagaraComponent>(SpawnedNiagara);
    TestEqual(TEXT("No duplicate Niagara components"), SpawnedNiagara.Num(), 2);

    Item->MeshComponent->SetWorldRotation(FRotator(19.f, 37.f, 11.f));
    const FVector ExpectedLocation = Item->MeshComponent->GetComponentTransform().TransformPosition(
        Permanent.RelativeTransform.GetLocation());
    TestTrue(TEXT("Effects follow physical mesh rotation"),
        PermanentComponent->GetComponentLocation().Equals(ExpectedLocation, 0.1f));

    Item->OnRemovedFromWorld();
    TestFalse(TEXT("World removal hides permanent effect"), PermanentComponent->IsVisible());
    Item->OnPlacedInWorld();
    TestTrue(TEXT("World re-placement restores permanent effect"), PermanentComponent->IsVisible());

    Item->InitializeFromItemDefinitionId(TEXT("VFX011_Unresolved"), FGuid::NewGuid());
    TestEqual(TEXT("Unknown definition clears obsolete effect config"), Visuals->GetConfiguredEffectCount(), 0);
    TestNull(TEXT("Unknown definition removes obsolete emitter"), Visuals->GetEffectComponent(0));

    Item->Destroy();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
