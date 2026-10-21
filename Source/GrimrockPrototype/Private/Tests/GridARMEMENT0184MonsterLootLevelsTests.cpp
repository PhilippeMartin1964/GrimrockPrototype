#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDeathComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterLootResolver.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0184
{
    constexpr const TCHAR* DaggerAssetPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01");
    constexpr const TCHAR* BowAssetPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.DA_Weapon_ShortBow_01");

    struct FFixture
    {
        UWorld* World = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;
        UGridItemDefinitionAsset* Dagger = nullptr;
        UGridItemDefinitionAsset* Bow = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM0184_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr, DaggerAssetPath);
            Bow = LoadObject<UGridItemDefinitionAsset>(nullptr, BowAssetPath);
            if (!World) return;
            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            Level = NewObject<UGridLevelAsset>(Runtime);
            Level->Width = 5;
            Level->Height = 5;
            Level->EnsureCellCount();
            for (FGridLevelCellData& Cell : Level->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            Runtime->LevelAsset = Level;
            Runtime->WorldItemPickupReach = 600.f;
        }

        ~FFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }

        bool Ready() const { return World && Runtime && Level && Dagger && Bow; }

        FGridMonsterLootEntry MakeEntry(UGridItemDefinitionAsset* Definition, int32 LevelNumber) const
        {
            FGridMonsterLootEntry Entry;
            Entry.ItemDefinitionAsset = Definition;
            Entry.ItemDefinitionId = Definition ? Definition->ItemDefinitionId : NAME_None;
            Entry.ItemLevel = LevelNumber;
            Entry.DropChance = 1.0f;
            Entry.MinQuantity = 1;
            Entry.MaxQuantity = 1;
            return Entry;
        }

        AGridMonsterActor* SpawnMonster(const TArray<FGridMonsterLootEntry>& Entries)
        {
            UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
            Definition->MonsterId = TEXT("ARM0184_LootMonster");
            Definition->DisplayName = FText::FromString(TEXT("Levelled loot test monster"));
            Definition->CategoryId = TEXT("Vermin");
            Definition->MaxHealth = 8;
            Definition->ActionPointsPerTurn = 2;
            Definition->DeathExpectedDuration = 0.05f;
            Definition->LootTable = Entries;
            AGridMonsterActor* Monster = World->SpawnActor<AGridMonsterActor>();
            if (!Monster || !Monster->InitializeMonster(Definition, FGuid::NewGuid(), FIntPoint(2, 2), EGridEdge::North))
            {
                return nullptr;
            }
            if (Monster->DeathComponent)
            {
                Monster->DeathComponent->InitializeDeathComponent(Runtime);
            }
            return Monster;
        }

        AGridItemActor* FindWorldItem(FGuid Id) const
        {
            if (!World) return nullptr;
            for (TActorIterator<AGridItemActor> It(World); It; ++It)
            {
                AGridItemActor* Actor = *It;
                if (IsValid(Actor) && !Actor->IsActorBeingDestroyed() && Actor->GetRuntimeObjectId() == Id)
                {
                    return Actor;
                }
            }
            return nullptr;
        }

        AGrimrockPartyPawn* SpawnParty()
        {
            AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Party || !Party->PartyInventoryComponent) return nullptr;
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 2;
            Party->CurrentCellY = 2;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation(Runtime->GetCellCenterWorld(2, 2, Party->EyeHeight));
            Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
            if (!Party->PartyInventoryComponent->RegisterItemDefinition(Dagger) ||
                !Party->PartyInventoryComponent->RegisterItemDefinition(Bow))
            {
                return nullptr;
            }
            return Party;
        }

        const FGridItemInstance* FindInBag(const UGridPartyInventoryComponent* Inventory, FGuid Id) const
        {
            if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return nullptr;
            for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
            {
                if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id) return &Slot.Item;
            }
            return nullptr;
        }

        const FGridRuntimeItemState* SavedItem(FGuid Id) const
        {
            const FGridLevelRuntimeState* State = Runtime ? Runtime->FindRuntimeStateForCurrentLevel() : nullptr;
            return State ? State->Items.Find(Id) : nullptr;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0184LevelContractTest,
    "Grimrock.RPG.ARMEMENT01.8.MonsterLoot.AuthoredLevelContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0184LevelContractTest::RunTest(const FString&)
{
    ARMEMENT0184::FFixture Fixture;
    if (!TestTrue(TEXT("Pilot weapon assets load"), Fixture.Ready())) return false;
    const FProperty* Field = FGridMonsterLootEntry::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("Per-entry level is serialized in monster asset"), Field && !Field->HasAnyPropertyFlags(CPF_Transient));

    FGridMonsterLootEntry Fixed = Fixture.MakeEntry(Fixture.Dagger, 9);
    TestTrue(TEXT("Level 9 loot is valid"), Fixed.IsValidDefinition());
    int32 Resolved = INDEX_NONE;
    TestTrue(TEXT("Level 9 resolves from catalogue"), FGridMonsterLootResolver::TryResolveItemLevel(Fixed, Fixture.Dagger, Resolved));
    TestEqual(TEXT("Fixed authored level unchanged"), Resolved, 9);

    FGridMonsterLootEntry Inherited = Fixture.MakeEntry(Fixture.Bow, 0);
    TestTrue(TEXT("Default level resolves once"), FGridMonsterLootResolver::TryResolveItemLevel(Inherited, Fixture.Bow, Resolved));
    TestEqual(TEXT("Default catalogue level selected"), Resolved, Fixture.Bow->DefaultWeaponItemLevel);

    // The authored item level must not add any random consumption to MON8.
    TArray<FGridMonsterLootEntry> LevelNineTable;
    LevelNineTable.Add(Fixed);
    const FGridMonsterLootRollResult LevelNineRoll = FGridMonsterLootResolver::ResolveLoot(LevelNineTable, 184)[0];
    Fixed.ItemLevel = 18;
    TArray<FGridMonsterLootEntry> LevelEighteenTable;
    LevelEighteenTable.Add(Fixed);
    const FGridMonsterLootRollResult LevelEighteenRoll = FGridMonsterLootResolver::ResolveLoot(LevelEighteenTable, 184)[0];
    TestEqual(TEXT("Changing the level does not change the drop roll"), LevelNineRoll.DropRoll, LevelEighteenRoll.DropRoll);
    TestEqual(TEXT("Changing the level does not change the quantity"), LevelNineRoll.Quantity, LevelEighteenRoll.Quantity);
    TestTrue(TEXT("Guaranteed entry remains a drop"), LevelNineRoll.bDropped && LevelEighteenRoll.bDropped);

    Fixed.ItemLevel = 21;
    TestFalse(TEXT("Above maximum level is invalid authoring"), Fixed.IsValidDefinition());
    TestFalse(TEXT("Invalid level is rejected by resolver"), FGridMonsterLootResolver::TryResolveItemLevel(Fixed, Fixture.Dagger, Resolved));
    Fixed.ItemLevel = -1;
    TestFalse(TEXT("Negative levels are invalid authoring"), Fixed.IsValidDefinition());

    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(Fixture.Runtime);
    Legacy->ItemDefinitionId = TEXT("ARM0184_LegacyKey");
    FGridMonsterLootEntry NonCatalogue = Fixture.MakeEntry(Legacy, 0);
    TestTrue(TEXT("Legacy loot still resolves with level 0"), FGridMonsterLootResolver::TryResolveItemLevel(NonCatalogue, Legacy, Resolved));
    TestEqual(TEXT("Legacy item stays unlevelled"), Resolved, 0);
    NonCatalogue.ItemLevel = 9;
    TestFalse(TEXT("Cannot assign weapon level to legacy item"), FGridMonsterLootResolver::TryResolveItemLevel(NonCatalogue, Legacy, Resolved));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0184DeathTest,
    "Grimrock.RPG.ARMEMENT01.8.MonsterLoot.DeathWorldAndPickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0184DeathTest::RunTest(const FString&)
{
    AddExpectedError(TEXT("Reason=MissingMonsterMovement; continuing death."), EAutomationExpectedErrorFlags::Contains, 1);
    ARMEMENT0184::FFixture Fixture;
    if (!TestTrue(TEXT("Runtime level and pilot weapons exist"), Fixture.Ready())) return false;
    TArray<FGridMonsterLootEntry> Entries;
    Entries.Add(Fixture.MakeEntry(Fixture.Dagger, 9));
    Entries.Add(Fixture.MakeEntry(Fixture.Bow, 0));
    AGridMonsterActor* Monster = Fixture.SpawnMonster(Entries);
    if (!TestNotNull(TEXT("Monster with two guaranteed drops exists"), Monster) ||
        !TestNotNull(TEXT("Death component exists"), Monster->DeathComponent.Get())) return false;

    Monster->MarkDead();
    if (!TestEqual(TEXT("Both guaranteed drops placed"), Monster->DeathComponent->PlacedLootCount, 2) ||
        !TestEqual(TEXT("Two generated item instances recorded"), Monster->DeathComponent->GeneratedLoot.Num(), 2)) return false;
    TestEqual(TEXT("Zero drop failures"), Monster->DeathComponent->FailedLootCount, 0);
    const FGridItemInstance DroppedDagger = Monster->DeathComponent->GeneratedLoot[0];
    const FGridItemInstance DroppedBow = Monster->DeathComponent->GeneratedLoot[1];
    TestTrue(TEXT("Distinct physical object identities"), DroppedDagger.RuntimeObjectId != DroppedBow.RuntimeObjectId);
    TestEqual(TEXT("Level 9 on generated dagger"), DroppedDagger.ItemLevel, 9);
    TestEqual(TEXT("Default level materialized for bow"), DroppedBow.ItemLevel, Fixture.Bow->DefaultWeaponItemLevel);
    TestNotNull(TEXT("Dagger pickup actor exists"), Fixture.FindWorldItem(DroppedDagger.RuntimeObjectId));

    if (!TestTrue(TEXT("Capture the monster's world loot"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridRuntimeItemState* SavedDagger = Fixture.SavedItem(DroppedDagger.RuntimeObjectId);
    const FGridRuntimeItemState* SavedBow = Fixture.SavedItem(DroppedBow.RuntimeObjectId);
    if (!TestNotNull(TEXT("Dagger has a saved world state"), SavedDagger) ||
        !TestNotNull(TEXT("Bow has a saved world state"), SavedBow)) return false;
    TestEqual(TEXT("Saved dagger retains level 9"), SavedDagger->ItemLevel, 9);
    TestEqual(TEXT("Saved bow retains default level"), SavedBow->ItemLevel, Fixture.Bow->DefaultWeaponItemLevel);

    AGrimrockPartyPawn* Party = Fixture.SpawnParty();
    if (!TestNotNull(TEXT("Pickup pawn and inventory exist"), Party)) return false;
    if (!TestTrue(TEXT("Pickup death-dropped dagger"), Fixture.Runtime->TryPickupItemActor(
        Fixture.FindWorldItem(DroppedDagger.RuntimeObjectId), Party))) return false;
    const FGridItemInstance* Owned = Fixture.FindInBag(Party->PartyInventoryComponent, DroppedDagger.RuntimeObjectId);
    if (!TestNotNull(TEXT("Original dagger entered inventory"), Owned)) return false;
    TestEqual(TEXT("Level 9 survives pickup"), Owned->ItemLevel, 9);
    const int32 PlacedBeforeRepeat = Monster->DeathComponent->PlacedLootCount;
    Monster->MarkDead();
    TestEqual(TEXT("Repeated death does not duplicate loot"), Monster->DeathComponent->PlacedLootCount, PlacedBeforeRepeat);
    TestEqual(TEXT("Generated loot remains a single pair"), Monster->DeathComponent->GeneratedLoot.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0184SaveTest,
    "Grimrock.RPG.ARMEMENT01.8.MonsterLoot.SaveRestoreAndPickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0184SaveTest::RunTest(const FString&)
{
    AddExpectedError(TEXT("Reason=MissingMonsterMovement; continuing death."), EAutomationExpectedErrorFlags::Contains, 1);
    ARMEMENT0184::FFixture Fixture;
    if (!TestTrue(TEXT("Runtime fixture exists"), Fixture.Ready())) return false;
    TArray<FGridMonsterLootEntry> Entries;
    Entries.Add(Fixture.MakeEntry(Fixture.Dagger, 13));
    AGridMonsterActor* Monster = Fixture.SpawnMonster(Entries);
    if (!TestNotNull(TEXT("Monster exists"), Monster)) return false;
    Monster->MarkDead();
    if (!TestEqual(TEXT("One drop exists"), Monster->DeathComponent->GeneratedLoot.Num(), 1)) return false;

    const FGuid DropId = Monster->DeathComponent->GeneratedLoot[0].RuntimeObjectId;
    if (!TestTrue(TEXT("Save the death-created pickup"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    FGridLevelRuntimeState* Saved = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    FGridRuntimeItemState* SavedDrop = Saved ? Saved->Items.Find(DropId) : nullptr;
    if (!TestNotNull(TEXT("Dropped dagger has serialized state"), SavedDrop)) return false;
    TestEqual(TEXT("Initial saved fixed level"), SavedDrop->ItemLevel, 13);

    // Rehydration must use the saved instance level, not the loot-table authoring
    // value or the definition default; no loot roll runs on restore.
    SavedDrop->ItemLevel = 17;
    if (!TestTrue(TEXT("Restore saved world"), Fixture.Runtime->ApplyCurrentLevelRuntimeState())) return false;
    if (!TestTrue(TEXT("Recapture restored world"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridRuntimeItemState* Restored = Fixture.SavedItem(DropId);
    if (!TestNotNull(TEXT("Restored pickup has world state"), Restored)) return false;
    TestEqual(TEXT("Saved level overrides original drop level"), Restored->ItemLevel, 17);
    TestEqual(TEXT("Death is not re-executed during restore"), Monster->DeathComponent->GeneratedLoot.Num(), 1);

    AGrimrockPartyPawn* Party = Fixture.SpawnParty();
    if (!TestNotNull(TEXT("Inventory pawn exists"), Party)) return false;
    if (!TestTrue(TEXT("Pickup restored dagger"), Fixture.Runtime->TryPickupItemActor(Fixture.FindWorldItem(DropId), Party))) return false;
    const FGridItemInstance* Owned = Fixture.FindInBag(Party->PartyInventoryComponent, DropId);
    if (!TestNotNull(TEXT("Restored original object is in inventory"), Owned)) return false;
    TestEqual(TEXT("Saved level 17 survives pickup"), Owned->ItemLevel, 17);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
