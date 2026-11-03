#if WITH_DEV_AUTOMATION_TESTS

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridItemVFX013RuntimeTests
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
                FName(*FString::Printf(TEXT("ITEM_VFX013_%s"),
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

    UGridItemDefinitionAsset* LoadTorch()
    {
        return LoadObject<UGridItemDefinitionAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.DA_Item_Torch"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013TorchRuntimeTest,
    "Grimrock.Items.VFX01_3.TorchRuntimeAndHeldVisual",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013TorchRuntimeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UGridItemDefinitionAsset* Torch = GridItemVFX013RuntimeTests::LoadTorch();
    if (!TestNotNull(TEXT("Production DA_Item_Torch loads"), Torch) ||
        !TestTrue(TEXT("Torch retains its light emitter"), Torch ? Torch->HasLightEmitter() : false) ||
        !TestTrue(TEXT("Torch has migrated Niagara"), Torch ? Torch->VisualEffects.Effects.Num() > 0 : false))
    {
        return false;
    }

    GridItemVFX013RuntimeTests::FTestWorld TestWorld;
    if (!TestNotNull(TEXT("Transient game world created"), TestWorld.World))
    {
        return false;
    }

    AGridItemActor* WorldTorch = TestWorld.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Torch actor spawned"), WorldTorch))
    {
        return false;
    }

    WorldTorch->InitializeFromItemDefinition(Torch, FGuid::NewGuid());
    UGridItemVisualEffectsComponent* Visuals = WorldTorch->VisualEffectsComponent;
    if (!TestNotNull(TEXT("Independent Niagara executor exists"), Visuals))
    {
        return false;
    }
    TestTrue(TEXT("VFX attaches directly to physics mesh"),
        Visuals->GetAttachParent() == WorldTorch->MeshComponent.Get());
    TestNull(TEXT("No Niagara spawned while the torch is extinguished"),
        Visuals->GetEffectComponent(0));
    TestFalse(TEXT("World torch begins off until the instance state is restored"),
        WorldTorch->AreItemLightsEnabled());

    WorldTorch->SetItemLightsEnabled(true);
    UNiagaraComponent* Flame = Visuals->GetEffectComponent(0);
    UPointLightComponent* PointLight = WorldTorch->FindComponentByClass<UPointLightComponent>();
    if (!TestNotNull(TEXT("Lighting creates the conditional torch flame"), Flame) ||
        !TestNotNull(TEXT("Lighting creates the original point light"), PointLight))
    {
        return false;
    }
    TestTrue(TEXT("Flame is visible while lit"), Flame->IsVisible());
    TestTrue(TEXT("PointLight is visible while lit"), PointLight->IsVisible());
    TestTrue(TEXT("Instance reports lit"), WorldTorch->AreItemLightsEnabled());

    WorldTorch->SetItemLightsEnabled(false);
    TestFalse(TEXT("Extinguishing hides the flame"), Flame->IsVisible());
    TestFalse(TEXT("Extinguishing hides the PointLight"), PointLight->IsVisible());
    TestFalse(TEXT("Extinguishing disables light emitter tick"),
        WorldTorch->LightEmitterComponent->IsComponentTickEnabled());

    WorldTorch->SetItemLightsEnabled(true);
    TestTrue(TEXT("Relighting shows the original flame"), Flame->IsVisible());
    TestTrue(TEXT("Relighting reuses the same Niagara component"),
        Visuals->GetEffectComponent(0) == Flame);

    TArray<UNiagaraComponent*> NiagaraComponents;
    WorldTorch->GetComponents<UNiagaraComponent>(NiagaraComponents);
    TestEqual(TEXT("Exactly one Niagara component, no duplicated legacy flame"),
        NiagaraComponents.Num(), 1);

    WorldTorch->OnRemovedFromWorld();
    TestFalse(TEXT("Removed torch hides Niagara"), Flame->IsVisible());
    WorldTorch->OnPlacedInWorld();
    WorldTorch->SetItemLightsEnabled(true);
    TestTrue(TEXT("Replaced lit torch restores its flame"), Flame->IsVisible());

    AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Party pawn spawns"), Party) ||
        !TestNotNull(TEXT("Party inventory is present"),
            Party ? Party->PartyInventoryComponent.Get() : nullptr))
    {
        return false;
    }

    Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
    if (!TestTrue(TEXT("Torch definition registers for held-item resolution"),
        Party->PartyInventoryComponent->RegisterItemDefinition(Torch)) ||
        !TestTrue(TEXT("Generic pawn can equip the authored torch"),
            Party->EquipHeldItem(Torch->ItemDefinitionId)))
    {
        return false;
    }

    AGridItemActor* Held = Party->HeldItemActor.Get();
    if (!TestNotNull(TEXT("Held torch is a generic AGridItemActor"), Held))
    {
        return false;
    }
    Held->SetItemLightsEnabled(true);
    UNiagaraComponent* HeldFlame = Held->VisualEffectsComponent->GetEffectComponent(0);
    TestTrue(TEXT("Held torch displays the independent conditional flame"),
        HeldFlame && HeldFlame->IsVisible());
    TestNull(TEXT("Held item does not create a competing PointLight"),
        Held->FindComponentByClass<UPointLightComponent>());

    Held->SetItemLightsEnabled(false);
    TestFalse(TEXT("Held torch flame extinguishes without affecting other instances"),
        HeldFlame && HeldFlame->IsVisible());
    TestTrue(TEXT("Independent world torch stays lit"),
        WorldTorch->AreItemLightsEnabled() && Flame->IsVisible());

    Party->ClearHeldItem();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
