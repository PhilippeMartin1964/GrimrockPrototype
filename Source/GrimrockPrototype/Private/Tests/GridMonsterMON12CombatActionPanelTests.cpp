#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "UI/GridCombatActionPanelWidget.h"

namespace
{
    struct FGridMON12TestWorld
    {
        UWorld* World = nullptr;

        FGridMON12TestWorld ()
        {
            const UWorld::InitializationValues InitializationValues =
                UWorld::InitializationValues ()
                    .AllowAudioPlayback (false)
                    .RequiresHitProxies (false)
                    .CreatePhysicsScene (false)
                    .CreateNavigation (false)
                    .CreateAISystem (false)
                    .ShouldSimulatePhysics (false)
                    .SetTransactional (false);
            World = UWorld::CreateWorld (
                EWorldType::Game,
                false,
                FName (*FString::Printf (
                    TEXT ("MON12TestWorld_%s"),
                    *FGuid::NewGuid ().ToString (
                        EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &InitializationValues);
            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (
                        EWorldType::Game);
                Context.SetCurrentWorld (World);
            }
        }

        ~FGridMON12TestWorld ()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld (false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext (World);
            }
        }
    };

    FGridCharacterInventoryState MakeCharacter (
        const FGuid& CharacterId,
        const TCHAR* DisplayName,
        int32 CurrentHealth,
        int32 MaxHealth,
        int32 CurrentMana,
        int32 MaxMana,
        UTexture2D* Portrait)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = CharacterId;
        Character.DisplayName =
            FText::FromString (DisplayName);
        Character.DerivedStats.CurrentHealth = CurrentHealth;
        Character.DerivedStats.MaxHealth = MaxHealth;
        Character.DerivedStats.CurrentMana = CurrentMana;
        Character.DerivedStats.MaxMana = MaxMana;
        Character.Portrait = Portrait;
        Character.InventorySlots.SetNum (4);
        return Character;
    }

    FGridItemInstance MakeEquippedItem (
        const FGuid& RuntimeObjectId,
        FName ItemDefinitionId,
        const TCHAR* DisplayName,
        int32 Quantity,
        int32 CharacterIndex,
        EGridEquipmentSlot EquipmentSlot)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = RuntimeObjectId;
        Item.ItemDefinitionId = ItemDefinitionId;
        Item.DisplayName =
            FText::FromString (DisplayName);
        Item.Quantity = Quantity;
        Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
        Item.OwnerCharacterIndex = CharacterIndex;
        Item.EquipmentSlot = EquipmentSlot;
        return Item;
    }

    UGridItemDefinitionAsset* MakeItemDefinition (
        UObject* Outer,
        FName ItemDefinitionId,
        const TCHAR* DisplayName,
        bool bStackable,
        UTexture2D* Icon)
    {
        UGridItemDefinitionAsset* Definition =
            NewObject<UGridItemDefinitionAsset> (Outer);
        Definition->ItemDefinitionId = ItemDefinitionId;
        Definition->DisplayName =
            FText::FromString (DisplayName);
        Definition->bStackable = bStackable;
        Definition->MaxStackSize = bStackable ? 20 : 1;
        Definition->Icon = Icon;
        return Definition;
    }

    void ConfigureOffensiveDefinition (
        UGridItemDefinitionAsset* Definition,
        FName AttackId,
        EGridEquipmentSlot EquipmentSlot,
        int32 RangeCells = 1)
    {
        if (!Definition)
        {
            return;
        }

        Definition->bProvidesAttack = true;
        Definition->CompatibleEquipmentSlots.AddUnique (EquipmentSlot);
        Definition->OffensiveProfile.AttackId = AttackId;
        Definition->OffensiveProfile.AttackDefinition.DamageType =
            EGridDamageType::Physical;
        Definition->OffensiveProfile.AttackDefinition.PhysicalSubtype =
            EGridPhysicalDamageSubtype::Piercing;
        Definition->OffensiveProfile.AttackDefinition.MinDamage = 1;
        Definition->OffensiveProfile.AttackDefinition.MaxDamage = 2;
        Definition->OffensiveProfile.DamageScalingAttribute =
            EGridAttackScalingAttribute::Dexterity;
        Definition->OffensiveProfile.RangeCells = RangeCells;
    }

    struct FGridMON12Fixture
    {
        FGridMON12TestWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* LevelAsset = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        UGridMonsterDefinitionAsset* MonsterDefinition = nullptr;
        AGridMonsterActor* Monster = nullptr;
        UGridMonsterOccupancySubsystem* Occupancy = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        UGridCombatActionPanelWidget* Panel = nullptr;
        UTexture2D* EliasPortrait = nullptr;
        UTexture2D* MinaPortrait = nullptr;
        UTexture2D* ShurikenIcon = nullptr;
        UTexture2D* TorchIcon = nullptr;
        FGuid EliasId = FGuid (12, 1, 0, 1);
        FGuid MinaId = FGuid (12, 1, 0, 2);

        FGridMON12Fixture ()
        {
            if (!TestWorld.World)
            {
                return;
            }

            Runtime =
                TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
            LevelAsset = NewObject<UGridLevelAsset> (Runtime);
            LevelAsset->Width = 3;
            LevelAsset->Height = 3;
            LevelAsset->EnsureCellCount ();
            for (FGridLevelCellData& Cell : LevelAsset->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            Runtime->LevelAsset = LevelAsset;
            Runtime->CurrentDungeonLevelId = TEXT ("MON12_Test");

            Party =
                TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 1;
            Party->CurrentCellY = 1;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation (
                Runtime->GetCellCenterWorld (
                    1,
                    1,
                    Party->EyeHeight));
            Party->SetActorRotation (FRotator (
                0.0f,
                GridDirectionUtils::ToYaw (Party->Facing),
                0.0f));

            EliasPortrait = NewObject<UTexture2D> (Party);
            MinaPortrait = NewObject<UTexture2D> (Party);
            ShurikenIcon = NewObject<UTexture2D> (Party);
            TorchIcon = NewObject<UTexture2D> (Party);

            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters = {
                    MakeCharacter (
                        EliasId,
                        TEXT ("Elias"),
                        18,
                        24,
                        7,
                        12,
                        EliasPortrait),
                    MakeCharacter (
                        MinaId,
                        TEXT ("Mina"),
                        15,
                        20,
                        3,
                        9,
                        MinaPortrait)
                };
            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveEquipment.SetNum (2);
            Party->PartyInventoryComponent->PartyInventoryState
                .SelectedCharacterIndex = 0;

            FGridCharacterEquipmentState& EliasEquipment =
                Party->PartyInventoryComponent->PartyInventoryState
                    .ActiveEquipment[0];
            EliasEquipment.MainHand = MakeEquippedItem (
                FGuid (12, 2, 0, 1),
                TEXT ("Shuriken"),
                TEXT ("Shuriken"),
                3,
                0,
                EGridEquipmentSlot::MainHand);
            EliasEquipment.OffHand = MakeEquippedItem (
                FGuid (12, 2, 0, 2),
                TEXT ("Item_Torch"),
                TEXT ("Torche"),
                1,
                0,
                EGridEquipmentSlot::OffHand);

            UGridItemDefinitionAsset* ShurikenDefinition =
                MakeItemDefinition (
                    Party,
                    TEXT ("Shuriken"),
                    TEXT ("Shuriken"),
                    true,
                    ShurikenIcon);
            ConfigureOffensiveDefinition (
                ShurikenDefinition,
                TEXT ("Attack_Shuriken"),
                EGridEquipmentSlot::MainHand,
                2);
            Party->PartyInventoryComponent->RegisterItemDefinition (
                ShurikenDefinition);
            Party->PartyInventoryComponent->RegisterItemDefinition (
                MakeItemDefinition (
                    Party,
                    TEXT ("Item_Torch"),
                    TEXT ("Torche"),
                    false,
                    TorchIcon));

            MonsterDefinition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            MonsterDefinition->MonsterId = TEXT ("MON12_Rat");
            MonsterDefinition->DisplayName =
                FText::FromString (TEXT ("Rat MON12"));
            MonsterDefinition->CategoryId = TEXT ("Vermin");
            MonsterDefinition->MaxHealth = 1000;
            MonsterDefinition->Evasion = 0;
            MonsterDefinition->ActionPointsPerTurn = 2;
            Occupancy = TestWorld.World
                ->GetSubsystem<UGridMonsterOccupancySubsystem> ();
            Monster = TestWorld.World->SpawnActor<AGridMonsterActor> ();
            if (Monster && Occupancy)
            {
                Monster->InitializeMonster (
                    MonsterDefinition,
                    FGuid (12, 3, 0, 1),
                    FIntPoint (1, 2),
                    EGridEdge::South);
                Occupancy->RegisterMonster (
                    Monster,
                    FIntPoint (1, 2));
            }

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Runtime,
                TEXT ("MON12TurnManager"));
            TurnManager->bAutoInitialize = false;
            Runtime->AddInstanceComponent (TurnManager);
            TurnManager->RegisterComponent ();
            TurnManager->InitializeTurnManager (Runtime, Party);
            TurnManager->bCombatActive = true;
            TurnManager->CurrentPhase =
                EGridCombatPhase::PlayerPhase;
            TurnManager->RoundNumber = 1;
            if (Monster)
            {
                TurnManager->CombatMonsters = { Monster };
            }

            Panel =
                NewObject<UGridCombatActionPanelWidget> (Party);
        }

        bool IsReady () const
        {
            return TestWorld.World &&
                Runtime &&
                LevelAsset &&
                Party &&
                Party->PartyInventoryComponent &&
                MonsterDefinition &&
                Monster &&
                Occupancy &&
                TurnManager &&
                Panel &&
                EliasPortrait &&
                MinaPortrait &&
                ShurikenIcon &&
                TorchIcon;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON12CombatActionPanelLiveDataTest,
    "Grimrock.Monsters.MON12.CombatActionPanel.LiveData",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatActionPanelLiveDataTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON12Fixture Fixture;
    if (!TestTrue (
        TEXT ("The MON12 fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.Panel->InitializeCombatActionPanel (
        Fixture.Party,
        0,
        Fixture.TurnManager);

    const FGridCombatActionPanelView& View =
        Fixture.Panel->View;
    TestTrue (
        TEXT ("The panel resolves its assigned character"),
        View.bHasValidCharacter);
    TestEqual (
        TEXT ("The panel keeps the assigned member index"),
        View.CharacterIndex,
        0);
    TestEqual (
        TEXT ("The panel reads the real character name"),
        View.DisplayName.ToString (),
        FString (TEXT ("Elias")));
    TestEqual (
        TEXT ("The panel reads current health"),
        View.CurrentHealth,
        18);
    TestEqual (
        TEXT ("The panel reads maximum health"),
        View.MaxHealth,
        24);
    TestEqual (
        TEXT ("The panel reads current mana"),
        View.CurrentMana,
        7);
    TestEqual (
        TEXT ("The panel reads maximum mana"),
        View.MaxMana,
        12);
    TestTrue (
        TEXT ("The panel reads the real portrait"),
        View.Portrait.Get () == Fixture.EliasPortrait);
    TestTrue (
        TEXT ("MainHand is occupied"),
        View.MainHand.bOccupied);
    TestTrue (
        TEXT ("MainHand uses the registered shuriken icon"),
        View.MainHand.Icon.Get () == Fixture.ShurikenIcon);
    TestEqual (
        TEXT ("MainHand exposes the live stack quantity"),
        View.MainHand.Quantity,
        3);
    TestTrue (
        TEXT ("The stackable quantity is visible"),
        View.MainHand.bShowQuantity);
    TestTrue (
        TEXT ("OffHand is occupied"),
        View.OffHand.bOccupied);
    TestTrue (
        TEXT ("OffHand uses the registered torch icon"),
        View.OffHand.Icon.Get () == Fixture.TorchIcon);
    TestFalse (
        TEXT ("A non-stackable quantity remains hidden"),
        View.OffHand.bShowQuantity);
    TestEqual (
        TEXT ("The initial turn state is Active"),
        View.TurnState,
        EGridCombatantTurnState::Active);
    TestEqual (
        TEXT ("The initial action-point budget is full"),
        View.RemainingActionPoints,
        4);
    TestEqual (
        TEXT ("The panel exposes the maximum action points"),
        View.MaximumActionPoints,
        4);
    TestEqual (
        TEXT ("The panel exposes the attack action-point cost"),
        View.AttackActionPointCost,
        2);
    TestTrue (
        TEXT ("The turn manager authorizes the living ready member"),
        View.bCanAct);

    FGridItemInstance ExtractedItem;
    TestTrue (
        TEXT ("One equipped shuriken is extracted"),
        Fixture.Party->PartyInventoryComponent
            ->TryExtractOneEquippedItemForWorldTransfer (
                0,
                EGridEquipmentSlot::MainHand,
                TEXT ("Shuriken"),
                ExtractedItem));
    TestEqual (
        TEXT ("The inventory event refreshes the quantity without Tick"),
        Fixture.Panel->View.MainHand.Quantity,
        2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON12CombatActionPanelTurnAuthorityTest,
    "Grimrock.Monsters.MON12.CombatActionPanel.TurnAuthority",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatActionPanelTurnAuthorityTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON12Fixture Fixture;
    if (!TestTrue (
        TEXT ("The MON12 fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.Panel->InitializeCombatActionPanel (
        Fixture.Party,
        INDEX_NONE,
        Fixture.TurnManager);
    TestTrue (
        TEXT ("INDEX_NONE enables selected-member following"),
        Fixture.Panel->bFollowSelectedCharacter);
    TestEqual (
        TEXT ("The panel initially follows character zero"),
        Fixture.Panel->View.CharacterIndex,
        0);

    TestTrue (
        TEXT ("Selecting the second character succeeds"),
        Fixture.Party->PartyInventoryComponent
            ->SetSelectedCharacterIndex (1));
    TestEqual (
        TEXT ("The selection notification retargets the same panel"),
        Fixture.Panel->View.CharacterIndex,
        1);
    TestEqual (
        TEXT ("The retargeted panel reads Mina"),
        Fixture.Panel->View.DisplayName.ToString (),
        FString (TEXT ("Mina")));
    TestTrue (
        TEXT ("The second living member is initially actionable"),
        Fixture.Panel->View.bCanAct);

    Fixture.TurnManager->CurrentPhase =
        EGridCombatPhase::EnemyPhase;
    Fixture.TurnManager->OnPhaseChanged.Broadcast (
        EGridCombatPhase::EnemyPhase);
    TestFalse (
        TEXT ("A Ready member is disabled outside the player phase"),
        Fixture.Panel->View.bCanAct);

    Fixture.TurnManager->CurrentPhase =
        EGridCombatPhase::PlayerPhase;
    Fixture.TurnManager->OnPhaseChanged.Broadcast (
        EGridCombatPhase::PlayerPhase);
    TestTrue (
        TEXT ("The member is enabled again in the player phase"),
        Fixture.Panel->View.bCanAct);

    FGridPlayerCharacterTurnState CompletedState;
    CompletedState.CharacterIndex = 1;
    CompletedState.CharacterId = Fixture.MinaId;
    CompletedState.State = EGridCombatantTurnState::Completed;
    CompletedState.MaximumActionPoints = 4;
    CompletedState.RemainingActionPoints = 0;
    Fixture.TurnManager->PlayerCharacterTurnStates = {
        CompletedState
    };
    Fixture.TurnManager->OnPlayerCharacterTurnStateChanged.Broadcast (
        CompletedState);

    TestEqual (
        TEXT ("The TurnManager event produces Completed"),
        Fixture.Panel->View.TurnState,
        EGridCombatantTurnState::Completed);
    TestFalse (
        TEXT ("Completed disables the panel"),
        Fixture.Panel->View.bCanAct);

    Fixture.Party->PartyInventoryComponent->PartyInventoryState
        .ActiveCharacters[1].DerivedStats.CurrentHealth = 9;
    FGridAttackResult Result;
    Fixture.TurnManager->OnAttackResolved.Broadcast (
        nullptr,
        1,
        Result);
    TestEqual (
        TEXT ("Monster attack notification refreshes current health"),
        Fixture.Panel->View.CurrentHealth,
        9);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON12CombatActionPanelSlotRoutingTest,
    "Grimrock.Monsters.MON12.CombatActionPanel.SlotAttackRouting",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatActionPanelSlotRoutingTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON12Fixture Fixture;
    if (!TestTrue (
        TEXT ("The MON12 slot-routing fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    UGridItemDefinitionAsset* DaggerDefinition =
        MakeItemDefinition (
            Fixture.Party,
            TEXT ("Item_Dagger"),
            TEXT ("Dague"),
            false,
            Fixture.TorchIcon);
    ConfigureOffensiveDefinition (
        DaggerDefinition,
        TEXT ("Attack_Dagger"),
        EGridEquipmentSlot::OffHand);
    Fixture.Party->PartyInventoryComponent->RegisterItemDefinition (
        DaggerDefinition);
    Fixture.Party->PartyInventoryComponent->PartyInventoryState
        .ActiveEquipment[0].OffHand = MakeEquippedItem (
            FGuid (12, 2, 0, 3),
            DaggerDefinition->ItemDefinitionId,
            TEXT ("Dague"),
            1,
            0,
            EGridEquipmentSlot::OffHand);

    Fixture.Panel->InitializeCombatActionPanel (
        Fixture.Party,
        0,
        Fixture.TurnManager);
    TestTrue (
        TEXT ("MainHand exposes an offensive action"),
        Fixture.Panel->View.MainHand.bCanAttack);
    TestTrue (
        TEXT ("OffHand exposes its own offensive action"),
        Fixture.Panel->View.OffHand.bCanAttack);

    TestTrue (
        TEXT ("Clicking OffHand requests an attack"),
        Fixture.Panel->RequestAttackFromSlot (
            EGridEquipmentSlot::OffHand));
    TestEqual (
        TEXT ("The turn manager keeps the clicked OffHand"),
        Fixture.TurnManager->LastPlayerAttackRequest
            .OffensiveEquipmentSlot,
        EGridEquipmentSlot::OffHand);
    TestEqual (
        TEXT ("The turn manager uses the OffHand item"),
        Fixture.TurnManager->LastPlayerAttackRequest
            .OffensiveItemDefinitionId,
        DaggerDefinition->ItemDefinitionId);
    TestEqual (
        TEXT ("The OffHand attack id is retained"),
        Fixture.TurnManager->LastPlayerAttackRequest.AttackId,
        FName (TEXT ("Attack_Dagger")));
    TestEqual (
        TEXT ("The accepted click keeps the character Active"),
        Fixture.Panel->View.TurnState,
        EGridCombatantTurnState::Active);
    TestEqual (
        TEXT ("The accepted click spends two action points"),
        Fixture.Panel->View.RemainingActionPoints,
        2);
    TestTrue (
        TEXT ("The panel permits a second two-AP attack"),
        Fixture.Panel->View.bCanAct);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON12CombatActionPanelSlotRejectionTest,
    "Grimrock.Monsters.MON12.CombatActionPanel.SlotAttackRejection",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatActionPanelSlotRejectionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON12Fixture Fixture;
    if (!TestTrue (
        TEXT ("The MON12 slot-rejection fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.Panel->InitializeCombatActionPanel (
        Fixture.Party,
        0,
        Fixture.TurnManager);
    TestFalse (
        TEXT ("The equipped torch is visible but not offensive"),
        Fixture.Panel->View.OffHand.bCanAttack);
    TestFalse (
        TEXT ("A non-offensive OffHand request is rejected"),
        Fixture.Panel->RequestAttackFromSlot (
            EGridEquipmentSlot::OffHand));
    TestEqual (
        TEXT ("The rejection reason identifies non-offensive equipment"),
        Fixture.TurnManager->LastPlayerAttackRejectReason,
        EGridPlayerAttackRejectReason::InvalidOffensiveEquipment);
    FGridPlayerCharacterTurnState TurnState;
    TestTrue (
        TEXT ("The rejected character turn state is available"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (
            0,
            TurnState));
    TestEqual (
        TEXT ("A rejected click consumes no action points"),
        TurnState.RemainingActionPoints,
        4);
    TestEqual (
        TEXT ("A rejected click leaves the panel Active"),
        Fixture.Panel->View.TurnState,
        EGridCombatantTurnState::Active);
    TestTrue (
        TEXT ("A rejected click leaves the character actionable"),
        Fixture.Panel->View.bCanAct);

    Fixture.Party->PartyInventoryComponent->PartyInventoryState
        .ActiveEquipment[0].OffHand = FGridItemInstance ();
    Fixture.Panel->RefreshFromSources ();
    TestTrue (
        TEXT ("An empty OffHand exposes the unarmed action"),
        Fixture.Panel->View.OffHand.bCanAttack);
    TestTrue (
        TEXT ("An empty OffHand request resolves unarmed"),
        Fixture.Panel->RequestAttackFromSlot (
            EGridEquipmentSlot::OffHand));
    TestTrue (
        TEXT ("The unarmed request has no item definition"),
        Fixture.TurnManager->LastPlayerAttackRequest
            .OffensiveItemDefinitionId.IsNone ());
    TestEqual (
        TEXT ("The unarmed request has no equipment slot"),
        Fixture.TurnManager->LastPlayerAttackRequest
            .OffensiveEquipmentSlot,
        EGridEquipmentSlot::None);
    TestEqual (
        TEXT ("The unarmed attack id is retained"),
        Fixture.TurnManager->LastPlayerAttackRequest.AttackId,
        FName (TEXT ("Attack_Unarmed")));
    TestEqual (
        TEXT ("The accepted unarmed attack spends two action points"),
        Fixture.Panel->View.RemainingActionPoints,
        2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON12CharacterActionPointLifecycleTest,
    "Grimrock.Monsters.MON12.CharacterActionPoints.Lifecycle",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CharacterActionPointLifecycleTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridMON12Fixture Fixture;
    if (!TestTrue (
        TEXT ("The MON12 action-point fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.Panel->InitializeCombatActionPanel (
        Fixture.Party,
        0,
        Fixture.TurnManager);
    Fixture.TurnManager->BeginPlayerCharacterPhase ();

    FGridPlayerCharacterTurnState EliasTurn;
    FGridPlayerCharacterTurnState MinaTurn;
    TestTrue (
        TEXT ("Elias has an authoritative turn state"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (
            0,
            EliasTurn));
    TestEqual (
        TEXT ("Elias begins the round Active"),
        EliasTurn.State,
        EGridCombatantTurnState::Active);
    TestEqual (
        TEXT ("Elias begins with four action points"),
        EliasTurn.RemainingActionPoints,
        4);
    TestTrue (
        TEXT ("Mina has an independent turn state"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (
            1,
            MinaTurn));
    TestEqual (
        TEXT ("Mina also begins with four action points"),
        MinaTurn.RemainingActionPoints,
        4);

    FGridPlayerAttackRequest Request;
    FGridAttackResult Result;
    EGridPlayerAttackRejectReason RejectReason =
        EGridPlayerAttackRejectReason::None;
    TestTrue (
        TEXT ("Elias can make a first two-AP attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestEqual (
        TEXT ("The request records its authoritative AP cost"),
        Request.ActionPointCost,
        2);
    TestEqual (
        TEXT ("The panel refreshes to two remaining AP without Tick"),
        Fixture.Panel->View.RemainingActionPoints,
        2);
    TestEqual (
        TEXT ("Elias remains Active after the first attack"),
        Fixture.Panel->View.TurnState,
        EGridCombatantTurnState::Active);
    TestTrue (
        TEXT ("Elias can still afford a second attack"),
        Fixture.Panel->View.bCanPayAttackCost);

    TestTrue (
        TEXT ("Elias can make a second two-AP attack"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestEqual (
        TEXT ("The second attack exhausts Elias action points"),
        Fixture.Panel->View.RemainingActionPoints,
        0);
    TestEqual (
        TEXT ("An exhausted character becomes Completed"),
        Fixture.Panel->View.TurnState,
        EGridCombatantTurnState::Completed);
    TestFalse (
        TEXT ("A Completed character cannot act"),
        Fixture.Panel->View.bCanAct);

    const int32 SeedBeforeRefusal =
        Fixture.TurnManager->CombatRandomStream.GetCurrentSeed ();
    const int32 HealthBeforeRefusal = Fixture.Monster->CurrentHealth;
    TestFalse (
        TEXT ("A third attack is refused"),
        Fixture.TurnManager->RequestCharacterAttack (
            0,
            Request,
            Result,
            RejectReason));
    TestEqual (
        TEXT ("The refusal reports insufficient action points"),
        RejectReason,
        EGridPlayerAttackRejectReason::InsufficientActionPoints);
    TestEqual (
        TEXT ("The AP refusal consumes no random draw"),
        Fixture.TurnManager->CombatRandomStream.GetCurrentSeed (),
        SeedBeforeRefusal);
    TestEqual (
        TEXT ("The AP refusal applies no damage"),
        Fixture.Monster->CurrentHealth,
        HealthBeforeRefusal);

    TestTrue (
        TEXT ("Mina remains independently actionable"),
        Fixture.TurnManager->CanCharacterSpendActionPoints (1, 2));
    TestTrue (
        TEXT ("Mina still retains her full budget"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (
            1,
            MinaTurn));
    TestEqual (
        TEXT ("Elias spending does not change Mina AP"),
        MinaTurn.RemainingActionPoints,
        4);

    Fixture.TurnManager->CompletePlayerCharacterPhase ();
    TestTrue (
        TEXT ("Mina state remains available after ending the phase"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (
            1,
            MinaTurn));
    TestEqual (
        TEXT ("Ending the player phase marks unspent characters Completed"),
        MinaTurn.State,
        EGridCombatantTurnState::Completed);
    TestEqual (
        TEXT ("Ending a phase does not rewrite the diagnostic AP remainder"),
        MinaTurn.RemainingActionPoints,
        4);

    Fixture.TurnManager->BeginPlayerCharacterPhase ();
    TestTrue (
        TEXT ("Elias state is restored for the next round"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (
            0,
            EliasTurn));
    TestEqual (
        TEXT ("The next round restores four action points"),
        EliasTurn.RemainingActionPoints,
        4);
    TestEqual (
        TEXT ("The next round restores Active"),
        EliasTurn.State,
        EGridCombatantTurnState::Active);

    Fixture.Party->PartyInventoryComponent->PartyInventoryState
        .ActiveCharacters[1].DerivedStats.CurrentHealth = 0;
    Fixture.TurnManager->RefreshPlayerCharacterVitalState (1);
    TestTrue (
        TEXT ("The defeated Mina state remains queryable"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (
            1,
            MinaTurn));
    TestEqual (
        TEXT ("Zero health produces Defeated"),
        MinaTurn.State,
        EGridCombatantTurnState::Defeated);
    TestEqual (
        TEXT ("A defeated character has no action points"),
        MinaTurn.RemainingActionPoints,
        0);
    TestFalse (
        TEXT ("A defeated character cannot act"),
        Fixture.TurnManager->CanCharacterAct (1));
    return true;
}

#endif
