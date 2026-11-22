#if WITH_DEV_AUTOMATION_TESTS

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridItemVFX013RuntimeTests
{
    struct FTestWorld
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;

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
                GI = NewObject<UGameInstance>(GetTransientPackage());
                GI->Init();
                World->SetGameInstance(GI);
                World->InitializeActorsForPlay(FURL());
            }
        }

        ~FTestWorld()
        {
            if (World)
            {
                World->SetGameInstance(nullptr);
                if (GI) GI->Shutdown();
                World->DestroyWorld(false);
                if (GEngine)
                {
                    GEngine->DestroyWorldContext(World);
                }
            }
        }
    };

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX013TorchRuntimeTest,
    "Grimrock.Items.VFX01_3.TorchRuntimeAndHeldVisual",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX013TorchRuntimeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FName TorchId(TEXT("Item_Torch"));
    GridItemVFX013RuntimeTests::FTestWorld TestWorld;
    if (!TestNotNull(TEXT("Transient game world created"), TestWorld.World) ||
        !TestNotNull(TEXT("Transient game instance created"), TestWorld.GI))
    {
        return false;
    }

    const UGridItemSubsystem* Registry = TestWorld.GI->GetSubsystem<UGridItemSubsystem>();
    FGridResolvedItem Torch;
    if (!TestNotNull(TEXT("Canonical registry available"), Registry) ||
        !TestTrue(TEXT("Production torch N0 resolves from ItemSystem"),
            Registry && Registry->ResolveItem(TorchId, 0, Torch)) ||
        !TestFalse(TEXT("Torch is a non-ARM N0 physical item"), Torch.bIsArmament) ||
        !TestTrue(TEXT("Torch retains its point light"), Torch.LightEmitter.bUsePointLight) ||
        !TestTrue(TEXT("Torch has its canonical Niagara effects"), !Torch.VisualEffects.Effects.IsEmpty()))
    {
        return false;
    }

    AGridItemActor* WorldTorch = TestWorld.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Torch actor spawned"), WorldTorch))
    {
        return false;
    }

    FGridItemInstance WorldTorchItem;
    WorldTorchItem.RuntimeObjectId = FGuid::NewGuid();
    WorldTorchItem.ItemDefinitionId = TorchId;
    WorldTorchItem.ItemLevel = 0;
    WorldTorchItem.Quantity = 1;
    WorldTorchItem.OwnerType = EGridItemOwnerType::World;
    if (!TestTrue(TEXT("Canonical world torch initializes"),
            WorldTorch->InitializeFromItemInstance(WorldTorchItem, Registry)) ||
        !TestNull(TEXT("Canonical world torch holds no legacy definition"),
            WorldTorch->GetItemDefinitionAsset()))
        return false;
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
    if (!TestTrue(TEXT("Canonical torch is registered"), Registry && Registry->ContainsItem(TorchId)) ||
        !TestTrue(TEXT("Generic pawn can equip the canonical torch"),
            Party->EquipHeldItem(TorchId)))
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
