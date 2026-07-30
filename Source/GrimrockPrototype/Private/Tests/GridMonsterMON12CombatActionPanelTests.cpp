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

    struct FGridMON12Fixture
    {
        FGridMON12TestWorld TestWorld;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* LevelAsset = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
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

            Party->PartyInventoryComponent->RegisterItemDefinition (
                MakeItemDefinition (
                    Party,
                    TEXT ("Shuriken"),
                    TEXT ("Shuriken"),
                    true,
                    ShurikenIcon));
            Party->PartyInventoryComponent->RegisterItemDefinition (
                MakeItemDefinition (
                    Party,
                    TEXT ("Item_Torch"),
                    TEXT ("Torche"),
                    false,
                    TorchIcon));

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
        TEXT ("The initial turn state is Ready"),
        View.ActionState,
        EGridCombatActionPanelState::Ready);
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

    Fixture.TurnManager->PlayerAttackCommittedCharacterIds.Add (
        Fixture.MinaId);
    FGridPlayerAttackRequest Request;
    Request.AttackerCharacterIndex = 1;
    Fixture.TurnManager->OnPlayerAttackRequested.Broadcast (
        Request);

    TestEqual (
        TEXT ("The TurnManager commit produces AlreadyActed"),
        Fixture.Panel->View.ActionState,
        EGridCombatActionPanelState::AlreadyActed);
    TestFalse (
        TEXT ("AlreadyActed disables the panel"),
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

#endif
