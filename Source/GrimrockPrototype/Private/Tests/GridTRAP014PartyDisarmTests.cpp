#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP014BestMember,
    "Grimrock.TRAP01.4.BestEligibleMemberAndDexterity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP014BestMember::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("TRAP014_%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

    AGridLevelRuntimeActor* Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime"), Runtime) ||
        !TestNotNull(TEXT("Party"), Party) ||
        !TestNotNull(TEXT("Party Inventory"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
    {
        Cleanup();
        return false;
    }

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Level->GetCellMutable(1, 1).CellType = EGridCellType::Floor;
    Runtime->LevelAsset = Level;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 1;
    Party->CurrentCellY = 1;

    UGridWorldObjectDefinitionAsset* Def =
        NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Def->DefinitionId = TEXT("TRAP014_Disarm_Floor");
    Def->SupportedType = EGridLevelObjectType::Trigger;
    Def->DungeonTrap.bIsDungeonTrap = true;
    Def->bCanBeDisarmed = true;
    Def->DisarmDifficulty = 40; // Impossible at the test party's level.
    Runtime->WorldObjectDefinitions.Add(Def);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.WorldObjectDefinitionId = Def->DefinitionId;
    Trap.CellX = 1;
    Trap.CellY = 1;
    Level->WorldObjectInstances.Add(Trap);
    TestTrue(TEXT("Explicitly detected before any attempt"),
        Runtime->TryRecordDungeonTrapDetection(Trap.InstanceId));

    UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
    Inventory->PartyInventoryState.ActiveCharacters.SetNum(4);
    for (int32 i = 0; i < 4; ++i)
    {
        FGridCharacterInventoryState& Hero =
            Inventory->PartyInventoryState.ActiveCharacters[i];
        Hero.CharacterId = FGuid::NewGuid();
        Hero.DisplayName = FText::FromString(
            FString::Printf(TEXT("Test member %d"), i));
        Hero.Resources.CurrentHealth = i == 3 ? 0 : 30;
        Hero.InventorySlots.SetNum(1);
    }

    // Person 0 holds the clicked tool: high INT but poor DEX and only rank 1.
    Inventory->PartyInventoryState.ActiveCharacters[0].Attributes =
        FRPGAttributes(10, 10, 10, 20, 10, 10);
    // Person 1: trained rank 3, high DEX, low INT => should be selected.
    Inventory->PartyInventoryState.ActiveCharacters[1].Attributes =
        FRPGAttributes(10, 20, 10, 6, 10, 10);
    // Person 2 is even better, but never has a tool.
    Inventory->PartyInventoryState.ActiveCharacters[2].Attributes =
        FRPGAttributes(10, 20, 10, 20, 10, 10);
    // Person 3 is well trained with tools, but already dead.
    Inventory->PartyInventoryState.ActiveCharacters[3].Attributes =
        FRPGAttributes(10, 20, 10, 20, 10, 10);

    const int32 Ranks[] = { 1, 3, 5, 5 };
    for (int32 i = 0; i < 4; ++i)
    {
        FRPGSkillRank Rank;
        Rank.SkillId = TEXT("Skill_Traps");
        Rank.Rank = Ranks[i];
        Inventory->PartyInventoryState.ActiveCharacters[i].SkillRanks.Add(Rank);
    }

    UGridItemDefinitionAsset* Tool = NewObject<UGridItemDefinitionAsset>(Runtime);
    Tool->ItemDefinitionId = TEXT("Item_DisarmTools_01");
    Tool->DisplayName = FText::FromString(TEXT("Tools"));
    Tool->Description = FText::FromString(TEXT("Reusable precision tools"));
    Tool->Weight = 0.5f;
    Tool->HandUsage = EGridItemHandUsage::NotHandHeld;
    Tool->ItemType = EGridItemType::Misc;
    Tool->ItemTags = { TEXT("Tool.Disarm") };
    if (!TestTrue(TEXT("Register authentic tool type"),
        Inventory->RegisterItemDefinition(Tool)))
    {
        Cleanup();
        return false;
    }
    FGridItemInstance Tools;
    Tools.RuntimeObjectId = FGuid::NewGuid();
    Tools.ItemDefinitionId = Tool->ItemDefinitionId;
    Tools.Quantity = 1;
    if (!TestTrue(TEXT("Real inventory tool instance prepared"),
        Inventory->ApplyItemDefinitionToInstance(Tools)))
    {
        Cleanup();
        return false;
    }

    // The incoming asset may still be legacy Intelligence on disk.
    // The gameplay check MUST be Dexterity without mutating that source asset.
    URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(Runtime);
    Skill->SkillId = TEXT("Skill_Traps");
    Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Intelligence;
    Skill->MaxRank = 5;
    Skill->bAllowUntrainedChecks = false;
    int32 UsedCharacter = INDEX_NONE;
    FRPGSkillCheckResult Check;
    FRandomStream Random(1401);
    const int32 SeedBefore = Random.GetCurrentSeed();
    TestFalse(TEXT("No tool anywhere rejects without a roll"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill, Random, Check,
            &UsedCharacter));
    TestEqual(TEXT("No RNG consumed when all characters lack tools"),
        Random.GetCurrentSeed(), SeedBefore);

    // Only the UI source member has the tool; the others cannot 'borrow' it.
    if (!TestTrue(TEXT("Source tools inserted through authoritative inventory API"),
        Inventory->AddItemToCharacterInventory(0, Tools)))
    {
        Cleanup();
        return false;
    }
    TestTrue(TEXT("Source slot is genuinely occupied"),
        Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].bOccupied);
    // Training is an eligibility condition, not merely a negative bonus.
    Inventory->PartyInventoryState.ActiveCharacters[0].SkillRanks.Reset();
    const int32 SeedBeforeUntrained = Random.GetCurrentSeed();
    TestFalse(TEXT("Tool without trained Skill_Traps cannot disarm"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill, Random, Check,
            &UsedCharacter));
    TestEqual(TEXT("Untrained rejection consumes no d20"),
        Random.GetCurrentSeed(), SeedBeforeUntrained);
    FRPGSkillRank TrainedSource;
    TrainedSource.SkillId = TEXT("Skill_Traps");
    TrainedSource.Rank = 1;
    Inventory->PartyInventoryState.ActiveCharacters[0].SkillRanks.Add(TrainedSource);
    TestTrue(TEXT("Source with tools can resolve skill check"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill, Random, Check,
            &UsedCharacter));
    TestEqual(TEXT("Only actual tool holder is eligible"), UsedCharacter, 0);
    TestEqual(TEXT("Runtime projects governing attribute to Dexterity"),
        Check.GoverningAttribute, ERPGSkillGoverningAttribute::Dexterity);
    TestEqual(TEXT("Low Dex is used instead of high Intelligence"),
        Check.AttributeValue, 10);
    TestFalse(TEXT("Very difficult trap stays armed"),
        Runtime->IsTriggerTrapDisarmed(Trap.InstanceId));

    // Add a second real tool to the trained, dexterous member.
    Tools.RuntimeObjectId = FGuid::NewGuid();
    if (!TestTrue(TEXT("Second specialist tools inserted through authoritative inventory API"),
        Inventory->AddItemToCharacterInventory(1, Tools)))
    {
        Cleanup();
        return false;
    }
    TestTrue(TEXT("Specialist slot is genuinely occupied"),
        Inventory->PartyInventoryState.ActiveCharacters[1].InventorySlots[0].bOccupied);
    FRandomStream SecondRoll(1402);
    TestTrue(TEXT("Best trained Dexterity candidate performs the action"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
            SecondRoll, Check, &UsedCharacter));
    TestEqual(TEXT("Best eligible member selected regardless of clicked bag"), UsedCharacter, 1);
    TestEqual(TEXT("Selected member uses 20 Dexterity"), Check.AttributeValue, 20);
    TestEqual(TEXT("Selected member has rank 3"), Check.Rank, 3);
    TestEqual(TEXT("Skill d20 includes rank + Dexterity modifier"),
        Check.Total, Check.Roll + Check.Rank + Check.AttributeModifier + Check.ProgressionModifier);
    TestFalse(TEXT("High difficulty again prevents a disarm"), Check.bSuccess);
    TestEqual(TEXT("Source canonical asset stays Intelligence (no runtime mutation)"),
        Skill->GoverningAttribute, ERPGSkillGoverningAttribute::Intelligence);

    // A failed attempt does not consume either tool; success does not either.
    TestEqual(TEXT("Clicked tool still present"),
        Inventory->CountItemDefinitionInCharacterInventory(0, Tool->ItemDefinitionId), 1);
    TestEqual(TEXT("Performer's tool still present"),
        Inventory->CountItemDefinitionInCharacterInventory(1, Tool->ItemDefinitionId), 1);

    // TRAP01.4.1: failure and rejection must not award any group XP.
    for (const FGridCharacterInventoryState& Hero : Inventory->PartyInventoryState.ActiveCharacters)
    {
        TestEqual(TEXT("No XP for failed/untrained/unavailable attempts"), Hero.Experience, 0);
    }
    Def->DisarmDifficulty = 1;
    FRandomStream SuccessRoll(1403);
    TestTrue(TEXT("Best party disarmer resolves easy trap"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
            SuccessRoll, Check, &UsedCharacter));
    TestTrue(TEXT("Easy disarm succeeds"), Check.bSuccess);
    TestEqual(TEXT("Success still belongs to best character"), UsedCharacter, 1);
    TestTrue(TEXT("Success persists under trap GUID"),
        Runtime->IsTriggerTrapDisarmed(Trap.InstanceId));
    TestEqual(TEXT("Reward pool DC1 is two points for active party"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Experience, 1);
    TestEqual(TEXT("Second active receives deterministic remainder"),
        Inventory->PartyInventoryState.ActiveCharacters[1].Experience, 1);
    TestEqual(TEXT("Third active receives zero share"), 
        Inventory->PartyInventoryState.ActiveCharacters[2].Experience, 0);
    TestEqual(TEXT("Fourth active receives zero share"),
        Inventory->PartyInventoryState.ActiveCharacters[3].Experience, 0);
    const int32 SeedAfter = SuccessRoll.GetCurrentSeed();
    TestFalse(TEXT("Disarmed hazard never rolls again"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
            SuccessRoll, Check, &UsedCharacter));
    TestEqual(TEXT("No new roll on disarmed hazard"),
        SuccessRoll.GetCurrentSeed(), SeedAfter);
    TestEqual(TEXT("No duplicate XP on replay"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Experience, 1);

    const FGridDungeonRuntimeState SavedDungeon = Runtime->DungeonRuntimeState;
    const FGridPartyInventoryState SavedParty = Inventory->PartyInventoryState;
    AGridLevelRuntimeActor* RestoredRuntime = World->SpawnActor<AGridLevelRuntimeActor>();
    if (TestNotNull(TEXT("Restored dungeon runtime actor"), RestoredRuntime))
    {
        RestoredRuntime->LevelAsset = Level;
        RestoredRuntime->WorldObjectDefinitions.Add(Def);
        RestoredRuntime->DungeonRuntimeState = SavedDungeon;
        Inventory->PartyInventoryState = SavedParty;
        Party->LevelRuntimeActor = RestoredRuntime;
        TestTrue(TEXT("Save snapshot remembers disarmed GUID"),
            RestoredRuntime->IsTriggerTrapDisarmed(Trap.InstanceId));
        TestFalse(TEXT("Continue cannot disarm already awarded trap"),
            RestoredRuntime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
                SuccessRoll, Check, &UsedCharacter));
        TestEqual(TEXT("No additional roll after Continue"),
            SuccessRoll.GetCurrentSeed(), SeedAfter);
        TestEqual(TEXT("No additional XP after Continue"),
            Inventory->PartyInventoryState.ActiveCharacters[0].Experience, 1);
        Party->LevelRuntimeActor = Runtime;
    }

    // TRAP01.5.1: with the tools still in the inventory, clicking another
    // revealed mechanism must disarm it directly without dragging the kit.
    // The same party-best Dexterity resolver and once-only XP guard apply.
    FGridWorldObjectInstance ClickedTrap = Trap;
    ClickedTrap.InstanceId = FGuid::NewGuid();
    Level->WorldObjectInstances.Add(ClickedTrap);
    TestTrue(TEXT("Second world-click trap can be explicitly discovered"),
        Runtime->TryRecordDungeonTrapDetection(ClickedTrap.InstanceId));
    const int32 PreviousXP0 =
        Inventory->PartyInventoryState.ActiveCharacters[0].Experience;
    const int32 PreviousXP1 =
        Inventory->PartyInventoryState.ActiveCharacters[1].Experience;
    TestTrue(TEXT("Direct interaction handles a revealed armed trap"),
        Runtime->TryInteractWithRevealedDungeonTrap(Party, ClickedTrap.InstanceId));
    TestEqual(TEXT("World click uses the same durable disarm state"),
        Runtime->GetDungeonTrapState(ClickedTrap.InstanceId), EGridDungeonTrapState::Disarmed);
    TestEqual(TEXT("World click awards the first active member once"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Experience, PreviousXP0 + 1);
    TestEqual(TEXT("World click awards the next active member once"),
        Inventory->PartyInventoryState.ActiveCharacters[1].Experience, PreviousXP1 + 1);
    TestTrue(TEXT("Clicking a disarmed mechanism shows its status rather than rerolling"),
        Runtime->TryInteractWithRevealedDungeonTrap(Party, ClickedTrap.InstanceId));
    TestEqual(TEXT("Repeated click does not award another XP"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Experience, PreviousXP0 + 1);

    Cleanup();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS