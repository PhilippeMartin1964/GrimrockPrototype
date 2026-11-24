#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridWorldSnapshotReadModel.h"

namespace ARMCLEAN064X18
{
struct FFixture
{
    UWorld* World = nullptr;
    UGameInstance* GI = nullptr;
    UGridItemSubsystem* Registry = nullptr;
    AGridLevelRuntimeActor* Runtime = nullptr;
    AGrimrockPartyPawn* Party = nullptr;
    UGridPartyInventoryComponent* Inventory = nullptr;
    FFixture()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false)
            .SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::Game, false,
            FName(*FString::Printf(TEXT("ARMCLEAN064X18_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World || !GEngine) return;
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
        Context.SetCurrentWorld(World);
        GI = NewObject<UGameInstance>(GetTransientPackage());
        GI->Init();
        World->SetGameInstance(GI);
        Registry = GI->GetSubsystem<UGridItemSubsystem>();
        Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
        if (!Runtime) return;
        UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
        Level->Width = 4;
        Level->Height = 4;
        Level->CellSize = 200.f;
        Level->EnsureCellCount();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
            Cell.NorthWall = EGridWallType::None;
            Cell.EastWall = EGridWallType::None;
            Cell.SouthWall = EGridWallType::None;
            Cell.WestWall = EGridWallType::None;
        }
        Runtime->LevelAsset = Level;
        Party = World->SpawnActor<AGrimrockPartyPawn>();
        if (!Party || !Party->PartyInventoryComponent) return;
        Party->LevelRuntimeActor = Runtime;
        Party->CurrentCellX = 1;
        Party->CurrentCellY = 1;
        Party->Facing = EGridEdge::North;
        Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
        Inventory = Party->PartyInventoryComponent;
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid();
        Character.InventorySlots.SetNum(20);
        Inventory->PartyInventoryState.ActiveCharacters = {Character};
        Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
        Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
    }
    ~FFixture()
    {
        if (World) World->SetGameInstance(nullptr);
        if (GI) GI->Shutdown();
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
    AGridItemActor* Find(FGuid Id) const
    {
        if (!World) return nullptr;
        for (TActorIterator<AGridItemActor> It(World); It; ++It)
        {
            AGridItemActor* Actor = *It;
            if (IsValid(Actor) && !Actor->IsActorBeingDestroyed() &&
                Actor->GetRuntimeObjectId() == Id) return Actor;
        }
        return nullptr;
    }
    static FGridItemInstance Make(FName Id, bool bOn)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Id;
        Item.ItemLevel = 0;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::World;
        Item.OwnerCharacterIndex = INDEX_NONE;
        Item.bLightsEnabled = bOn;
        return Item;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X18TorchFloorLightState,
    "Grimrock.RPG.ARM.CLEAN06_4X18.TorchFloorDropAndPickupLightState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X18TorchFloorLightState::RunTest(const FString&)
{
    ARMCLEAN064X18::FFixture F;
    const FName TorchId(TEXT("Item_Torch"));
    if (!TestNotNull(TEXT("World runtime"), F.Runtime) ||
        !TestNotNull(TEXT("Inventory"), F.Inventory) ||
        !TestNotNull(TEXT("Canonical registry"), F.Registry) ||
        !TestTrue(TEXT("Torch exists in canonical registry"),
            F.Registry && F.Registry->ContainsItem(TorchId)))
        return false;

    FGridResolvedItem Definition;
    if (!TestTrue(TEXT("Torch canonical N0 resolves"),
        F.Registry->ResolveItem(TorchId, 0, Definition)) ||
        !TestFalse(TEXT("Torch remains non-ARM N0"), Definition.bIsArmament) ||
        !TestTrue(TEXT("Torch uses point light"), Definition.LightEmitter.bUsePointLight) ||
        !TestTrue(TEXT("Torch has authored flame Niagara"),
            !Definition.VisualEffects.Effects.IsEmpty()))
        return false;

    const FGridItemInstance Lit = ARMCLEAN064X18::FFixture::Make(TorchId, true);
    if (!TestTrue(TEXT("Lit torch can be dropped on dungeon floor"),
        F.Runtime->TryDropItemInstanceAtCell(Lit, 1, 1, EGridEdge::None, FVector::ZeroVector)))
        return false;
    AGridItemActor* Actor = F.Find(Lit.RuntimeObjectId);
    if (!TestNotNull(TEXT("Ground torch spawned"), Actor) ||
        !TestTrue(TEXT("Ground torch did not go out"), Actor->AreItemLightsEnabled()) ||
        !TestTrue(TEXT("Ground actor has canonical light data"),
            Actor->LightEmitterComponent &&
            Actor->LightEmitterComponent->RuntimeConfig.bUsePointLight &&
            Actor->LightEmitterComponent->RuntimeConfig.LightIntensity > 0.f))
        return false;

    if (!TestTrue(TEXT("World capture for torch succeeds"),
        F.Runtime->CaptureCurrentLevelRuntimeState()))
        return false;
    const FGridLevelRuntimeState* SavedLevel = F.Runtime->FindRuntimeStateForCurrentLevel();
    const FGridRuntimeItemState* Saved = SavedLevel
        ? SavedLevel->Items.Find(Lit.RuntimeObjectId) : nullptr;
    if (!TestNotNull(TEXT("World runtime state has torch GUID"), Saved) ||
        !TestTrue(TEXT("Saved world item stays lit"), Saved->bLightsEnabled))
        return false;

    if (!TestTrue(TEXT("Ground torch can be picked up"),
        F.Runtime->TryPickupItemActor(Actor, F.Party)))
        return false;
    const int32 Slot = F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.IndexOfByPredicate(
        [&Lit](const FGridInventorySlot& Entry)
        {
            return !Entry.IsEmpty() && Entry.Item.RuntimeObjectId == Lit.RuntimeObjectId;
        });
    if (!TestTrue(TEXT("Same torch GUID arrives in bag"), Slot != INDEX_NONE) ||
        !TestTrue(TEXT("Bag preserves lit state"),
            F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[Slot].Item.bLightsEnabled) ||
        !TestTrue(TEXT("Equip retrieved torch to MainHand"),
            F.Inventory->EquipItemFromInventorySlot(0, Slot, EGridEquipmentSlot::MainHand)))
        return false;
    FGridItemInstance Equipped;
    if (!TestTrue(TEXT("Hand contains same torch"), F.Inventory->GetEquippedItem(
        0, EGridEquipmentSlot::MainHand, Equipped)) ||
        !TestTrue(TEXT("Hand remains lit with original GUID"),
            Equipped.bLightsEnabled && Equipped.RuntimeObjectId == Lit.RuntimeObjectId))
        return false;

    UGridPartyIlluminationComponent* Light =
        NewObject<UGridPartyIlluminationComponent>(F.Party, TEXT("ARMCLEAN064X18PartyLight"));
    if (!TestNotNull(TEXT("Party light added"), Light)) return false;
    Light->SetupAttachment(F.Party->GetRootComponent());
    F.Party->AddInstanceComponent(Light);
    Light->RegisterComponent();
    Light->RefreshFromEquipment(F.Inventory, F.Runtime);
    if (!TestTrue(TEXT("Torch lights party after ground pickup"),
        Light->HasActiveIlluminationSource()) ||
        !TestEqual(TEXT("Party light uses exact torch identity"),
            Light->ActiveSourceId, TorchId))
        return false;

    const FGridItemInstance Off = ARMCLEAN064X18::FFixture::Make(TorchId, false);
    if (!TestTrue(TEXT("An unlit torch can be dropped"),
        F.Runtime->TryDropItemInstanceAtCell(Off, 1, 1, EGridEdge::None, FVector(20.f, 0.f, 0.f))))
        return false;
    AGridItemActor* OffActor = F.Find(Off.RuntimeObjectId);
    if (!TestNotNull(TEXT("Unlit ground torch actor"), OffActor) ||
        !TestFalse(TEXT("Unlit torch stays unlit on floor"), OffActor->AreItemLightsEnabled()) ||
        !TestTrue(TEXT("Unlit actor still has canonical light profile"),
            OffActor->LightEmitterComponent &&
            OffActor->LightEmitterComponent->RuntimeConfig.bUsePointLight))
        return false;
    if (!TestTrue(TEXT("Unlit torch can be picked up"),
        F.Runtime->TryPickupItemActor(OffActor, F.Party)))
        return false;
    const int32 OffSlot = F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.IndexOfByPredicate(
        [&Off](const FGridInventorySlot& Entry)
        {
            return !Entry.IsEmpty() && Entry.Item.RuntimeObjectId == Off.RuntimeObjectId;
        });
    return TestTrue(TEXT("Unlit torch reaches bag"), OffSlot != INDEX_NONE) &&
        TestFalse(TEXT("Explicitly unlit state survives pickup"),
            F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[OffSlot].Item.bLightsEnabled);
}


namespace ARMCLEAN064X1610
{
    const FName TorchId(TEXT("Item_Torch"));
    const FGuid ReceptacleId(101, 102, 103, 104);

    FGridRuntimeItemState Torch(FGuid Guid, bool bLit)
    {
        FGridRuntimeItemState Item;
        Item.ObjectId = Guid;
        Item.ItemDefinitionId = TorchId;
        Item.ItemLevel = 0;
        Item.Quantity = 1;
        Item.CellX = 1;
        Item.CellY = 1;
        Item.Edge = EGridEdge::None;
        Item.Transform = FTransform(FVector(300.0, 300.0, 12.0));
        Item.bLightsEnabled = bLit;
        return Item;
    }

    bool Reload(FAutomationTestBase& Test, ARMCLEAN064X18::FFixture& F)
    {
        UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
        Save->DungeonRuntimeState = F.Runtime->DungeonRuntimeState;
        TArray<uint8> Bytes;
        if (!Test.TestTrue(TEXT("Save snapshot in memory"),
                UGameplayStatics::SaveGameToMemory(Save, Bytes)))
            return false;
        UGrimrockPartySaveGame* Loaded = Cast<UGrimrockPartySaveGame>(
            UGameplayStatics::LoadGameFromMemory(Bytes));
        if (!Test.TestNotNull(TEXT("Load snapshot from memory"), Loaded) ||
            !Test.TestTrue(TEXT("Current exact-match schema"), Loaded->IsCompatible()))
            return false;
        F.Runtime->DungeonRuntimeState = Loaded->DungeonRuntimeState;
        F.Runtime->RebuildLevel();
        return Test.TestTrue(TEXT("Apply saved state in rebuilt world"),
            F.Runtime->ApplyCurrentLevelRuntimeState());
    }

    bool CheckTorch(FAutomationTestBase& Test, AGridItemActor* Actor,
        FGuid Guid, bool bLit, int32 Effects)
    {
        if (!Test.TestNotNull(TEXT("Restored torch exists"), Actor)) return false;
        Test.TestEqual(TEXT("Same runtime GUID"), Actor->GetRuntimeObjectId(), Guid);
        Test.TestEqual(TEXT("Same canonical ItemId"), Actor->GetItemDefinitionId(), TorchId);
        Test.TestEqual(TEXT("Light state survives"), Actor->AreItemLightsEnabled(), bLit);
        Test.TestTrue(TEXT("Canonical point-light profile"), Actor->LightEmitterComponent &&
            Actor->LightEmitterComponent->RuntimeConfig.bUsePointLight &&
            Actor->LightEmitterComponent->RuntimeConfig.LightIntensity > 0.0f);
        Test.TestTrue(TEXT("Canonical Niagara effects"), Actor->VisualEffectsComponent &&
            Actor->VisualEffectsComponent->GetConfiguredEffectCount() == Effects && Effects > 0);
        return !Test.HasAnyErrors();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X1610WorldRestore,
    "Grimrock.RPG.ARM.CLEAN06_4X1610.WorldTorchSaveContinue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X1610WorldRestore::RunTest(const FString&)
{
    ARMCLEAN064X18::FFixture F;
    if (!TestTrue(TEXT("Runtime registry has canonical torch"),
            F.Runtime && F.Registry && F.Registry->ContainsItem(ARMCLEAN064X1610::TorchId)))
        return false;
    FGridResolvedItem Projected;
    if (!TestTrue(TEXT("Torch N0 resolves"), F.Registry->ResolveItem(
            ARMCLEAN064X1610::TorchId, 0, Projected)))
        return false;

    const FGuid Lit(11, 12, 13, 14), Unlit(21, 22, 23, 24);
    FGridLevelRuntimeState* State = F.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    State->bHasBeenVisited = true;
    State->Items.Add(Lit, ARMCLEAN064X1610::Torch(Lit, true));
    FGridRuntimeItemState UnlitState = ARMCLEAN064X1610::Torch(Unlit, false);
    UnlitState.CellX = 2;
    UnlitState.Transform = FTransform(FVector(500.0, 300.0, 12.0));
    State->Items.Add(Unlit, UnlitState);
    if (!ARMCLEAN064X1610::Reload(*this, F)) return false;

    AGridItemActor* LitActor = nullptr;
    AGridItemActor* UnlitActor = nullptr;
    for (TActorIterator<AGridItemActor> It(F.World); It; ++It)
    {
        AGridItemActor* Candidate = *It;
        if (IsValid(Candidate) && !Candidate->IsActorBeingDestroyed() &&
            Candidate->GetOwner() == F.Runtime)
        {
            if (Candidate->GetRuntimeObjectId() == Lit) LitActor = Candidate;
            if (Candidate->GetRuntimeObjectId() == Unlit) UnlitActor = Candidate;
        }
    }
    return ARMCLEAN064X1610::CheckTorch(*this, LitActor, Lit, true,
               Projected.VisualEffects.Effects.Num()) &&
           ARMCLEAN064X1610::CheckTorch(*this, UnlitActor, Unlit, false,
               Projected.VisualEffects.Effects.Num());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X1610ReceptacleRestore,
    "Grimrock.RPG.ARM.CLEAN06_4X1610.ReceptacleTorchSaveContinue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X1610ReceptacleRestore::RunTest(const FString&)
{
    ARMCLEAN064X18::FFixture F;
    if (!TestTrue(TEXT("Runtime registry has canonical torch"),
            F.Runtime && F.Registry && F.Registry->ContainsItem(ARMCLEAN064X1610::TorchId)))
        return false;
    FGridResolvedItem Projected;
    if (!TestTrue(TEXT("Torch N0 resolves"), F.Registry->ResolveItem(
            ARMCLEAN064X1610::TorchId, 0, Projected)))
        return false;

    const FName DefinitionId(TEXT("ARM064X1610_Receptacle"));
    FGridWorldObjectInstance Placed;
    Placed.InstanceId = ARMCLEAN064X1610::ReceptacleId;
    Placed.WorldObjectDefinitionId = DefinitionId;
    Placed.Type = EGridLevelObjectType::Receptacle;
    Placed.CellX = 2;
    Placed.CellY = 2;
    Placed.WallSide = EGridEdge::North;
    F.Runtime->LevelAsset->WorldObjectInstances.Add(Placed);
    UGridWorldObjectDefinitionAsset* Definition =
        NewObject<UGridWorldObjectDefinitionAsset>(F.Runtime);
    Definition->DefinitionId = DefinitionId;
    Definition->SupportedType = EGridLevelObjectType::Receptacle;
    Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
    Definition->bIsInteractable = true;
    Definition->RuntimeActorClass = AGridReceptacleActor::StaticClass();
    Definition->StaticPart.Mesh = NewObject<UStaticMesh>(F.Runtime);
    F.Runtime->WorldObjectDefinitions.Add(Definition);

    const FGuid Guid(31, 32, 33, 34);
    FGridRuntimeItemState Torch = ARMCLEAN064X1610::Torch(Guid, true);
    Torch.bIsContainedInReceptacle = true;
    Torch.ReceptacleObjectId = ARMCLEAN064X1610::ReceptacleId;
    FGridRuntimeReceptacleState Contained;
    Contained.ObjectId = ARMCLEAN064X1610::ReceptacleId;
    Contained.bCanInsertItems = false;
    Contained.bCanRemoveItem = true;
    Contained.ContainedItems.Add(Torch);
    FGridLevelRuntimeState* State = F.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    State->bHasBeenVisited = true;
    State->Receptacles.Add(ARMCLEAN064X1610::ReceptacleId, Contained);
    if (!ARMCLEAN064X1610::Reload(*this, F)) return false;

    AGridReceptacleActor* Receptacle = F.Runtime->FindRuntimeObjectActor<
        AGridReceptacleActor>(ARMCLEAN064X1610::ReceptacleId);
    if (!TestNotNull(TEXT("Rebuilt receptacle exists"), Receptacle) ||
        !TestEqual(TEXT("One item restored"), Receptacle->GetContainedItemCount(), 1))
        return false;
    TestFalse(TEXT("Insertion disabled after restore"), Receptacle->bCanInsertItems);
    TestTrue(TEXT("Removal permitted after restore"), Receptacle->bCanRemoveItem);
    return ARMCLEAN064X1610::CheckTorch(*this, Receptacle->GetContainedItemActor(0),
        Guid, true, Projected.VisualEffects.Effects.Num());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean5G34WorldPreflight,
    "Grimrock.RPG.ARM.CLEAN06_5G34.CanonicalWorldSnapshotPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean5G34WorldPreflight::RunTest(const FString&)
{
    ARMCLEAN064X18::FFixture F;
    if (!TestTrue(TEXT("Configured runtime registry"),
        F.Runtime && F.Registry && F.Registry->IsConfigured())) return false;
    const FGuid TorchGuid = FGuid::NewGuid();
    FGridRuntimeItemState Torch = ARMCLEAN064X1610::Torch(TorchGuid, true);
    FGridLevelRuntimeState* State = F.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    State->bHasBeenVisited = true;
    State->Items.Add(TorchGuid, Torch);
    if (!TestTrue(TEXT("Valid physical snapshot applies"), F.Runtime->ApplyCurrentLevelRuntimeState()))
        return false;
    AGridItemActor* Original = F.Find(TorchGuid);
    if (!TestNotNull(TEXT("Existing canonical torch"), Original)) return false;
    const FTransform Before = Original->GetActorTransform();
    const int32 BeforeCount = State->Items.Num();
    FString Error;
    TestTrue(TEXT("Complete valid snapshot preflights"),
        FGridWorldSnapshotReadModel::ValidateLevel(F.Registry, *State, Error));
    State->Items[TorchGuid].ItemLevel = 1;
    State->Items[TorchGuid].Transform.SetLocation(FVector(9999.f, 9999.f, 9999.f));
    TestFalse(TEXT("Invalid N1 torch rejects before touching actors"), F.Runtime->ApplyCurrentLevelRuntimeState());
    TestTrue(TEXT("Original actor remains live"), F.Find(TorchGuid) == Original && IsValid(Original));
    TestTrue(TEXT("Original transform unchanged"), Original->GetActorTransform().Equals(Before));
    State->Items[TorchGuid] = Torch;
    const FGuid OtherGuid = FGuid::NewGuid();
    FGridRuntimeItemState Unknown = Torch;
    Unknown.ObjectId = OtherGuid;
    Unknown.ItemDefinitionId = TEXT("ARM_CLEAN06_UnknownWorldItem");
    State->Items.Add(OtherGuid, Unknown);
    TestFalse(TEXT("Unknown item cannot create a placeholder"), F.Runtime->ApplyCurrentLevelRuntimeState());
    TestNull(TEXT("Unknown actor absent"), F.Find(OtherGuid));
    State->Items.Remove(OtherGuid);
    FGridPendingWorldItemState Pending;
    Pending.ItemState = Torch;
    State->PendingInboundItems.Add(TorchGuid, Pending);
    TestFalse(TEXT("Pending duplicate of a ground GUID is rejected"),
        FGridWorldSnapshotReadModel::ValidateLevel(F.Registry, *State, Error));
    State->PendingInboundItems.Reset();
    Pending.ItemState.ObjectId = OtherGuid;
    Pending.ItemState.Quantity = 0;
    State->PendingInboundItems.Add(OtherGuid, Pending);
    TestFalse(TEXT("Malformed pending physical item rejected"),
        FGridWorldSnapshotReadModel::ValidateDungeon(F.Registry, F.Runtime->DungeonRuntimeState, Error));
    State->PendingInboundItems.Reset();
    const FGuid Owner = FGuid::NewGuid();
    FGridRuntimeReceptacleState Contained;
    Contained.ObjectId = Owner;
    FGridRuntimeItemState ContainedTorch = Torch;
    ContainedTorch.ObjectId = OtherGuid;
    ContainedTorch.bIsContainedInReceptacle = true;
    ContainedTorch.ReceptacleObjectId = FGuid::NewGuid();
    Contained.ContainedItems.Add(ContainedTorch);
    State->Receptacles.Add(Owner, Contained);
    TestFalse(TEXT("Contained item must identify its actual owner"),
        FGridWorldSnapshotReadModel::ValidateLevel(F.Registry, *State, Error));
    State->Receptacles.Remove(Owner);
    TestEqual(TEXT("Original number of ground items"), State->Items.Num(), BeforeCount);
    TestTrue(TEXT("Corrected snapshot applies"), F.Runtime->ApplyCurrentLevelRuntimeState());
    TestTrue(TEXT("Corrected world retains original GUID"), F.Find(TorchGuid) == Original);
    return !HasAnyErrors();
}

#endif
