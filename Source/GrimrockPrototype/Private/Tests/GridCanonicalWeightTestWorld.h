#pragma once

#if WITH_DEV_AUTOMATION_TESTS
#include "GridCanonicalLootTestScope.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Runtime/GridPartyInventoryComponent.h"

/** An isolated inventory with the actual ItemSystem weight reader. */
struct FGridCanonicalWeightTestWorld
{
    UWorld* World = nullptr;
    FGridCanonicalLootTestScope Scope;

    explicit FGridCanonicalWeightTestWorld(const TMap<FName, float>& Weights,
        const TFunction<void(UGridItemSystemAsset&)>& Extend = {})
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("CanonicalWeight_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World || !GEngine) return;
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        // UUserWidget::GetWorld() resolves through the owning GameInstance.
        // Bind the real test GI to the engine context BEFORE publishing its
        // UWorld, exactly as SetCurrentWorld expects. Otherwise pure inventory
        // tests pass but canonical starter-kit descriptions lack a UMG world.
        if (!Scope.AttachWithWeights(World, Weights, Extend))
        {
            Scope.Detach();
            Context.SetCurrentWorld(World); // still register for safe teardown
            return;
        }
        Context.OwningGameInstance = Scope.GI.Get();
        Context.SetCurrentWorld(World);
    }

    UGridPartyInventoryComponent* MakeInventory() const
    {
        if (!World || !Scope.Registry || !Scope.Registry->IsConfigured()) return nullptr;
        AActor* Owner = World->SpawnActor<AActor>();
        return Owner ? NewObject<UGridPartyInventoryComponent>(Owner) : nullptr;
    }

    ~FGridCanonicalWeightTestWorld()
    {
        Scope.Detach();
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
};
#endif
