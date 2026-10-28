#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridThrownItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "UObject/UObjectGlobals.h"

// Real weapon assets in a transient UWorld, not a substitute for player-driven PIE.
namespace ARMLOT02132
{
    struct FFixture
    {
        UWorld* World = nullptr;
        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARMLOT02132_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
        }
        ~FFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };

    AGridItemActor* Pickup(UWorld* World, const FGuid& Guid)
    {
        for (TActorIterator<AGridItemActor> It(World); It; ++It)
            if (!It->IsActorBeingDestroyed() && !It->IsA<AGridThrownItemActor>() &&
                It->GetRuntimeObjectId() == Guid) return *It;
        return nullptr;
    }

    int32 BagIndex(const UGridPartyInventoryComponent* Inv, const FGuid& Guid)
    {
        const auto& Characters = Inv->PartyInventoryState.ActiveCharacters;
        if (Characters.IsEmpty()) return INDEX_NONE;
        const auto& Bag = Characters[0].InventorySlots;
        for (int32 I = 0; I < Bag.Num(); ++I)
            if (!Bag[I].IsEmpty() && Bag[I].Item.RuntimeObjectId == Guid) return I;
        return INDEX_NONE;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMLot02132RealWorld,
    "Grimrock.RPG.ARMEMENT.LOT02.RealWorldPickupAndReturn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMLot02132RealWorld::RunTest(const FString&)
{
    using namespace ARMLOT02132;
    FFixture Fixture;
    if (!TestNotNull(TEXT("Transient game world"), Fixture.World)) return false;
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Transient level runtime actor"), Runtime)) return false;
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
        Cell.SouthWall = EGridWallType::None;
        Cell.EastWall = EGridWallType::None;
        Cell.WestWall = EGridWallType::None;
    }
    Runtime->LevelAsset = Level;
    AGrimrockPartyPawn* Party = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Test party pawn"), Party) ||
        !TestNotNull(TEXT("Party inventory component"), Party->PartyInventoryComponent)) return false;
    Party->LevelRuntimeActor = Runtime;
    Party->CurrentCellX = 1; Party->CurrentCellY = 1;
    Party->Facing = EGridEdge::North;
    Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
    UGridPartyInventoryComponent* Inv = Party->PartyInventoryComponent;
    FGridCharacterInventoryState Hero;
    Hero.CharacterId = FGuid::NewGuid();
    Hero.DisplayName = FText::FromString(TEXT("LOT02.13.2"));
    Hero.InventorySlots.SetNum(32);
    Inv->PartyInventoryState.ActiveCharacters = { Hero };
    Inv->PartyInventoryState.ActiveEquipment.SetNum(1);
    Inv->PartyInventoryState.SelectedCharacterIndex = 0;

    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Manifest parsed"), FGridWeaponPipelineManifest::Load(
        FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
    { AddError(Error); return false; }

    const TCHAR* Ids[] = {
        TEXT("ARM038"),TEXT("ARM048"),TEXT("ARM054"),TEXT("ARM057"),
        TEXT("ARM058"),TEXT("ARM061"),TEXT("ARM062"),TEXT("ARM067"),
        TEXT("ARM069"),TEXT("ARM070"),TEXT("ARM081")
    };
    TArray<FString> Report = {
        TEXT("ArchetypeId,Mesh,Picked,Equipped,Extracted,Redropped,Repicked,Projectile,Status")
    };
    int32 Ready = 0, ThrownSpawned = 0;
    for (const TCHAR* RawId : Ids)
    {
        const FString Id(RawId);
        bool bOkay = true;
        auto Check = [this, &Id, &bOkay](const TCHAR* Name, bool Pass)
        {
            bOkay &= TestTrue(*FString::Printf(TEXT("%s %s"), *Id, Name), Pass);
        };
        const FGridWeaponPipelineRow* Row = FGridWeaponPipelineManifest::Find(Manifest, Id);
        if (!TestNotNull(*FString::Printf(TEXT("%s row"), *Id), Row)) continue;
        UGridItemDefinitionAsset* Definition = LoadObject<UGridItemDefinitionAsset>(nullptr, *Row->ItemPath);
        if (!TestNotNull(*FString::Printf(TEXT("%s package"), *Id), Definition)) continue;
        Check(TEXT("remains Planned"), Row->ProductionState == TEXT("Planned"));
        Check(TEXT("real definition registered"), Inv->RegisterItemDefinition(Definition));
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Definition->ItemDefinitionId;
        Item.ItemLevel = 1;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::World;
        const bool bDrop = Runtime->TryDropItemInstanceAtCell(
            Item, Definition, 1, 2, EGridEdge::None, FVector::ZeroVector);
        Check(TEXT("world item spawned"), bDrop);
        AGridItemActor* Actor = bDrop ? Pickup(Fixture.World, Item.RuntimeObjectId) : nullptr;
        Check(TEXT("original GUID has world actor"), Actor != nullptr);
        const bool bMesh = Actor && Actor->MeshComponent &&
            Actor->MeshComponent->GetStaticMesh() == Definition->WorldMesh.LoadSynchronous();
        Check(TEXT("actual static mesh in world"), bMesh);
        const bool bPicked = Actor && Runtime->CanPartyPickupItemActor(Actor, Party) &&
            Runtime->TryPickupItemActor(Actor, Party);
        Check(TEXT("picked from the world"), bPicked);
        Check(TEXT("original no longer in the world"),
            Pickup(Fixture.World, Item.RuntimeObjectId) == nullptr);
        int32 Index = BagIndex(Inv, Item.RuntimeObjectId);
        Check(TEXT("original GUID in bag"), Index != INDEX_NONE);
        const EGridEquipmentSlot Slot = Row->IsPassive()
            ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand;
        const bool bEquipped = Index != INDEX_NONE &&
            Inv->EquipItemFromInventorySlot(0, Index, Slot);
        Check(TEXT("equipped via real inventory"), bEquipped);
        FGridItemInstance Held;
        Check(TEXT("equipped GUID and N1 preserved"),
            bEquipped && Inv->GetEquippedItem(0, Slot, Held) &&
            Held.RuntimeObjectId == Item.RuntimeObjectId && Held.ItemLevel == 1);

        FGridItemInstance Extracted;
        const bool bExtracted = bEquipped &&
            Inv->TryExtractOneEquippedItemForWorldTransfer(
                0, Slot, Definition->ItemDefinitionId, Extracted);
        Check(TEXT("real equipment transfer to world"), bExtracted);
        Check(TEXT("physical N1 identity retained"),
            bExtracted && Extracted.RuntimeObjectId == Item.RuntimeObjectId &&
            Extracted.ItemLevel == 1 && Extracted.Quantity == 1);
        Check(TEXT("main/off hands cleared"),
            bExtracted &&
            !Inv->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::MainHand) &&
            !Inv->IsEquipmentSlotOccupied(0, EGridEquipmentSlot::OffHand));
        const bool bRedropped = bExtracted && Runtime->TryDropItemInstanceAtCell(
            Extracted, Definition, 1, 2, EGridEdge::None, FVector::ZeroVector);
        Check(TEXT("extracted item reappeared in the world"), bRedropped);
        AGridItemActor* Returned = bRedropped ? Pickup(Fixture.World, Item.RuntimeObjectId) : nullptr;
        const bool bRepicked = Returned && Runtime->TryPickupItemActor(Returned, Party);
        Check(TEXT("returned item picked again"), bRepicked);
        Check(TEXT("single item remains out of world"),
            Pickup(Fixture.World, Item.RuntimeObjectId) == nullptr);
        Index = BagIndex(Inv, Item.RuntimeObjectId);
        Check(TEXT("returned GUID in bag exactly once"), Index != INDEX_NONE);

        FString Projectile = TEXT("N_A");
        if (Row->IsThrowable())
        {
            FGridItemInstance ProjectileItem = Item;
            ProjectileItem.RuntimeObjectId = FGuid::NewGuid();
            AGridThrownItemActor* Thrown = Runtime->SpawnThrownItemProjectile(
                ProjectileItem, Definition, Runtime->GetCellCenterWorld(1, 1, 120.f),
                FVector(750.f, 0.f, 100.f), 1, 1);
            const bool bValid = Thrown &&
                Thrown->ThrownItemInstance.RuntimeObjectId == ProjectileItem.RuntimeObjectId &&
                Thrown->SourceCellX == 1 && Thrown->SourceCellY == 1;
            Check(TEXT("actual thrown actor spawned"), bValid);
            Projectile = bValid ? TEXT("SPAWNED_NO_IMPACT") : TEXT("FAILED");
            if (bValid) ++ThrownSpawned;
            if (Thrown) Thrown->Destroy();
        }

        FGridItemInstance Removed;
        Check(TEXT("test item removed from bag after roundtrip"),
            Index != INDEX_NONE &&
            Inv->RemoveItemFromCharacterInventoryByRuntimeId(
                0, Item.RuntimeObjectId, Removed) &&
            Removed.RuntimeObjectId == Item.RuntimeObjectId);
        if (bOkay) ++Ready;
        Report.Add(FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%s,%s"),
            *Id, bMesh ? TEXT("true") : TEXT("false"),
            bPicked ? TEXT("true") : TEXT("false"),
            bEquipped ? TEXT("true") : TEXT("false"),
            bExtracted ? TEXT("true") : TEXT("false"),
            bRedropped ? TEXT("true") : TEXT("false"),
            bRepicked ? TEXT("true") : TEXT("false"), *Projectile,
            bOkay ? TEXT("AUTOMATED_WORLD_PASS_PIE_PENDING") : TEXT("FAILED")));
    }
    TestEqual(TEXT("Eleven world-inventory-world cycles"), Ready, 11);
    TestEqual(TEXT("Three authentic projectile actors spawned"), ThrownSpawned, 3);
    const FString File = FPaths::Combine(FPaths::ProjectSavedDir(),
        TEXT("ARMEMENT-LOT02/LOT02_13_2_WorldAcceptance.csv"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
    TestTrue(TEXT("Save CSV report under Saved"), FFileHelper::SaveStringArrayToFile(
        Report, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    AddInfo(FString::Printf(TEXT("[LOT02.13.2] World=%d/11 ProjectileSpawn=%d/3 PIE=NOT_RUN"),
        Ready, ThrownSpawned));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMLot02132Ammo,
    "Grimrock.RPG.ARMEMENT.LOT02.RecurveBowRealArrows",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMLot02132Ammo::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Recurve = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RecurveBow_01.DA_Weapon_RecurveBow_01"));
    UGridItemDefinitionAsset* ShortBow = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.DA_Weapon_ShortBow_01"));
    UGridItemDefinitionAsset* Arrows = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01"));
    if (!TestNotNull(TEXT("Actual ARM048"), Recurve) ||
        !TestNotNull(TEXT("Actual ARM045"), ShortBow) ||
        !TestNotNull(TEXT("Actual Item_Arrows_01"), Arrows)) return false;
    const bool RDirty = Recurve->GetOutermost()->IsDirty();
    const bool SDirty = ShortBow->GetOutermost()->IsDirty();
    const bool ADirty = Arrows->GetOutermost()->IsDirty();
    const FName ArrowId(TEXT("Item_Arrows_01"));
    TestEqual(TEXT("ARM048 linked to real arrows"), Recurve->RequiredAmmunitionItemDefinitionId, ArrowId);
    TestEqual(TEXT("ARM045 unchanged"), ShortBow->RequiredAmmunitionItemDefinitionId, ArrowId);
    TestTrue(TEXT("Arrow stack and bow valid"), Arrows->bStackable &&
        Arrows->IsValidDefinition() && Recurve->IsValidDefinition());
    UGridPartyInventoryComponent* Inv = NewObject<UGridPartyInventoryComponent>();
    Inv->InitializeDefaultPartyIfNeeded();
    if (!TestTrue(TEXT("Register real arrows"), Inv->RegisterItemDefinition(Arrows))) return false;
    TestFalse(TEXT("Cannot shoot with zero arrows"),
        FGridRangedAmmunitionPolicy::HasRequiredAmmunition(*Inv, 0, *Recurve));
    TestFalse(TEXT("Cannot consume nonexistent arrow"),
        FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunition(*Inv, 0, *Recurve));
    FGridItemInstance Stack;
    Stack.RuntimeObjectId = FGuid::NewGuid();
    Stack.ItemDefinitionId = ArrowId;
    Stack.Quantity = 2;
    if (!TestTrue(TEXT("Two arrows added"), Inv->AddItemToCharacterInventory(0, Stack))) return false;
    TestTrue(TEXT("Can shoot with two arrows"),
        FGridRangedAmmunitionPolicy::HasRequiredAmmunition(*Inv, 0, *Recurve));
    TestTrue(TEXT("Consume arrow one"),
        FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunition(*Inv, 0, *Recurve));
    TestEqual(TEXT("One arrow remains"), Inv->CountItemDefinitionInCharacterInventory(0, ArrowId), 1);
    TestTrue(TEXT("Consume arrow two"),
        FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunition(*Inv, 0, *Recurve));
    TestEqual(TEXT("Zero arrows remain"), Inv->CountItemDefinitionInCharacterInventory(0, ArrowId), 0);
    TestFalse(TEXT("Third shot disallowed"),
        FGridRangedAmmunitionPolicy::HasRequiredAmmunition(*Inv, 0, *Recurve));
    TestEqual(TEXT("Recurve package read only"), Recurve->GetOutermost()->IsDirty(), RDirty);
    TestEqual(TEXT("Short bow package read only"), ShortBow->GetOutermost()->IsDirty(), SDirty);
    TestEqual(TEXT("Arrows package read only"), Arrows->GetOutermost()->IsDirty(), ADirty);
    AddInfo(TEXT("[LOT02.13.2] ARM048 ammo policy: 0 denied; 2->1->0; 0 denied. PIE=NOT_RUN"));
    return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
