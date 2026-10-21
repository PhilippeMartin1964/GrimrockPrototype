#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace ARMEMENT0192SaveFix
{
    struct FFixture
    {
        UWorld* World = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;
        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM0192_Save_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            if (!World) return;
            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            UGridLevelAsset* EmptyLevel = NewObject<UGridLevelAsset>(Runtime);
            EmptyLevel->Width = 2;
            EmptyLevel->Height = 2;
            EmptyLevel->EnsureCellCount();
            Runtime->LevelAsset = EmptyLevel;
            Inventory = NewObject<UGridPartyInventoryComponent>(Runtime);
            Inventory->InitializeDefaultPartyIfNeeded();
        }
        ~FFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
        bool Ready() const
        {
            return World && Runtime && Inventory && 
                Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(0) &&
                Inventory->PartyInventoryState.ActiveEquipment.IsValidIndex(0);
        }
        FGridItemInstance SaveItem(FName Id) const
        {
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.Quantity = 1;
            Item.ItemLevel = 1;
            Item.OwnerType = EGridItemOwnerType::CharacterInventory;
            Item.OwnerGuid = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
            Item.OwnerCharacterIndex = 0;
            return Item;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0192MissingDungeonTest,
    "Grimrock.RPG.ARMEMENT01.9.SaveRehydrate.MissingDungeonDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0192MissingDungeonTest::RunTest(const FString&)
{
    ARMEMENT0192SaveFix::FFixture Fixture;
    if (!TestTrue(TEXT("Empty-dungeon fixture ready"), Fixture.Ready())) return false;
    const FName BowId(TEXT("Weapon_ShortBow_01"));
    const FName DaggerId(TEXT("Weapon_Dagger_01"));

    for (const FName ItemId : { BowId, DaggerId })
    {
        UGridItemDefinitionAsset* Definition = Fixture.Runtime->ResolveRuntimeItemDefinition(ItemId);
        if (!TestNotNull(TEXT("Starting item resolves without dungeon placement"), Definition)) return false;
        TestEqual(TEXT("Resolved definition is exactly the saved ID"), Definition->ItemDefinitionId, ItemId);
        TestTrue(TEXT("Resolved definition is a valid production asset"), Definition->IsValidDefinition());
    }
    TestNull(TEXT("Unknown item definition does not resolve"),
        Fixture.Runtime->ResolveRuntimeItemDefinition(TEXT("Weapon_NeverExisting_0192")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0192SaveRestoreTest,
    "Grimrock.RPG.ARMEMENT01.9.SaveRehydrate.InventoryAndEquipment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0192SaveRestoreTest::RunTest(const FString&)
{
    ARMEMENT0192SaveFix::FFixture Fixture;
    if (!TestTrue(TEXT("Saved-party fixture ready"), Fixture.Ready())) return false;
    const FName DaggerId(TEXT("Weapon_Dagger_01"));
    const FName ShieldId(TEXT("Weapon_RoundShield_01"));
    FGridItemInstance Dagger = Fixture.SaveItem(DaggerId);
    FGridItemInstance Shield = Fixture.SaveItem(ShieldId);
    Shield.OwnerType = EGridItemOwnerType::EquipmentSlot;
    Shield.EquipmentSlot = EGridEquipmentSlot::OffHand;

    FGridInventorySlot& Bag = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0];
    Bag.bOccupied = true;
    Bag.Item = Dagger;
    Fixture.Inventory->PartyInventoryState.ActiveEquipment[0].OffHand = Shield;

    FName MissingDefinitionId;
    const bool bResolved = Fixture.Inventory->RehydrateOwnedItemDefinitions(
        [&Fixture](FName Id) { return Fixture.Runtime->ResolveRuntimeItemDefinition(Id); },
        MissingDefinitionId);
    if (!TestTrue(TEXT("Saved bag and equipment rehydrate without level references"), bResolved)) return false;
    TestTrue(TEXT("No missing definition"), MissingDefinitionId.IsNone());
    TestNotNull(TEXT("Dagger definition registered"), Fixture.Inventory->FindItemDefinition(DaggerId));
    TestNotNull(TEXT("Shield definition registered"), Fixture.Inventory->FindItemDefinition(ShieldId));
    TestEqual(TEXT("Bag item identity survives"), Bag.Item.RuntimeObjectId, Dagger.RuntimeObjectId);
    TestEqual(TEXT("Bag item fixed level survives"), Bag.Item.ItemLevel, 1);
    TestEqual(TEXT("Equipment identity survives"),
        Fixture.Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId,
        Shield.RuntimeObjectId);

    // Unknown IDs still abort rehydration instead of producing a false
    // Legacy item or silently substituting a known starting definition.
    FGridInventorySlot& Other = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[1];
    Other.bOccupied = true;
    Other.Item = Fixture.SaveItem(TEXT("Weapon_Unknown_0192"));
    MissingDefinitionId = NAME_None;
    TestFalse(TEXT("Unknown saved item is rejected"),
        Fixture.Inventory->RehydrateOwnedItemDefinitions(
            [&Fixture](FName Id) { return Fixture.Runtime->ResolveRuntimeItemDefinition(Id); },
            MissingDefinitionId));
    TestEqual(TEXT("Missing ID is reported"), MissingDefinitionId, FName(TEXT("Weapon_Unknown_0192")));
    TestNotNull(TEXT("Failed rehydration leaves previously registered dagger intact"),
        Fixture.Inventory->FindItemDefinition(DaggerId));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
