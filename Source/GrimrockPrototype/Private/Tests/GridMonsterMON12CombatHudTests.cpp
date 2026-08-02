#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "UI/GridCombatHudWidget.h"

namespace
{
    struct FGridCombatHudTestWorld
    {
        UWorld* World = nullptr;

        FGridCombatHudTestWorld ()
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
                    TEXT ("MON12CombatHud_%s"),
                    *FGuid::NewGuid ().ToString (
                        EGuidFormats::Digits))),
                nullptr,
                true,
                ERHIFeatureLevel::Num,
                &InitializationValues);
            if (World && GEngine)
            {
                FWorldContext& Context =
                    GEngine->CreateNewWorldContext (EWorldType::Game);
                Context.SetCurrentWorld (World);
            }
        }

        ~FGridCombatHudTestWorld ()
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

    FGridCharacterInventoryState MakeHudCharacter (
        const FGuid& CharacterId,
        const TCHAR* Name)
    {
        FGridCharacterInventoryState Character;
        Character.CharacterId = CharacterId;
        Character.DisplayName = FText::FromString (Name);
        Character.DerivedStats.CurrentHealth = 20;
        Character.DerivedStats.MaxHealth = 20;
        Character.DerivedStats.CurrentMana = 8;
        Character.DerivedStats.MaxMana = 8;
        Character.InventorySlots.SetNum (4);
        return Character;
    }

    FGridCombatantInitiativeEntry MakeInitiativeEntry (
        const FGuid& Id,
        int32 CharacterIndex,
        EGridCombatantSide Side,
        EGridCombatantTurnState State,
        const TCHAR* Name)
    {
        FGridCombatantInitiativeEntry Entry;
        Entry.CombatantId = Id;
        Entry.CharacterIndex = CharacterIndex;
        Entry.Side = Side;
        Entry.State = State;
        Entry.DisplayName = FText::FromString (Name);
        Entry.CurrentHealth = 20;
        Entry.MaximumHealth = 20;
        Entry.InitiativeTotal = 30 - FMath::Max (0, CharacterIndex);
        return Entry;
    }

    struct FGridCombatHudFixture
    {
        FGridCombatHudTestWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        AGridMonsterActor* Monster = nullptr;
        UGridTurnManagerComponent* TurnManager = nullptr;
        UGridCombatHudWidget* Hud = nullptr;
        FGuid CharacterIds[4] = {
            FGuid (12, 7, 1, 1),
            FGuid (12, 7, 1, 2),
            FGuid (12, 7, 1, 3),
            FGuid (12, 7, 1, 4)
        };

        FGridCombatHudFixture ()
        {
            if (!TestWorld.World)
            {
                return;
            }

            Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor> ();
            UGridLevelAsset* LevelAsset =
                NewObject<UGridLevelAsset> (Runtime);
            LevelAsset->Width = 3;
            LevelAsset->Height = 3;
            LevelAsset->EnsureCellCount ();
            for (FGridLevelCellData& Cell : LevelAsset->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            Runtime->LevelAsset = LevelAsset;
            Runtime->CurrentDungeonLevelId = TEXT ("MON12_7_Test");

            Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 1;
            Party->CurrentCellY = 1;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation (Runtime->GetCellCenterWorld (
                1,
                1,
                Party->EyeHeight));
            Party->SetActorRotation (FRotator (
                0.0f,
                GridDirectionUtils::ToYaw (Party->Facing),
                0.0f));

            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveCharacters = {
                    MakeHudCharacter (CharacterIds[0], TEXT ("Elias")),
                    MakeHudCharacter (CharacterIds[1], TEXT ("Mina")),
                    MakeHudCharacter (CharacterIds[2], TEXT ("Orin")),
                    MakeHudCharacter (CharacterIds[3], TEXT ("Sana"))
                };
            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveEquipment.SetNum (4);

            FGridItemInstance Weapon;
            Weapon.RuntimeObjectId = FGuid (12, 7, 2, 1);
            Weapon.ItemDefinitionId = TEXT ("MON12_7_Sword");
            Weapon.DisplayName = FText::FromString (TEXT ("Épée de test"));
            Weapon.Quantity = 1;
            Weapon.OwnerType = EGridItemOwnerType::EquipmentSlot;
            Weapon.OwnerCharacterIndex = 0;
            Weapon.EquipmentSlot = EGridEquipmentSlot::MainHand;
            Party->PartyInventoryComponent->PartyInventoryState
                .ActiveEquipment[0].MainHand = Weapon;

            UGridItemDefinitionAsset* WeaponDefinition =
                NewObject<UGridItemDefinitionAsset> (Party);
            WeaponDefinition->ItemDefinitionId = Weapon.ItemDefinitionId;
            WeaponDefinition->DisplayName = Weapon.DisplayName;
            WeaponDefinition->bProvidesAttack = true;
            WeaponDefinition->CompatibleEquipmentSlots.Add (
                EGridEquipmentSlot::MainHand);
            WeaponDefinition->OffensiveProfile.AttackId =
                TEXT ("Attack_MON12_7_Sword");
            WeaponDefinition->OffensiveProfile.AttackDefinition.DamageType =
                EGridDamageType::Physical;
            WeaponDefinition->OffensiveProfile.AttackDefinition
                .PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
            WeaponDefinition->OffensiveProfile.AttackDefinition.MinDamage = 1;
            WeaponDefinition->OffensiveProfile.AttackDefinition.MaxDamage = 2;
            WeaponDefinition->OffensiveProfile.RangeCells = 1;
            Party->PartyInventoryComponent->RegisterItemDefinition (
                WeaponDefinition);

            UGridMonsterDefinitionAsset* MonsterDefinition =
                NewObject<UGridMonsterDefinitionAsset> (Runtime);
            MonsterDefinition->MonsterId = TEXT ("MON12_7_Rat");
            MonsterDefinition->DisplayName =
                FText::FromString (TEXT ("Rat MON12.7"));
            MonsterDefinition->CategoryId = TEXT ("Vermin");
            MonsterDefinition->MaxHealth = 1000;
            MonsterDefinition->Evasion = 0;
            MonsterDefinition->ActionPointsPerTurn = 2;
            Monster = TestWorld.World->SpawnActor<AGridMonsterActor> ();
            UGridMonsterOccupancySubsystem* Occupancy =
                TestWorld.World
                    ->GetSubsystem<UGridMonsterOccupancySubsystem> ();
            if (Monster && Occupancy)
            {
                Monster->InitializeMonster (
                    MonsterDefinition,
                    FGuid (12, 7, 3, 1),
                    FIntPoint (1, 2),
                    EGridEdge::South);
                Occupancy->RegisterMonster (Monster, FIntPoint (1, 2));

                UGridMonsterMovementComponent* Movement =
                    NewObject<UGridMonsterMovementComponent> (
                        Monster,
                        TEXT ("MON12_7_Movement"));
                Movement->bAutoInitialize = false;
                Movement->bInferCellFromActorLocation = false;
                Monster->AddInstanceComponent (Movement);
                Movement->RegisterComponent ();

                UGridMonsterBehaviorComponent* Behavior =
                    NewObject<UGridMonsterBehaviorComponent> (
                        Monster,
                        TEXT ("MON12_7_Behavior"));
                Behavior->bAutoInitialize = false;
                Monster->AddInstanceComponent (Behavior);
                Behavior->RegisterComponent ();
            }

            TurnManager = NewObject<UGridTurnManagerComponent> (
                Runtime,
                TEXT ("MON12_7_TurnManager"));
            TurnManager->bAutoInitialize = false;
            Runtime->AddInstanceComponent (TurnManager);
            TurnManager->RegisterComponent ();
            TurnManager->InitializeTurnManager (Runtime, Party);
            TurnManager->bCombatActive = true;
            TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
            TurnManager->RoundNumber = 1;
            TurnManager->CombatMonsters = { Monster };
            TurnManager->PartyMobilityState.RoundNumber = 1;
            TurnManager->PartyMobilityState.MaximumMobilityActionPoints = 2;
            TurnManager->PartyMobilityState.RemainingMobilityActionPoints = 2;

            for (int32 Index = 0; Index < 4; ++Index)
            {
                FGridPlayerCharacterTurnState State;
                State.CharacterIndex = Index;
                State.CharacterId = CharacterIds[Index];
                State.State = Index == 0
                    ? EGridCombatantTurnState::Active
                    : EGridCombatantTurnState::Waiting;
                State.MaximumActionPoints = 4;
                State.RemainingActionPoints = Index == 0 ? 4 : 0;
                TurnManager->PlayerCharacterTurnStates.Add (State);
                TurnManager->InitiativeOrder.Add (MakeInitiativeEntry (
                    CharacterIds[Index],
                    Index,
                    EGridCombatantSide::Party,
                    State.State,
                    *Party->PartyInventoryComponent->PartyInventoryState
                        .ActiveCharacters[Index].DisplayName.ToString ()));
            }
            TurnManager->InitiativeOrder.Add (MakeInitiativeEntry (
                Monster->ResolvePersistenceId (),
                INDEX_NONE,
                EGridCombatantSide::Monster,
                EGridCombatantTurnState::Waiting,
                TEXT ("Rat MON12.7")));
            TurnManager->CurrentInitiativeIndex = 0;

            Hud = NewObject<UGridCombatHudWidget> (Party);
            Hud->InitializeCombatHud (Party, TurnManager);
        }

        bool IsReady () const
        {
            return TestWorld.World && Runtime && Party && Monster &&
                TurnManager && Hud;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON12CombatHudViewModelTest,
    "Grimrock.Monsters.MON12.CombatHUD.ViewModel",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatHudViewModelTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    TArray<FGridPlayerCharacterTurnState> TurnStates;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        FGridPlayerCharacterTurnState& State =
            TurnStates.AddDefaulted_GetRef ();
        State.CharacterIndex = Index;
        State.State = Index == 0
            ? EGridCombatantTurnState::Active
            : EGridCombatantTurnState::Waiting;
        State.MaximumActionPoints = 4;
        State.RemainingActionPoints = 4;
    }
    TArray<FGridCombatHudPartyMemberView> Members;
    FGridCombatHudViewModelBuilder::BuildPartyMembers (
        4,
        TurnStates,
        Members);
    TestEqual (TEXT ("Exactly four party panels are projected"),
        Members.Num (), 4);
    TestTrue (TEXT ("The active member is identified"),
        Members[0].bActive);

    TArray<FGridAvailableCombatAction> Catalog;
    for (int32 Index = 0; Index < 3; ++Index)
    {
        FGridAvailableCombatAction& Action = Catalog.AddDefaulted_GetRef ();
        Action.Definition.ActionId = *FString::Printf (
            TEXT ("Action_%d"), Index);
        Action.CurrentActionPointCost = Index + 1;
        Action.bEnabled = Index != 2;
        Action.DisabledReason = Action.bEnabled
            ? FText::GetEmpty ()
            : FText::FromString (TEXT ("PA insuffisants"));
    }
    TArray<FGridCombatHudActionView> Actions;
    FGridCombatHudViewModelBuilder::BuildActions (Catalog, Actions);
    TestEqual (TEXT ("Every catalog action creates one action view"),
        Actions.Num (), Catalog.Num ());
    TestEqual (TEXT ("A disabled action keeps its reason"),
        Actions[2].DisabledReason.ToString (),
        FString (TEXT ("PA insuffisants")));

    FGridPartyMobilityState MobilityState;
    MobilityState.MaximumMobilityActionPoints = 2;
    MobilityState.RemainingMobilityActionPoints = 1;
    const FGridCombatHudMobilityView Mobility =
        FGridCombatHudViewModelBuilder::BuildMobility (MobilityState);
    TestEqual (TEXT ("The shared PAM snapshot is copied"),
        Mobility.RemainingMobilityActionPoints, 1);

    TArray<FGridCombatantInitiativeEntry> Upcoming;
    for (int32 Index = 0; Index < 10; ++Index)
    {
        Upcoming.Add (MakeInitiativeEntry (
            FGuid (12, 7, 10, Index + 1),
            Index,
            Index % 2 == 0
                ? EGridCombatantSide::Party
                : EGridCombatantSide::Monster,
            Index == 0
                ? EGridCombatantTurnState::Active
                : EGridCombatantTurnState::Waiting,
            TEXT ("Participant")));
    }
    TArray<FGridCombatHudInitiativeView> Initiative;
    int32 OverflowCount = 0;
    FGridCombatHudViewModelBuilder::BuildInitiative (
        Upcoming,
        Initiative,
        OverflowCount);
    TestEqual (TEXT ("At most eight initiative entries are visible"),
        Initiative.Num (), 8);
    TestEqual (TEXT ("The initiative overflow is exact"),
        OverflowCount, 2);
    TestTrue (TEXT ("The first active combatant is emphasized"),
        Initiative[0].bActive);
    TestEqual (TEXT ("The runtime initiative order is preserved"),
        Initiative[3].Combatant.CombatantId,
        Upcoming[3].CombatantId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON12CombatHudLifecycleTest,
    "Grimrock.Monsters.MON12.CombatHUD.Lifecycle",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON12CombatHudLifecycleTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.7 fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    TestEqual (TEXT ("The live HUD exposes four party panels"),
        Fixture.Hud->View.PartyMembers.Num (), 4);
    TestEqual (TEXT ("The first runtime combatant is active"),
        Fixture.Hud->View.ActiveCharacterIndex, 0);
    TestEqual (TEXT ("The HUD reads the shared PAM authority"),
        Fixture.Hud->View.Mobility.RemainingMobilityActionPoints, 2);
    TestTrue (TEXT ("The MON12.6 catalog generated action buttons"),
        Fixture.Hud->View.Actions.Num () > 0);

    const FGridCombatHudActionView* SwordAction =
        Fixture.Hud->View.Actions.FindByPredicate (
            [] (const FGridCombatHudActionView& Candidate)
            {
                return Candidate.Action.SourceDefinitionId ==
                    FName (TEXT ("MON12_7_Sword"));
            });
    if (!TestNotNull (TEXT ("The equipped sword contributes an action"),
        SwordAction))
    {
        return false;
    }

    FGridCombatActionRequestResult AcceptedResult;
    TestTrue (
        TEXT ("HUD action routes through RequestCharacterCombatAction"),
        Fixture.Hud->RequestCombatAction (
            *SwordAction,
            AcceptedResult));
    TestTrue (TEXT ("The TurnManager accepted the generic request"),
        AcceptedResult.bAccepted);
    TestEqual (TEXT ("The accepted action spent the MON12.5/12.6 AP cost"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
    TestEqual (TEXT ("The attack cost remains two AP"),
        Fixture.TurnManager->PlayerAttackActionPointCost, 2);
    TestEqual (TEXT ("The translation cost remains one AP"),
        Fixture.TurnManager->PartyTranslationActionPointCost, 1);
    TestEqual (TEXT ("The shared translation cost remains one PAM"),
        Fixture.TurnManager->PartyTranslationMobilityActionPointCost, 1);

    Fixture.TurnManager->PlayerAttackActionPointCost = 3;
    Fixture.Hud->RefreshFromSources ();
    const FGridCombatHudActionView* DisabledSwordAction =
        Fixture.Hud->View.Actions.FindByPredicate (
            [] (const FGridCombatHudActionView& Candidate)
            {
                return Candidate.Action.SourceDefinitionId ==
                    FName (TEXT ("MON12_7_Sword"));
            });
    TestNotNull (TEXT ("The unavailable action remains visible"),
        DisabledSwordAction);
    if (DisabledSwordAction)
    {
        TestFalse (TEXT ("The unavailable action is disabled"),
            DisabledSwordAction->Action.bEnabled);
        TestFalse (TEXT ("The unavailable action exposes a reason"),
            DisabledSwordAction->DisabledReason.IsEmpty ());
        FGridCombatActionRequestResult RejectedResult;
        TestFalse (TEXT ("The authoritative retry is refused"),
            Fixture.Hud->RequestCombatAction (
                *DisabledSwordAction,
                RejectedResult));
        TestEqual (TEXT ("A refused action consumes no AP"),
            Fixture.Hud->View.PartyMembers[0].RemainingActionPoints, 2);
    }

    Fixture.TurnManager->PlayerCharacterTurnStates[0].State =
        EGridCombatantTurnState::Completed;
    Fixture.TurnManager->PlayerCharacterTurnStates[1].State =
        EGridCombatantTurnState::Active;
    Fixture.TurnManager->PlayerCharacterTurnStates[1]
        .RemainingActionPoints = 4;
    Fixture.TurnManager->InitiativeOrder[0].State =
        EGridCombatantTurnState::Completed;
    Fixture.TurnManager->InitiativeOrder[1].State =
        EGridCombatantTurnState::Active;
    Fixture.TurnManager->CurrentInitiativeIndex = 1;
    Fixture.TurnManager->OnActiveCombatantChanged.Broadcast (
        Fixture.TurnManager->InitiativeOrder[1]);
    TestEqual (TEXT ("The active-combatant event refreshes the HUD"),
        Fixture.Hud->View.ActiveCharacterIndex, 1);
    TestTrue (TEXT ("The second member is marked active"),
        Fixture.Hud->View.PartyMembers[1].bActive);

    Fixture.TurnManager->PendingPartyMotionType =
        EGridPendingPartyMotionType::Translation;
    Fixture.Hud->RefreshFromSources ();
    TestFalse (TEXT ("End turn is disabled during movement"),
        Fixture.Hud->View.bCanEndTurn);
    TestFalse (TEXT ("The TurnManager refuses end turn during movement"),
        Fixture.Hud->RequestEndTurn ());
    TestEqual (TEXT ("The refused end-turn keeps the active member AP"),
        Fixture.Hud->View.PartyMembers[1].RemainingActionPoints, 4);
    Fixture.TurnManager->PendingPartyMotionType =
        EGridPendingPartyMotionType::None;
    return true;
}

#endif
