#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP013DamageResolver,
    "Grimrock.TRAP01.3.DamageResistanceAndArmor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP013DamageResolver::RunTest(const FString&)
{
    // The canonical resolver has no hit/crit/evasion for direct hazard damage.
    FGridAttackTargetStats First;
    First.CurrentHealth = 40;
    First.MagicalArmor = 5;
    First.PhysicalArmor = 8;
    First.ResistancePercent = 50;
    First.DamageMultiplier = 1.0f;

    const FGridAttackResult Fire = FGridCombatResolver::ResolveDirectDamage(
        First, EGridDamageType::Fire, 30);
    TestTrue(TEXT("Direct fire damage always hits a living target"), Fire.bHit);
    TestEqual(TEXT("50% fire resistance halves raw damage"), Fire.DamageAfterModifiers, 15);
    TestEqual(TEXT("Magic armor depleted before HP"), Fire.MagicalArmorDamage, 5);
    TestEqual(TEXT("Fire removes only 10 HP"), Fire.HealthDamage, 10);
    TestEqual(TEXT("Physical armor is not affected by fire"), Fire.PhysicalArmorDamage, 0);
    TestEqual(TEXT("After fire, target HP is 30"), Fire.TargetHealthAfter, 30);

    FGridAttackTargetStats Second = First;
    Second.ResistancePercent = 0;
    Second.MagicalArmor = 0;
    const FGridAttackResult Unprotected = FGridCombatResolver::ResolveDirectDamage(
        Second, EGridDamageType::Fire, 30);
    TestEqual(TEXT("Unprotected target takes 30 HP from same roll"),
        Unprotected.HealthDamage, 30);

    const FGridAttackResult Piercing = FGridCombatResolver::ResolveDirectDamage(
        First, EGridDamageType::Physical, 20, EGridPhysicalDamageSubtype::Piercing);
    TestEqual(TEXT("Piercing physical damage retains subtype"),
        Piercing.PhysicalSubtype, EGridPhysicalDamageSubtype::Piercing);
    TestEqual(TEXT("Physical armor absorbs damage after resistance"),
        Piercing.PhysicalArmorDamage, 8);
    TestEqual(TEXT("Remaining damage reaches HP"), Piercing.HealthDamage, 2);
    TestEqual(TEXT("Physical trap does not touch magic armor"), Piercing.MagicalArmorDamage, 0);

    FGridAttackTargetStats Dead = First;
    Dead.CurrentHealth = 0;
    const FGridAttackResult DeadResult = FGridCombatResolver::ResolveDirectDamage(
        Dead, EGridDamageType::Fire, 30);
    TestFalse(TEXT("Dead party member cannot be damaged again"), DeadResult.bHit);
    TestEqual(TEXT("Dead member HP damage stays zero"), DeadResult.HealthDamage, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTRAP013RuntimeOnEnter,
    "Grimrock.TRAP01.3.OnEnterGroupDamageAndSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP013RuntimeOnEnter::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
        FName(*FString::Printf(TEXT("TRAP013_%s"),
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
    if (!TestNotNull(TEXT("Runtime actor"), Runtime) ||
        !TestNotNull(TEXT("Party pawn"), Party) ||
        !TestNotNull(TEXT("Party inventory"),
            Party ? Party->PartyInventoryComponent.Get() : nullptr))
    {
        Cleanup();
        return false;
    }

    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
    Level->EnsureCellCount();
    Level->GetCellMutable(2, 2).CellType = EGridCellType::Floor;
    Level->GetCellMutable(2, 3).CellType = EGridCellType::Floor;
    Runtime->LevelAsset = Level;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 2;
    Party->CurrentCellY = 3;
    Party->Facing = EGridEdge::North;

    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    Definition->DefinitionId = TEXT("Test_Fire_Ceiling_Trap");
    Definition->SupportedType = EGridLevelObjectType::Trigger;
    Definition->PlacementSurface = EGridObjectPlacementKind::Ceiling;
    Definition->DungeonTrap.bIsDungeonTrap = true;
    Definition->DungeonTrap.ActivationPolicy = EGridDungeonTrapActivationPolicy::OnEnterCell;
    Definition->DungeonTrap.DamageType = EGridDamageType::Fire;
    Definition->DungeonTrap.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
    Definition->DungeonTrap.MinimumDamage = 30;
    Definition->DungeonTrap.MaximumDamage = 30;
    Definition->DungeonTrap.bOneShot = true;
    Definition->DungeonTrap.bAffectsEntireParty = true;
    Runtime->WorldObjectDefinitions.Add(Definition);

    FGridWorldObjectInstance Trap;
    Trap.InstanceId = FGuid::NewGuid();
    Trap.WorldObjectDefinitionId = Definition->DefinitionId;
    Trap.Type = EGridLevelObjectType::Trigger;
    Trap.CellX = 2;
    Trap.CellY = 3;
    Level->WorldObjectInstances.Add(Trap);

    UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
    for (int32 CharacterIndex = 0; CharacterIndex < 3; ++CharacterIndex)
    {
        FGridCharacterInventoryState& Hero =
            Inventory->PartyInventoryState.ActiveCharacters.AddDefaulted_GetRef();
        Hero.CharacterId = FGuid::NewGuid();
        Hero.DisplayName = FText::FromString(
            FString::Printf(TEXT("Hero %d"), CharacterIndex + 1));
        Hero.Resources.CurrentHealth = CharacterIndex == 2 ? 0 : 40;
        Hero.Resources.CurrentMagicalArmor = CharacterIndex == 0 ? 10 : 0;
        Hero.Resources.CurrentPhysicalArmor = CharacterIndex == 0 ? 5 : 0;
    }
    FRandomStream Random(1303);
    const int32 InitialSeed = Random.GetCurrentSeed();

    // Direct request cannot damage a party standing in another cell.
    TestEqual(TEXT("Wrong entered cell is rejected"),
        Runtime->TriggerDungeonTrapsOnPartyEnterCell(Party, 2, 2, Random), 0);
    TestEqual(TEXT("Invalid attempt consumes no RNG"), Random.GetCurrentSeed(), InitialSeed);
    TestEqual(TEXT("Initially hidden"), Runtime->GetDungeonTrapState(Trap.InstanceId),
        EGridDungeonTrapState::Hidden);
    TestEqual(TEXT("One entered-cell trap fires exactly once"),
        Runtime->TriggerDungeonTrapsOnPartyEnterCell(Party, 2, 3, Random), 1);

    TestEqual(TEXT("First member magic armor depleted"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor, 0);
    TestEqual(TEXT("First member loses 20 HP after magic armor"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 20);
    TestEqual(TEXT("Second member without magic protection loses 30 HP"),
        Inventory->PartyInventoryState.ActiveCharacters[1].Resources.CurrentHealth, 10);
    TestEqual(TEXT("Dead third member remains dead"),
        Inventory->PartyInventoryState.ActiveCharacters[2].Resources.CurrentHealth, 0);
    TestEqual(TEXT("Fire cannot deplete physical armor"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 5);
    TestEqual(TEXT("Trap triggered and thus revealed"),
        Runtime->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Triggered);
    TestTrue(TEXT("Undetected triggered trap is marked discovered"),
        Runtime->IsTriggerTrapDetected(Trap.InstanceId));

    const int32 SeedAfterTrigger = Random.GetCurrentSeed();
    TestEqual(TEXT("One shot cannot damage party twice"),
        Runtime->TriggerDungeonTrapsOnPartyEnterCell(Party, 2, 3, Random), 0);
    TestEqual(TEXT("One-shot replay cannot consume RNG"),
        Random.GetCurrentSeed(), SeedAfterTrigger);
    TestEqual(TEXT("Second member HP unchanged on replay"),
        Inventory->PartyInventoryState.ActiveCharacters[1].Resources.CurrentHealth, 10);

    // Save/Continue must preserve one-shot consumption and trap discovery.
    const FGridDungeonRuntimeState Saved = Runtime->DungeonRuntimeState;
    AGridLevelRuntimeActor* Reloaded = World->SpawnActor<AGridLevelRuntimeActor>();
    if (TestNotNull(TEXT("Restored runtime"), Reloaded))
    {
        Reloaded->LevelAsset = Level;
        Reloaded->WorldObjectDefinitions.Add(Definition);
        Reloaded->DungeonRuntimeState = Saved;
        Party->LevelRuntimeActor = Reloaded;
        TestEqual(TEXT("Restored instance remains triggered"),
            Reloaded->GetDungeonTrapState(Trap.InstanceId), EGridDungeonTrapState::Triggered);
        TestEqual(TEXT("Save/Continue does not permit replay"),
            Reloaded->TriggerDungeonTrapsOnPartyEnterCell(Party, 2, 3, Random), 0);
        Party->LevelRuntimeActor = Runtime;
    }

    // A real discovered-but-disarmed trap is never activated.
    FGridWorldObjectInstance DisarmedTrap = Trap;
    DisarmedTrap.InstanceId = FGuid::NewGuid();
    Level->WorldObjectInstances.Add(DisarmedTrap);
    FGridLevelRuntimeState* State = Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (TestNotNull(TEXT("Writable level state"), State))
    {
        State->DisarmedTriggerTrapIds.Add(DisarmedTrap.InstanceId);
        TestEqual(TEXT("Only already-fired one shot and disarmed traps in cell"),
            Runtime->TriggerDungeonTrapsOnPartyEnterCell(Party, 2, 3, Random), 0);
    }

    // The actual movement hook, not a mouse click, must fire a new hazard.
    UGridWorldObjectDefinitionAsset* PhysicalDefinition =
        NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
    PhysicalDefinition->DefinitionId = TEXT("Test_Piercing_Floor_Trap");
    PhysicalDefinition->SupportedType = EGridLevelObjectType::Trigger;
    PhysicalDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
    PhysicalDefinition->DungeonTrap.bIsDungeonTrap = true;
    PhysicalDefinition->DungeonTrap.DamageType = EGridDamageType::Physical;
    PhysicalDefinition->DungeonTrap.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
    PhysicalDefinition->DungeonTrap.MinimumDamage = 10;
    PhysicalDefinition->DungeonTrap.MaximumDamage = 10;
    PhysicalDefinition->DungeonTrap.bOneShot = true;
    Runtime->WorldObjectDefinitions.Add(PhysicalDefinition);
    FGridWorldObjectInstance PhysicalTrap = Trap;
    PhysicalTrap.InstanceId = FGuid::NewGuid();
    PhysicalTrap.WorldObjectDefinitionId = PhysicalDefinition->DefinitionId;
    Level->WorldObjectInstances.Add(PhysicalTrap);

    Runtime->HandlePartyCellChanged(2, 3, 2, 3); // Initial stable cell, not entry.
    TestEqual(TEXT("Stable-cell refresh cannot spring a new trap"),
        Runtime->GetDungeonTrapState(PhysicalTrap.InstanceId), EGridDungeonTrapState::Hidden);
    TestEqual(TEXT("Stable-cell refresh preserves HP"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 20);

    Runtime->HandlePartyCellChanged(2, 2, 2, 3); // Genuine completed translation.
    TestEqual(TEXT("Real translation triggers the physical trap"),
        Runtime->GetDungeonTrapState(PhysicalTrap.InstanceId), EGridDungeonTrapState::Triggered);
    TestEqual(TEXT("Physical armor is spent before HP"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 0);
    TestEqual(TEXT("First character loses 5 HP after 5 physical armor"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 15);
    TestEqual(TEXT("Unprotected second character loses 10 HP"),
        Inventory->PartyInventoryState.ActiveCharacters[1].Resources.CurrentHealth, 0);
    TestEqual(TEXT("Dead third party member unchanged"),
        Inventory->PartyInventoryState.ActiveCharacters[2].Resources.CurrentHealth, 0);

    Cleanup();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
