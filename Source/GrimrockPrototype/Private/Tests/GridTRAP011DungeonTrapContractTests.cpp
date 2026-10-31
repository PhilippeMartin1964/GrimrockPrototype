#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace TRAP011
{
    bool HasContractError(const TArray<FGridWorldObjectDefinitionValidationMessage>& Messages,
        const TCHAR* Text)
    {
        return Messages.ContainsByPredicate([Text](const FGridWorldObjectDefinitionValidationMessage& Message)
        {
            return Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error &&
                Message.Message.Contains(Text);
        });
    }

    bool PropertyIsSaveGame(const TCHAR* PropertyName)
    {
        const FProperty* Property = FGridLevelRuntimeState::StaticStruct()->FindPropertyByName(FName(PropertyName));
        return Property && Property->HasAnyPropertyFlags(CPF_SaveGame);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP011DefinitionContract,
    "Grimrock.TRAP01.1.DefinitionContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP011DefinitionContract::RunTest(const FString&)
{
    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
    Definition->DefinitionId = TEXT("TRAP01_Test_Trigger");
    Definition->SupportedType = EGridLevelObjectType::Trigger;
    TestFalse(TEXT("Legacy Trigger is never a dungeon trap"),
        Definition->DungeonTrap.bIsDungeonTrap);
    TestTrue(TEXT("Dungeon trap defaults hidden"), Definition->DungeonTrap.bInitiallyHidden);
    TestFalse(TEXT("Ordinary Trigger remains non-disarmable"), Definition->bCanBeDisarmed);

    Definition->DungeonTrap.bIsDungeonTrap = true;
    Definition->bCanBeDisarmed = true;
    Definition->DisarmDifficulty = 10;
    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    Definition->ValidateDefinition(Messages);
    TestFalse(TEXT("Valid floor trap contract"), TRAP011::HasContractError(Messages, TEXT("Dungeon trap")));
    TestFalse(TEXT("Valid opt-in disarm contract"), TRAP011::HasContractError(Messages, TEXT("Disarmable dungeon traps")));

    Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
    Definition->ValidateDefinition(Messages);
    TestFalse(TEXT("Wall-mounted Trigger trap uses same core contract"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap")));
    Definition->PlacementSurface = EGridObjectPlacementKind::Ceiling;
    Definition->ValidateDefinition(Messages);
    TestFalse(TEXT("Ceiling-mounted Trigger trap uses same core contract"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap")));

    Definition->DungeonTrap.bInitiallyHidden = false;
    Definition->ValidateDefinition(Messages);
    TestTrue(TEXT("Visible-by-default trap is rejected"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap contract invalid")));
    Definition->DungeonTrap.bInitiallyHidden = true;

    Definition->DungeonTrap.DetectionDifficulty = 0;
    Definition->ValidateDefinition(Messages);
    TestTrue(TEXT("Detection DC below 1 rejected"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap contract invalid")));
    Definition->DungeonTrap.DetectionDifficulty = 10;
    Definition->DungeonTrap.MaxDetectionRangeCells = 5;
    Definition->ValidateDefinition(Messages);
    TestTrue(TEXT("Detection range above cap rejected"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap contract invalid")));
    Definition->DungeonTrap.MaxDetectionRangeCells = 4;

    Definition->DungeonTrap.MinimumDamage = 9;
    Definition->DungeonTrap.MaximumDamage = 3;
    Definition->ValidateDefinition(Messages);
    TestTrue(TEXT("Inverted damage interval rejected"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap contract invalid")));
    Definition->DungeonTrap.MinimumDamage = 4;
    Definition->DungeonTrap.MaximumDamage = 8;

    Definition->DungeonTrap.DamageType = EGridDamageType::Fire;
    Definition->DungeonTrap.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
    Definition->ValidateDefinition(Messages);
    TestTrue(TEXT("Fire cannot masquerade as piercing physical damage"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap contract invalid")));
    Definition->DungeonTrap.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
    Definition->ValidateDefinition(Messages);
    TestFalse(TEXT("Valid Fire damage subtype accepted"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap contract invalid")));

    Definition->SupportedType = EGridLevelObjectType::Decoration;
    Definition->ValidateDefinition(Messages);
    TestTrue(TEXT("Dungeon trap cannot use Decoration gameplay"),
        TRAP011::HasContractError(Messages, TEXT("Dungeon trap gameplay requires")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP011RuntimeKnowledge,
    "Grimrock.TRAP01.1.PersistentInstanceStates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP011RuntimeKnowledge::RunTest(const FString&)
{
    TestTrue(TEXT("Detected state is SaveGame"), TRAP011::PropertyIsSaveGame(TEXT("DetectedTriggerTrapIds")));
    TestTrue(TEXT("Disarmed state is SaveGame"), TRAP011::PropertyIsSaveGame(TEXT("DisarmedTriggerTrapIds")));
    TestTrue(TEXT("Triggered state is SaveGame"), TRAP011::PropertyIsSaveGame(TEXT("TriggeredTriggerTrapIds")));
    TestTrue(TEXT("Disabled state is SaveGame"), TRAP011::PropertyIsSaveGame(TEXT("DisabledTriggerTrapIds")));
    TestTrue(TEXT("Perception attempt masks are SaveGame"),
        TRAP011::PropertyIsSaveGame(TEXT("TrapPerceptionAttemptedRangeMask")));

    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
        .ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("TRAP011_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient world"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
    }
    auto DestroyWorld = [&]()
    {
        World->DestroyWorld(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
    };

    AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Runtime actor"), Runtime))
    {
        DestroyWorld();
        return false;
    }
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Runtime->LevelAsset = Level;

    UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Definition->DefinitionId = TEXT("TRAP01_Test_InvisibleHazard");
    Definition->SupportedType = EGridLevelObjectType::Trigger;
    Definition->DungeonTrap.bIsDungeonTrap = true;
    Definition->bCanBeDisarmed = true;
    Runtime->WorldObjectDefinitions.Add(Definition);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.WorldObjectDefinitionId = Definition->DefinitionId;
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.CellX = 3;
    Trap.CellY = 7;
    Level->WorldObjectInstances.Add(Trap);

    TestEqual(TEXT("Unknown GUID has no trap state"),
        Runtime->GetDungeonTrapState(FGuid::NewGuid()), EGridDungeonTrapState::Unknown);
    TestEqual(TEXT("Newly authored trap is hidden"),
        Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Hidden);
    int32 Difficulty = 0;
    TestFalse(TEXT("Unseen trap cannot disclose disarm difficulty"),
        Runtime->GetTriggerTrapDisarmDifficulty(Trap.InstanceId, Difficulty));
    TestTrue(TEXT("First explicit detection records durable knowledge"),
        Runtime->TryRecordDungeonTrapDetection(Trap.InstanceId));
    TestFalse(TEXT("Repeated detection cannot re-award discovery"),
        Runtime->TryRecordDungeonTrapDetection(Trap.InstanceId));
    TestEqual(TEXT("Detected trap remains armed"),
        Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Detected);
    TestTrue(TEXT("Known trap exposes its disarm difficulty"),
        Runtime->GetTriggerTrapDisarmDifficulty(Trap.InstanceId, Difficulty));
    TestEqual(TEXT("Difficulty comes from definition"), Difficulty, 10);

    FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Runtime state"), State))
    {
        DestroyWorld();
        return false;
    }
    State->TrapPerceptionAttemptedRangeMask.Add(Trap.InstanceId, static_cast<uint8>(0x03));
    State->TriggeredTriggerTrapIds.Add(Trap.InstanceId);
    TestEqual(TEXT("Triggered state takes precedence over detection"),
        Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Triggered);
    State->DisarmedTriggerTrapIds.Add(Trap.InstanceId);
    TestEqual(TEXT("Disarmed state takes precedence over previous activation"),
        Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Disarmed);
    State->DisabledTriggerTrapIds.Add(Trap.InstanceId);
    TestEqual(TEXT("Explicit puzzle disablement takes highest precedence"),
        Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Disabled);

    const FGridDungeonRuntimeState Snapshot = Runtime->DungeonRuntimeState;
    AGridLevelRuntimeActor* Restored = World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Restored runtime actor"), Restored))
    {
        DestroyWorld();
        return false;
    }
    Restored->LevelAsset = Level;
    Restored->WorldObjectDefinitions.Add(Definition);
    Restored->DungeonRuntimeState = Snapshot;
    TestEqual(TEXT("Persisted trap state after snapshot round trip"),
        Restored->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Disabled);
    const FGridLevelRuntimeState* RestoredState = Restored->FindRuntimeStateForCurrentLevel();
    TestNotNull(TEXT("Restored level state"), RestoredState);
    if (RestoredState)
    {
        TestEqual(TEXT("Perception attempts persist by GUID"),
            RestoredState->TrapPerceptionAttemptedRangeMask.FindRef(Trap.InstanceId), static_cast<uint8>(0x03));
        TestTrue(TEXT("Detected IDs persist"), RestoredState->DetectedTriggerTrapIds.Contains(Trap.InstanceId));
        TestTrue(TEXT("Triggered IDs persist"), RestoredState->TriggeredTriggerTrapIds.Contains(Trap.InstanceId));
    }
    DestroyWorld();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
