#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/HorizontalBox.h"
#include "Components/InputComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
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
#include "UI/GridInventoryDragDropOperation.h"

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
            Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded ();

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

            Hud = CreateWidget<UGridCombatHudWidget> (
                TestWorld.World,
                UGridCombatHudWidget::StaticClass ());
            if (Hud)
            {
                Hud->InitializeCombatHud (Party, TurnManager);
            }
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
    TArray<FGridCombatHotbarBinding> Bindings;
    Bindings.SetNum (FGridCombatHotbarBinding::SlotCount);
    for (int32 SlotIndex = 0;
        SlotIndex < Bindings.Num ();
        ++SlotIndex)
    {
        Bindings[SlotIndex].Reset (SlotIndex);
    }
    for (int32 Index = 0; Index < 3; ++Index)
    {
        FGridAvailableCombatAction& Action = Catalog.AddDefaulted_GetRef ();
        Action.Definition.ActionId = *FString::Printf (
            TEXT ("Action_%d"), Index);
        Action.Definition.SourcePolicy =
            EGridCombatActionSourcePolicy::Universal;
        Action.CurrentActionPointCost = Index + 1;
        Action.bEnabled = Index != 2;
        Action.DisabledReason = Action.bEnabled
            ? FText::GetEmpty ()
            : FText::FromString (TEXT ("PA insuffisants"));
        Bindings[Index].ActionId = Action.Definition.ActionId;
        Bindings[Index].SourcePolicy =
            EGridCombatActionSourcePolicy::Universal;
    }
    TArray<FGridCombatHudActionView> Actions;
    FGridCombatHudViewModelBuilder::BuildHotbarActions (
        Bindings,
        Catalog,
        Actions);
    TestEqual (TEXT ("The view always exposes ten hotbar slots"),
        Actions.Num (), 10);
    TestEqual (TEXT ("The first slot displays key 1"),
        Actions[0].ShortcutText.ToString (), FString (TEXT ("1")));
    TestEqual (TEXT ("The last slot displays key 0"),
        Actions[9].ShortcutText.ToString (), FString (TEXT ("0")));
    TestTrue (TEXT ("An assigned action is resolved from the catalog"),
        Actions[0].bResolved);
    TestFalse (TEXT ("An unassigned slot stays empty"),
        Actions[9].bHasBinding);
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

    TArray<FGridInitiativePreviewEntry> Upcoming;
    for (int32 Index = 0; Index < 10; ++Index)
    {
        FGridInitiativePreviewEntry& Preview =
            Upcoming.AddDefaulted_GetRef ();
        Preview.Combatant = MakeInitiativeEntry (
            FGuid (12, 7, 10, Index + 1),
            Index,
            Index % 2 == 0
                ? EGridCombatantSide::Party
                : EGridCombatantSide::Monster,
            Index == 0
                ? EGridCombatantTurnState::Active
                : EGridCombatantTurnState::Waiting,
            TEXT ("Participant"));
        Preview.RoundNumber = Index < 5 ? 1 : 2;
        Preview.ActivationIndex = Index % 5;
        Preview.bIsActive = Index == 0;
        Preview.bStartsNewRound = Index == 5;
        Preview.Combatant.CurrentHealth = Index == 0 ? 7 : 20;
        Preview.Combatant.MaximumHealth = 20;
    }
    TArray<FGridCombatHudInitiativeView> Initiative;
    FGridCombatHudViewModelBuilder::BuildInitiative (
        Upcoming,
        Initiative);
    TestEqual (TEXT ("At most eight initiative entries are visible"),
        Initiative.Num (), 8);
    TestTrue (TEXT ("The first active combatant is emphasized"),
        Initiative[0].bActive);
    TestEqual (TEXT ("The runtime initiative order is preserved"),
        Initiative[3].Combatant.CombatantId,
        Upcoming[3].Combatant.CombatantId);
    TestTrue (TEXT ("The projected round boundary is preserved"),
        Initiative[5].bStartsNewRound);
    TestEqual (TEXT ("The projected round number is preserved"),
        Initiative[5].RoundNumber, 2);
    TestEqual (TEXT ("Initiative health percent reflects current health"),
        Initiative[0].HealthPercent, 0.35f);
    TestEqual (TEXT ("Initiative health percent clamps overhealing"),
        FGridCombatHudViewModelBuilder::CalculateHealthPercent (25, 20),
        1.0f);
    TestEqual (TEXT ("Initiative health percent rejects invalid maximum"),
        FGridCombatHudViewModelBuilder::CalculateHealthPercent (7, 0),
        0.0f);
    TestEqual (TEXT ("Initiative health percent clamps negative health"),
        FGridCombatHudViewModelBuilder::CalculateHealthPercent (-2, 20),
        0.0f);

    UGridCombatHudInitiativeSlotWidget* InitiativeSlot =
        NewObject<UGridCombatHudInitiativeSlotWidget> ();
    InitiativeSlot->ProgressBar_Health = NewObject<UProgressBar> (
        InitiativeSlot);
    InitiativeSlot->Text_State = NewObject<UTextBlock> (
        InitiativeSlot);
    InitiativeSlot->InitializeInitiativeSlot (Initiative[0]);
    TestEqual (TEXT ("The initiative health bar receives the view percent"),
        InitiativeSlot->ProgressBar_Health->GetPercent (),
        0.35f);
    TestTrue (TEXT ("The default health bar fill is visibly red"),
        InitiativeSlot->HealthBarFillColor.R >
            InitiativeSlot->HealthBarFillColor.G &&
        InitiativeSlot->HealthBarFillColor.R >
            InitiativeSlot->HealthBarFillColor.B);
    TestEqual (TEXT ("The active initiative scale stays compact"),
        InitiativeSlot->ActiveScale,
        1.12f);
    TestEqual (TEXT ("Only the active state remains visible"),
        InitiativeSlot->Text_State->GetText ().ToString (),
        FString (TEXT ("ACTIF")));
    InitiativeSlot->InitializeInitiativeSlot (Initiative[1]);
    TestEqual (TEXT ("Waiting state labels are hidden"),
        InitiativeSlot->Text_State->GetVisibility (),
        ESlateVisibility::Collapsed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1271InitiativePreviewTest,
    "Grimrock.Monsters.MON12.CombatHUD.SlidingInitiative",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1271InitiativePreviewTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.7.1 preview fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    TArray<FGridInitiativePreviewEntry> Preview;
    Fixture.TurnManager->GetInitiativePreview (Preview, 8);
    TestEqual (TEXT ("The sliding timeline always predicts eight activations"),
        Preview.Num (), 8);
    TestEqual (TEXT ("The active combatant remains the first activation"),
        Preview[0].Combatant.CombatantId,
        Fixture.CharacterIds[0]);
    TestTrue (TEXT ("Only the first activation is active"),
        Preview[0].bIsActive);
    TestEqual (TEXT ("Five current-round activations are retained"),
        Preview[4].RoundNumber, 1);
    TestTrue (TEXT ("A separator precedes the first round-two activation"),
        Preview[5].bStartsNewRound);
    TestEqual (TEXT ("The separator announces round two"),
        Preview[5].RoundNumber, 2);

    Fixture.Monster->CurrentHealth = 375;
    Fixture.TurnManager->GetInitiativePreview (Preview, 8);
    for (const FGridInitiativePreviewEntry& Entry : Preview)
    {
        if (Entry.Combatant.CombatantId ==
            Fixture.Monster->ResolvePersistenceId ())
        {
            TestEqual (TEXT ("Every rat activation reads current runtime health"),
                Entry.Combatant.CurrentHealth,
                375);
            TestEqual (TEXT ("Every rat activation reads maximum definition health"),
                Entry.Combatant.MaximumHealth,
                1000);
        }
    }

    Fixture.TurnManager->InitiativeOrder.Last ().State =
        EGridCombatantTurnState::Defeated;
    Fixture.TurnManager->InitiativeOrder.Last ().CurrentHealth = 0;
    Fixture.TurnManager->GetInitiativePreview (Preview, 8);
    TestEqual (TEXT ("A defeated combatant does not reduce slot coverage"),
        Preview.Num (), 8);
    TestTrue (TEXT ("Round two now starts after four living activations"),
        Preview[4].bStartsNewRound);
    TestEqual (TEXT ("The updated separator still announces round two"),
        Preview[4].RoundNumber, 2);
    for (const FGridInitiativePreviewEntry& Entry : Preview)
    {
        TestNotEqual (TEXT ("The defeated monster is absent from every projected round"),
            Entry.Combatant.CombatantId,
            Fixture.Monster->ResolvePersistenceId ());
    }

    Fixture.TurnManager->GetInitiativePreview (Preview, 7);
    TestEqual (TEXT ("Seven configured slots are supported"),
        Preview.Num (), 7);
    Fixture.TurnManager->GetInitiativePreview (Preview, 10);
    TestEqual (TEXT ("Ten configured slots are supported"),
        Preview.Num (), 10);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1271DynamicInitiativeTest,
    "Grimrock.Monsters.MON12.CombatHUD.DynamicInitiative",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1271DynamicInitiativeTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.7.1 dynamic fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.TurnManager->InitiativeOrder[0].InitiativeTotal = 40;
    Fixture.TurnManager->InitiativeOrder[1].InitiativeTotal = 30;
    Fixture.TurnManager->InitiativeOrder[2].InitiativeTotal = 20;
    Fixture.TurnManager->InitiativeOrder[3].InitiativeTotal = 10;
    Fixture.TurnManager->InitiativeOrder[4].InitiativeTotal = 15;

    const FGuid ActiveId =
        Fixture.TurnManager->InitiativeOrder[0].CombatantId;
    const FGuid HastedId =
        Fixture.TurnManager->InitiativeOrder[3].CombatantId;
    TestTrue (TEXT ("A haste modifier is accepted for a known combatant"),
        Fixture.TurnManager->SetCombatantInitiativeModifier (
            EGridCombatantSide::Party,
            HastedId,
            100));
    TestEqual (TEXT ("The already active combatant never moves retroactively"),
        Fixture.TurnManager->InitiativeOrder[0].CombatantId,
        ActiveId);
    TestEqual (TEXT ("The hasted combatant becomes the next future activation"),
        Fixture.TurnManager->InitiativeOrder[1].CombatantId,
        HastedId);

    TArray<FGridInitiativePreviewEntry> Preview;
    Fixture.TurnManager->GetInitiativePreview (Preview, 8);
    TestEqual (TEXT ("The active activation stays first in the prediction"),
        Preview[0].Combatant.CombatantId,
        ActiveId);
    TestEqual (TEXT ("The current-round future order updates immediately"),
        Preview[1].Combatant.CombatantId,
        HastedId);
    TestEqual (TEXT ("The next round uses the complete modified order"),
        Preview[5].Combatant.CombatantId,
        HastedId);

    Fixture.TurnManager->ResetInitiativeRound ();
    TestEqual (TEXT ("A new round starts with the modified initiative leader"),
        Fixture.TurnManager->InitiativeOrder[0].CombatantId,
        HastedId);
    TestFalse (TEXT ("An unknown combatant cannot change initiative"),
        Fixture.TurnManager->SetCombatantInitiativeModifier (
            EGridCombatantSide::Party,
            FGuid (99, 99, 99, 99),
            10));
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
    TestEqual (TEXT ("The HUD exposes ten fixed hotbar slots"),
        Fixture.Hud->View.Actions.Num (), 10);
    TestFalse (TEXT ("A new hotbar starts empty"),
        Fixture.Hud->View.Actions[0].bHasBinding);

    UWrapBox* LegacyWrapPanel =
        NewObject<UWrapBox> (Fixture.Hud, TEXT ("Panel_Actions_Test"));
    Fixture.Hud->Panel_Actions = LegacyWrapPanel;
    Fixture.Hud->ActionWidgetClass =
        UGridCombatHudActionWidget::StaticClass ();
    Fixture.Hud->RefreshFromSources ();
    TestNotNull (TEXT ("A horizontal hotbar row is created"),
        Fixture.Hud->HotbarRow.Get ());
    TestEqual (TEXT ("The legacy wrap panel owns one row only"),
        LegacyWrapPanel->GetChildrenCount (), 1);
    if (Fixture.Hud->HotbarRow)
    {
        TestEqual (TEXT ("The HUD owns ten runtime shortcut widgets"),
            Fixture.Hud->HotbarActionWidgets.Num (), 10);
        TestEqual (TEXT ("All ten shortcuts share the same row"),
            Fixture.Hud->HotbarRow->GetChildrenCount (), 10);
        if (Fixture.Hud->HotbarActionWidgets.IsValidIndex (0))
        {
            TestEqual (TEXT ("An empty shortcut frame remains visible"),
                Fixture.Hud->HotbarActionWidgets[0]->GetRenderOpacity (),
                0.8f);
        }
        for (const UGridCombatHudActionWidget* ActionWidget :
            Fixture.Hud->HotbarActionWidgets)
        {
            TestTrue (TEXT ("Each shortcut belongs to the horizontal row"),
                IsValid (ActionWidget) &&
                    ActionWidget->GetParent () ==
                        Fixture.Hud->HotbarRow);
        }
    }

    FGridItemInstance EquippedSword;
    TestTrue (TEXT ("The fixture exposes the equipped sword"),
        Fixture.Party->PartyInventoryComponent->GetEquippedItem (
            0,
            EGridEquipmentSlot::MainHand,
            EquippedSword));
    UGridInventoryDragDropOperation* SwordDrag =
        NewObject<UGridInventoryDragDropOperation> (Fixture.Hud);
    SwordDrag->InitializeFromSlot (
        EGridInventoryUiSlotType::MainHand,
        0,
        EquippedSword);
    TestTrue (TEXT ("Dragging the sword can configure slot zero"),
        Fixture.Hud->HandleHotbarDrop (0, SwordDrag));

    const FGridCombatHudActionView* SwordAction =
        Fixture.Hud->View.Actions.FindByPredicate (
            [] (const FGridCombatHudActionView& Candidate)
            {
                return Candidate.bHasBinding &&
                    Candidate.Action.SourceDefinitionId ==
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
            Fixture.Hud->RequestHotbarSlot (0, RejectedResult));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1283HotbarClickExecutionTest,
    "Grimrock.Monsters.MON12.8.3.HotbarClickExecution",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1283HotbarClickExecutionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.8.3 click fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    FGridItemInstance EquippedSword;
    if (!TestTrue (TEXT ("The click fixture exposes its sword"),
        Fixture.Party->PartyInventoryComponent->GetEquippedItem (
            0,
            EGridEquipmentSlot::MainHand,
            EquippedSword)))
    {
        return false;
    }
    TestTrue (TEXT ("The sword is assigned to keyboard slot 1"),
        Fixture.Party->PartyInventoryComponent
            ->SetCharacterCombatHotbarBindingFromItem (
                0,
                0,
                EquippedSword,
                EGridEquipmentSlot::MainHand));

    UWrapBox* LegacyWrapPanel =
        NewObject<UWrapBox> (Fixture.Hud, TEXT ("Panel_Actions_1283_Click"));
    Fixture.Hud->Panel_Actions = LegacyWrapPanel;
    Fixture.Hud->ActionWidgetClass =
        UGridCombatHudActionWidget::StaticClass ();
    Fixture.Hud->RefreshFromSources ();
    if (!TestTrue (TEXT ("The first clickable slot exists"),
        Fixture.Hud->HotbarActionWidgets.IsValidIndex (0)))
    {
        return false;
    }

    TestTrue (TEXT ("A short click executes the configured attack"),
        Fixture.Hud->HotbarActionWidgets[0]->TryExecuteAction ());
    TestEqual (TEXT ("The click pays exactly two action points"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints,
        2);

    FGridCombatActionRequestResult EmptyResult;
    TestFalse (TEXT ("An empty shortcut cannot execute"),
        Fixture.Hud->RequestHotbarSlot (9, EmptyResult));
    TestEqual (TEXT ("The empty shortcut consumes no action points"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints,
        2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1283UnarmedHotbarExecutionTest,
    "Grimrock.Monsters.MON12.8.3.UnarmedHotbarExecution",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1283UnarmedHotbarExecutionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.8.3 unarmed fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    Fixture.Party->PartyInventoryComponent->PartyInventoryState
        .ActiveEquipment[0].MainHand = FGridItemInstance ();
    FGridCombatHotbarBinding UnarmedBinding;
    UnarmedBinding.Reset (0);
    UnarmedBinding.ActionId = TEXT ("Attack_Unarmed");
    UnarmedBinding.SourcePolicy =
        EGridCombatActionSourcePolicy::Universal;
    TestTrue (TEXT ("The unarmed action can be configured explicitly"),
        Fixture.Party->PartyInventoryComponent
            ->SetCharacterCombatHotbarBinding (
                0,
                0,
                UnarmedBinding));

    FGridCombatActionRequestResult Result;
    TestTrue (TEXT ("The configured unarmed shortcut executes"),
        Fixture.Hud->RequestHotbarSlot (0, Result));
    TestTrue (TEXT ("The generic result accepts the unarmed attack"),
        Result.bAccepted);
    TestEqual (TEXT ("The unarmed shortcut pays two action points"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints,
        2);
    TestEqual (TEXT ("The unarmed shortcut uses no equipment slot"),
        Result.Action.SourceEquipmentSlot,
        EGridEquipmentSlot::None);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1283HotbarKeyboardGuardTest,
    "Grimrock.Monsters.MON12.8.3.HotbarKeyboardAndModalGuard",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1283HotbarKeyboardGuardTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.8.3 keyboard fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    UInputComponent* InputComponent = NewObject<UInputComponent> (
        Fixture.Party,
        TEXT ("MON12_8_3_Input"));
    Fixture.Party->SetupPlayerInputComponent (InputComponent);
    const FKey ExpectedKeys[] = {
        EKeys::One,
        EKeys::Two,
        EKeys::Three,
        EKeys::Four,
        EKeys::Five,
        EKeys::Six,
        EKeys::Seven,
        EKeys::Eight,
        EKeys::Nine,
        EKeys::Zero
    };
    for (const FKey& ExpectedKey : ExpectedKeys)
    {
        const FInputKeyBinding* Binding =
            InputComponent->KeyBindings.FindByPredicate (
                [&ExpectedKey] (const FInputKeyBinding& Candidate)
                {
                    return Candidate.Chord.Key == ExpectedKey &&
                        Candidate.KeyEvent == IE_Pressed;
                });
        TestTrue (
            *FString::Printf (
                TEXT ("Key %s is bound to the combat hotbar"),
                *ExpectedKey.ToString ()),
            Binding && Binding->bConsumeInput &&
                !Binding->bExecuteWhenPaused);
    }

    FGridItemInstance EquippedSword;
    if (!TestTrue (TEXT ("The keyboard fixture exposes its sword"),
        Fixture.Party->PartyInventoryComponent->GetEquippedItem (
            0,
            EGridEquipmentSlot::MainHand,
            EquippedSword)))
    {
        return false;
    }
    TestTrue (TEXT ("The sword is assigned before keyboard execution"),
        Fixture.Party->PartyInventoryComponent
            ->SetCharacterCombatHotbarBindingFromItem (
                0,
                0,
                EquippedSword,
                EGridEquipmentSlot::MainHand));
    Fixture.Party->CombatHudWidgetInstance = Fixture.Hud;

    Fixture.Party->bInventoryWidgetVisible = true;
    TestFalse (TEXT ("The inventory intercepts the numeric shortcut"),
        Fixture.Party->TryExecuteCombatHotbarSlot (0));
    Fixture.Party->bInventoryWidgetVisible = false;
    Fixture.Party->bCharacterCreationModalActive = true;
    TestFalse (TEXT ("A modal screen intercepts the numeric shortcut"),
        Fixture.Party->TryExecuteCombatHotbarSlot (0));
    Fixture.Party->bCharacterCreationModalActive = false;

    FGridPlayerCharacterTurnState TurnState;
    TestTrue (TEXT ("The guarded character state remains readable"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (0, TurnState));
    TestEqual (TEXT ("Blocked shortcuts consume no action points"),
        TurnState.RemainingActionPoints,
        4);
    TestTrue (TEXT ("The numeric shortcut executes after the modal closes"),
        Fixture.Party->TryExecuteCombatHotbarSlot (0));
    TestTrue (TEXT ("The post-keyboard character state remains readable"),
        Fixture.TurnManager->GetPlayerCharacterTurnState (0, TurnState));
    TestEqual (TEXT ("The accepted numeric shortcut pays two action points"),
        TurnState.RemainingActionPoints,
        2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1284QuickItemEffectTest,
    "Grimrock.Monsters.MON12.8.4.QuickItemEffectAndPersistence",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1284QuickItemEffectTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.8.4 effect fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    UGridPartyInventoryComponent* Inventory =
        Fixture.Party->PartyInventoryComponent;
    FGridCharacterInventoryState& Character =
        Inventory->PartyInventoryState.ActiveCharacters[0];
    Character.DerivedStats.CurrentHealth = 5;
    Character.DerivedStats.CurrentMana = 4;

    UGridItemDefinitionAsset* PotionDefinition =
        NewObject<UGridItemDefinitionAsset> (Fixture.Party);
    PotionDefinition->ItemDefinitionId = TEXT ("Potion_MON1284_Health");
    PotionDefinition->DisplayName =
        FText::FromString (TEXT ("Potion de soins MON12.8.4"));
    PotionDefinition->ItemType = EGridItemType::Potion;
    PotionDefinition->bStackable = true;
    PotionDefinition->MaxStackSize = 10;
    PotionDefinition->bProvidesQuickItemCombatAction = true;
    PotionDefinition->QuickItemCombatAction.ActionType =
        EGridCombatActionType::Ability;
    PotionDefinition->QuickItemCombatAction.TargetingPolicy =
        EGridCombatTargetingPolicy::Self;
    PotionDefinition->QuickItemCombatAction.ResolutionProfile =
        EGridCombatActionResolutionProfile::Effect;
    PotionDefinition->QuickItemCombatAction.ActionPointCost = 1;
    PotionDefinition->QuickItemCombatAction.EffectProfile.RestoreHealth = 7;
    PotionDefinition->QuickItemCombatAction.EffectProfile.RestoreMana = 3;
    if (!TestTrue (TEXT ("The combat potion definition is registered"),
        Inventory->RegisterItemDefinition (PotionDefinition)))
    {
        return false;
    }

    FGridItemInstance Potion;
    Potion.RuntimeObjectId = FGuid (12, 8, 4, 1);
    Potion.ItemDefinitionId = PotionDefinition->ItemDefinitionId;
    Potion.DisplayName = PotionDefinition->DisplayName;
    Potion.Quantity = 2;
    TestTrue (TEXT ("Two potion units enter the inventory"),
        Inventory->AddItemToCharacterInventory (0, Potion));
    TestTrue (TEXT ("The potion configures shortcut one"),
        Inventory->SetCharacterCombatHotbarBindingFromItem (
            0,
            0,
            Potion,
            EGridEquipmentSlot::None));

    Fixture.Hud->RefreshFromSources ();
    TestTrue (TEXT ("The configured potion resolves from the catalogue"),
        Fixture.Hud->View.Actions[0].bResolved);
    TestTrue (TEXT ("The potion is initially usable"),
        Fixture.Hud->View.Actions[0].Action.bEnabled);
    TestEqual (TEXT ("The catalogue aggregates both potion units"),
        Fixture.Hud->View.Actions[0].Action.CurrentSourceItemQuantity,
        2);

    FGridCombatActionRequestResult FirstUse;
    TestTrue (TEXT ("The first potion use is accepted"),
        Fixture.Hud->RequestHotbarSlot (0, FirstUse));
    TestEqual (TEXT ("The potion restores seven health"),
        Character.DerivedStats.CurrentHealth,
        12);
    TestEqual (TEXT ("The same potion restores three mana"),
        Character.DerivedStats.CurrentMana,
        7);
    TestEqual (TEXT ("The potion spends one action point"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints,
        3);
    TestEqual (TEXT ("Exactly one potion remains"),
        Inventory->CountItemDefinitionInCharacterInventory (
            0,
            PotionDefinition->ItemDefinitionId),
        1);
    TestEqual (TEXT ("The result records the consumed unit"),
        FirstUse.QuickItemResult.SourceQuantityAfter,
        1);

    Character.DerivedStats.CurrentHealth = 20;
    Character.DerivedStats.CurrentMana = 8;
    Fixture.Hud->RefreshFromSources ();
    TestFalse (TEXT ("A full-health character cannot waste the potion"),
        Fixture.Hud->View.Actions[0].Action.bEnabled);
    TestEqual (TEXT ("The disabled reason identifies a useless effect"),
        Fixture.Hud->View.Actions[0].Action.AvailabilityReason,
        EGridCombatActionAvailabilityReason::NoApplicableEffect);
    FGridCombatActionRequestResult RefusedUse;
    TestFalse (TEXT ("The useless potion request is rejected"),
        Fixture.Hud->RequestHotbarSlot (0, RefusedUse));
    TestEqual (TEXT ("A refused potion consumes no unit"),
        Inventory->CountItemDefinitionInCharacterInventory (
            0,
            PotionDefinition->ItemDefinitionId),
        1);
    TestEqual (TEXT ("A refused potion consumes no action point"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints,
        3);

    Character.DerivedStats.CurrentHealth = 10;
    FGridCombatActionRequestResult LastUse;
    TestTrue (TEXT ("The last potion unit can be consumed"),
        Fixture.Hud->RequestHotbarSlot (0, LastUse));
    TestEqual (TEXT ("The inventory quantity reaches zero"),
        Inventory->CountItemDefinitionInCharacterInventory (
            0,
            PotionDefinition->ItemDefinitionId),
        0);
    FGridCombatHotbarBinding PersistentBinding;
    TestTrue (TEXT ("The shortcut still exists at quantity zero"),
        Inventory->GetCharacterCombatHotbarBinding (
            0,
            0,
            PersistentBinding));
    TestFalse (TEXT ("The zero-quantity shortcut is not cleared"),
        PersistentBinding.IsEmpty ());
    TestTrue (TEXT ("The zero-quantity shortcut remains resolved"),
        Fixture.Hud->View.Actions[0].bResolved);
    TestEqual (TEXT ("The catalogue reports the missing source"),
        Fixture.Hud->View.Actions[0].Action.AvailabilityReason,
        EGridCombatActionAvailabilityReason::InsufficientSourceItems);

    FGridItemInstance ReplacementPotion = Potion;
    ReplacementPotion.RuntimeObjectId = FGuid (12, 8, 4, 2);
    ReplacementPotion.Quantity = 3;
    TestTrue (TEXT ("A replacement stack can be added"),
        Inventory->AddItemToCharacterInventory (
            0,
            ReplacementPotion));
    Fixture.Hud->RefreshFromSources ();
    TestTrue (TEXT ("The same shortcut reactivates for the new stack"),
        Fixture.Hud->View.Actions[0].Action.bEnabled);
    TestEqual (TEXT ("The replacement quantity is projected"),
        Fixture.Hud->View.Actions[0].Action.CurrentSourceItemQuantity,
        3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1284QuickItemScrollAttackTest,
    "Grimrock.Monsters.MON12.8.4.ScrollAttackConsumption",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1284QuickItemScrollAttackTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    FGridCombatHudFixture Fixture;
    if (!TestTrue (TEXT ("The MON12.8.4 scroll fixture is ready"),
        Fixture.IsReady ()))
    {
        return false;
    }

    UGridPartyInventoryComponent* Inventory =
        Fixture.Party->PartyInventoryComponent;
    UGridItemDefinitionAsset* ScrollDefinition =
        NewObject<UGridItemDefinitionAsset> (Fixture.Party);
    ScrollDefinition->ItemDefinitionId = TEXT ("Scroll_MON1284_Fire");
    ScrollDefinition->DisplayName =
        FText::FromString (TEXT ("Parchemin de feu MON12.8.4"));
    ScrollDefinition->ItemType = EGridItemType::Scroll;
    ScrollDefinition->bStackable = true;
    ScrollDefinition->MaxStackSize = 10;
    ScrollDefinition->bProvidesQuickItemCombatAction = true;
    FGridCombatActionDefinition& ScrollAction =
        ScrollDefinition->QuickItemCombatAction;
    ScrollAction.ActionType = EGridCombatActionType::RangedAttack;
    ScrollAction.TargetingPolicy =
        EGridCombatTargetingPolicy::FirstAxialTarget;
    ScrollAction.ResolutionProfile =
        EGridCombatActionResolutionProfile::Attack;
    ScrollAction.ActionPointCost = 2;
    ScrollAction.RangeCells = 2;
    ScrollAction.OffensiveProfile.AttackId =
        TEXT ("Attack_MON1284_FireScroll");
    ScrollAction.OffensiveProfile.AttackDefinition.DamageType =
        EGridDamageType::Fire;
    ScrollAction.OffensiveProfile.AttackDefinition.MinDamage = 2;
    ScrollAction.OffensiveProfile.AttackDefinition.MaxDamage = 2;
    ScrollAction.OffensiveProfile.AttackDefinition.AccuracyBonus = 100;
    ScrollAction.OffensiveProfile.DamageScalingAttribute =
        EGridAttackScalingAttribute::None;
    ScrollAction.OffensiveProfile.RangeCells = 2;
    if (!TestTrue (TEXT ("The combat scroll definition is registered"),
        Inventory->RegisterItemDefinition (ScrollDefinition)))
    {
        return false;
    }

    FGridItemInstance Scroll;
    Scroll.RuntimeObjectId = FGuid (12, 8, 4, 3);
    Scroll.ItemDefinitionId = ScrollDefinition->ItemDefinitionId;
    Scroll.DisplayName = ScrollDefinition->DisplayName;
    Scroll.Quantity = 2;
    TestTrue (TEXT ("Two scroll units enter the inventory"),
        Inventory->AddItemToCharacterInventory (0, Scroll));
    TestTrue (TEXT ("The scroll configures shortcut two"),
        Inventory->SetCharacterCombatHotbarBindingFromItem (
            0,
            1,
            Scroll,
            EGridEquipmentSlot::None));

    Fixture.Party->Facing = EGridEdge::South;
    Fixture.Hud->RefreshFromSources ();
    FGridCombatActionRequestResult RejectedScroll;
    TestFalse (TEXT ("A scroll attack without a target is rejected"),
        Fixture.Hud->RequestHotbarSlot (1, RejectedScroll));
    TestEqual (TEXT ("A rejected scroll consumes no unit"),
        Inventory->CountItemDefinitionInCharacterInventory (
            0,
            ScrollDefinition->ItemDefinitionId),
        2);
    TestEqual (TEXT ("A rejected scroll consumes no action point"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints,
        4);

    Fixture.Party->Facing = EGridEdge::North;
    FGridCombatActionRequestResult AcceptedScroll;
    TestTrue (TEXT ("The axial scroll attack is accepted"),
        Fixture.Hud->RequestHotbarSlot (1, AcceptedScroll));
    TestTrue (TEXT ("The generic result records acceptance"),
        AcceptedScroll.bAccepted);
    TestTrue (TEXT ("The quick-item attack request is structurally valid"),
        AcceptedScroll.AttackRequest.IsValid ());
    TestEqual (TEXT ("The scroll source is recorded on the attack"),
        AcceptedScroll.AttackRequest.OffensiveItemDefinitionId,
        ScrollDefinition->ItemDefinitionId);
    TestEqual (TEXT ("A scroll never claims an equipment slot"),
        AcceptedScroll.AttackRequest.OffensiveEquipmentSlot,
        EGridEquipmentSlot::None);
    TestEqual (TEXT ("Exactly one accepted scroll is consumed"),
        Inventory->CountItemDefinitionInCharacterInventory (
            0,
            ScrollDefinition->ItemDefinitionId),
        1);
    TestEqual (TEXT ("The scroll result records the remaining unit"),
        AcceptedScroll.QuickItemResult.SourceQuantityAfter,
        1);
    TestEqual (TEXT ("The accepted scroll spends two action points"),
        Fixture.Hud->View.PartyMembers[0].RemainingActionPoints,
        2);
    return true;
}

#endif
