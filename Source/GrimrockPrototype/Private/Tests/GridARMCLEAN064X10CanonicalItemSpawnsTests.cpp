#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "../Runtime/GridCanonicalItemSpawnReadModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10ItemSpawns,
    "Grimrock.RPG.ARM.CLEAN06_4X10.CanonicalItemSpawnMaterialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X10ItemSpawns::RunTest(const FString&)
{
    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    int32 ActualSpawns = 0;
    for (const TCHAR* Path : Paths)
    {
        UGridLevelAsset* Level = LoadObject<UGridLevelAsset>(nullptr, Path);
        if (!TestNotNull(TEXT("Native level exists"), Level)) {return false;}
        const bool bWasDirty = Level->GetOutermost()->IsDirty();
        ActualSpawns += Level->ItemSpawns.Num();
        if (!TestEqual(TEXT("Native level left unchanged"),
            Level->GetOutermost()->IsDirty(), bWasDirty)) {return false;}
    }
    if (!TestEqual(TEXT("No real generator to remigrate"), ActualSpawns, 0))
        {return false;}

    UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("ARM_CLEAN064X10_%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Temporary runtime world"), World) || !GEngine)
        {return false;}
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
    GI->Init();
    World->SetGameInstance(GI);
    UGridItemSubsystem* Registry = GI->GetSubsystem<UGridItemSubsystem>();
    AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
    bool bPassed = TestNotNull(TEXT("Native item registry"), Registry) &&
        TestNotNull(TEXT("Dungeon runtime actor"), Runtime);
    if (bPassed)
    {
        Runtime->LevelAsset = NewObject<UGridLevelAsset>(Runtime);
        Runtime->LevelAsset->Width = 1;
        Runtime->LevelAsset->Height = 1;
        Runtime->LevelAsset->CellSize = 200.f;
        Runtime->LevelAsset->EnsureCellCount();

        FGridItemSpawnInstance Spawn;
        Spawn.SpawnId = FGuid::NewGuid();
        Spawn.CanonicalItemId = TEXT("Item_Arrows_01");
        Spawn.ItemLevel = 0;
        Spawn.Quantity = 3;
        FGridLooseItemInstance Materialized;
        bPassed &= TestTrue(TEXT("N0 canonical source materializes without legacy pointer"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry, Materialized));
        bPassed &= TestTrue(TEXT("Spawn GUID survives"), Materialized.InstanceId == Spawn.SpawnId);
        bPassed &= TestEqual(TEXT("Persisted ItemId survives"),
            Materialized.CanonicalItemId, Spawn.CanonicalItemId);
        bPassed &= TestEqual(TEXT("Real physical N0 survives"), Materialized.ItemLevel, 0);
        bPassed &= TestEqual(TEXT("Quantity survives"), Materialized.Quantity, 3);
        if (bPassed)
        {
            Runtime->AddPlacedItemActor(Materialized);
            AGridItemActor* Generated = nullptr;
            for (TActorIterator<AGridItemActor> It(World); It; ++It)
            {
                if (It->GetRuntimeObjectId() == Spawn.SpawnId)
                    {Generated = *It; break;}
            }
            bPassed &= TestNotNull(TEXT("Canonical spawn produces physical actor"), Generated);
            if (Generated)
            {
                bPassed &= TestEqual(TEXT("Spawn actor ItemId"), Generated->GetItemDefinitionId(),
                    Spawn.CanonicalItemId);
                bPassed &= TestNull(TEXT("Canonical N0 spawn actor does not use legacy"),
                    Generated->GetItemDefinitionAsset());
                bPassed &= TestTrue(TEXT("Canonical source configured on actor"),
                    Generated->CanonicalWorldPresentation.IsValid());
            }
        }

        Spawn.ItemLevel = 1;
        bPassed &= TestFalse(TEXT("Canonical N0 family rejects N1"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry, Materialized));
        Spawn.CanonicalItemId = TEXT("Weapon_StraightSword_01");
        Spawn.ItemLevel = 0;
        bPassed &= TestFalse(TEXT("ARM N0 cannot inherit invented N1"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry, Materialized));
        Spawn.ItemLevel = 20;
        bPassed &= TestTrue(TEXT("Known ARM N20 is physically valid"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry, Materialized));
        Spawn.CanonicalItemId = TEXT("ARM_CLEAN06_X10_UNKNOWN");
        bPassed &= TestFalse(TEXT("Unknown persisted identity has no fallback"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry, Materialized));
        Spawn.CanonicalItemId = TEXT("Item_Arrows_01");
        Spawn.ItemLevel = 0;
        Spawn.Quantity = 0;
        bPassed &= TestFalse(TEXT("Persisted nonpositive quantity is rejected"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry, Materialized));
        Spawn.Quantity = 1;
        Spawn.ItemDefinition = NewObject<UGridItemDefinitionAsset>(World);
        Spawn.ItemDefinition->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
        bPassed &= TestFalse(TEXT("Contradictory old pointer cannot rename canonical item"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry, Materialized));
        Spawn.CanonicalItemId = NAME_None;
        bPassed &= TestTrue(TEXT("Legacy-only synthetic fixture preserved"),
            FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, nullptr, Materialized));
        bPassed &= TestTrue(TEXT("Synthetic fixture remains without persisted identity"),
            Materialized.CanonicalItemId.IsNone());
    }
    World->SetGameInstance(nullptr);
    GI->Shutdown();
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    if (bPassed)
        {AddInfo(TEXT("[ARM-CLEAN06.4X.10] zero native spawns; synthetic generation preserves GUID, N0, quantity, no legacy downgrade."));}
    return bPassed && !HasAnyErrors();
}
#endif
