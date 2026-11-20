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

    explicit FGridCanonicalWeightTestWorld(const TMap<FName, float>& Weights)
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
        Context.SetCurrentWorld(World);
        if (!Scope.AttachWithWeights(World, Weights)) Scope.Detach();
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
