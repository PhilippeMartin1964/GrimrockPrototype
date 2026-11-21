#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace ARMEMENT01171
{
    struct FCanonicalWorld
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;

        FCanonicalWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARMEMENT01171_Canonical_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
        }

        ~FCanonicalWorld()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GI) GI->Shutdown();
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }

        UGridPartyInventoryComponent* NewInventory() const
        {
            AGrimrockPartyPawn* Pawn = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
            return Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr;
        }
    };

    URPGClassAsset* Alchemist()
    {
        return LoadObject<URPGClassAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist"));
    }

    URPGRaceAsset* Human()
    {
        return LoadObject<URPGRaceAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    }

    const FGridItemInstance* FindInBag(const FGridCharacterInventoryState& Hero, FName ItemId)
    {
        for (const FGridInventorySlot& Slot : Hero.InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemId)
            {
                return &Slot.Item;
            }
        }
        return nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01171OilContract,
    "Grimrock.RPG.ARMEMENT01.17.1.RealOilFlaskNoTalentBypass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01171OilContract::RunTest(const FString&)
{
    const FName OilId(TEXT("Item_Flask_Oil"));
    const FName ActionId(TEXT("Action_Alchemist_OilSlick"));
    FGridCanonicalRegistryFixture Canonical;
    FGridResolvedItem Resolved;
    if (!TestTrue(TEXT("Production Oil N0 resolves through ItemSystem"),
            Canonical.Resolve(OilId, 0, Resolved)) ||
        !TestFalse(TEXT("Oil is a physical non-ARM item"), Resolved.bIsArmament))
        return false;

    TestEqual(TEXT("Canonical oil-flask inventory identity"), Resolved.ItemId, OilId);
    TestTrue(TEXT("Oil flask is an approved canonical definition"), Resolved.IsValid());
    TestTrue(TEXT("Oil has an explicit QuickItem action binding"),
        Resolved.AvailableActions.ContainsByPredicate([ActionId](const FGridItemActionBinding& Binding)
        {
            return Binding.ActionId == ActionId;
        }));
    TestTrue(TEXT("Alchemy source tag exists"),
        Resolved.Tags.Contains(FName(TEXT("QuickItem.Alchemy"))));
    TestTrue(TEXT("Flask source tag exists"),
        Resolved.Tags.Contains(FName(TEXT("QuickItem.Flask"))));

    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Oil flask resolves its authored detailed QuickItem action"),
        Canonical.Registry->ResolveDetailedNonArmAction(OilId, 0, ActionId, Action))) return false;
    TestEqual(TEXT("Correct existing action identity"), Action.ActionId,
        FName(TEXT("Action_Alchemist_OilSlick")));
    TestEqual(TEXT("Uses established inventory QuickItem authority"), Action.SourcePolicy,
        EGridCombatActionSourcePolicy::QuickItem);
    TestEqual(TEXT("Targets ground area"), Action.TargetingPolicy,
        EGridCombatTargetingPolicy::Area);
    TestEqual(TEXT("Two action points, not a free spell"), Action.ActionPointCost, 2);
    TestEqual(TEXT("Throws up to four cells"), Action.RangeCells, 4);
    TestEqual(TEXT("One physical flask is spent on accepted execution"),
        Action.ResourceCosts.SourceItemQuantityCost, 1);
    TestTrue(TEXT("QuickItem source does not grant or demand a level-two talent"),
        Action.Requirements.IsEmpty());
    if (!TestEqual(TEXT("One canonical surface contribution"), Action.SurfaceEffects.Num(), 1))
        return false;
    TestEqual(TEXT("Exactly the existing Oil surface"), Action.SurfaceEffects[0].SurfaceType,
        EGridCombatSurfaceType::Oil);
    TestEqual(TEXT("Existing oil lasts four rounds"), Action.SurfaceEffects[0].DurationRounds, 4);
    TestEqual(TEXT("Oil terrain traversal penalty is preserved"),
        Action.SurfaceEffects[0].TraversalCostModifier, 1);
    TestTrue(TEXT("Exact oil save identity is canonical N0"),
        Resolved.ItemId == OilId && Resolved.ItemLevel == 0 && !Resolved.bIsArmament);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01171StarterAndSave,
    "Grimrock.RPG.ARMEMENT01.17.1.TransmuterNewGameSaveContinue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01171StarterAndSave::RunTest(const FString&)
{
    const FName OilId(TEXT("Item_Flask_Oil"));
    URPGClassAsset* Original = ARMEMENT01171::Alchemist();
    URPGRaceAsset* Race = ARMEMENT01171::Human();
    ARMEMENT01171::FCanonicalWorld Fixture;
    if (!TestNotNull(TEXT("Production Alchemist class"), Original) ||
        !TestNotNull(TEXT("Production Human race"), Race) ||
        !TestNotNull(TEXT("Canonical New Game world"), Fixture.World) ||
        !TestTrue(TEXT("Configured production ItemSystem"),
            Fixture.Registry && Fixture.Registry->IsConfigured()))
        return false;
    FGridResolvedItem Oil;
    if (!TestTrue(TEXT("Real Oil N0 resolves without historical definition"),
        Fixture.Registry->ResolveItem(OilId, 0, Oil)) ||
        !TestFalse(TEXT("Physical Oil is non-ARM"), Oil.bIsArmament))
        return false;

    const int32 OriginalKitCount = Original->StartingEquipmentKits.Num();
    const FName OriginalDefault = Original->DefaultStartingEquipmentKitId;
    URPGClassAsset* Preview = DuplicateObject<URPGClassAsset>(Original, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Approved Alchemist kits materialize from ItemSystem"),
        Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *Preview, Error, Fixture.Registry)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Production Alchemist class is unchanged"),
        Original->StartingEquipmentKits.Num(), OriginalKitCount);
    TestEqual(TEXT("Production default is unchanged"),
        Original->DefaultStartingEquipmentKitId, OriginalDefault);
    if (!TestEqual(TEXT("Three Alchemist orientations are retained"),
        Preview->StartingEquipmentKits.Num(), 3))
        return false;
    for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
    {
        const bool bTransmuter = Kit.KitId == FName(TEXT("Kit_Alchemist_Transmuter"));
        int32 OilEntries = 0;
        for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
        {
            if (Entry.CanonicalItemId == OilId)
            {
                ++OilEntries;
                TestNull(TEXT("Approved Oil kit entry has no historical pointer"),
                    Entry.ItemDefinition.Get());
                TestEqual(TEXT("A single finished oil flask is granted"), Entry.Quantity, 1);
                TestEqual(TEXT("Starter oil item stays in bag"), Entry.TargetSlot,
                    EGridEquipmentSlot::None);
                TestEqual(TEXT("Non-arm Oil flask is physical N0"), Entry.ItemLevel, 0);
            }
        }
        TestEqual(TEXT("Only Transmuter gets the oil flask"),
            OilEntries, bTransmuter ? 1 : 0);
        if (bTransmuter)
        {
            TestEqual(TEXT("Transmuter retains dagger, orb and oil"), Kit.Items.Num(), 3);
            TestEqual(TEXT("Canonical dagger stays main hand"),
                Kit.Items[0].CanonicalItemId, FName(TEXT("Weapon_Dagger_01")));
            TestEqual(TEXT("Orb stays passive off-hand"),
                Kit.Items[1].CanonicalItemId, FName(TEXT("Weapon_ArcaneOrb_01")));
        }
    }

    UGridPartyInventoryComponent* Inventory = Fixture.NewInventory();
    if (!TestNotNull(TEXT("Real frontend inventory"), Inventory))
        return false;
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Transmuter N1"));
    Request.ClassDefinition = Original;
    Request.CombatActionSourceClassDefinition = Original;
    Request.RaceDefinition = Race;
    Request.StartingEquipmentKitId = TEXT("Kit_Alchemist_Transmuter");
    FText CreationError;
    if (!TestTrue(TEXT("New Game creates canonical Transmuter with oil starter"),
        Inventory->CreateInitialCharacter(Request, CreationError)))
    {
        AddError(CreationError.ToString());
        return false;
    }

    const FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* Flask = ARMEMENT01171::FindInBag(Hero, OilId);
    if (!TestNotNull(TEXT("Oil flask is physically in Transmuter bag"), Flask))
        return false;
    const FGuid Guid = Flask->RuntimeObjectId;
    TestTrue(TEXT("Starter oil item has a unique GUID"), Guid.IsValid());
    TestEqual(TEXT("Starter has one oil flask"), Flask->Quantity, 1);
    TestEqual(TEXT("Physical owner is hero"), Flask->OwnerGuid, Hero.CharacterId);
    TestEqual(TEXT("Physical owner type is bag"), Flask->OwnerType,
        EGridItemOwnerType::CharacterInventory);
    TestNull(TEXT("No historical Oil definition registered"),
        Inventory->FindItemDefinition(OilId));
    TestEqual(TEXT("Dagger remains equipped"),
        Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.ItemDefinitionId,
        FName(TEXT("Weapon_Dagger_01")));
    TestEqual(TEXT("Arcane orb remains in off hand"),
        Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.ItemDefinitionId,
        FName(TEXT("Weapon_ArcaneOrb_01")));
    TestEqual(TEXT("N1 starts without the OilSlick N2 talent"), Hero.Level, 1);
    TestEqual(TEXT("Stable kit identifier"), Hero.StartingEquipmentKitId,
        FName(TEXT("Kit_Alchemist_Transmuter")));
    FString OwnershipError;
    TestTrue(TEXT("Starter ownership remains valid"),
        Inventory->ValidateInventoryOwnership(OwnershipError));

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->PartyInventoryState = Inventory->PartyInventoryState;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Complete SaveGame binary serializes canonical starter oil"),
        UGameplayStatics::SaveGameToMemory(Save, Bytes)))
        return false;
    const UGrimrockPartySaveGame* Loaded =
        Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Binary SaveGame reload succeeds"), Loaded))
        return false;
    const FGridCharacterInventoryState& SavedHero =
        Loaded->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* SavedFlask = ARMEMENT01171::FindInBag(SavedHero, OilId);
    if (!TestNotNull(TEXT("Saved Transmuter retains physical flask"), SavedFlask))
        return false;
    TestEqual(TEXT("Oil flask GUID persists"), SavedFlask->RuntimeObjectId, Guid);
    TestEqual(TEXT("Kit identity persists"), SavedHero.StartingEquipmentKitId,
        FName(TEXT("Kit_Alchemist_Transmuter")));

    UGridPartyInventoryComponent* Continued = Fixture.NewInventory();
    if (!TestNotNull(TEXT("Real dungeon inventory"), Continued))
        return false;
    Continued->InitializeDefaultPartyIfNeeded();
    FText RestoreError;
    if (!TestTrue(TEXT("Continue restores canonical player inventory"),
        Continued->RestorePartyInventoryState(Loaded->PartyInventoryState, RestoreError)))
    {
        AddError(RestoreError.ToString());
        return false;
    }
    TestEqual(TEXT("Restored Transmuter carries one oil flask"),
        Continued->CountItemDefinitionInCharacterInventory(0, OilId), 1);
    TestNull(TEXT("Restored bag contains no legacy Oil definition"),
        Continued->FindItemDefinition(OilId));
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
