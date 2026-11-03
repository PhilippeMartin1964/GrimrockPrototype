#if WITH_DEV_AUTOMATION_TESTS

#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridLightEmitterTypes.h"
#include "UObject/UnrealType.h"

namespace GridItemVFX014BTests
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
                FName(*FString::Printf(TEXT("ITEM_VFX014B_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX014BSchemaTest,
    "Grimrock.Items.VFX01_4B.PointLightOnlySchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX014BSchemaTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UScriptStruct* Config = FGridLightEmitterConfig::StaticStruct();
    if (!TestNotNull(TEXT("Light config reflection exists"), Config))
    {
        return false;
    }

    TestNull(TEXT("LightEmitter.NiagaraSystem removed"),
        FindFProperty<FProperty>(Config, TEXT("NiagaraSystem")));
    TestNull(TEXT("LightEmitter.NiagaraRelativeLocation removed"),
        FindFProperty<FProperty>(Config, TEXT("NiagaraRelativeLocation")));
    TestNull(TEXT("LightEmitter.NiagaraRelativeRotation removed"),
        FindFProperty<FProperty>(Config, TEXT("NiagaraRelativeRotation")));

    TestNotNull(TEXT("Point light base intensity retained"),
        FindFProperty<FProperty>(Config, TEXT("LightIntensity")));
    TestNotNull(TEXT("Point light base radius retained"),
        FindFProperty<FProperty>(Config, TEXT("LightRadius")));
    TestNotNull(TEXT("Point light base color retained"),
        FindFProperty<FProperty>(Config, TEXT("LightColor")));
    TestNotNull(TEXT("Gameplay default state retained"),
        FindFProperty<FProperty>(Config, TEXT("bDefaultEnabled")));

    UClass* RuntimeClass = UGridLightEmitterComponent::StaticClass();
    TestNull(TEXT("Runtime light no longer owns a NiagaraComponent UPROPERTY"),
        FindFProperty<FProperty>(RuntimeClass, TEXT("NiagaraComponent")));
    TestNull(TEXT("Legacy two-channel presentation function is removed"),
        RuntimeClass->FindFunctionByName(TEXT("SetEmitterChannelsEnabled")));
    TestNull(TEXT("Legacy Niagara presentation getter is removed"),
        RuntimeClass->FindFunctionByName(TEXT("IsNiagaraPresentationEnabled")));
    TestNotNull(TEXT("PointLight-only presentation function is exposed"),
        RuntimeClass->FindFunctionByName(TEXT("SetPointLightPresentationEnabled")));
    TestNotNull(TEXT("Existing shadow configuration remains exposed"),
        RuntimeClass->FindFunctionByName(TEXT("SetPointLightCastShadows")));

    FGridLightEmitterConfig NoLight;
    NoLight.bDefaultEnabled = true;
    TestFalse(TEXT("A default-enabled flag is not itself a point light"),
        NoLight.HasEmitter());
    TestFalse(TEXT("No false-positive light tick on an empty config"),
        NoLight.RequiresRuntimeTick());

    FGridLightEmitterConfig Light;
    Light.bUsePointLight = true;
    Light.LightIntensity = 524.f;
    Light.LightRadius = 617.f;
    TestTrue(TEXT("PointLight-only config is a valid emitter"),
        Light.HasEmitter() && Light.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX014BRuntimeTest,
    "Grimrock.Items.VFX01_4B.RuntimeSeparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX014BRuntimeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    UNiagaraSystem* FlameSystem = LoadObject<UNiagaraSystem>(nullptr,
        TEXT("/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch.NS_Flame_8_Torch"));
    if (!TestNotNull(TEXT("Torch flame Niagara loads"), FlameSystem))
    {
        return false;
    }

    GridItemVFX014BTests::FTestWorld TestWorld;
    if (!TestNotNull(TEXT("Transient game world exists"), TestWorld.World))
    {
        return false;
    }

    UGridItemDefinitionAsset* Torch = NewObject<UGridItemDefinitionAsset>();
    Torch->ItemDefinitionId = TEXT("VFX014B_Torch");
    Torch->ItemType = EGridItemType::Torch;
    Torch->LightEmitter.bUsePointLight = true;
    Torch->LightEmitter.bDefaultEnabled = true;
    Torch->LightEmitter.LightIntensity = 474.f;
    Torch->LightEmitter.LightRadius = 385.f;
    Torch->LightEmitter.bEnableLightFlicker = false;
    Torch->LightEmitter.bEnableLightColorFlicker = false;
    Torch->LightEmitter.bEnableLightPositionFlicker = false;

    FGridItemNiagaraEffectConfig Flame;
    Flame.EffectId = TEXT("TorchFlame");
    Flame.NiagaraSystem = FlameSystem;
    Flame.ActivationMode = EGridItemVfxActivationMode::WhenLightEnabled;
    Flame.RelativeTransform = FTransform(FRotator(0.f, 25.f, 0.f), FVector(4.f, 6.f, 8.f));
    Torch->VisualEffects.Effects.Add(Flame);

    if (!TestTrue(TEXT("Torch light and visual effect config are valid"), Torch->IsValidDefinition()))
    {
        return false;
    }

    AGridItemActor* Item = TestWorld.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Generic item actor exists"), Item) ||
        !TestNotNull(TEXT("Light emitter component exists"),
            Item ? Item->LightEmitterComponent.Get() : nullptr) ||
        !TestNotNull(TEXT("Visual effects component exists"),
            Item ? Item->VisualEffectsComponent.Get() : nullptr))
    {
        return false;
    }

    Item->InitializeFromItemDefinition(Torch, FGuid::NewGuid());
    TestFalse(TEXT("Initialization respects the instance-owned light state"),
        Item->AreItemLightsEnabled());
    TestNull(TEXT("Conditional flame is initially absent"),
        Item->VisualEffectsComponent->GetEffectComponent(0));

    Item->SetItemLightsEnabled(true);
    UNiagaraComponent* FlameComponent = Item->VisualEffectsComponent->GetEffectComponent(0);
    UPointLightComponent* PointLight = Item->FindComponentByClass<UPointLightComponent>();
    if (!TestNotNull(TEXT("Independent VFX creates flame"), FlameComponent) ||
        !TestNotNull(TEXT("Light executor creates PointLight"), PointLight))
    {
        return false;
    }
    TestTrue(TEXT("Flame active while lit"), FlameComponent->IsVisible());
    TestTrue(TEXT("PointLight active while lit"), PointLight->IsVisible());
    TestTrue(TEXT("PointLight matches the authored intensity"),
        FMath::IsNearlyEqual(PointLight->Intensity, 474.f));

    TArray<UNiagaraComponent*> Components;
    Item->GetComponents<UNiagaraComponent>(Components);
    TestEqual(TEXT("Only VisualEffects creates Niagara; no legacy duplicate"),
        Components.Num(), 1);

    Item->LightEmitterComponent->SetPointLightPresentationEnabled(false);
    TestTrue(TEXT("Held-item light state remains enabled"),
        Item->AreItemLightsEnabled());
    TestFalse(TEXT("Delegated PointLight is not visible"), PointLight->IsVisible());
    TestTrue(TEXT("VisualEffects Niagara is unaffected by PointLight presentation"),
        FlameComponent->IsVisible());

    Item->SetItemLightsEnabled(false);
    TestFalse(TEXT("Extinguishing hides the conditional flame"),
        FlameComponent->IsVisible());
    TestFalse(TEXT("Light emitter stops unnecessary flicker ticks"),
        Item->LightEmitterComponent->IsComponentTickEnabled());

    Item->SetItemLightsEnabled(true);
    TestTrue(TEXT("Relighting restores flame even while local PointLight is delegated"),
        FlameComponent->IsVisible());
    TestFalse(TEXT("Delegated PointLight stays hidden after relighting"),
        PointLight->IsVisible());

    Item->LightEmitterComponent->SetPointLightPresentationEnabled(true);
    TestTrue(TEXT("World presentation restores the same PointLight"),
        PointLight->IsVisible());
    Components.Reset();
    Item->GetComponents<UNiagaraComponent>(Components);
    TestEqual(TEXT("No Niagara duplication after toggling light and presentation"),
        Components.Num(), 1);

    Item->Destroy();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
