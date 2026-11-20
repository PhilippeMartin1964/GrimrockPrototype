#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

namespace ARMCLEAN064X92
{
struct FWorldFixture
{
    UWorld* World = nullptr;
    UGameInstance* GI = nullptr;
    UGridItemSubsystem* Registry = nullptr;
    AGridLevelRuntimeActor* Runtime = nullptr;

    FWorldFixture()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("ARM_CLEAN064X92_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World || !GEngine) {return;}
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        GI = NewObject<UGameInstance>(GetTransientPackage());
        if (!GI) {return;}
        GI->Init();
        World->SetGameInstance(GI);
        Registry = GI->GetSubsystem<UGridItemSubsystem>();
        Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
        if (Runtime)
        {
            Runtime->LevelAsset = NewObject<UGridLevelAsset>(Runtime);
            Runtime->LevelAsset->Width = 1;
            Runtime->LevelAsset->Height = 1;
            Runtime->LevelAsset->CellSize = 200.f;
            Runtime->LevelAsset->EnsureCellCount();
        }
    }

    ~FWorldFixture()
    {
        if (World) {World->SetGameInstance(nullptr);}
        if (GI) {GI->Shutdown();}
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) {GEngine->DestroyWorldContext(World);}
        }
    }

    AGridReceptacleActor* MakeReceptacle(const FGridReceptacleInitialItemConfig& Initial) const
    {
        if (!World || !Runtime) {return nullptr;}
        FActorSpawnParameters Params;
        Params.Owner = Runtime;
        AGridReceptacleActor* Receptacle = World->SpawnActor<AGridReceptacleActor>(
            FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (!Receptacle) {return nullptr;}
        FGridRuntimeWorldObjectData Data;
        Data.ObjectId = FGuid::NewGuid();
        Data.CellX = 0;
        Data.CellY = 0;
        Data.Behavior.Receptacle.MaxContainedItems = 1;
        Data.Behavior.Receptacle.InitialContent.Add(Initial);
        Receptacle->InitializeRuntimeWorldObject(Data, nullptr, FTransform::Identity);
        return Receptacle;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X92RealInitialPresentation,
    "Grimrock.RPG.ARM.CLEAN06_4X92.RealInitialCanonicalReceptaclePresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X92RealInitialPresentation::RunTest(const FString&)
{
    ARMCLEAN064X92::FWorldFixture F;
    if (!TestNotNull(TEXT("Native world"), F.World) ||
        !TestNotNull(TEXT("Canonical registry"), F.Registry) ||
        !TestNotNull(TEXT("Dungeon runtime"), F.Runtime))
    {
        return false;
    }

    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    int32 RealCount = 0;
    bool bHasNonArm = false;
    bool bHasArm = false;
    FGridReceptacleInitialItemConfig NonArm;
    FGridReceptacleInitialItemConfig Arm;
    for (const TCHAR* Path : Paths)
    {
        UGridLevelAsset* Level = LoadObject<UGridLevelAsset>(nullptr, Path);
        if (!TestNotNull(TEXT("Real level asset"), Level)) {return false;}
        const bool bWasDirty = Level->GetOutermost()->IsDirty();
        for (const FGridWorldObjectInstance& World : Level->WorldObjectInstances)
        {
            for (const FGridReceptacleInitialItemConfig& Source : World.InstanceConfig.ReceptacleInitialContent)
            {
                ++RealCount;
                FGridResolvedItem Resolved;
                if (!TestTrue(TEXT("Persisted source is in one registry"),
                    F.Registry->ResolveItem(Source.CanonicalItemId, Source.ItemLevel, Resolved)))
                    {return false;}
                if (!Resolved.bIsArmament && !bHasNonArm)
                    {NonArm = Source; bHasNonArm = true;}
                if (Resolved.bIsArmament && !bHasArm)
                    {Arm = Source; bHasArm = true;}
            }
        }
        if (!TestEqual(TEXT("No level package modified"), Level->GetOutermost()->IsDirty(), bWasDirty))
            {return false;}
    }
    if (!TestEqual(TEXT("18 real initial entries remain"), RealCount, 18) ||
        !TestTrue(TEXT("At least one nonARM initial object"), bHasNonArm))
        {return false;}

    AGridReceptacleActor* R = F.MakeReceptacle(NonArm);
    if (!TestNotNull(TEXT("Real nonARM initial receptacle"), R) ||
        !TestEqual(TEXT("Initial logical item remains"), R->GetContainedItemCount(), 1))
        {return false;}
    const FGridContainedReceptacleItem& Entry = R->ContainedItems[0];
    AGridItemActor* Actor = R->GetContainedItemActor(0);
    FGridResolvedItem Canonical;
    if (!TestNotNull(TEXT("Physical actor exists"), Actor) ||
        !TestTrue(TEXT("Physical N0 projection resolves"),
            F.Registry->ResolveItem(NonArm.CanonicalItemId, NonArm.ItemLevel, Canonical)))
        {return false;}
    if (!TestTrue(TEXT("Initial logical status retained"), Entry.bWasInitialItem) ||
        !TestEqual(TEXT("Logical identity retained"), Entry.ItemDefinitionId, NonArm.CanonicalItemId) ||
        !TestEqual(TEXT("Actual quantity retained"), Entry.Quantity, NonArm.Quantity) ||
        !TestEqual(TEXT("Physical N0 retained"), Entry.ItemLevel, NonArm.ItemLevel) ||
        !TestNull(TEXT("Physical actor no longer reads legacy definition"),
            Actor->GetItemDefinitionAsset()) ||
        !TestTrue(TEXT("Physical actor projects canonical family"),
            Actor->CanonicalWorldPresentation.IsValid()) ||
        !TestEqual(TEXT("Actor ItemId"), Actor->GetItemDefinitionId(), NonArm.CanonicalItemId) ||
        !TestEqual(TEXT("Actor level"), Actor->CanonicalWorldItemLevel, NonArm.ItemLevel) ||
        !TestTrue(TEXT("Actor GUID is the contained item GUID"),
            Actor->GetRuntimeObjectId() == Entry.RuntimeObjectId) ||
        !TestEqual(TEXT("Light configuration from canonical source"),
            Actor->LightEmitterComponent->RuntimeConfig.bUsePointLight,
            Canonical.LightEmitter.bUsePointLight) ||
        !TestEqual(TEXT("Initial enabled light from canonical source"),
            Entry.bLightsEnabled, Canonical.LightEmitter.bDefaultEnabled) ||
        !TestEqual(TEXT("Niagara systems from canonical source"),
            Actor->VisualEffectsComponent->GetConfiguredEffectCount(),
            Canonical.VisualEffects.Effects.Num()))
        {return false;}

    FGridReceptacleInitialItemConfig Pointerless = NonArm;
    AGridReceptacleActor* PR = F.MakeReceptacle(Pointerless);
    if (!TestNotNull(TEXT("Pointerless real nonARM host"), PR) ||
        !TestEqual(TEXT("Pointerless nonARM content count"), PR->GetContainedItemCount(), 1))
        return false;
    AGridItemActor* PA = PR->GetContainedItemActor(0);
    if (!TestNotNull(TEXT("Pointerless physical actor"), PA) ||
        !TestNull(TEXT("No source on pointerless physical actor"), PA->GetItemDefinitionAsset()) ||
        !TestTrue(TEXT("Exact item quantity, level, light and identity"),
            PR->ContainedItems[0].Quantity == Entry.Quantity &&
            PR->ContainedItems[0].ItemLevel == Entry.ItemLevel &&
            PR->ContainedItems[0].ItemDefinitionId == Entry.ItemDefinitionId &&
            PR->ContainedItems[0].bLightsEnabled == Entry.bLightsEnabled) ||
        !TestEqual(TEXT("Pointerless Niagara parity"),
            PA->VisualEffectsComponent->GetConfiguredEffectCount(),
            Actor->VisualEffectsComponent->GetConfiguredEffectCount()))
        return false;

    // A persisted nonARM cannot become N1 or fall back to its old pointer.
    FGridReceptacleInitialItemConfig Invalid = NonArm;
    Invalid.ItemLevel = 1;
    AGridReceptacleActor* Rejected = F.MakeReceptacle(Invalid);
    if (!TestNotNull(TEXT("Invalid source host"), Rejected) ||
        !TestEqual(TEXT("Persisted invalid level creates no item"),
            Rejected->GetContainedItemCount(), 0))
        {return false;}

    FGridReceptacleInitialItemConfig Empty;
    Empty.Quantity = 1;
    AGridReceptacleActor* EmptyHost = F.MakeReceptacle(Empty);
    if (!TestNotNull(TEXT("Empty-ID fixture host"), EmptyHost) ||
        !TestEqual(TEXT("Empty identity creates no content"), EmptyHost->GetContainedItemCount(), 0))
        return false;

    if (bHasArm)
    {
        AGridReceptacleActor* AR = F.MakeReceptacle(Arm);
        if (!TestNotNull(TEXT("ARM host remains"), AR) ||
            !TestEqual(TEXT("ARM initial count unchanged"), AR->GetContainedItemCount(), 1) ||
            !TestNotNull(TEXT("ARM physical actor"), AR->GetContainedItemActor(0)))
            {return false;}
        if (!TestNull(TEXT("Migrated ARM actor retains no historical source"),
                AR->GetContainedItemActor(0)->GetItemDefinitionAsset()) ||
            !TestTrue(TEXT("Migrated ARM has full canonical presentation"),
                AR->GetContainedItemActor(0)->CanonicalWorldPresentation.IsValid()))
            return false;
        if (!TestEqual(TEXT("Native ARM physical level"), AR->ContainedItems[0].ItemLevel, Arm.ItemLevel))
            return false;
    }
    AddInfo(TEXT("[ARM-CLEAN06.4X.9.2] 18 real initial entries audited; "
        "migrated nonARM/ARM actors canonical, empty identity rejected; no binary writes."));
    return true;
}
#endif
