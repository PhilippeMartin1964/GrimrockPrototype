#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridPlayerAttackPresentationComponent.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"

namespace
{
    struct FGridMON1141TestWorld
    {
        UWorld* World = nullptr;

        FGridMON1141TestWorld ()
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
                    TEXT ("MON1141ThrownWeaponWorld_%s"),
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

        ~FGridMON1141TestWorld ()
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

    FGridItemInstance MakeEquippedShuriken (
        const FGuid& CharacterId,
        int32 Quantity)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid ();
        Item.ItemDefinitionId = TEXT ("Shuriken");
        Item.DisplayName =
            FText::FromString (TEXT ("Shuriken"));
        Item.Quantity = Quantity;
        Item.Weight = 0.1f;
        Item.OwnerType =
            EGridItemOwnerType::EquipmentSlot;
        Item.OwnerGuid = CharacterId;
        Item.OwnerCharacterIndex = 0;
        Item.EquipmentSlot =
            EGridEquipmentSlot::MainHand;
        return Item;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1141ThrownWeaponLifecycleTest,
    "Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1141ThrownWeaponLifecycleTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    UGridItemDefinitionAsset* Definition =
        NewObject<UGridItemDefinitionAsset> ();
    Definition->ItemDefinitionId = TEXT ("Shuriken");
    Definition->DisplayName =
        FText::FromString (TEXT ("Shuriken"));
    Definition->ItemType = EGridItemType::Weapon;
    Definition->Weight = 0.1f;
    Definition->bStackable = true;
    Definition->MaxStackSize = 20;
    Definition->bThrowable = true;
    Definition->ThrowSpeed = 1800.0f;
    Definition->ThrowArc = 0.08f;
    Definition->ThrowLifeSeconds = 5.0f;
    Definition->ThrowImpactDropOffset = 12.0f;
    Definition->CompatibleEquipmentSlots.Add (
        EGridEquipmentSlot::MainHand);
    Definition->bProvidesAttackPresentation = true;
    Definition->PlayerAttackPresentationProfile.MotionStyle =
        EGridPlayerAttackMotionStyle::Throw;
    Definition->PlayerAttackPresentationProfile
        .bAnimateHeldItem = true;

    UStaticMesh* WorldMesh = NewObject<UStaticMesh> ();
    UStaticMesh* EquippedMesh = NewObject<UStaticMesh> ();
    Definition->WorldMesh = WorldMesh;
    Definition->EquippedMesh = EquippedMesh;
    TestEqual (
        TEXT ("EquippedMesh is preferred for the held visual"),
        Definition->LoadHeldMesh (),
        EquippedMesh);
    Definition->EquippedMesh.Reset ();
    TestEqual (
        TEXT ("WorldMesh is the held visual fallback"),
        Definition->LoadHeldMesh (),
        WorldMesh);
    Definition->EquippedMesh = EquippedMesh;
    TestTrue (
        TEXT ("A throwable Throw presentation is valid"),
        Definition->HasValidPlayerAttackPresentation ());
    Definition->bThrowable = false;
    TestFalse (
        TEXT ("A non-throwable Throw presentation is rejected"),
        Definition->HasValidPlayerAttackPresentation ());
    Definition->bThrowable = true;

    UGridPartyInventoryComponent* TransferInventory =
        NewObject<UGridPartyInventoryComponent> ();
    FGridCharacterInventoryState TransferCharacter;
    TransferCharacter.CharacterId = FGuid::NewGuid ();
    TransferInventory->PartyInventoryState.ActiveCharacters = {
        TransferCharacter
    };
    TransferInventory->PartyInventoryState.ActiveEquipment
        .SetNum (1);
    TransferInventory->PartyInventoryState.ActiveEquipment[0]
        .MainHand = MakeEquippedShuriken (
            TransferCharacter.CharacterId,
            2);
    const FGuid StackRuntimeId =
        TransferInventory->PartyInventoryState
            .ActiveEquipment[0].MainHand.RuntimeObjectId;

    FGridItemInstance ExtractedItem;
    TestTrue (
        TEXT ("One equipped unit can be extracted"),
        TransferInventory->
            TryExtractOneEquippedItemForWorldTransfer (
                0,
                EGridEquipmentSlot::MainHand,
                TEXT ("Shuriken"),
                ExtractedItem));
    TestEqual (
        TEXT ("The equipped stack is decremented"),
        TransferInventory->PartyInventoryState
            .ActiveEquipment[0].MainHand.Quantity,
        1);
    TestEqual (
        TEXT ("The extracted unit belongs to the world"),
        ExtractedItem.OwnerType,
        EGridItemOwnerType::World);
    TestNotEqual (
        TEXT ("A split unit has its own runtime identity"),
        ExtractedItem.RuntimeObjectId,
        StackRuntimeId);
    TestTrue (
        TEXT ("A failed spawn can restore the extracted unit"),
        TransferInventory->
            TryRestoreExtractedItemToEquipment (
                0,
                EGridEquipmentSlot::MainHand,
                ExtractedItem));
    TestEqual (
        TEXT ("Rollback restores the original stack quantity"),
        TransferInventory->PartyInventoryState
            .ActiveEquipment[0].MainHand.Quantity,
        2);

    FGridMON1141TestWorld TestWorld;
    TestNotNull (
        TEXT ("The transient world is created"),
        TestWorld.World);
    if (!TestWorld.World)
    {
        return false;
    }

    AGridLevelRuntimeActor* Runtime =
        TestWorld.World->SpawnActor<
            AGridLevelRuntimeActor> ();
    UGridLevelAsset* LevelAsset =
        NewObject<UGridLevelAsset> (Runtime);
    LevelAsset->Width = 4;
    LevelAsset->Height = 4;
    LevelAsset->EnsureCellCount ();
    for (FGridLevelCellData& Cell : LevelAsset->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    FGridLevelObjectData DefinitionLookup;
    DefinitionLookup.Type = EGridLevelObjectType::Item;
    DefinitionLookup.ItemDefinitionAsset = Definition;
    LevelAsset->Objects.Add (DefinitionLookup);
    Runtime->LevelAsset = LevelAsset;

    AGrimrockPartyPawn* Party =
        TestWorld.World->SpawnActor<AGrimrockPartyPawn> ();
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 1;
    Party->CurrentCellY = 1;
    Party->Facing = EGridEdge::North;
    Party->SetActorLocation (
        Runtime->GetCellCenterWorld (
            Party->CurrentCellX,
            Party->CurrentCellY,
            Party->EyeHeight));
    Party->PartyInventoryComponent->
        RegisterItemDefinition (Definition);

    FGridCharacterInventoryState Character;
    Character.CharacterId = FGuid::NewGuid ();
    Character.DisplayName =
        FText::FromString (TEXT ("Mina"));
    Party->PartyInventoryComponent->PartyInventoryState
        .ActiveCharacters = { Character };
    Party->PartyInventoryComponent->PartyInventoryState
        .ActiveEquipment.SetNum (1);
    Party->PartyInventoryComponent->PartyInventoryState
        .SelectedCharacterIndex = 0;
    Party->PartyInventoryComponent->PartyInventoryState
        .ActiveEquipment[0].MainHand =
            MakeEquippedShuriken (
                Character.CharacterId,
                2);

    TestTrue (
        TEXT ("The held shuriken actor is created"),
        Party->EquipHeldItem (TEXT ("Shuriken")));
    TestNotNull (
        TEXT ("The held shuriken uses a visible mesh"),
        Party->HeldItemActor
            ? Party->HeldItemActor->MeshComponent->
                GetStaticMesh ()
            : nullptr);

    AGridMonsterActor* TargetMonster =
        TestWorld.World->SpawnActor<AGridMonsterActor> ();
    TargetMonster->PersistentMonsterId =
        FGuid::NewGuid ();
    TargetMonster->CurrentCell = FIntPoint (1, 2);
    TargetMonster->SetActorLocation (
        Runtime->GetCellCenterWorld (1, 2, 0.0f));

    UGridTurnManagerComponent* TurnManager =
        NewObject<UGridTurnManagerComponent> (
            Runtime,
            TEXT ("MON1141TurnManager"));
    TurnManager->bAutoInitialize = false;
    Runtime->AddInstanceComponent (TurnManager);
    TurnManager->RegisterComponent ();
    TestTrue (
        TEXT ("The turn manager is initialized"),
        TurnManager->InitializeTurnManager (
            Runtime,
            Party));

    UGridPlayerAttackPresentationComponent* Presentation =
        Runtime->GetPlayerAttackPresentationComponent ();
    TestNotNull (
        TEXT ("The native presentation component exists"),
        Presentation);
    if (!Presentation)
    {
        return false;
    }
    Presentation->bNativeAudioPlaybackEnabled = false;
    Presentation->bNativeVFXSpawnEnabled = false;
    Presentation->bNativeFeedbackEnabled = false;
    Presentation->InitializePresentation (TurnManager);

    FGridPlayerAttackRequest Request;
    Request.RequestId = FGuid::NewGuid ();
    Request.RoundNumber = 1;
    Request.AttackerCharacterIndex = 0;
    Request.AttackerCharacterId = Character.CharacterId;
    Request.TargetMonsterId =
        TargetMonster->ResolvePersistenceId ();
    Request.PartyCell = FIntPoint (1, 1);
    Request.TargetCell = FIntPoint (1, 2);
    Request.PartyFacing = EGridEdge::North;
    Request.RangeCells = 3;
    Request.AttackId = TEXT ("Attack_Shuriken");
    Request.OffensiveItemDefinitionId =
        TEXT ("Shuriken");
    Request.OffensiveEquipmentSlot =
        EGridEquipmentSlot::MainHand;

    TurnManager->OnPlayerAttackRequested.Broadcast (
        Request);
    TestEqual (
        TEXT ("Exactly one thrown launch is requested"),
        Presentation->ThrownItemLaunchRequestCount,
        1);
    TestEqual (
        TEXT ("Exactly one thrown launch starts"),
        Presentation->ThrownItemLaunchStartedCount,
        1);
    TestTrue (
        TEXT ("The throw never starts boomerang held motion"),
        !Presentation->bHeldItemMotionStarted &&
            !Presentation->IsHeldItemMotionActive ());
    TestEqual (
        TEXT ("One equipped shuriken remains"),
        Party->PartyInventoryComponent->PartyInventoryState
            .ActiveEquipment[0].MainHand.Quantity,
        1);

    AGridThrownItemActor* ThrownItem = nullptr;
    for (TActorIterator<AGridThrownItemActor> It (
            TestWorld.World);
        It;
        ++It)
    {
        ThrownItem = *It;
        break;
    }
    TestNotNull (
        TEXT ("A recoverable thrown item actor exists"),
        ThrownItem);
    if (!ThrownItem)
    {
        return false;
    }
    TestEqual (
        TEXT ("The projectile carries exactly one shuriken"),
        ThrownItem->ThrownItemInstance.Quantity,
        1);

    FGridAttackResult HitResult;
    HitResult.bHit = true;
    HitResult.TargetHealthBefore = 5;
    HitResult.TargetHealthAfter = 4;
    HitResult.HealthDamage = 1;
    TurnManager->OnPlayerAttackResolved.Broadcast (
        Request,
        TargetMonster,
        HitResult);
    TestTrue (
        TEXT ("A hit configures target interception"),
        ThrownItem->bStopsAtCombatPresentationTarget);

    const FVector TargetLocation =
        ThrownItem->CombatPresentationTargetLocation;
    const FVector TravelDirection =
        (TargetLocation - ThrownItem->GetActorLocation ())
            .GetSafeNormal ();
    ThrownItem->SetActorLocation (
        TargetLocation + TravelDirection * 50.0f);
    ThrownItem->Tick (1.0f / 60.0f);
    TestTrue (
        TEXT ("Crossing the hit target converts the projectile"),
        ThrownItem->HasCompletedImpactConversion ());
    TestTrue (
        TEXT ("The shuriken becomes a world pickup in the target cell"),
        Runtime->GetWorldItemWeightAtCell (1, 2) > 0.0f);

    TurnManager->OnPlayerAttackRejected.Broadcast (
        0,
        EGridPlayerAttackRejectReason::PassageBlocked);
    TestEqual (
        TEXT ("A rejected attack does not consume the remaining shuriken"),
        Party->PartyInventoryComponent->PartyInventoryState
            .ActiveEquipment[0].MainHand.Quantity,
        1);
    TestEqual (
        TEXT ("A rejected attack creates no additional projectile"),
        Presentation->ThrownItemLaunchStartedCount,
        1);
    TestFalse (
        TEXT ("Rejected diagnostics report no new launch"),
        Presentation->bThrownItemLaunchStarted);

    AGridThrownItemActor* LastThrownItem =
        Party->TryLaunchEquippedItemForAttack (
            0,
            EGridEquipmentSlot::MainHand,
            TEXT ("Shuriken"),
            TargetLocation,
            FIntPoint (1, 1));
    TestNotNull (
        TEXT ("The final equipped shuriken can be launched"),
        LastThrownItem);
    TestFalse (
        TEXT ("The final shuriken clears MainHand"),
        Party->PartyInventoryComponent->
            IsEquipmentSlotOccupied (
                0,
                EGridEquipmentSlot::MainHand));
    TestNull (
        TEXT ("The held visual disappears with the final shuriken"),
        Party->HeldItemActor.Get ());
    if (LastThrownItem)
    {
        LastThrownItem->Destroy ();
    }

    return true;
}

#endif
