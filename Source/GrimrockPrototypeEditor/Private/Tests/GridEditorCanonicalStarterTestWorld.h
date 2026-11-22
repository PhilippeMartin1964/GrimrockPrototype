#pragma once
#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

// A real game-world ItemSystem, never the worldless legacy-DA fixtures.
// All instances are transient; no user SaveGame or authored class is changed.
struct FGridEditorCanonicalStarterTestWorld
{
    UWorld* World = nullptr;
    TStrongObjectPtr<UGameInstance> GameInstance;
    UGridItemSubsystem* Registry = nullptr;

    FGridEditorCanonicalStarterTestWorld()
    {
        const UWorld::InitializationValues Settings = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("CanonicalStarter_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Settings);
        if (!World || !GEngine) return;
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        GameInstance.Reset(NewObject<UGameInstance>(GetTransientPackage()));
        if (!GameInstance.IsValid()) return;
        GameInstance->Init();
        Context.OwningGameInstance = GameInstance.Get();
        Context.SetCurrentWorld(World);
        World->SetGameInstance(GameInstance.Get());
        Registry = GameInstance->GetSubsystem<UGridItemSubsystem>();
    }

    bool Ready() const
    {
        return World && Registry && Registry->IsConfigured() &&
            Registry->HasCanonicalItem(TEXT("Item_Potion_Healing_01")) &&
            Registry->HasCanonicalItem(TEXT("Item_Arrows_01")) &&
            Registry->HasCanonicalItem(TEXT("Item_DisarmTools_01")) &&
            Registry->HasCanonicalItem(TEXT("Item_SurfaceCatalyst_01"));
    }

    UGridPartyInventoryComponent* MakeInventory() const
    {
        if (!Ready()) return nullptr;
        AActor* Owner = World->SpawnActor<AActor>();
        if (!Owner) return nullptr;
        UGridPartyInventoryComponent* Inventory =
            NewObject<UGridPartyInventoryComponent>(Owner);
        if (Inventory) Inventory->InitializeDefaultPartyIfNeeded();
        return Inventory;
    }

    ~FGridEditorCanonicalStarterTestWorld()
    {
        if (World) World->SetGameInstance(nullptr);
        if (GameInstance.IsValid()) GameInstance->Shutdown();
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
};

#endif // WITH_DEV_AUTOMATION_TESTS
