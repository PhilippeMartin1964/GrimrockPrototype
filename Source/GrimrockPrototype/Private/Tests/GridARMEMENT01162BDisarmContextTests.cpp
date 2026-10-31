#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01162BDisarmContext,
    "Grimrock.RPG.ARMEMENT01.16.TrapInteraction.ContextAndTargeting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01162BDisarmContext::RunTest(const FString&)
{
    // The production path must exist: UI intentionally never fabricates a
    // transient Skill_Traps when its cooked DataAsset is missing.
    URPGSkillAsset* CanonicalSkill = LoadObject<URPGSkillAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps.DA_Skill_Traps"));
    if (!TestNotNull(TEXT("Production Skill_Traps is materialized"), CanonicalSkill))
        return false;
    TestEqual(TEXT("Skill_Traps identity"), CanonicalSkill->SkillId, FName(TEXT("Skill_Traps")));
    TestEqual(TEXT("Skill_Traps uses intelligence in production"),
        CanonicalSkill->GoverningAttribute, ERPGSkillGoverningAttribute::Intelligence);
    TestFalse(TEXT("N1 disarm requires a trained rank"),
        CanonicalSkill->bAllowUntrainedChecks);

    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("ARM01162B_%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient grid world"), World)) return false;

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
    AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime"), Runtime) || !TestNotNull(TEXT("Party"), Party))
    {
        DestroyWorld();
        return false;
    }

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Level->GetCellMutable(0, 0).CellType = EGridCellType::Floor;
    Level->GetCellMutable(0, 1).CellType = EGridCellType::Floor;
    Runtime->LevelAsset = Level;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 0;
    Party->CurrentCellY = 0;
    Party->Facing = EGridEdge::North;

    UGridWorldObjectDefinitionAsset* Def = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Def->DefinitionId = TEXT("Disarmable_Trap");
    Def->SupportedType = EGridLevelObjectType::Trigger;
    Def->DungeonTrap.bIsDungeonTrap = true;
    Def->bCanBeDisarmed = true;
    Def->DisarmDifficulty = 12;
    Runtime->WorldObjectDefinitions.Add(Def);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.WorldObjectDefinitionId = Def->DefinitionId;
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.CellX = 0;
    Trap.CellY = 1;
    Level->WorldObjectInstances.Add(Trap);

    UGridItemDefinitionAsset* Tool = NewObject<UGridItemDefinitionAsset>(Runtime);
    Tool->ItemDefinitionId = TEXT("Item_DisarmTools_01");
    Tool->DisplayName = FText::FromString(TEXT("Disarm Tools"));
    Tool->ItemType = EGridItemType::Misc;
    Tool->ItemTags = { TEXT("Tool.Disarm") };

    FGridItemInstance Instance;
    Instance.RuntimeObjectId = FGuid::NewGuid();
    Instance.ItemDefinitionId = Tool->ItemDefinitionId;
    Instance.Quantity = 1;

    FGridItemActionContext ItemContext;
    ItemContext.PartyPawn = Party;
    ItemContext.Item = Instance;
    ItemContext.ItemDefinition = Tool;
    ItemContext.CharacterIndex = 0;
    ItemContext.InventorySlotIndex = 0;

    FGuid Target;
    TestFalse(TEXT("Hidden trap never shows as a selectable target"),
        Runtime->FindDisarmableTriggerTrapForParty(Party, Target));
    FGridFacingTargetContext HiddenFacing;
    TArray<FGridItemContextAction> HiddenActions;
    UGridItemContextActionLibrary::BuildItemContextActions(ItemContext, HiddenFacing, HiddenActions);
    TestFalse(TEXT("Right click on tools never leaks an undiscovered trap"),
        HiddenActions.ContainsByPredicate([](const FGridItemContextAction& A)
        {
            return A.ActionType == EGridItemActionType::DisarmTrap;
        }));
    FGridLevelRuntimeState* DiscoveredState = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Per-level knowledge is available"), DiscoveredState))
    {
        DestroyWorld();
        return false;
    }
    DiscoveredState->DetectedTriggerTrapIds.Add(Trap.InstanceId);
    TestTrue(TEXT("Only after detection can the front-cell trap be selected"),
        Runtime->FindDisarmableTriggerTrapForParty(Party, Target));
    TestEqual(TEXT("Target is stable authored instance ID"), Target, Trap.InstanceId);

    FGridFacingTargetContext Facing;
    TArray<FGridItemContextAction> Actions;
    TestTrue(TEXT("Tool context builds actions without any visible Trigger actor"),
        UGridItemContextActionLibrary::BuildItemContextActions(ItemContext, Facing, Actions));
    TestEqual(TEXT("Tool has an explicit dungeon trap context"), Facing.TargetType,
        EGridFacingTargetType::DungeonTrap);
    TestTrue(TEXT("Action targets the exact same stable Trigger"),
        Facing.TargetObjectId == Trap.InstanceId);
    TestEqual(TEXT("Exactly one disarm action is offered"), Actions.FilterByPredicate(
        [](const FGridItemContextAction& A)
        {
            return A.ActionType == EGridItemActionType::DisarmTrap;
        }).Num(), 1);

    Level->GetCellMutable(0, 0).NorthWall = EGridWallType::Solid;
    TestFalse(TEXT("Solid wall prevents adjacent trap selection"),
        Runtime->FindDisarmableTriggerTrapForParty(Party, Target));
    UGridItemContextActionLibrary::BuildItemContextActions(ItemContext, Facing, Actions);
    TestFalse(TEXT("No disarm menu action through a wall"), Actions.ContainsByPredicate(
        [](const FGridItemContextAction& A) { return A.ActionType == EGridItemActionType::DisarmTrap; }));
    Level->GetCellMutable(0, 0).NorthWall = EGridWallType::None;

    UGridWorldObjectDefinitionAsset* Ordinary = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Ordinary->DefinitionId = TEXT("Ordinary_Decoration");
    Ordinary->SupportedType = EGridLevelObjectType::Decoration;
    TestFalse(TEXT("Normal world object cannot be mistaken for a disarmable trap"),
        Ordinary->bCanBeDisarmed);

    FGridWorldObjectInstance Duplicate = Trap;
    Duplicate.InstanceId = FGuid::NewGuid();
    Level->WorldObjectInstances.Add(Duplicate);
    TestFalse(TEXT("Two traps in one cell are ambiguous and fail closed"),
        Runtime->FindDisarmableTriggerTrapForParty(Party, Target));
    Level->WorldObjectInstances.Pop();

    FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (TestNotNull(TEXT("Persistent runtime level state"), State))
    {
        State->DisarmedTriggerTrapIds.Add(Trap.InstanceId);
        TestFalse(TEXT("Disarmed trigger disappears from context selection"),
            Runtime->FindDisarmableTriggerTrapForParty(Party, Target));
        UGridItemContextActionLibrary::BuildItemContextActions(ItemContext, Facing, Actions);
        TestFalse(TEXT("No repeat disarm menu action after success"),
            Actions.ContainsByPredicate([](const FGridItemContextAction& A)
            {
                return A.ActionType == EGridItemActionType::DisarmTrap;
            }));
    }

    DestroyWorld();
    return true;
}

#endif
