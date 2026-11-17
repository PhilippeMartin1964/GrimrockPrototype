#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
struct FGridClean064X8World
{
    UWorld* World = nullptr;
    UGameInstance* GI = nullptr;
    UGridItemSubsystem* Registry = nullptr;
    AGridLevelRuntimeActor* Runtime = nullptr;

    FGridClean064X8World()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("ARM_CLEAN064X8_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World || !GEngine) { return; }
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        GI = NewObject<UGameInstance>(GetTransientPackage());
        if (!GI) { return; }
        GI->Init();
        World->SetGameInstance(GI);
        Registry = GI->GetSubsystem<UGridItemSubsystem>();
        Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
        if (!Runtime) {return;}
        Runtime->LevelAsset = NewObject<UGridLevelAsset>(Runtime);
        Runtime->LevelAsset->Width = 1;
        Runtime->LevelAsset->Height = 1;
        Runtime->LevelAsset->CellSize = 200.f;
        Runtime->LevelAsset->EnsureCellCount();
    }

    ~FGridClean064X8World()
    {
        if (World) {World->SetGameInstance(nullptr);}
        if (GI) {GI->Shutdown();}
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) {GEngine->DestroyWorldContext(World);}
        }
    }

    AGridItemActor* FindItem(FGuid Id) const
    {
        if (!World) { return nullptr; }
        for (TActorIterator<AGridItemActor> It(World); It; ++It)
        {
            if (It->GetRuntimeObjectId() == Id) { return *It; }
        }
        return nullptr;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X8LooseCutover,
    "Grimrock.RPG.ARM.CLEAN06_4X8.RealLooseItemCanonicalNonArmCutover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X8LooseCutover::RunTest(const FString&)
{
    FGridClean064X8World F;
    if (!TestNotNull(TEXT("World exists"), F.World) ||
        !TestNotNull(TEXT("Real registry configured"), F.Registry) ||
        !TestNotNull(TEXT("Transient dungeon runtime"), F.Runtime))
    {
        return false;
    }

    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/"
             "DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };

    FGridLooseItemInstance NonArm;
    FGridLooseItemInstance Arm;
    bool bFoundNonArm = false, bFoundArm = false;
    int32 RealLoose = 0;
    TArray<TStrongObjectPtr<UGridLevelAsset>> SourceLevels;
    for (const TCHAR* Path : Paths)
    {
        TStrongObjectPtr<UGridLevelAsset> Level(
            LoadObject<UGridLevelAsset>(nullptr, Path));
        if (!TestNotNull(TEXT("Migrated native level"), Level.Get()))
        {
            return false;
        }
        for (const FGridLooseItemInstance& Entry : Level->LooseItemInstances)
        {
            ++RealLoose;
            FGridResolvedItem Resolved;
            if (!TestTrue(TEXT("Real physical level resolves without default inheritance"),
                F.Registry->ResolveItem(Entry.CanonicalItemId, Entry.ItemLevel, Resolved)))
            {
                return false;
            }
            if (Resolved.bIsArmament && !bFoundArm)
            {
                Arm = Entry;
                bFoundArm = true;
            }
            else if (!Resolved.bIsArmament && !bFoundNonArm)
            {
                NonArm = Entry;
                bFoundNonArm = true;
            }
        }
        SourceLevels.Emplace(MoveTemp(Level));
    }

    if (!TestEqual(TEXT("Exactly 44 live authored loose instances"),
        RealLoose, 44) ||
        !TestTrue(TEXT("At least one registered ARM loose item exists"), bFoundArm) ||
        !TestTrue(TEXT("At least one nonARM loose item exists"), bFoundNonArm))
    {
        return false;
    }

    const auto PlaceAtOrigin = [&F](FGridLooseItemInstance Source) -> AGridItemActor*
    {
        Source.InstanceId = FGuid::NewGuid();
        Source.CellX = 0;
        Source.CellY = 0;
        Source.SurfaceSide = EGridEdge::None;
        Source.LocalOffset = FVector::ZeroVector;
        Source.LocalYaw = 0.f;
        F.Runtime->AddPlacedItemActor(Source);
        return F.FindItem(Source.InstanceId);
    };

    const auto FindRuntimeEntry = [&F](const AGridItemActor* Actor)
        -> const FGridSpawnedItemRuntimeEntry*
    {
        return F.Runtime->SpawnedItemEntries.FindByPredicate(
            [Actor](const FGridSpawnedItemRuntimeEntry& Entry)
            {
                return Entry.ItemActor.Get() == Actor;
            });
    };

    AGridItemActor* NonArmActor = PlaceAtOrigin(NonArm);
    if (!TestNotNull(TEXT("NonARM actor was spawned from persisted ID"),
        NonArmActor))
    {
        return false;
    }
    const FGridSpawnedItemRuntimeEntry* NonArmEntry = FindRuntimeEntry(NonArmActor);
    if (!TestNotNull(TEXT("Registered nonARM has a runtime entry"), NonArmEntry) ||
        !TestNull(TEXT("Registered nonARM runtime entry has no legacy source"),
            NonArmEntry->ItemDefinitionAsset.Get())) return false;

    FGridResolvedItem NonArmResolved;
    if (!TestTrue(TEXT("NonARM physical N0 resolves"),
        F.Registry->ResolveItem(NonArm.CanonicalItemId, NonArm.ItemLevel, NonArmResolved)) ||
        !TestNull(TEXT("NonARM actor no longer attaches legacy ItemDefinition"),
            NonArmActor->GetItemDefinitionAsset()) ||
        !TestTrue(TEXT("NonARM actor owns a transient canonical projection"),
            NonArmActor->CanonicalWorldPresentation.IsValid()) ||
        !TestEqual(TEXT("Physical N0 remains N0"),
            NonArmActor->CanonicalWorldItemLevel, 0) ||
        !TestEqual(TEXT("World identity persists"),
            NonArmActor->GetItemDefinitionId(), NonArm.CanonicalItemId) ||
        !TestEqual(TEXT("Canonical light configuration used at placement"),
            NonArmActor->LightEmitterComponent->RuntimeConfig.bUsePointLight,
            NonArmResolved.LightEmitter.bUsePointLight) ||
        !TestEqual(TEXT("Initial world light state from canonical authoring"),
            NonArmActor->AreItemLightsEnabled(),
            NonArmResolved.LightEmitter.bDefaultEnabled &&
                NonArmResolved.LightEmitter.bUsePointLight) ||
        !TestEqual(TEXT("Niagara systems configured from single registry"),
            NonArmActor->VisualEffectsComponent->GetConfiguredEffectCount(),
            NonArmResolved.VisualEffects.Effects.Num()))
    {
        return false;
    }

    // X.39: this is a real authored ARM row from one of four production
    // levels, WITH its original persisted DA_Weapon pointer still present.
    // The actor must not import a single presentation property from it.
    FGridResolvedItem ArmResolved;
    if (!TestNotNull(TEXT("Real ARM placement retains its historical source in the level asset"),
            Arm.ItemDefinition.Get()) ||
        !TestTrue(TEXT("Real ARM projection resolves at authored physical level"),
            F.Registry->ResolveItem(Arm.CanonicalItemId, Arm.ItemLevel, ArmResolved)))
    {
        return false;
    }
    AGridItemActor* ArmActor = PlaceAtOrigin(Arm);
    const FGridSpawnedItemRuntimeEntry* ArmEntry = FindRuntimeEntry(ArmActor);
    if (!TestNotNull(TEXT("Real ARM runtime entry exists"), ArmEntry) ||
        !TestNull(TEXT("X.40: Real ARM entry discards DA_Weapon source"),
            ArmEntry->ItemDefinitionAsset.Get()) ||
        !TestEqual(TEXT("X.40: Runtime entry retains authoritative ARM ID"),
            ArmEntry->ItemDefinitionId, Arm.CanonicalItemId) ||
        !TestEqual(TEXT("X.40: Runtime entry preserves authored ARM level"),
            ArmEntry->ItemLevel, Arm.ItemLevel))
    {
        return false;
    }
    if (!TestNotNull(TEXT("Real authored ARM actor spawned from canonical ItemId"),
            ArmActor) ||
        !TestNull(TEXT("Real ARM actor must never inherit the historic pointer"),
            ArmActor->GetItemDefinitionAsset()) ||
        !TestTrue(TEXT("Real authored ARM has full canonical world presentation"),
            ArmActor->CanonicalWorldPresentation.IsValid()) ||
        !TestEqual(TEXT("Persisted ARM physical level is never invented"),
            ArmActor->CanonicalWorldItemLevel, Arm.ItemLevel) ||
        !TestEqual(TEXT("ARM canonical identity remains authoritative"),
            ArmActor->GetItemDefinitionId(), Arm.CanonicalItemId) ||
        !TestTrue(TEXT("Real authored ARM has a stable valid GUID"),
            ArmActor->GetRuntimeObjectId().IsValid()) ||
        !TestTrue(TEXT("Real ARM mesh is canonical"),
            ArmActor->MeshComponent->GetStaticMesh() == ArmResolved.WorldMesh.LoadSynchronous()) ||
        !TestTrue(TEXT("Real ARM mesh transform is canonical"),
            ArmActor->MeshComponent->GetRelativeTransform().Equals(ArmResolved.WorldVisualTransform)) ||
        !TestEqual(TEXT("Real ARM persistent Niagara from canonical catalogue"),
            ArmActor->VisualEffectsComponent->GetConfiguredEffectCount(),
            ArmResolved.VisualEffects.Effects.Num()) ||
        !TestEqual(TEXT("Real ARM emitter configuration from canonical catalogue"),
            ArmActor->LightEmitterComponent->RuntimeConfig.bUsePointLight,
            ArmResolved.LightEmitter.bUsePointLight))
    {
        return false;
    }

    // A malicious/outdated same-ID legacy source is no longer visual
    // authority, even though it passes the authored ID consistency gate.
    FGridLooseItemInstance Contradictory = Arm;
    UGridItemDefinitionAsset* Fake = NewObject<UGridItemDefinitionAsset>(F.Runtime);
    Fake->ItemDefinitionId = Arm.CanonicalItemId;
    Fake->LightEmitter.bUsePointLight = !ArmResolved.LightEmitter.bUsePointLight;
    Fake->VisualEffects.Effects.Reset();
    Contradictory.ItemDefinition = Fake;
    AGridItemActor* ContradictoryActor = PlaceAtOrigin(Contradictory);
    const FGridSpawnedItemRuntimeEntry* FakeEntry = FindRuntimeEntry(ContradictoryActor);
    if (!TestNotNull(TEXT("Contradictory same-ID native runtime entry exists"), FakeEntry) ||
        !TestNull(TEXT("X.40: Contradictory same-ID source not retained in transient entry"),
            FakeEntry->ItemDefinitionAsset.Get()))
    {
        return false;
    }
    if (!TestNotNull(TEXT("Same-ID legacy source can no longer override canonical visuals"),
            ContradictoryActor) ||
        !TestNull(TEXT("Same-ID contradictory old asset is never attached"),
            ContradictoryActor->GetItemDefinitionAsset()) ||
        !TestEqual(TEXT("Canonical emitter unchanged by old contradictory source"),
            ContradictoryActor->LightEmitterComponent->RuntimeConfig.bUsePointLight,
            ArmResolved.LightEmitter.bUsePointLight) ||
        !TestEqual(TEXT("Canonical Niagara unchanged by old contradictory source"),
            ContradictoryActor->VisualEffectsComponent->GetConfiguredEffectCount(),
            ArmResolved.VisualEffects.Effects.Num()))
    {
        return false;
    }

    // Pointerless canonical ARM remain supported at both N1 and N20.
    for (const int32 Level : {1, 20})
    {
        FGridLooseItemInstance Pointerless = Arm;
        Pointerless.ItemDefinition = nullptr;
        Pointerless.ItemLevel = Level;
        Pointerless.InstanceId = FGuid::NewGuid();
        Pointerless.CellX = 0;
        Pointerless.CellY = 0;
        Pointerless.SurfaceSide = EGridEdge::None;
        Pointerless.LocalOffset = FVector::ZeroVector;
        Pointerless.LocalYaw = 0.f;
        F.Runtime->AddPlacedItemActor(Pointerless);
        AGridItemActor* Canonical = F.FindItem(Pointerless.InstanceId);
        FGridResolvedItem Projection;
        if (!TestNotNull(TEXT("Pointerless ARM actor is spawned"), Canonical) ||
            !TestTrue(TEXT("Pointerless ARM physical level is registered"),
                F.Registry->ResolveItem(Pointerless.CanonicalItemId, Level, Projection)) ||
            !TestNull(TEXT("Canonical ARM actor has no legacy definition"),
                Canonical->GetItemDefinitionAsset()) ||
            !TestEqual(TEXT("Canonical ARM runtime GUID unchanged"),
                Canonical->GetRuntimeObjectId(), Pointerless.InstanceId) ||
            !TestEqual(TEXT("Canonical ARM N level unchanged"),
                Canonical->CanonicalWorldItemLevel, Level) ||
            !TestEqual(TEXT("Canonical ARM ItemId unchanged"),
                Canonical->GetItemDefinitionId(), Pointerless.CanonicalItemId) ||
            !TestTrue(TEXT("Canonical ARM presentation is transient"),
                Canonical->CanonicalWorldPresentation.IsValid()) ||
            !TestTrue(TEXT("Canonical ARM world mesh from visual catalogue"),
                Canonical->MeshComponent->GetStaticMesh() == Projection.WorldMesh.LoadSynchronous()) ||
            !TestEqual(TEXT("Canonical ARM persistent Niagara configuration"),
                Canonical->VisualEffectsComponent->GetConfiguredEffectCount(),
                Projection.VisualEffects.Effects.Num()) ||
            !TestEqual(TEXT("Canonical ARM light configuration"),
                Canonical->LightEmitterComponent->RuntimeConfig.bUsePointLight,
                Projection.LightEmitter.bUsePointLight))
        {
            return false;
        }
    }

    // Every refusal must occur BEFORE the transient actor is created.
    FGridLooseItemInstance InvalidLevel = Arm;
    InvalidLevel.InstanceId = FGuid::NewGuid();
    InvalidLevel.ItemLevel = 0;
    InvalidLevel.CellX = 0;
    InvalidLevel.CellY = 0;
    F.Runtime->AddPlacedItemActor(InvalidLevel);
    if (!TestNull(TEXT("Known registered ARM N0 cannot spawn"),
        F.FindItem(InvalidLevel.InstanceId))) {return false;}

    FGridLooseItemInstance WrongSource = NonArm;
    WrongSource.InstanceId = FGuid::NewGuid();
    WrongSource.CanonicalItemId = Arm.CanonicalItemId;
    WrongSource.ItemLevel = Arm.ItemLevel;
    WrongSource.CellX = 0;
    WrongSource.CellY = 0;
    F.Runtime->AddPlacedItemActor(WrongSource);
    if (!TestNull(TEXT("Contradictory legacy source cannot spoof canonical ID"),
        F.FindItem(WrongSource.InstanceId))) {return false;}

    AddInfo(TEXT("[ARM-CLEAN06.4X40] 44 real loose items audited; "
        "all known canonical actors and their runtime entries exclude old "
        "source pointers, including real ARM and fake same-ID legacy. "
        "Mesh/light/Niagara/N levels remain canonical; no native assets changed."));
    return true;
}
#endif
