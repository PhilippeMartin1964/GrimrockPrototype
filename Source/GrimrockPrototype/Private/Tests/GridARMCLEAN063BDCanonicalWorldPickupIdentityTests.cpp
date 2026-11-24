#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridWorldPickupIdentityReadModel.h"

namespace ARMCLEAN063BD
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;
        AGrimrockPartyPawn* Party = nullptr;

        explicit FFixture(bool bWithParty)
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063BD_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            if (!GI) return;
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
            Level->Width = 4;
            Level->Height = 4;
            Level->CellSize = 200.0f;
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
            if (!bWithParty) return;

            Party = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Party || !Party->PartyInventoryComponent) return;
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 1;
            Party->CurrentCellY = 1;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
            FGridCharacterInventoryState Character;
            Character.CharacterId = FGuid::NewGuid();
            Character.InventorySlots.SetNum(20);
            Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
            Party->PartyInventoryComponent->PartyInventoryState.ActiveEquipment.SetNum(1);
            Party->PartyInventoryComponent->PartyInventoryState.SelectedCharacterIndex = 0;
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

        bool Ready() const
        {
            return World && Registry && Runtime &&
                Registry->ContainsItem(TEXT("Item_Arrows_01")) &&
                Registry->ContainsItem(TEXT("Weapon_StraightSword_01")) &&
                Runtime->IsWalkableCell(1, 1);
        }

        FGridItemInstance MakeItem(FName Id, int32 Level) const
        {
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
            Item.OwnerType = EGridItemOwnerType::World;
            Item.OwnerCharacterIndex = INDEX_NONE;
            return Item;
        }

        AGridItemActor* FindPickup(FGuid Guid) const
        {
            if (!World) return nullptr;
            for (TActorIterator<AGridItemActor> It(World); It; ++It)
            {
                AGridItemActor* Actor = *It;
                if (IsValid(Actor) && !Actor->IsActorBeingDestroyed() &&
                    Actor->GetRuntimeObjectId() == Guid)
                    return Actor;
            }
            return nullptr;
        }
    };

    bool Run(FAutomationTestBase& Test, const TCHAR* Scenario)
    {
        const bool bPick = FCString::Strcmp(Scenario, TEXT("Pickup")) == 0;
        FFixture F(bPick);
        if (!Test.TestTrue(TEXT("Production canonical registry and runtime"), F.Ready()))
            return false;
        const FName ArrowId(TEXT("Item_Arrows_01"));
        const FGridItemInstance Arrow = F.MakeItem(ArrowId, 0);
        if (!Test.TestTrue(TEXT("Canonical N0 arrow dropped as actual world object"),
            F.Runtime->TryDropItemInstanceAtCell(Arrow, 1, 1, EGridEdge::None, FVector::ZeroVector)))
            return false;
        AGridItemActor* Actor = F.FindPickup(Arrow.RuntimeObjectId);
        if (!Test.TestNotNull(TEXT("Real world pickup spawned"), Actor))
            return false;

        Test.TestEqual(TEXT("Canonical getter retains the physical identity"),
            Actor->GetItemDefinitionId(), ArrowId);
        Test.TestEqual(TEXT("Shared pickup/save identity uses real ItemId"),
            FGridWorldPickupIdentityReadModel::Resolve(Actor, ArrowId, 0), ArrowId);
        Test.TestTrue(TEXT("Canonical presentation still applies to the physical item"),
            Actor->ApplyResolvedWorldVisual(Arrow));
        Test.TestTrue(TEXT("Canonical world mesh present"),
            Actor->MeshComponent && Actor->MeshComponent->GetStaticMesh() != nullptr);

        // Name separate assertion families in the Windows public test
        // report so a failure can be diagnosed without private machine logs.
        if (FCString::Strcmp(Scenario, TEXT("Presentation")) == 0)
        {
            FGridResolvedItem Resolved;
            if (!Test.TestTrue(TEXT("Actual canonical tags resolve"),
                F.Registry->ResolveItem(ArrowId, 0, Resolved))) return false;
            const FName FakeTag(TEXT("ARM5G65_ContradictoryTag"));
            Actor->ItemTags.Reset();
            Actor->ItemTags.Add(FakeTag);
            Test.TestFalse(TEXT("Copied actor tags cannot grant a capability"), Actor->HasItemTag(FakeTag));
            for (FName Tag : Resolved.Tags)
                Test.TestTrue(TEXT("Canonical tag survives contradictory actor cache"), Actor->HasItemTag(Tag));
            Actor->WorldItemLevel = 1;
            for (FName Tag : Resolved.Tags)
                Test.TestFalse(TEXT("Invalid non-ARM physical level grants no tag"), Actor->HasItemTag(Tag));
            Actor->WorldItemLevel = 0;
            return !Test.HasAnyErrors();
        }
        if (FCString::Strcmp(Scenario, TEXT("RegisteredLevelGuards")) == 0)
        {
            Test.TestTrue(TEXT("Actor and entry mismatch cannot silently switch item"),
                FGridWorldPickupIdentityReadModel::Resolve(
                    Actor, TEXT("Weapon_StraightSword_01"), 1).IsNone());
            Test.TestTrue(TEXT("Registered ammo at invalid N1 cannot borrow old DA_Item"),
                FGridWorldPickupIdentityReadModel::Resolve(Actor, ArrowId, 1).IsNone());
            AGridItemActor* Sword = F.World->SpawnActor<AGridItemActor>();
            if (!Test.TestNotNull(TEXT("Transient registered weapon actor"), Sword)) return false;
            Sword->InitializeFromItemDefinitionId(TEXT("Weapon_StraightSword_01"), FGuid::NewGuid());
            Test.TestTrue(TEXT("Known ARM N0 fails closed"),
                FGridWorldPickupIdentityReadModel::Resolve(
                    Sword, Sword->ItemDefinitionId, 0).IsNone());
            Test.TestEqual(TEXT("Known ARM N20 preserved"),
                FGridWorldPickupIdentityReadModel::Resolve(
                    Sword, Sword->ItemDefinitionId, 20), Sword->ItemDefinitionId);

            // Creation accepts only a registered definition ID in this world.
            Test.TestNull(TEXT("Configured world refuses unknown canonical identity"),
                F.Runtime->SpawnItemActorForDefinitionId(
                    TEXT("Unknown_H7_Spawn"), F.Runtime, nullptr));
            AGridItemActor* CanonicalSpawn = F.Runtime->SpawnItemActorForDefinitionId(
                ArrowId, F.Runtime, nullptr);
            if (!Test.TestNotNull(TEXT("Canonical actor still spawns"), CanonicalSpawn))
                return false;
            Test.TestEqual(TEXT("Canonical actor preserves its requested identity"),
                CanonicalSpawn->GetItemDefinitionId(), ArrowId);
            CanonicalSpawn->Destroy();
            return !Test.HasAnyErrors();
        }
        if (FCString::Strcmp(Scenario, TEXT("LegacyIdentity")) == 0)
        {
            AGridItemActor* LegacyActor = F.World->SpawnActor<AGridItemActor>();
            if (!Test.TestNotNull(TEXT("Unregistered actor spawned"), LegacyActor)) return false;
            FGridItemInstance Unknown = Arrow;
            Unknown.ItemDefinitionId = TEXT("Legacy_WorldPickup_BD");
            Test.TestFalse(TEXT("Unregistered physical instance cannot initialize"),
                LegacyActor->InitializeFromItemInstance(Unknown, F.Registry));
            Test.TestTrue(TEXT("Failed initialization leaves identity and GUID unset"),
                LegacyActor->GetItemDefinitionId().IsNone() && !LegacyActor->GetRuntimeObjectId().IsValid());
            LegacyActor->InitializeFromItemDefinitionId(Unknown.ItemDefinitionId, Unknown.RuntimeObjectId);
            Test.TestTrue(TEXT("Configured registry rejects unknown physical identity"),
                FGridWorldPickupIdentityReadModel::Resolve(LegacyActor, Unknown.ItemDefinitionId, 0).IsNone());
            Test.TestTrue(TEXT("Registered placement entry cannot rescue unknown actor"),
                FGridWorldPickupIdentityReadModel::Resolve(LegacyActor, ArrowId, 0).IsNone());
            return !Test.HasAnyErrors();
        }

        if (!Test.TestTrue(TEXT("Actual SaveGame world capture succeeds"),
            F.Runtime->CaptureCurrentLevelRuntimeState()))
            return false;
        const FGridLevelRuntimeState* LevelState = F.Runtime->FindRuntimeStateForCurrentLevel();
        const FGridRuntimeItemState* Saved = LevelState
            ? LevelState->Items.Find(Arrow.RuntimeObjectId) : nullptr;
        if (!Test.TestNotNull(TEXT("Original GUID persisted"), Saved)) return false;
        Test.TestEqual(TEXT("World capture keeps canonical ItemId"),
            Saved->ItemDefinitionId, ArrowId);
        Test.TestEqual(TEXT("Unlevelled ammo remains N0"),
            Saved->ItemLevel, 0);
        if (FCString::Strcmp(Scenario, TEXT("Capture")) == 0)
        {
            Actor->SetRuntimeObjectId(FGuid::NewGuid());
            Test.TestFalse(TEXT("Conflicting physical GUID rejects entire level capture"),
                F.Runtime->CaptureCurrentLevelRuntimeState());
            LevelState = F.Runtime->FindRuntimeStateForCurrentLevel();
            Saved = LevelState ? LevelState->Items.Find(Arrow.RuntimeObjectId) : nullptr;
            if (!Test.TestNotNull(TEXT("Previous snapshot retains original GUID"), Saved)) return false;
            Test.TestEqual(TEXT("Previous snapshot retains canonical identity"), Saved->ItemDefinitionId, ArrowId);
            Actor->SetRuntimeObjectId(Arrow.RuntimeObjectId);
            Actor->ItemDefinitionId = TEXT("Unknown_5G53");
            Test.TestFalse(TEXT("Unknown physical identity rejects entire level capture"),
                F.Runtime->CaptureCurrentLevelRuntimeState());
            LevelState = F.Runtime->FindRuntimeStateForCurrentLevel();
            Saved = LevelState ? LevelState->Items.Find(Arrow.RuntimeObjectId) : nullptr;
            if (!Test.TestNotNull(TEXT("Identity failure retains original snapshot"), Saved)) return false;
            Actor->ItemDefinitionId = ArrowId;
            Test.TestTrue(TEXT("Corrected physical identity captures normally"),
                F.Runtime->CaptureCurrentLevelRuntimeState());
            return !Test.HasAnyErrors();
        }

        if (bPick)
        {
            if (!Test.TestNotNull(TEXT("Party and inventory configured"), F.Party))
                return false;
            Actor->SetRuntimeObjectId(FGuid::NewGuid());
            Test.TestFalse(TEXT("Click pickup rejects conflicting physical GUID"),
                F.Runtime->TryPickupItemActor(Actor, F.Party));
            Test.TestEqual(TEXT("Rejected click does not grant ammo"),
                F.Party->PartyInventoryComponent->CountItemDefinitionInCharacterInventory(0, ArrowId), 0);
            Actor->SetRuntimeObjectId(Arrow.RuntimeObjectId);
            Actor->ItemDefinitionId = TEXT("Unknown_5G54");
            Test.TestFalse(TEXT("Cell pickup rejects conflicting identity"),
                F.Runtime->TryPickupItemAtCell(1, 1, F.Party));
            Test.TestEqual(TEXT("Rejected cell pickup does not grant ammo"),
                F.Party->PartyInventoryComponent->CountItemDefinitionInCharacterInventory(0, ArrowId), 0);
            if (!Test.TestTrue(TEXT("Refusal retains pickup actor"), IsValid(Actor))) return false;
            Actor->ItemDefinitionId = ArrowId;
            if (!Test.TestTrue(TEXT("Actual click pickup uses canonical ItemId"),
                F.Runtime->TryPickupItemActor(Actor, F.Party)))
                return false;
            Test.TestEqual(TEXT("One canonical arrow added to character bag"),
                F.Party->PartyInventoryComponent->CountItemDefinitionInCharacterInventory(0, ArrowId), 1);
            Test.TestEqual(TEXT("Unknown item never added"),
                F.Party->PartyInventoryComponent->CountItemDefinitionInCharacterInventory(
                    0, TEXT("Unknown_5G54")), 0);
            return !Test.HasAnyErrors();
        }
        return false;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BDPresentation,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity.Presentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BDPresentation::RunTest(const FString&)
{
    return ARMCLEAN063BD::Run(*this, TEXT("Presentation"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BDRegisteredGuards,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity.RegisteredLevelGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BDRegisteredGuards::RunTest(const FString&)
{
    return ARMCLEAN063BD::Run(*this, TEXT("RegisteredLevelGuards"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BDLegacy,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity.LegacyIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BDLegacy::RunTest(const FString&)
{
    return ARMCLEAN063BD::Run(*this, TEXT("LegacyIdentity"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BDCapture,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity.Capture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BDCapture::RunTest(const FString&)
{
    return ARMCLEAN063BD::Run(*this, TEXT("Capture"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMCLEAN063BDPickup,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity.Pickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMCLEAN063BDPickup::RunTest(const FString&)
{
    return ARMCLEAN063BD::Run(*this, TEXT("Pickup"));
}

#endif // WITH_DEV_AUTOMATION_TESTS
