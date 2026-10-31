#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "Runtime/GridDungeonTrapPerceptionService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/Class.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP012ObserverRules,
    "Grimrock.TRAP01.2.ObserverAndRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP012ObserverRules::RunTest(const FString&)
{
    URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(GetTransientPackage());
    Skill->SkillId = TEXT("Skill_Perception");
    Skill->DisplayName = FText::FromString(TEXT("Perception"));
    Skill->Description = FText::FromString(TEXT("Detect hidden dangers"));
    Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Wisdom;
    Skill->bAllowUntrainedChecks = true;
    Skill->MaxRank = 5;

    TArray<FGridCharacterInventoryState> Party;
    Party.SetNum(3);
    for (FGridCharacterInventoryState& Hero : Party)
    {
        Hero.CharacterId = FGuid::NewGuid();
        Hero.Resources.CurrentHealth = 10;
    }
    Party[0].Attributes.Wisdom = 18; // +4, untrained
    Party[1].Attributes.Wisdom = 12; // +1, rank 3 => +4, wins tie by rank
    FRPGSkillRank Trained;
    Trained.SkillId = TEXT("Skill_Perception");
    Trained.Rank = 3;
    Party[1].SkillRanks.Add(Trained);
    Party[2].Attributes.Wisdom = 20;
    Party[2].SkillRanks.Add(Trained);
    Party[2].Resources.CurrentHealth = 0; // Dead characters cannot observe.

    int32 Score = INDEX_NONE;
    TestEqual(TEXT("Best living observer wins score tie by rank"),
        FGridDungeonTrapPerceptionService::SelectBestObserver(Party, Skill, Score), 1);
    TestEqual(TEXT("Score is rank plus Wisdom modifier"), Score, 4);

    TestEqual(TEXT("Score 4 with light reaches three cells"),
        FGridDungeonTrapPerceptionService::GetMaxRangeCells(4, true,
            EGridDungeonTrapDetectionSense::Visual, 4), 3);
    TestEqual(TEXT("Score 4 in darkness sees only two cells"),
        FGridDungeonTrapPerceptionService::GetMaxRangeCells(4, false,
            EGridDungeonTrapDetectionSense::Visual, 4), 2);
    TestEqual(TEXT("Audible danger is not penalized for darkness"),
        FGridDungeonTrapPerceptionService::GetMaxRangeCells(4, false,
            EGridDungeonTrapDetectionSense::Audible, 4), 3);
    TestEqual(TEXT("Definition range cap always applies"),
        FGridDungeonTrapPerceptionService::GetMaxRangeCells(8, true,
            EGridDungeonTrapDetectionSense::Both, 2), 2);

    EGridEdge Direction = EGridEdge::None;
    int32 Distance = INDEX_NONE;
    TestTrue(TEXT("Straight cardinal trap can be scanned"),
        FGridDungeonTrapPerceptionService::GetCardinalDirectionAndDistance(
            FIntPoint(2, 2), FIntPoint(2, 4), Direction, Distance));
    TestEqual(TEXT("North is positive Y"), Direction, EGridEdge::North);
    TestEqual(TEXT("Correct distance"), Distance, 2);
    TestFalse(TEXT("Diagonals are excluded at v0.1"),
        FGridDungeonTrapPerceptionService::GetCardinalDirectionAndDistance(
            FIntPoint(2, 2), FIntPoint(3, 3), Direction, Distance));
    TestFalse(TEXT("Same-cell trap cannot be pre-detected after stepping on it"),
        FGridDungeonTrapPerceptionService::GetCardinalDirectionAndDistance(
            FIntPoint(2, 2), FIntPoint(2, 2), Direction, Distance));

    const FText Location = FGridDungeonTrapPerceptionService::FormatRelativeLocation(
        EGridEdge::North, EGridEdge::East, 2, EGridObjectPlacementKind::Wall);
    TestTrue(TEXT("Feed locates wall trap on the party's right"),
        Location.ToString().Contains(TEXT("droite")));
    TestTrue(TEXT("Feed includes cell distance"), Location.ToString().Contains(TEXT("2")));
    TestTrue(TEXT("Feed includes placement surface"),
        Location.ToString().Contains(TEXT("mur")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP012RuntimeDetection,
    "Grimrock.TRAP01.2.RuntimeDetectionAndPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP012RuntimeDetection::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("TRAP012_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient world"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
    }
    auto Cleanup = [&]()
    {
        World->DestroyWorld(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
    };

    // AActor::ProcessEvent rejects even native events until actors are initialized.
    // CreateWorld alone is insufficient; use the normal game-world lifecycle.
    World->InitializeActorsForPlay(FURL());
    TestTrue(TEXT("World permits actor ProcessEvent dispatch"), World->AreActorsInitialized());

    AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime actor"), Runtime) || !TestNotNull(TEXT("Party pawn"), Party) ||
        !TestNotNull(TEXT("Inventory component"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
    {
        Cleanup();
        return false;
    }
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    for (int32 Y = 1; Y <= 4; ++Y)
    {
        Level->GetCellMutable(3, Y).CellType = EGridCellType::Floor;
    }
    Runtime->LevelAsset = Level;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 3;
    Party->CurrentCellY = 1;
    Party->Facing = EGridEdge::North;

    UGridWorldObjectDefinitionAsset* Def = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Def->DefinitionId = TEXT("Test_Hidden_Trap");
    Def->SupportedType = EGridLevelObjectType::Trigger;
    Def->DungeonTrap.bIsDungeonTrap = true;
    Def->DungeonTrap.DetectionDifficulty = 1; // Guaranteed with a real d20.
    Def->DungeonTrap.DetectionSense = EGridDungeonTrapDetectionSense::Audible;
    Def->DungeonTrap.MaxDetectionRangeCells = 4;
    Def->bCanBeDisarmed = true;
    Def->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
    Def->StaticPart.Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!TestNotNull(TEXT("Existing engine test mesh"), Def->StaticPart.Mesh.Get()))
    {
        Cleanup();
        return false;
    }

    Runtime->WorldObjectDefinitions.Add(Def);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.WorldObjectDefinitionId = Def->DefinitionId;
    Trap.CellX = 3;
    Trap.CellY = 3;
    Level->WorldObjectInstances.Add(Trap);

    FGridCharacterInventoryState Hero;
    Hero.CharacterId = FGuid::NewGuid();
    Hero.DisplayName = FText::FromString(TEXT("Observatrice"));
    Hero.Attributes.Wisdom = 18;
    Hero.Resources.CurrentHealth = 10;
    FRPGSkillRank Rank;
    Rank.SkillId = TEXT("Skill_Perception");
    Rank.Rank = 5;
    Hero.SkillRanks.Add(Rank);
    Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Add(Hero);

    TestNull(TEXT("Hidden trap has no visible runtime actor"),
        Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Trap.InstanceId));
    TestEqual(TEXT("Trap starts hidden"), Runtime->GetDungeonTrapState(Trap.InstanceId),
        EGridDungeonTrapState::Hidden);
    Level->GetCellMutable(3, 2).NorthWall = EGridWallType::Solid;
    FRandomStream BlockedRandom(12);
    const int32 SeedBefore = BlockedRandom.GetCurrentSeed();
    TestEqual(TEXT("Solid wall prevents detection"),
        Runtime->EvaluatePassiveDungeonTrapPerception(Party, BlockedRandom), 0);
    TestEqual(TEXT("Blocked sight does not consume RNG"), BlockedRandom.GetCurrentSeed(), SeedBefore);
    FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Runtime state"), State))
    {
        Cleanup();
        return false;
    }
    TestFalse(TEXT("Blocked path does not spend a distance attempt"),
        State->TrapPerceptionAttemptedRangeMask.Contains(Trap.InstanceId));
    const UGridEventFeedSubsystem* Feed = World->GetSubsystem<UGridEventFeedSubsystem>();
    TestNotNull(TEXT("Perception feedback uses existing feed"), Feed);
    if (Feed)
    {
        TestFalse(TEXT("Blocked passive scan publishes no neutral perception event"),
            Feed->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& E)
            {
                return E.EventCode == FName(TEXT("Dungeon.Trap.PerceptionScan"));
            }));
    }
    Level->GetCellMutable(3, 2).NorthWall = EGridWallType::None;

    FRandomStream Roll(12);
    TestEqual(TEXT("Genuine Perception check discovers an unobstructed trap"),
        Runtime->EvaluatePassiveDungeonTrapPerception(Party, Roll), 1);
    TestEqual(TEXT("Discovery state persisted"), Runtime->GetDungeonTrapState(Trap.InstanceId),
        EGridDungeonTrapState::Detected);
    if (Feed)
    {
        TestTrue(TEXT("Successful discovery still publishes a positive detection event"),
            Feed->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& E)
            {
                return E.EventCode == FName(TEXT("Dungeon.Trap.Detected"));
            }));
        TestFalse(TEXT("Successful discovery never emits the removed neutral scan event"),
            Feed->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& E)
            {
                return E.EventCode == FName(TEXT("Dungeon.Trap.PerceptionScan"));
            }));
    }
    AGridRuntimeObjectActor* Revealed = Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Trap.InstanceId);
    TestNotNull(TEXT("Detection materializes the 3D model"), Revealed);
    if (Revealed && Revealed->MeshComponent)
    {
        TestTrue(TEXT("Revealed mesh uses custom depth for optional red stencil"),
            Revealed->MeshComponent->bRenderCustomDepth);
        TestEqual(TEXT("Dedicated trap reveal stencil"),
            Revealed->MeshComponent->CustomDepthStencilValue, 252);
        TestFalse(TEXT("Revealed model is not hidden"), Revealed->IsHidden());
        TestEqual(TEXT("Revealed mechanism has mouse query collision without world physics"),
            Revealed->MeshComponent->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
        TestEqual(TEXT("Revealed mechanism can receive visibility mouse ray"),
            Revealed->MeshComponent->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
        // TRAP01.5.3: distinguish bad visual identity/owner/state from
        // an interface event-dispatch issue. Keep the actual public API checks.
        TestTrue(TEXT("Revealed visual is the generic interactive actor"),
            Revealed->IsA<AGridGenericObjectActor>());
        TestTrue(TEXT("Revealed actor implements the interaction interface"),
            Revealed->GetClass()->ImplementsInterface(UGridInteractableInterface::StaticClass()));
        TestTrue(TEXT("Revealed visual preserves the authored trap GUID"),
            Revealed->ObjectId == Trap.InstanceId);
        TestTrue(TEXT("Revealed visual is owned by its level runtime actor"),
            Revealed->GetOwner() == Runtime);
        TestEqual(TEXT("Revealed GUID resolves to detected trap state"),
            Runtime->GetDungeonTrapState(Revealed->ObjectId), EGridDungeonTrapState::Detected);
        if (const AGridGenericObjectActor* GenericVisual = Cast<AGridGenericObjectActor>(Revealed))
        {
            TestTrue(TEXT("Direct generic actor can interact with revealed trap"),
                GenericVisual->CanInteract_Implementation(Party, Revealed->MeshComponent));
            TestEqual(TEXT("Direct generic actor returns use cursor"),
                GenericVisual->GetInteractionCursor_Implementation(Revealed->MeshComponent), EGridInteractionCursor::Use);
        }
        TestTrue(TEXT("Revealed actor is native, without a Blueprint override"),
            Revealed->GetClass() == AGridGenericObjectActor::StaticClass());
        for (const FName EventName : {FName(TEXT("CanInteract")), FName(TEXT("GetInteractionCursor")),
                 FName(TEXT("InteractWithHit"))})
        {
            const UFunction* Function = Revealed->FindFunction(EventName);
            AddInfo(FString::Printf(TEXT("Interface resolution: %s -> %s"),
                *EventName.ToString(), *GetPathNameSafe(Function)));
            if (TestNotNull(TEXT("Interface event resolves through reflection"), Function))
            {
                TestTrue(TEXT("Resolved event belongs to the native interface"),
                    Function->GetOwnerClass() == UGridInteractableInterface::StaticClass());
                TestTrue(TEXT("Resolved event has a native thunk"), Function->HasAnyFunctionFlags(FUNC_Native));
                TestTrue(TEXT("Resolved event has no Blueprint bytecode"), Function->Script.IsEmpty());
            }
        }
        TestTrue(TEXT("Revealed mechanism can be clicked by the party"),
            IGridInteractableInterface::Execute_CanInteract(
                Revealed, Party, Revealed->MeshComponent));
        TestEqual(TEXT("Hover advertises use cursor"),
            IGridInteractableInterface::Execute_GetInteractionCursor(
                Revealed, Revealed->MeshComponent), EGridInteractionCursor::Use);
        if (Feed)
        {
            auto HasOutOfReachFeedback = [Feed]()
            {
                return Feed->GetEntries().ContainsByPredicate([](const FGridHudEventEntry& Entry)
                {
                    return Entry.EventCode == FName(TEXT("Dungeon.Trap.OutOfReach"));
                });
            };
            TestFalse(TEXT("No click feedback before interaction"), HasOutOfReachFeedback());
            // Same interface entry point as the mouse controller. The trap is two
            // cells away: reaching gameplay must report range, without disarming.
            IGridInteractableInterface::Execute_InteractWithHit(
                Revealed, Party, Revealed->MeshComponent, FHitResult());
            TestTrue(TEXT("Mouse click interface reaches runtime range feedback"), HasOutOfReachFeedback());
            TestEqual(TEXT("Out-of-reach click preserves detected state"),
                Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Detected);
        }
    }
    TestEqual(TEXT("Distance-two attempt bit recorded"),
        State->TrapPerceptionAttemptedRangeMask.FindRef(Trap.InstanceId), static_cast<uint8>(0x02));

    const int32 SeedAfterSuccess = Roll.GetCurrentSeed();
    TestEqual(TEXT("Repeated scan does not rediscover"),
        Runtime->EvaluatePassiveDungeonTrapPerception(Party, Roll), 0);
    TestEqual(TEXT("Repeated scan consumes no more RNG"), Roll.GetCurrentSeed(), SeedAfterSuccess);

    // TRAP01.3 will invoke this hook together with actual damage. Test the
    // reveal transition now without inventing a zero-damage gameplay trigger.
    FGridWorldObjectInstance Surprise = Trap;
    Surprise.InstanceId = FGuid::NewGuid();
    Surprise.CellY = 4;
    Level->WorldObjectInstances.Add(Surprise);
    TestTrue(TEXT("Activation callback marks a hidden trap known"),
        Runtime->TryRecordDungeonTrapTriggered(Surprise.InstanceId, Party));
    TestNotNull(TEXT("Activation materializes the 3D model even if hidden before"),
        Runtime->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Surprise.InstanceId));
    TestEqual(TEXT("Triggered trap is revealed, even without perception"),
        Runtime->GetDungeonTrapState(Surprise.InstanceId), EGridDungeonTrapState::Triggered);
    TestTrue(TEXT("Triggered GUID enters the known-trap set"),
        State->DetectedTriggerTrapIds.Contains(Surprise.InstanceId));
    TestTrue(TEXT("Clicking already triggered one-shot is handled without disarm tools"),
        Runtime->TryInteractWithRevealedDungeonTrap(Party, Surprise.InstanceId));
    TestEqual(TEXT("Spent trap remains triggered after click"),
        Runtime->GetDungeonTrapState(Surprise.InstanceId), EGridDungeonTrapState::Triggered);
    TestFalse(TEXT("One-shot activation does not trigger twice"),
        Runtime->TryRecordDungeonTrapTriggered(Surprise.InstanceId, Party));

    const FGridDungeonRuntimeState Saved = Runtime->DungeonRuntimeState;
    AGridLevelRuntimeActor* Reloaded = World->SpawnActor<AGridLevelRuntimeActor>();
    if (TestNotNull(TEXT("Restored runtime"), Reloaded))
    {
        Reloaded->LevelAsset = Level;
        Reloaded->WorldObjectDefinitions.Add(Def);
        Reloaded->DungeonRuntimeState = Saved;
        TestEqual(TEXT("Discovered trap persists after snapshot restore"),
            Reloaded->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Detected);
        Reloaded->RestoreDungeonTrapRevealVisuals();
        TestNotNull(TEXT("Save/Continue restores the known 3D trap"),
            Reloaded->FindRuntimeObjectActor<AGridRuntimeObjectActor>(Trap.InstanceId));
        TestEqual(TEXT("Triggered trap persists after snapshot restore"),
            Reloaded->GetDungeonTrapState(Surprise.InstanceId), EGridDungeonTrapState::Triggered);
        const FGridLevelRuntimeState* SavedState = Reloaded->FindRuntimeStateForCurrentLevel();
        TestNotNull(TEXT("Restored level state"), SavedState);
        if (SavedState)
        {
            TestEqual(TEXT("Attempt mask preserved across restore"),
                SavedState->TrapPerceptionAttemptedRangeMask.FindRef(Trap.InstanceId),
                static_cast<uint8>(0x02));
        }
    }
    Cleanup();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
