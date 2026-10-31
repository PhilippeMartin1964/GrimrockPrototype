#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace ARMEMENT01162A
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
                FName(*FString::Printf(TEXT("ARM01162A_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
        }
        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01162AOptIn,
    "Grimrock.RPG.ARMEMENT01.16.TrapCore.DefinitionOptIn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01162AOptIn::RunTest(const FString&)
{
    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
    Definition->DefinitionId = TEXT("Test_Mechanical_Trigger_Trap");
    Definition->SupportedType = EGridLevelObjectType::Trigger;
    Definition->DungeonTrap.bIsDungeonTrap = true;
    TestFalse(TEXT("Legacy triggers are never disarmable by default"),
        Definition->bCanBeDisarmed);

    Definition->bCanBeDisarmed = true;
    Definition->DisarmDifficulty = 10;
    TArray<FGridWorldObjectDefinitionValidationMessage> Messages;
    Definition->ValidateDefinition(Messages);
    TestFalse(TEXT("Valid trap contract has no disarm-specific errors"),
        Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& M)
        {
            return M.Severity == EGridWorldObjectDefinitionValidationSeverity::Error &&
                M.Message.Contains(TEXT("Disarmable dungeon traps"));
        }));

    Definition->SupportedType = EGridLevelObjectType::Decoration;
    Definition->ValidateDefinition(Messages);
    TestTrue(TEXT("Non-trigger object cannot masquerade as disarmable trap"),
        Messages.ContainsByPredicate([](const FGridWorldObjectDefinitionValidationMessage& M)
        {
            return M.Severity == EGridWorldObjectDefinitionValidationSeverity::Error &&
                M.Message.Contains(TEXT("Disarmable dungeon traps"));
        }));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01162ARuntime,
    "Grimrock.RPG.ARMEMENT01.16.TrapCore.SkillAndPersistentState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01162ARuntime::RunTest(const FString&)
{
    ARMEMENT01162A::FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Transient world"), Fixture.World)) return false;
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime actor"), Runtime) ||
        !TestNotNull(TEXT("Party pawn"), Party) ||
        !TestNotNull(TEXT("Party inventory"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
        return false;

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Runtime->LevelAsset = Level;

    UGridWorldObjectDefinitionAsset* Def =
        NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Def->DefinitionId = TEXT("Test_Dungeon_Disarmable_Trigger");
    Def->SupportedType = EGridLevelObjectType::Trigger;
    Def->DungeonTrap.bIsDungeonTrap = true;
    Def->bCanBeDisarmed = true;
    Def->DisarmDifficulty = 40;
    Runtime->WorldObjectDefinitions.Add(Def);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.WorldObjectDefinitionId = Def->DefinitionId;
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.CellX = 0;
    Trap.CellY = 0;
    Level->WorldObjectInstances.Add(Trap);

    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 0;
    Party->CurrentCellY = 0;
    Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();

    URPGClassAsset* Rogue = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Production Rogue"), Rogue) ||
        !TestNotNull(TEXT("Production Human"), Human)) return false;

    FRPGCharacterCreationRequest Hero;
    Hero.DisplayName = FText::FromString(TEXT("Trap Tester"));
    Hero.ClassDefinition = Rogue;
    Hero.CombatActionSourceClassDefinition = Rogue;
    Hero.RaceDefinition = Human;
    Hero.StartingEquipmentKitId = TEXT("Kit_Rogue_Saboteur");
    FText Error;
    UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
    if (!TestTrue(TEXT("Playable N1 Saboteur"),
        Inventory->CreateInitialCharacter(Hero, Error)))
    {
        AddError(Error.ToString());
        return false;
    }

    URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(Runtime);
    Skill->SkillId = TEXT("Skill_Traps");
    Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
    Skill->bAllowUntrainedChecks = true;
    Skill->MaxRank = 5;

    // Not even a caller holding a guessed GUID may query or disarm a
    // previously unseen trap; only explicit knowledge unlocks the action.
    int32 Difficulty = 0;
    FRandomStream HiddenRandomStream(162);
    FRPGSkillCheckResult HiddenResult;
    const int32 HiddenSeed = HiddenRandomStream.GetCurrentSeed();
    TestFalse(TEXT("Hidden trap difficulty does not leak"),
        Runtime->GetTriggerTrapDisarmDifficulty(Trap.InstanceId, Difficulty));
    TestFalse(TEXT("Hidden trap rejects a direct disarm API call"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill, HiddenRandomStream, HiddenResult));
    TestEqual(TEXT("No RNG consumed while hidden"), HiddenRandomStream.GetCurrentSeed(), HiddenSeed);
    FGridLevelRuntimeState* DetectedState = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Per-level trap knowledge state"), DetectedState)) return false;
    DetectedState->DetectedTriggerTrapIds.Add(Trap.InstanceId); // Simulates TRAP01.2 discovery, never menu proximity.
    TestTrue(TEXT("Fresh authored Trigger offers disarm difficulty after detection"),
        Runtime->GetTriggerTrapDisarmDifficulty(Trap.InstanceId, Difficulty));
    TestEqual(TEXT("Explicit difficulty projects"), Difficulty, 40);
    TestFalse(TEXT("Fresh trap is armed"), Runtime->IsTriggerTrapDisarmed(Trap.InstanceId));

    FRandomStream Stream(162);
    FRPGSkillCheckResult Check;
    const int32 SeedBeforeReject = Stream.GetCurrentSeed();
    TestFalse(TEXT("No disarm tool refuses without rolling"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill, Stream, Check));
    TestEqual(TEXT("Rejected attempt preserves RNG"),
        Stream.GetCurrentSeed(), SeedBeforeReject);

    UGridItemDefinitionAsset* Tool = NewObject<UGridItemDefinitionAsset>(Runtime);
    Tool->ItemDefinitionId = TEXT("Item_DisarmTools_01");
    Tool->DisplayName = FText::FromString(TEXT("Disarm Tools"));
    Tool->Description = FText::FromString(TEXT("Real mechanical tools"));
    Tool->ItemType = EGridItemType::Misc;
    Tool->Weight = 0.5f;
    Tool->HandUsage = EGridItemHandUsage::NotHandHeld;
    Tool->ItemTags = { TEXT("Tool.Disarm") };
    if (!TestTrue(TEXT("Transient tool registered"), Inventory->RegisterItemDefinition(Tool)))
        return false;

    FGridItemInstance ToolItem;
    ToolItem.RuntimeObjectId = FGuid::NewGuid();
    ToolItem.ItemDefinitionId = Tool->ItemDefinitionId;
    ToolItem.Quantity = 1;
    if (!TestTrue(TEXT("Tool instance configured"),
        Inventory->ApplyItemDefinitionToInstance(ToolItem)) ||
        !TestTrue(TEXT("Actual bag owns the disarm tool"),
        Inventory->AddItemToCharacterInventory(0, ToolItem)))
        return false;

    TestTrue(TEXT("With tool, check resolves even on failure"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill, Stream, Check));
    TestTrue(TEXT("Skill check was resolved"), Check.bResolved);
    TestFalse(TEXT("Difficulty 40 cannot be beaten at N1"), Check.bSuccess);
    TestFalse(TEXT("Failed check leaves trigger armed"), Runtime->IsTriggerTrapDisarmed(Trap.InstanceId));
    TestEqual(TEXT("Reusable tool not consumed on failure"),
        Inventory->CountItemDefinitionInCharacterInventory(0, Tool->ItemDefinitionId), 1);

    Def->DisarmDifficulty = 1;
    FRandomStream SuccessStream(162);
    TestTrue(TEXT("Real Skill_Traps success accepted"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill, SuccessStream, Check));
    TestTrue(TEXT("Low DC check succeeds"), Check.bSuccess);
    TestTrue(TEXT("Persistent instance is disarmed"), Runtime->IsTriggerTrapDisarmed(Trap.InstanceId));
    TestEqual(TEXT("Reusable tool remains after successful disarm"),
        Inventory->CountItemDefinitionInCharacterInventory(0, Tool->ItemDefinitionId), 1);
    TestFalse(TEXT("Disarmed trap no longer offers a second skill check"),
        Runtime->GetTriggerTrapDisarmDifficulty(Trap.InstanceId, Difficulty));

    const FGridDungeonRuntimeState Saved = Runtime->DungeonRuntimeState;
    AGridLevelRuntimeActor* Reloaded = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Reloaded runtime actor"), Reloaded)) return false;
    Reloaded->LevelAsset = Level;
    Reloaded->WorldObjectDefinitions.Add(Def);
    Reloaded->DungeonRuntimeState = Saved;
    TestTrue(TEXT("Disarmed trigger survives a runtime-state restore"),
        Reloaded->IsTriggerTrapDisarmed(Trap.InstanceId));
    TestFalse(TEXT("Save/Continue cannot rearm without explicit reset"),
        Reloaded->GetTriggerTrapDisarmDifficulty(Trap.InstanceId, Difficulty));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
