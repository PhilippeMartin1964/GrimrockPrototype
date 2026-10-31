#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridDungeonTrapDisarmOdds.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace TRAP018
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
                FName(*FString::Printf(TEXT("TRAP018_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            if (World) World->InitializeActorsForPlay(FURL());
        }
        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };

    URPGSkillAsset* MakeSkill(UObject* Outer)
    {
        URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(Outer);
        Skill->SkillId = TEXT("Skill_Traps");
        Skill->DisplayName = FText::FromString(TEXT("Pièges / Désamorçage"));
        Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
        Skill->bAllowUntrainedChecks = false;
        Skill->MaxRank = 5;
        return Skill;
    }

    bool SetupOperator(UObject* Outer, AGrimrockPartyPawn* Party,
        URPGClassAsset* ClassDefinition, FName TalentId)
    {
        UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
        if (!Inventory) return false;
        Inventory->PartyInventoryState.ActiveCharacters.SetNum(1);
        FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
        Hero.CharacterId = FGuid::NewGuid();
        Hero.DisplayName = FText::FromString(TEXT("Elias"));
        Hero.ClassDefinition = ClassDefinition;
        Hero.ClassId = ClassDefinition ? ClassDefinition->ClassId : NAME_None;
        Hero.Level = 2;
        Hero.Attributes.Dexterity = 10;
        Hero.Resources.CurrentHealth = 30;
        Hero.InventorySlots.SetNum(1);
        if (!TalentId.IsNone()) Hero.SelectedClassProgressionChoiceIds.Add(TalentId);
        FRPGSkillRank Rank;
        Rank.SkillId = TEXT("Skill_Traps");
        Rank.Rank = 1;
        Hero.SkillRanks.Add(Rank);

        UGridItemDefinitionAsset* Tool = NewObject<UGridItemDefinitionAsset>(Outer);
        Tool->ItemDefinitionId = TEXT("Item_DisarmTools_01");
        Tool->DisplayName = FText::FromString(TEXT("Outils"));
        Tool->Description = FText::FromString(TEXT("Trousse authentique"));
        Tool->ItemType = EGridItemType::Misc;
        Tool->Weight = 0.5f;
        Tool->HandUsage = EGridItemHandUsage::NotHandHeld;
        Tool->ItemTags = { TEXT("Tool.Disarm") };
        if (!Inventory->RegisterItemDefinition(Tool)) return false;
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Tool->ItemDefinitionId;
        Item.Quantity = 1;
        return Inventory->ApplyItemDefinitionToInstance(Item) &&
            Inventory->AddItemToCharacterInventory(0, Item);
    }

    UGridWorldObjectDefinitionAsset* MakeTrap(UObject* Outer, bool bOneShot)
    {
        UGridWorldObjectDefinitionAsset* Def =
            NewObject<UGridWorldObjectDefinitionAsset>(Outer);
        Def->DefinitionId = TEXT("TRAP018_Floor");
        Def->SupportedType = EGridLevelObjectType::Trigger;
        Def->PlacementSurface = EGridObjectPlacementKind::Floor;
        Def->DungeonTrap.bIsDungeonTrap = true;
        Def->DungeonTrap.bOneShot = bOneShot;
        Def->DungeonTrap.MinimumDamage = 3;
        Def->DungeonTrap.MaximumDamage = 3;
        Def->bCanBeDisarmed = true;
        Def->DungeonTrap.RequiredToolTag = TEXT("Tool.Disarm");
        return Def;
    }

    FGridWorldObjectInstance MakeTrapPlacement(const UGridWorldObjectDefinitionAsset* Def,
        int32 X, int32 Y, bool bNoDamage, bool bPit)
    {
        FGridWorldObjectInstance Trap;
        Trap.InstanceId = FGuid::NewGuid();
        Trap.Type = EGridLevelObjectType::Trigger;
        Trap.WorldObjectDefinitionId = Def->DefinitionId;
        Trap.CellX = X;
        Trap.CellY = Y;
        Trap.InstanceConfig.DungeonTrap.bNoDamage = bNoDamage;
        Trap.InstanceConfig.DungeonTrap.bOpenPitOnTrigger = bPit;
        return Trap;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP018SafeFailure,
    "Grimrock.TRAP01.8.SafeFailureOnceAndSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP018SafeFailure::RunTest(const FString&)
{
    TRAP018::FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Transient world"), Fixture.World)) return false;
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime"), Runtime) ||
        !TestNotNull(TEXT("Party"), Party)) return false;

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Runtime->LevelAsset = Level;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 1;
    Party->CurrentCellY = 1;
    UGridWorldObjectDefinitionAsset* Def = TRAP018::MakeTrap(Runtime, true);
    Runtime->WorldObjectDefinitions.Add(Def);
    const FGridWorldObjectInstance Trap =
        TRAP018::MakeTrapPlacement(Def, 1, 1, false, false);
    Level->WorldObjectInstances.Add(Trap);
    FGridDungeonTrapDisarmOdds HiddenOdds;
    TestFalse(TEXT("Hidden trap must not disclose odds"),
        Runtime->TryGetDungeonTrapDisarmOdds(Party, Trap.InstanceId, HiddenOdds));
    if (!TestTrue(TEXT("Trap revealed before disarm"),
        Runtime->TryRecordDungeonTrapDetection(Trap.InstanceId))) return false;

    URPGClassAsset* Rogue = NewObject<URPGClassAsset>(Runtime);
    Rogue->ClassId = TEXT("Rogue_TRAP018");
    Rogue->DisplayName = FText::FromString(TEXT("Saboteur"));
    Rogue->HealthAtLevelOne = 10;
    FRPGClassProgressionChoiceDefinition Expert;
    Expert.ChoiceId = TEXT("Talent_TRAP018_Expert");
    Expert.DisplayName = FText::FromString(TEXT("Désamorçage expert"));
    Expert.MinimumLevel = 2;
    Expert.PointCost = 1;
    FRPGSkillProgressionModifier Bonus;
    Bonus.SkillId = TEXT("Skill_Traps");
    Bonus.CheckModifier = 2;
    Bonus.SafeFailureMargin = 2;
    Expert.SkillModifiers.Add(Bonus);
    Rogue->ProgressionChoices.Add(Expert);
    if (!TestTrue(TEXT("Expert talent valid"), Rogue->IsValidDefinition())) return false;
    if (!TestTrue(TEXT("Operator equipped and trained"),
        TRAP018::SetupOperator(Runtime, Party, Rogue, Expert.ChoiceId))) return false;
    URPGSkillAsset* Skill = TRAP018::MakeSkill(Runtime);
    if (!TestTrue(TEXT("Skill valid"), Skill->IsValidDefinition())) return false;

    FRandomStream Reference(1801);
    const int32 ExpectedRoll = Reference.RandRange(1, 20);
    Def->DisarmDifficulty = ExpectedRoll + 1 + 2 + 1; // Exactly one point short.

    FGridDungeonTrapDisarmOdds Before;
    TestTrue(TEXT("A trained member with their own tools can preview odds"),
        Runtime->TryGetDungeonTrapDisarmOdds(Party, Trap.InstanceId, Before));
    TestEqual(TEXT("Readonly odds select the real operator"), Before.BestCharacterIndex, 0);
    TestEqual(TEXT("Readonly odds include rank, DEX and expert class bonus"),
        Before.StaticBonus, 3);
    TestEqual(TEXT("Odds partition the d20 space"), Before.SuccessPercent +
        Before.SafeFailurePercent + Before.TriggerPercent, 100);
    TestTrue(TEXT("Unspent expert margin protects some outcomes"),
        Before.SafeFailurePercent > 0);

    // Going away and returning after earning one rank really improves odds.
    UGridPartyInventoryComponent* LiveInventory = Party->PartyInventoryComponent;
    LiveInventory->PartyInventoryState.ActiveCharacters[0].SkillRanks[0].Rank = 2;
    FGridDungeonTrapDisarmOdds Improved;
    TestTrue(TEXT("New rank immediately refreshes preview"),
        Runtime->TryGetDungeonTrapDisarmOdds(Party, Trap.InstanceId, Improved));
    TestEqual(TEXT("One rank raises successful d20 outcomes by five points"),
        Improved.SuccessPercent, Before.SuccessPercent + 5);
    LiveInventory->PartyInventoryState.ActiveCharacters[0].SkillRanks[0].Rank = 1;

    // No tools means there is no legal operator: never show misleading 0%.
    FGridInventorySlot& OriginalSlot = LiveInventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0];
    const bool bOccupied = OriginalSlot.bOccupied;
    OriginalSlot.bOccupied = false;
    FGridDungeonTrapDisarmOdds WithoutTools;
    TestFalse(TEXT("Readonly preview requires actual tools"),
        Runtime->TryGetDungeonTrapDisarmOdds(Party, Trap.InstanceId, WithoutTools));
    OriginalSlot.bOccupied = bOccupied;

    FRandomStream Dice(1801);
    FRPGSkillCheckResult Result;
    bool bTriggered = true;
    TestTrue(TEXT("First near miss resolves"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
            Dice, Result, nullptr, &bTriggered));
    TestFalse(TEXT("Near miss is a failed check"), Result.bSuccess);
    TestTrue(TEXT("Expert protects first failure by one point"), Result.bSafeFailure);
    TestFalse(TEXT("Safe failure never triggers trap"), bTriggered);
    TestEqual(TEXT("Safe failure leaves trap detected and armed"),
        Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Detected);
    TestEqual(TEXT("Safe failure never damages hero"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 30);
    FGridDungeonTrapDisarmOdds AfterSafe;
    TestTrue(TEXT("Odds remain available after the protected miss"),
        Runtime->TryGetDungeonTrapDisarmOdds(Party, Trap.InstanceId, AfterSafe));
    TestEqual(TEXT("Once spent, the protection covers no outcomes"),
        AfterSafe.SafeFailurePercent, 0);
    TestEqual(TEXT("The success chance is unaffected by spending protection"),
        AfterSafe.SuccessPercent, Before.SuccessPercent);
    TestEqual(TEXT("All former safe outcomes now risk activation"),
        AfterSafe.TriggerPercent, Before.TriggerPercent + Before.SafeFailurePercent);
    const FGridLevelRuntimeState* SavedLevel = Runtime->FindRuntimeStateForCurrentLevel();
    TestTrue(TEXT("Safe failure consumed once per trap GUID"),
        SavedLevel && SavedLevel->SafeDisarmFailureUsedTrapIds.Contains(Trap.InstanceId));

    const FGridDungeonRuntimeState Snapshot = Runtime->DungeonRuntimeState;
    AGridLevelRuntimeActor* Restored = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Restore actor"), Restored)) return false;
    Restored->LevelAsset = Level;
    Restored->WorldObjectDefinitions.Add(Def);
    Restored->DungeonRuntimeState = Snapshot;
    Party->LevelRuntimeActor = Restored;
    TestTrue(TEXT("Safe failure was persisted across Continue"),
        Restored->FindRuntimeStateForCurrentLevel() &&
        Restored->FindRuntimeStateForCurrentLevel()->SafeDisarmFailureUsedTrapIds.Contains(Trap.InstanceId));
    FGridDungeonTrapDisarmOdds RestoredOdds;
    TestTrue(TEXT("Readonly preview works after Continue"),
        Restored->TryGetDungeonTrapDisarmOdds(Party, Trap.InstanceId, RestoredOdds));
    TestEqual(TEXT("Continue never recharges a safe failure"),
        RestoredOdds.SafeFailurePercent, 0);

    FRandomStream Repeat(1801);
    bTriggered = false;
    TestTrue(TEXT("Same near miss resolves after Continue"),
        Restored->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
            Repeat, Result, nullptr, &bTriggered));
    TestFalse(TEXT("Once-only protection cannot be used twice"), Result.bSafeFailure);
    TestTrue(TEXT("Second near miss actually triggers the trap"), bTriggered);
    TestEqual(TEXT("One-shot trap becomes triggered"),
        Restored->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Triggered);
    TestTrue(TEXT("Unsafe failure applies normal trap damage"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth < 30);
    const int32 FinalSeed = Repeat.GetCurrentSeed();
    TestFalse(TEXT("Spent one-shot trap refuses repeated disarm"),
        Restored->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
            Repeat, Result, nullptr, &bTriggered));
    TestEqual(TEXT("Spent trap consumes no new RNG"), Repeat.GetCurrentSeed(), FinalSeed);
    TestEqual(TEXT("Failure never awards disarm XP"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Experience, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP018PitFailure,
    "Grimrock.TRAP01.8.UnsafeFailureOpensPit03",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP018PitFailure::RunTest(const FString&)
{
    TRAP018::FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Transient world"), Fixture.World)) return false;
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    AGrimrockPartyPawn* Party = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Runtime"), Runtime) ||
        !TestNotNull(TEXT("Party"), Party)) return false;

    UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
    UGridLevelAsset* Upper = NewObject<UGridLevelAsset>(Dungeon);
    UGridLevelAsset* Lower = NewObject<UGridLevelAsset>(Dungeon);
    Upper->EnsureCellCount();
    Lower->EnsureCellCount();
    for (FGridLevelCellData& Cell : Upper->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    for (FGridLevelCellData& Cell : Lower->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    const FName UpperId(TEXT("TRAP018_Upper")), LowerId(TEXT("TRAP018_Lower"));
    FGridDungeonLevelEntry UpperEntry;
    UpperEntry.LevelId = UpperId;
    UpperEntry.LevelAsset = Upper;
    UpperEntry.LogicalPosition = FIntVector(0, 0, 1);
    FGridDungeonLevelEntry LowerEntry;
    LowerEntry.LevelId = LowerId;
    LowerEntry.LevelAsset = Lower;
    LowerEntry.LogicalPosition = FIntVector(0, 0, 0);
    Dungeon->Levels = { UpperEntry, LowerEntry };
    Dungeon->DefaultLevelId = UpperId;
    Runtime->DungeonAsset = Dungeon;
    Runtime->LevelAsset = Upper;
    Runtime->CurrentDungeonLevelId = UpperId;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 2;
    Party->CurrentCellY = 2;

    UGridWorldObjectDefinitionAsset* Def = TRAP018::MakeTrap(Runtime, true);
    Runtime->WorldObjectDefinitions.Add(Def);
    UGridWorldObjectDefinitionAsset* PitDef =
        NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    PitDef->DefinitionId = TEXT("TRAP018_ControlledPit");
    PitDef->SupportedType = EGridLevelObjectType::Pit;
    PitDef->PlacementSurface = EGridObjectPlacementKind::Floor;
    PitDef->MovingParts.SetNum(2);
    PitDef->MovingParts[0].Mesh = NewObject<UStaticMesh>(Runtime);
    PitDef->MovingParts[1].Mesh = NewObject<UStaticMesh>(Runtime);
    Runtime->WorldObjectDefinitions.Add(PitDef);

    FGridWorldObjectInstance Trap = TRAP018::MakeTrapPlacement(Def, 2, 2, true, true);
    FGridWorldObjectInstance Pit;
    Pit.InstanceId = FGuid::NewGuid();
    Pit.Type = EGridLevelObjectType::Pit;
    Pit.WorldObjectDefinitionId = PitDef->DefinitionId;
    Pit.CellX = 2;
    Pit.CellY = 2;
    Pit.InstanceConfig.Pit.bInitiallyOpen = false;
    Pit.InstanceConfig.Pit.bUseSameCellCoordinates = true;
    Pit.InstanceConfig.Relocation.TargetLevelId = LowerId;
    Upper->WorldObjectInstances = { Trap, Pit };

    if (!TestTrue(TEXT("Pit trap discovered"), Runtime->TryRecordDungeonTrapDetection(Trap.InstanceId))) return false;
    URPGClassAsset* Class = NewObject<URPGClassAsset>(Runtime);
    Class->ClassId = TEXT("TRAP018_NoExpert");
    Class->DisplayName = FText::FromString(TEXT("Voleur sans spécialisation"));
    Class->HealthAtLevelOne = 10;
    if (!TestTrue(TEXT("Untrained class definition valid"), Class->IsValidDefinition())) return false;
    if (!TestTrue(TEXT("Disarm kit exists"), TRAP018::SetupOperator(Runtime, Party, Class, NAME_None))) return false;
    URPGSkillAsset* Skill = TRAP018::MakeSkill(Runtime);
    Def->DisarmDifficulty = 40; // Impossible for rank 1 and DEX 10.

    FRandomStream Dice(1802);
    FRPGSkillCheckResult Result;
    bool bTriggered = false;
    TestFalse(TEXT("Pit initially closed"), Runtime->IsPitOpen(Pit.InstanceId));
    TestTrue(TEXT("Unsafe disarm check resolves"),
        Runtime->TryDisarmTriggerTrap(Party, Trap.InstanceId, 0, Skill,
            Dice, Result, nullptr, &bTriggered));
    TestFalse(TEXT("Disarm attempt fails"), Result.bSuccess);
    TestFalse(TEXT("No expert talent yields no safe failure"), Result.bSafeFailure);
    TestTrue(TEXT("Failure actually activates PIT03 trigger"), bTriggered);
    TestTrue(TEXT("PIT03 opens via shared trap activation"), Runtime->IsPitOpen(Pit.InstanceId));
    TestTrue(TEXT("PIT01 fall started"), Party->IsPitFalling());
    TestEqual(TEXT("Mechanism-only trap inflicts no direct damage"),
        Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 30);
    TestTrue(TEXT("Triggered one-shot state survives activation"),
        Runtime->GetDungeonTrapState(Trap.InstanceId) == EGridDungeonTrapState::Triggered);
    const FGridLevelRuntimeState* State = Runtime->FindRuntimeStateForCurrentLevel();
    TestTrue(TEXT("PIT03 open state stored"), State && State->Pits.Contains(Pit.InstanceId) &&
        State->Pits.Find(Pit.InstanceId)->bIsOpen);
    return true;
}

#endif
