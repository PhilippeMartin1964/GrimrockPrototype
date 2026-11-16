#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GridPartyIlluminationComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"

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
    UGridItemDefinitionAsset* Torch = LoadObject<UGridItemDefinitionAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch.DA_Item_Torch"));
    if (!TestNotNull(TEXT("Real torch asset"), Torch) ||
        !TestNotNull(TEXT("World runtime"), F.Runtime) ||
        !TestNotNull(TEXT("Inventory"), F.Inventory) ||
        !TestNotNull(TEXT("Canonical registry"), F.Registry) ||
        !TestTrue(TEXT("Torch exists in canonical registry"),
            F.Registry->HasCanonicalItem(Torch->ItemDefinitionId)) ||
        !TestTrue(TEXT("Legacy torch available for held presentation"),
            F.Inventory->RegisterItemDefinition(Torch)))
        return false;

    FGridResolvedItem Definition;
    if (!TestTrue(TEXT("Torch canonical N0 resolves"),
        F.Registry->ResolveItem(Torch->ItemDefinitionId, 0, Definition)) ||
        !TestTrue(TEXT("Torch uses point light"), Definition.LightEmitter.bUsePointLight) ||
        !TestTrue(TEXT("Torch has authored flame Niagara"),
            !Definition.VisualEffects.Effects.IsEmpty()))
        return false;

    const FGridItemInstance Lit = ARMCLEAN064X18::FFixture::Make(Torch->ItemDefinitionId, true);
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
            Light->ActiveSourceId, Torch->ItemDefinitionId))
        return false;

    const FGridItemInstance Off = ARMCLEAN064X18::FFixture::Make(Torch->ItemDefinitionId, false);
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
#endif
