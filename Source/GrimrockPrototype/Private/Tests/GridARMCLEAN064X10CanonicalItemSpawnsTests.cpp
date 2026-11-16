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
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X10
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
            FName(*FString::Printf(TEXT("ARM_CLEAN064X10_%s"),
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
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10AInventory,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeA_NativeCount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10AInventory::RunTest(const FString&)
{
    const TCHAR* Paths[] = {
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.DA_GridLevel_00"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.DA_GridLevel_01"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.DA_GridLevel_New_Level_03"),
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.DA_GridLevel_Pillars_of_Light")
    };
    int32 Count = 0;
    for (const TCHAR* Path : Paths)
    {
        UGridLevelAsset* Level = LoadObject<UGridLevelAsset>(nullptr, Path);
        if (!TestNotNull(TEXT("Native level"), Level)) {return false;}
        const bool Dirty = Level->GetOutermost()->IsDirty();
        Count += Level->ItemSpawns.Num();
        if (!TestEqual(TEXT("Read-only"), Level->GetOutermost()->IsDirty(), Dirty))
            {return false;}
    }
    return TestEqual(TEXT("Four real levels have no typed spawn"), Count, 0);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10BIdentity,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeB_IdentityAndLevels",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10BIdentity::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Actual canonical catalogue"), Root.Get())) {return false;}
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Registry configured"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) {return false;}

    FGridItemSpawnInstance Spawn;
    Spawn.SpawnId = FGuid::NewGuid();
    Spawn.CanonicalItemId = TEXT("Item_Arrows_01");
    Spawn.ItemLevel = 0;
    Spawn.Quantity = 3;
    FGridLooseItemInstance Materialized;
    if (!TestTrue(TEXT("N0 canonical source materializes"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Materialized)))
        {return false;}
    return TestTrue(TEXT("Stable GUID"), Materialized.InstanceId == Spawn.SpawnId) &&
        TestEqual(TEXT("Canonical ID"), Materialized.CanonicalItemId, Spawn.CanonicalItemId) &&
        TestEqual(TEXT("Physical level"), Materialized.ItemLevel, 0) &&
        TestEqual(TEXT("Quantity"), Materialized.Quantity, 3) &&
        TestNull(TEXT("Legacy pointer not needed"), Materialized.ItemDefinition.Get());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10CRejections,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeC_RejectInvalidPersisted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10CRejections::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real catalogue"), Root.Get())) {return false;}
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Registry ready"), Registry->RegisterItemSystem(Root.Get(), Errors)))
        {return false;}
    FGridItemSpawnInstance Spawn;
    Spawn.SpawnId = FGuid::NewGuid();
    Spawn.CanonicalItemId = TEXT("Item_Arrows_01");
    Spawn.ItemLevel = 1;
    FGridLooseItemInstance Out;
    if (!TestFalse(TEXT("Arrows N1 invalid"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Out)))
        {return false;}
    Spawn.CanonicalItemId = TEXT("Weapon_StraightSword_01");
    Spawn.ItemLevel = 0;
    if (!TestFalse(TEXT("Weapon N0 invalid"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Out)))
        {return false;}
    Spawn.ItemLevel = 20;
    if (!TestTrue(TEXT("Weapon N20 valid"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Out)))
        {return false;}
    Spawn.CanonicalItemId = TEXT("ARM_CLEAN06_X10_UNKNOWN");
    if (!TestFalse(TEXT("Unknown ID invalid"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Out)))
        {return false;}
    Spawn.CanonicalItemId = TEXT("Item_Arrows_01");
    Spawn.ItemLevel = 0;
    Spawn.Quantity = 0;
    if (!TestFalse(TEXT("Zero quantity invalid"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Out)))
        {return false;}
    Spawn.Quantity = 1;
    Spawn.ItemDefinition = NewObject<UGridItemDefinitionAsset>(GI.Get());
    Spawn.ItemDefinition->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    return TestFalse(TEXT("Contradictory legacy pointer invalid"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Out));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10DWorld,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeD_RuntimeActor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10DWorld::RunTest(const FString&)
{
    ARMCLEAN064X10::FWorldFixture F;
    if (!TestNotNull(TEXT("Temporary world"), F.World) ||
        !TestNotNull(TEXT("Runtime"), F.Runtime) ||
        !TestNotNull(TEXT("Configured registry"), F.Registry))
        {return false;}
    FGridItemSpawnInstance Spawn;
    Spawn.SpawnId = FGuid::NewGuid();
    Spawn.CanonicalItemId = TEXT("Item_Arrows_01");
    Spawn.ItemLevel = 0;
    Spawn.Quantity = 3;
    FGridLooseItemInstance Initial;
    if (!TestTrue(TEXT("Read model creates placement"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Spawn, F.Registry, Initial)))
        {return false;}
    F.Runtime->AddPlacedItemActor(Initial);
    AGridItemActor* Actor = nullptr;
    for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
    {
        if (It->GetRuntimeObjectId() == Spawn.SpawnId) {Actor = *It; break;}
    }
    if (!TestNotNull(TEXT("Spawned physical actor"), Actor)) {return false;}
    return TestEqual(TEXT("World actor identity"), Actor->GetItemDefinitionId(), Spawn.CanonicalItemId) &&
        TestNull(TEXT("World actor uses no old definition"), Actor->GetItemDefinitionAsset()) &&
        TestTrue(TEXT("Actor has canonical projection"), Actor->CanonicalWorldPresentation.IsValid());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10ELegacy,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeE_SyntheticCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10ELegacy::RunTest(const FString&)
{
    FGridItemSpawnInstance Synthetic;
    Synthetic.SpawnId = FGuid::NewGuid();
    Synthetic.CanonicalItemId = NAME_None;
    Synthetic.ItemDefinition = NewObject<UGridItemDefinitionAsset>();
    Synthetic.ItemDefinition->ItemDefinitionId = TEXT("Legacy_Synthetic_Item");
    FGridLooseItemInstance Out;
    return TestTrue(TEXT("Fixture without canonical ID materializes"),
        FGridCanonicalItemSpawnReadModel::TryMaterialize(Synthetic, nullptr, Out)) &&
        TestTrue(TEXT("Fixture remains unconverted"), Out.CanonicalItemId.IsNone()) &&
        TestTrue(TEXT("Old source pointer remains"),
            Out.ItemDefinition == Synthetic.ItemDefinition);
}
#endif
