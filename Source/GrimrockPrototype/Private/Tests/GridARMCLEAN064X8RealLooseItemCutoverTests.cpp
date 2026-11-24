#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridItemCatalogues.h"
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
                F.Registry->ResolveItem(Entry.ItemId, Entry.ItemLevel, Resolved)))
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
        !TestNull(TEXT("World runtime entry schema has no legacy source"),
            FGridSpawnedItemRuntimeEntry::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")))) return false;

    FGridResolvedItem NonArmResolved;
    if (!TestTrue(TEXT("NonARM physical N0 resolves"),
        F.Registry->ResolveItem(NonArm.ItemId, NonArm.ItemLevel, NonArmResolved)) ||
        !TestTrue(TEXT("NonARM actor owns a transient canonical projection"),
            NonArmActor->ResolvedWorldPresentation.IsValid()) ||
        !TestEqual(TEXT("Physical N0 remains N0"),
            NonArmActor->WorldItemLevel, 0) ||
        !TestEqual(TEXT("World identity persists"),
            NonArmActor->GetItemDefinitionId(), NonArm.ItemId) ||
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

    // The real migrated ARM resolves only from the canonical placement schema.
    FGridResolvedItem ArmResolved;
    if (!TestTrue(TEXT("Real ARM projection resolves at authored physical level"),
            F.Registry->ResolveItem(Arm.ItemId, Arm.ItemLevel, ArmResolved)) ||
        !TestNull(TEXT("Legacy placement property removed from schema"), FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition"))))
        return false;
    AGridItemActor* ArmActor = PlaceAtOrigin(Arm);
    const FGridSpawnedItemRuntimeEntry* ArmEntry = FindRuntimeEntry(ArmActor);
    if (!TestNotNull(TEXT("Real ARM runtime entry exists"), ArmEntry) ||
        !TestEqual(TEXT("X.40: Runtime entry retains authoritative ARM ID"),
            ArmEntry->ItemDefinitionId, Arm.ItemId) ||
        !TestEqual(TEXT("X.40: Runtime entry preserves authored ARM level"),
            ArmEntry->ItemLevel, Arm.ItemLevel))
    {
        return false;
    }
    if (!TestNotNull(TEXT("Real authored ARM actor spawned from canonical ItemId"),
            ArmActor) ||
        !TestTrue(TEXT("Real authored ARM has full canonical world presentation"),
            ArmActor->ResolvedWorldPresentation.IsValid()) ||
        !TestEqual(TEXT("Persisted ARM physical level is never invented"),
            ArmActor->WorldItemLevel, Arm.ItemLevel) ||
        !TestEqual(TEXT("ARM canonical identity remains authoritative"),
            ArmActor->GetItemDefinitionId(), Arm.ItemId) ||
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

    FGridLooseItemInstance MissingId = Arm;
    MissingId.ItemId = NAME_None;
    if (!TestNull(TEXT("Missing authored ItemId cannot spawn"), PlaceAtOrigin(MissingId)))
        return false;

    // Pointerless canonical ARM remain supported at both N1 and N20.
    for (const int32 Level : {1, 20})
    {
        FGridLooseItemInstance Pointerless = Arm;
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
                F.Registry->ResolveItem(Pointerless.ItemId, Level, Projection)) ||
            !TestEqual(TEXT("Canonical ARM runtime GUID unchanged"),
                Canonical->GetRuntimeObjectId(), Pointerless.InstanceId) ||
            !TestEqual(TEXT("Canonical ARM N level unchanged"),
                Canonical->WorldItemLevel, Level) ||
            !TestEqual(TEXT("Canonical ARM ItemId unchanged"),
                Canonical->GetItemDefinitionId(), Pointerless.ItemId) ||
            !TestTrue(TEXT("Canonical ARM presentation is transient"),
                Canonical->ResolvedWorldPresentation.IsValid()) ||
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

    FGridLooseItemInstance WrongLevel = NonArm;
    WrongLevel.InstanceId = FGuid::NewGuid();
    WrongLevel.ItemLevel = 1;
    WrongLevel.CellX = 0;
    WrongLevel.CellY = 0;
    F.Runtime->AddPlacedItemActor(WrongLevel);
    if (!TestNull(TEXT("Non-ARM with invented physical N1 cannot spawn"),
        F.FindItem(WrongLevel.InstanceId))) {return false;}

    FGridLooseItemInstance Invalid = Arm;
    Invalid.Quantity = 0;
    if (!TestNull(TEXT("Zero quantity cannot spawn"), PlaceAtOrigin(Invalid))) return false;
    Invalid = Arm;
    Invalid.ItemId = TEXT("X8_UnknownIdentity");
    if (!TestNull(TEXT("Unknown identity cannot spawn"), PlaceAtOrigin(Invalid))) return false;
    Invalid = Arm;
    Invalid.InstanceId.Invalidate();
    Invalid.CellX = Invalid.CellY = 0;
    const int32 Before = F.Runtime->SpawnedItemEntries.Num();
    F.Runtime->AddPlacedItemActor(Invalid);
    if (!TestEqual(TEXT("Invalid GUID cannot generate a replacement"), F.Runtime->SpawnedItemEntries.Num(), Before))
        return false;
    AddInfo(TEXT("[ARM-CLEAN06.4X47] 44 native loose items; strict pointerless identity, physical level and presentation."));
    return true;
}
#endif
