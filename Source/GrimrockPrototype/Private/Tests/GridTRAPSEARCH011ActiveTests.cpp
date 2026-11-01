#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillTypes.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridDungeonTrapPerceptionService.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAPSEARCH011BonusAndState,
    "Grimrock.TRAP_SEARCH01.1.BonusAndSaveContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAPSEARCH011BonusAndState::RunTest(const FString&)
{
    FRPGSkillCheckResult Result;
    Result.bResolved = true;
    Result.Roll = 7;
    Result.Rank = 1;
    Result.AttributeModifier = 2;
    Result.ClassModifier = 0;
    Result.GroupModifier = 0;
    Result.ProgressionModifier = 0;
    Result.Total = 10;
    Result.Difficulty = 12;
    Result.FailureMargin = 2;
    Result.bSuccess = false;

    TestEqual(TEXT("Approved active search situational bonus"), 
        FGridDungeonTrapPerceptionService::ActiveSearchCheckBonus, 2);
    FGridDungeonTrapPerceptionService::ApplyActiveSearchBonus(Result);
    TestEqual(TEXT("The original d20 is never rerolled"), Result.Roll, 7);
    TestEqual(TEXT("Search bonus is added to the total"), Result.Total, 12);
    TestTrue(TEXT("The +2 can turn an otherwise failed check into a success"), Result.bSuccess);
    TestEqual(TEXT("Success resets failure margin"), Result.FailureMargin, 0);
    TestEqual(TEXT("The bonus is not disguised as a class modifier"), Result.ClassModifier, 0);
    TestEqual(TEXT("The bonus is not disguised as a group modifier"), Result.GroupModifier, 0);
    TestEqual(TEXT("The bonus does not modify RPG progression"), Result.ProgressionModifier, 0);
    TestEqual(TEXT("Actual authored difficulty remains unchanged"), Result.Difficulty, 12);

    Result.bResolved = false;
    Result.Total = 4;
    FGridDungeonTrapPerceptionService::ApplyActiveSearchBonus(Result);
    TestEqual(TEXT("Rejected skill check is never given a bonus"), Result.Total, 4);

    const FProperty* ActiveMask = FGridLevelRuntimeState::StaticStruct()
        ->FindPropertyByName(TEXT("TrapActiveSearchAttemptedRangeMask"));
    TestTrue(TEXT("Active search attempts are marked SaveGame"),
        ActiveMask && ActiveMask->HasAnyPropertyFlags(CPF_SaveGame));
    const FProperty* PassiveMask = FGridLevelRuntimeState::StaticStruct()
        ->FindPropertyByName(TEXT("TrapPerceptionAttemptedRangeMask"));
    TestTrue(TEXT("Passive attempts remain independently marked SaveGame"),
        PassiveMask && PassiveMask->HasAnyPropertyFlags(CPF_SaveGame));
    return true;
}

namespace TRAPSEARCH011
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;
        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("TRAPSEARCH011_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            if (World)
            {
                World->InitializeActorsForPlay(FURL());
            }
        }
        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };

    FGridWorldObjectInstance AddTrap(UGridLevelAsset* Level, FName DefinitionId,
        int32 X, int32 Y, int32 DC, int32 MaxRange = 4)
    {
        FGridWorldObjectInstance Trap;
        Trap.InstanceId = FGuid::NewGuid();
        Trap.Type = EGridLevelObjectType::Trigger;
        Trap.WorldObjectDefinitionId = DefinitionId;
        Trap.CellX = X;
        Trap.CellY = Y;
        Trap.InstanceConfig.DungeonTrap.bOverrideDetectionDifficulty = true;
        Trap.InstanceConfig.DungeonTrap.DetectionDifficulty = DC;
        Trap.InstanceConfig.DungeonTrap.bOverrideMaxDetectionRangeCells = true;
        Trap.InstanceConfig.DungeonTrap.MaxDetectionRangeCells = MaxRange;
        Level->WorldObjectInstances.Add(Trap);
        return Trap;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAPSEARCH011Runtime,
    "Grimrock.TRAP_SEARCH01.1.RuntimeDetectionAntiRerollAndSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAPSEARCH011Runtime::RunTest(const FString&)
{
    TRAPSEARCH011::FWorldFixture Scope;
    if (!TestNotNull(TEXT("Transient game world"), Scope.World)) return false;
    AGridLevelRuntimeActor* Runtime = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = Scope.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Dungeon runtime"), Runtime) ||
        !TestNotNull(TEXT("Party pawn"), Party) ||
        !TestNotNull(TEXT("Party inventory"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
    {
        return false;
    }
    const URPGSkillAsset* ProductionSkill = LoadObject<URPGSkillAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Perception.DA_Skill_Perception"));
    if (!TestNotNull(TEXT("Production Perception skill is available to the runtime"), ProductionSkill))
        return false;

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    for (int32 X = 2; X <= 7; ++X)
    {
        for (int32 Y = 1; Y <= 6; ++Y)
        {
            Level->GetCellMutable(X, Y).CellType = EGridCellType::Floor;
            Level->GetCellMutable(X, Y).bBlocksOccupancy = false;
        }
    }
    Runtime->LevelAsset = Level;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 3;
    Party->CurrentCellY = 2;
    Party->Facing = EGridEdge::North;
    FGridCharacterInventoryState Observer;
    Observer.CharacterId = FGuid::NewGuid();
    Observer.DisplayName = FText::FromString(TEXT("Observatrice"));
    Observer.Attributes.Wisdom = 10;
    Observer.Resources.CurrentHealth = 30;
    FRPGSkillRank Rank;
    Rank.SkillId = TEXT("Skill_Perception");
    Rank.Rank = 3; // Passive range must genuinely reach two cells.
    Observer.SkillRanks.Add(Rank);
    Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Add(Observer);

    UGridWorldObjectDefinitionAsset* Def = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Def->DefinitionId = TEXT("TRAPSEARCH011_Floor");
    Def->SupportedType = EGridLevelObjectType::Trigger;
    Def->PlacementSurface = EGridObjectPlacementKind::Floor;
    Def->DungeonTrap.bIsDungeonTrap = true;
    Def->DungeonTrap.DetectionSense = EGridDungeonTrapDetectionSense::Audible;
    Def->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
    Def->StaticPart.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Engine's test cube is present"), Def->StaticPart.Mesh.Get())) return false;
    Runtime->WorldObjectDefinitions.Add(Def);

    // Passive miss at two cells MUST NOT exhaust the distinct active attempt.
    const FGridWorldObjectInstance Hard =
        TRAPSEARCH011::AddTrap(Level, Def->DefinitionId, 3, 4, 40);
    FRandomStream Passive(15);
    TestEqual(TEXT("Difficult trap is not found passively"), 
        Runtime->EvaluatePassiveDungeonTrapPerception(Party, Passive), 0);
    FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Runtime level state"), State)) return false;
    TestEqual(TEXT("Passive distance 2 is spent"), 
        State->TrapPerceptionAttemptedRangeMask.FindRef(Hard.InstanceId), static_cast<uint8>(2));
    TestFalse(TEXT("Passive scan does not spend active search attempts"),
        State->TrapActiveSearchAttemptedRangeMask.Contains(Hard.InstanceId));

    // Only the success should be revealed. The other placements exercise
    // diagonal, range, authored cap and wall-blocked target exclusions.
    const FGridWorldObjectInstance Easy =
        TRAPSEARCH011::AddTrap(Level, Def->DefinitionId, 4, 2, 1);
    const FGridWorldObjectInstance Diagonal =
        TRAPSEARCH011::AddTrap(Level, Def->DefinitionId, 4, 3, 40);
    const FGridWorldObjectInstance TooFar =
        TRAPSEARCH011::AddTrap(Level, Def->DefinitionId, 3, 5, 40);
    const FGridWorldObjectInstance RangeCapped =
        TRAPSEARCH011::AddTrap(Level, Def->DefinitionId, 3, 4, 40, 1);
    const FGridWorldObjectInstance Blocked =
        TRAPSEARCH011::AddTrap(Level, Def->DefinitionId, 5, 2, 40);
    Level->GetCellMutable(4, 2).EastWall = EGridWallType::Solid;

    FRandomStream Active(213);
    const int32 BeforeActiveSeed = Active.GetCurrentSeed();
    TestEqual(TEXT("Only one eligible easy trap is revealed by active search"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 1);
    TestTrue(TEXT("An actual active roll consumed RNG"), Active.GetCurrentSeed() != BeforeActiveSeed);
    TestEqual(TEXT("Easy trap is discovered"), 
        Runtime->GetDungeonTrapState(Easy.InstanceId), EGridDungeonTrapState::Detected);
    TestNotNull(TEXT("Active detection uses the existing 3D reveal"),
        Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Easy.InstanceId));
    TestEqual(TEXT("Hard target spent active distance 2 after failing"), 
        State->TrapActiveSearchAttemptedRangeMask.FindRef(Hard.InstanceId), static_cast<uint8>(2));
    TestEqual(TEXT("Easy target spent active distance 1 on success"), 
        State->TrapActiveSearchAttemptedRangeMask.FindRef(Easy.InstanceId), static_cast<uint8>(1));
    TestEqual(TEXT("Passive mask remains separate and unchanged"),
        State->TrapPerceptionAttemptedRangeMask.FindRef(Hard.InstanceId), static_cast<uint8>(2));
    for (const FGuid Excluded : 
        { Diagonal.InstanceId, TooFar.InstanceId, RangeCapped.InstanceId, Blocked.InstanceId })
    {
        TestFalse(TEXT("Ineligible target spends no active attempt or RNG"),
            State->TrapActiveSearchAttemptedRangeMask.Contains(Excluded));
    }
    const UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
    if (TestNotNull(TEXT("Existing event feed"), Feed))
    {
        TestTrue(TEXT("A genuine active discovery produces the canonical detection event"),
            Feed->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& Entry)
            {
                return Entry.EventCode == FName(TEXT("Dungeon.Trap.Detected"));
            }));
        TestFalse(TEXT("A failed or empty search never publishes a negative clue"),
            Feed->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& Entry)
            {
                return Entry.EventCode == FName(TEXT("Dungeon.Trap.PerceptionScan"));
            }));
    }

    const int32 AfterFirstActive = Active.GetCurrentSeed();
    TestEqual(TEXT("Repeated active search cannot reroll same GUID/distance"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 0);
    TestEqual(TEXT("Repeat is RNG-free"), Active.GetCurrentSeed(), AfterFirstActive);

    // Moving one cell closer permits exactly one fresh distance-one attempt.
    Party->CurrentCellY = 3;
    TestEqual(TEXT("Previously missed hard trap remains unseen"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 0);
    TestEqual(TEXT("Separate active range bits are recorded"),
        State->TrapActiveSearchAttemptedRangeMask.FindRef(Hard.InstanceId), static_cast<uint8>(3));
    const int32 AfterSecondRange = Active.GetCurrentSeed();
    TestEqual(TEXT("Repeating the second range gives no further discoveries"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 0);
    TestEqual(TEXT("Repeating the second range uses no RNG"),
        Active.GetCurrentSeed(), AfterSecondRange);

    // Guard against combat before an unattempted, guaranteed-easy trap.
    Party->CurrentCellY = 2;
    const FGridWorldObjectInstance CombatTarget =
        TRAPSEARCH011::AddTrap(Level, Def->DefinitionId, 2, 2, 1);
    UGridTurnManagerComponent* Turns = Runtime->FindComponentByClass<UGridTurnManagerComponent>();
    if (!Turns)
    {
        Turns = NewObject<UGridTurnManagerComponent>(Runtime);
        Turns->RegisterComponent();
    }
    if (!TestNotNull(TEXT("Turn manager for combat guard"), Turns)) return false;
    Turns->bCombatActive = true;
    const int32 BeforeCombat = Active.GetCurrentSeed();
    TestEqual(TEXT("Active search is disabled in combat"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 0);
    TestEqual(TEXT("Combat denial uses no RNG"), Active.GetCurrentSeed(), BeforeCombat);
    TestFalse(TEXT("Combat denial consumes no search distance"),
        State->TrapActiveSearchAttemptedRangeMask.Contains(CombatTarget.InstanceId));
    Turns->bCombatActive = false;

    Runtime->bIsExecutingRelocation = true;
    TestEqual(TEXT("Relocation also blocks search"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 0);
    TestFalse(TEXT("Relocation denial does not mark the hidden trap"),
        State->TrapActiveSearchAttemptedRangeMask.Contains(CombatTarget.InstanceId));
    Runtime->bIsExecutingRelocation = false;

    TestEqual(TEXT("Available exploration search now succeeds"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 1);
    TestEqual(TEXT("Search after combat is actually recorded"),
        State->TrapActiveSearchAttemptedRangeMask.FindRef(CombatTarget.InstanceId), static_cast<uint8>(1));

    // An obstructed target remained unspent and can be attempted only when
    // the authoring geometry becomes transparent. Its high DC stays hidden.
    Level->GetCellMutable(4, 2).EastWall = EGridWallType::None;
    TestEqual(TEXT("Unblocked difficult target fails silently"),
        Runtime->EvaluateActiveDungeonTrapSearch(Party, Active), 0);
    TestEqual(TEXT("Unblocked target now has one distance-two attempt"),
        State->TrapActiveSearchAttemptedRangeMask.FindRef(Blocked.InstanceId), static_cast<uint8>(2));
    TestEqual(TEXT("No re-roll of already spent distance-two target"),
        State->TrapActiveSearchAttemptedRangeMask.FindRef(Hard.InstanceId), static_cast<uint8>(3));

    // Real UE SaveGame binary round-trip, not just a struct copy.
    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->DungeonRuntimeState = Runtime->DungeonRuntimeState;
    TArray<uint8> Bytes;
    TestTrue(TEXT("Active search state can be serialized"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    UGrimrockPartySaveGame* Reloaded =
        Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (TestNotNull(TEXT("Search state can be loaded"), Reloaded))
    {
        AGridLevelRuntimeActor* Restored = Scope.World->SpawnActor<AGridLevelRuntimeActor>();
        if (TestNotNull(TEXT("Restore runtime actor"), Restored))
        {
            Restored->LevelAsset = Level;
            Restored->WorldObjectDefinitions.Add(Def);
            Restored->DungeonRuntimeState = Reloaded->DungeonRuntimeState;
            const FGridLevelRuntimeState* Recovered = Restored->FindRuntimeStateForCurrentLevel();
            if (TestNotNull(TEXT("Recovered level state"), Recovered))
            {
                TestEqual(TEXT("Active failures survive save/reload"),
                    Recovered->TrapActiveSearchAttemptedRangeMask.FindRef(Hard.InstanceId), static_cast<uint8>(3));
                TestEqual(TEXT("Active success survives save/reload"),
                    Recovered->TrapActiveSearchAttemptedRangeMask.FindRef(Easy.InstanceId), static_cast<uint8>(1));
                TestEqual(TEXT("Passive mask remains intact after save/reload"),
                    Recovered->TrapPerceptionAttemptedRangeMask.FindRef(Hard.InstanceId), static_cast<uint8>(2));
            }
            TestEqual(TEXT("Detected target stays detected after Continue"),
                Restored->GetDungeonTrapState(Easy.InstanceId), EGridDungeonTrapState::Detected);
            Party->LevelRuntimeActor = Restored;
            const int32 BeforeRestoredSearch = Active.GetCurrentSeed();
            TestEqual(TEXT("Save/Continue cannot reroll spent distances"),
                Restored->EvaluateActiveDungeonTrapSearch(Party, Active), 0);
            TestEqual(TEXT("Save/Continue scan does not consume RNG"),
                Active.GetCurrentSeed(), BeforeRestoredSearch);
        }
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
