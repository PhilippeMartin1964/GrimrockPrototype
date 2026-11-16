#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FGridClean064X7World
{
    UWorld* World = nullptr;

    FGridClean064X7World()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("ARM_CLEAN064X7_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (World && GEngine)
        {
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
        }
    }
    ~FGridClean064X7World()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) { GEngine->DestroyWorldContext(World); }
        }
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X7ActorProjection,
    "Grimrock.RPG.ARM.CLEAN06_4X7.CanonicalItemActorInitialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X7ActorProjection::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/"
                      "DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real item system"), Root.Get()) ||
        !TestEqual(TEXT("Real canonical definition count"),
            Root->ItemDefinitions.Num(), 34))
    {
        return false;
    }

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(
        NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical registry registered"),
        Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) { AddError(Error); }
        return false;
    }

    FGridClean064X7World Fixture;
    if (!TestNotNull(TEXT("Isolated transient test world"), Fixture.World))
    {
        return false;
    }
    int32 Arms = 0;
    int32 NonArms = 0;
    int32 Ammo = 0;
    for (const UGridCanonicalItemDefinitionAsset* Definition :
        Root->ItemDefinitions)
    {
        if (!TestNotNull(TEXT("Real canonical definition"), Definition))
        {
            return false;
        }
        const bool bArm = Cast<UGridArmamentDefinitionAsset>(Definition) != nullptr;
        const bool bAmmo = Cast<UGridAmmoDefinitionAsset>(Definition) != nullptr;
        if (bArm) { ++Arms; }
        else { ++NonArms; }
        if (bAmmo) { ++Ammo; }

        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid::NewGuid();
        Instance.ItemDefinitionId = Definition->ItemId;
        Instance.ItemLevel = bArm ? 1 : 0;
        Instance.Quantity = 1;

        FGridResolvedItem Expected;
        if (!TestTrue(TEXT("Real item resolves at its physical level"),
            Registry->ResolveItem(Instance.ItemDefinitionId,
                Instance.ItemLevel, Expected)))
        {
            return false;
        }
        AGridItemActor* Actor = Fixture.World->SpawnActor<AGridItemActor>();
        if (!TestNotNull(TEXT("Actual item actor spawned"), Actor) ||
            !TestTrue(TEXT("Canonical actor init succeeds"),
                Actor->InitializeFromCanonicalItem(Instance, Registry.Get())))
        {
            return false;
        }

        if (!TestTrue(TEXT("No historical UObject source attached"),
                Actor->GetItemDefinitionAsset() == nullptr) ||
            !TestEqual(TEXT("Stable identity"), Actor->GetItemDefinitionId(),
                Instance.ItemDefinitionId) ||
            !TestEqual(TEXT("Stable runtime GUID"), Actor->GetRuntimeObjectId(),
                Instance.RuntimeObjectId) ||
            !TestEqual(TEXT("Real explicit item level"),
                Actor->CanonicalWorldItemLevel, Instance.ItemLevel) ||
            !TestTrue(TEXT("Presentation is transient and matches original ID"),
                Actor->CanonicalWorldPresentation.IsValid() &&
                Actor->CanonicalWorldPresentation.ItemId == Definition->ItemId) ||
            !TestEqual(TEXT("Tags are from canonical family"),
                Actor->ItemTags.Num(), Expected.Tags.Num()) ||
            !TestTrue(TEXT("The world item remains initially unlit"),
                !Actor->AreItemLightsEnabled()) ||
            !TestEqual(TEXT("Point-light configuration is canonical"),
                Actor->LightEmitterComponent->RuntimeConfig.bUsePointLight,
                Expected.LightEmitter.bUsePointLight) ||
            !TestEqual(TEXT("Light intensity is canonical"),
                Actor->LightEmitterComponent->RuntimeConfig.LightIntensity,
                Expected.LightEmitter.LightIntensity) ||
            !TestEqual(TEXT("All Niagara effect definitions are applied"),
                Actor->VisualEffectsComponent->GetConfiguredEffectCount(),
                Expected.VisualEffects.Effects.Num()) ||
            !TestTrue(TEXT("World mesh is owned by canonical resolver"),
                Actor->MeshComponent->GetStaticMesh() ==
                Expected.WorldMesh.LoadSynchronous()) ||
            !TestTrue(TEXT("World visual offset comes from canonical resolver"),
                Actor->MeshComponent->GetRelativeTransform().Equals(
                    Expected.WorldVisualTransform)))
        {
            return false;
        }

        // Invalid *registered* N0 for an ARM must fail without changing
        // the actor's existing identity, GUID or transient presentation.
        if (bArm)
        {
            FGridItemInstance Invalid = Instance;
            Invalid.ItemLevel = 0;
            if (!TestFalse(TEXT("ARM N0 is never guessed as N1"),
                    Actor->InitializeFromCanonicalItem(Invalid, Registry.Get())) ||
                !TestEqual(TEXT("Failed initialization preserves physical N1"),
                    Actor->CanonicalWorldItemLevel, 1) ||
                !TestEqual(TEXT("Failed initialization preserves GUID"),
                    Actor->GetRuntimeObjectId(), Instance.RuntimeObjectId))
            {
                return false;
            }
        }

        FGridItemInstance Unknown = Instance;
        Unknown.ItemDefinitionId = FName(TEXT("ARM_CLEAN064X7_MissingItem"));
        if (!TestFalse(TEXT("Unknown ItemId cannot use historical visual data"),
                Actor->InitializeFromCanonicalItem(Unknown, Registry.Get())) ||
            !TestEqual(TEXT("Unknown ID failure preserves existing item"),
                Actor->GetItemDefinitionId(), Instance.ItemDefinitionId))
        {
            return false;
        }

        // World-pickup presentation stays opt-in. Runtime light state
        // continues to be driven by FGridItemInstance.bLightsEnabled.
        if (Expected.LightEmitter.bUsePointLight)
        {
            Actor->SetItemLightsEnabled(true);
            if (!TestTrue(TEXT("Canonical point-light can be activated"),
                Actor->AreItemLightsEnabled()))
            {
                return false;
            }
            Actor->SetItemLightsEnabled(false);
        }
    }

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X7] 34 canonical actor initializations: "
        "%d ARM N1, %d non-ARM N0 (%d ammo); "
        "no legacy definition, UE package write or SaveGame mutation."),
        Arms, NonArms, Ammo));
    return TestEqual(TEXT("Registered ARM classes"), Arms, 22) &&
        TestEqual(TEXT("Registered non-ARM classes"), NonArms, 12) &&
        TestTrue(TEXT("Ammo path exercised"), Ammo > 0);
}
#endif
