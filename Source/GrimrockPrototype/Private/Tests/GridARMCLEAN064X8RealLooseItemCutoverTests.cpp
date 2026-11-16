#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemActor.h"
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

    AGridItemActor* NonArmActor = PlaceAtOrigin(NonArm);
    if (!TestNotNull(TEXT("NonARM actor was spawned from persisted ID"),
        NonArmActor))
    {
        return false;
    }
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

    AGridItemActor* ArmActor = PlaceAtOrigin(Arm);
    if (!TestNotNull(TEXT("ARM actor remains present during later visual cutover"),
        ArmActor) ||
        !TestTrue(TEXT("ARM presentation retains historic pointer temporarily"),
            ArmActor->GetItemDefinitionAsset() != nullptr) ||
        !TestEqual(TEXT("Persisted ARM physical level is never invented"),
            ArmActor->CanonicalWorldItemLevel, Arm.ItemLevel) ||
        !TestEqual(TEXT("ARM canonical identity remains authoritative"),
            ArmActor->GetItemDefinitionId(), Arm.CanonicalItemId))
    {
        return false;
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

    AddInfo(TEXT("[ARM-CLEAN06.4X8] 44 native loose items audited, "
        "nonARM physical actor uses canonical-only presentation, "
        "ARM visuals retained until own cutover; no UAsset/SaveGame write."));
    return true;
}
#endif
