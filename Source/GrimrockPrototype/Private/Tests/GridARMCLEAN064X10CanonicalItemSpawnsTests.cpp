#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "../Runtime/GridItemSpawnReadModel.h"
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
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Actual canonical catalogue"), Root.Get())) {return false;}
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Registry configured"),
        Registry->RegisterItemSystem(Root.Get(), Errors))) {return false;}

    FGridItemSpawnInstance Spawn;
    Spawn.SpawnId = FGuid::NewGuid();
    Spawn.ItemId = TEXT("Item_Arrows_01");
    Spawn.ItemLevel = 0;
    Spawn.Quantity = 3;
    FGridLooseItemInstance Materialized;
    if (!TestTrue(TEXT("N0 canonical source materializes"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, Registry.Get(), Materialized)))
        {return false;}
    return TestTrue(TEXT("Stable GUID"), Materialized.InstanceId == Spawn.SpawnId) &&
        TestEqual(TEXT("Canonical ID"), Materialized.ItemId, Spawn.ItemId) &&
        TestEqual(TEXT("Physical level"), Materialized.ItemLevel, 0) &&
        TestEqual(TEXT("Quantity"), Materialized.Quantity, 3) &&
        TestNull(TEXT("Materialized placement has no legacy property"), FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")));
}

namespace ARMCLEAN064X10
{
struct FCaseRegistry
{
    TStrongObjectPtr<UGridItemSystemAsset> Root;
    TStrongObjectPtr<UGameInstance> GI;
    TStrongObjectPtr<UGridItemSubsystem> Registry;
    bool bReady = false;
    FCaseRegistry()
        : Root(LoadObject<UGridItemSystemAsset>(
            nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")))
        , GI(NewObject<UGameInstance>())
        , Registry(NewObject<UGridItemSubsystem>(GI.Get()))
    {
        TArray<FString> Errors;
        bReady = Root.IsValid() && Registry.IsValid() &&
            Registry->RegisterItemSystem(Root.Get(), Errors);
    }
};
static FGridItemSpawnInstance MakeSpawn(FName Id, int32 Level, int32 Quantity = 1)
{
    FGridItemSpawnInstance Spawn;
    Spawn.SpawnId = FGuid::NewGuid();
    Spawn.ItemId = Id;
    Spawn.ItemLevel = Level;
    Spawn.Quantity = Quantity;
    return Spawn;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10C1AmmoN1,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeC1AmmoN1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10C1AmmoN1::RunTest(const FString&)
{
    ARMCLEAN064X10::FCaseRegistry Probe;
    if (!TestTrue(TEXT("Real canonical registry ready"), Probe.bReady)) {return false;}
    FGridItemSpawnInstance Spawn = ARMCLEAN064X10::MakeSpawn(TEXT("Item_Arrows_01"), 1, 1);
    FGridLooseItemInstance Out;
    return TestFalse(TEXT("Strict persisted physical identity condition"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, Probe.Registry.Get(), Out));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10C2WeaponN0,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeC2WeaponN0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10C2WeaponN0::RunTest(const FString&)
{
    ARMCLEAN064X10::FCaseRegistry Probe;
    if (!TestTrue(TEXT("Real canonical registry ready"), Probe.bReady)) {return false;}
    FGridItemSpawnInstance Spawn = ARMCLEAN064X10::MakeSpawn(TEXT("Weapon_StraightSword_01"), 0, 1);
    FGridLooseItemInstance Out;
    return TestFalse(TEXT("Strict persisted physical identity condition"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, Probe.Registry.Get(), Out));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10C3WeaponN20,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeC3WeaponN20",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10C3WeaponN20::RunTest(const FString&)
{
    ARMCLEAN064X10::FCaseRegistry Probe;
    if (!TestTrue(TEXT("Real canonical registry ready"), Probe.bReady)) {return false;}
    FGridItemSpawnInstance Spawn = ARMCLEAN064X10::MakeSpawn(TEXT("Weapon_StraightSword_01"), 20, 1);
    FGridLooseItemInstance Out;
    return TestTrue(TEXT("Strict persisted physical identity condition"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, Probe.Registry.Get(), Out));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10C4UnknownId,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeC4UnknownId",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10C4UnknownId::RunTest(const FString&)
{
    ARMCLEAN064X10::FCaseRegistry Probe;
    if (!TestTrue(TEXT("Real canonical registry ready"), Probe.bReady)) {return false;}
    FGridItemSpawnInstance Spawn = ARMCLEAN064X10::MakeSpawn(TEXT("ARM_CLEAN06_X10_UNKNOWN"), 0, 1);
    FGridLooseItemInstance Out;
    return TestFalse(TEXT("Strict persisted physical identity condition"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, Probe.Registry.Get(), Out));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10C5ZeroQuantity,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeC5ZeroQuantity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10C5ZeroQuantity::RunTest(const FString&)
{
    ARMCLEAN064X10::FCaseRegistry Probe;
    if (!TestTrue(TEXT("Real canonical registry ready"), Probe.bReady)) {return false;}
    FGridItemSpawnInstance Spawn = ARMCLEAN064X10::MakeSpawn(TEXT("Item_Arrows_01"), 0, 0);
    FGridLooseItemInstance Out;
    return TestFalse(TEXT("Strict persisted physical identity condition"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, Probe.Registry.Get(), Out));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10C6ContradictorySource,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeC6InvalidGuid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10C6ContradictorySource::RunTest(const FString&)
{
    ARMCLEAN064X10::FCaseRegistry Probe;
    if (!TestTrue(TEXT("Real canonical registry ready"), Probe.bReady)) {return false;}
    FGridItemSpawnInstance Spawn = ARMCLEAN064X10::MakeSpawn(TEXT("Item_Arrows_01"), 0, 1);
    Spawn.SpawnId.Invalidate();
    FGridLooseItemInstance Out;
    return TestFalse(TEXT("Strict persisted physical identity condition"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, Probe.Registry.Get(), Out));
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
    Spawn.ItemId = TEXT("Item_Arrows_01");
    Spawn.ItemLevel = 0;
    Spawn.Quantity = 3;
    FGridLooseItemInstance Initial;
    if (!TestTrue(TEXT("Read model creates placement"),
        FGridItemSpawnReadModel::TryMaterialize(Spawn, F.Registry, Initial)))
        {return false;}
    F.Runtime->AddPlacedItemActor(Initial);
    AGridItemActor* Actor = nullptr;
    for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
    {
        if (It->GetRuntimeObjectId() == Spawn.SpawnId) {Actor = *It; break;}
    }
    if (!TestNotNull(TEXT("Spawned physical actor"), Actor)) {return false;}
    return TestEqual(TEXT("World actor identity"), Actor->GetItemDefinitionId(), Spawn.ItemId) &&
        TestTrue(TEXT("Actor has canonical projection"), Actor->ResolvedWorldPresentation.IsValid());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X10ELegacy,
    "Grimrock.RPG.ARM.CLEAN06_4X10.ProbeE_StrictGeneratorSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean064X10ELegacy::RunTest(const FString&)
{
    ARMCLEAN064X10::FCaseRegistry Probe;
    if (!TestTrue(TEXT("Real canonical registry ready"), Probe.bReady)) return false;
    FGridItemSpawnInstance Synthetic;
    Synthetic.SpawnId = FGuid::NewGuid();
    FGridLooseItemInstance Out;
    if (!TestNull(TEXT("Historical generator field removed"),
            FGridItemSpawnInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition"))) ||
        !TestFalse(TEXT("Empty ID cannot materialize"),
            FGridItemSpawnReadModel::TryMaterialize(Synthetic, Probe.Registry.Get(), Out)))
        return false;
    Synthetic.ItemId = TEXT("Stone");
    if (!TestFalse(TEXT("Missing registry cannot materialize"),
            FGridItemSpawnReadModel::TryMaterialize(Synthetic, nullptr, Out)))
        return false;
    Synthetic.bSpawnAtStart = false;
    return TestFalse(TEXT("Disabled generator cannot materialize"),
        FGridItemSpawnReadModel::TryMaterialize(Synthetic, Probe.Registry.Get(), Out));
}
#endif
