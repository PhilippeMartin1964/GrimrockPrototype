#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
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

namespace ARMEMENT01172B
{
	const FName CatalystId(TEXT("Item_SurfaceCatalyst_01"));
	const FName SurfaceWeaverKit(TEXT("Kit_Mage_SurfaceWeaver"));

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
                FName(*FString::Printf(TEXT("ARMEMENT01172_Canonical_%s"),
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

	const FGridItemInstance* FindInBag(const FGridCharacterInventoryState& Hero)
	{
		for (const FGridInventorySlot& Slot : Hero.InventorySlots)
		{
			if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == CatalystId)
			{
				return &Slot.Item;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01172BStarterSave,
	"Grimrock.RPG.ARMEMENT01.17.2B.SurfaceWeaverNewGameSaveContinue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01172BStarterSave::RunTest(const FString&)
{
    URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    ARMEMENT01172B::FCanonicalWorld Fixture;
    if (!TestNotNull(TEXT("Real Mage asset"), Mage) ||
        !TestNotNull(TEXT("Real Human race"), Human) ||
        !TestNotNull(TEXT("Canonical New Game world"), Fixture.World) ||
        !TestTrue(TEXT("Production ItemSystem is configured"),
            Fixture.Registry && Fixture.Registry->IsConfigured()))
        return false;

    FGridResolvedItem Catalyst;
    if (!TestTrue(TEXT("Real catalyst N0 resolves through ItemSystem"),
        Fixture.Registry->ResolveItem(ARMEMENT01172B::CatalystId, 0, Catalyst)) ||
        !TestFalse(TEXT("Catalyst is a non-ARM N0 item"), Catalyst.bIsArmament))
        return false;
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Catalyst resolves its detailed canonical QuickItem action"),
        Fixture.Registry->ResolveDetailedNonArmAction(ARMEMENT01172B::CatalystId, 0,
            TEXT("Action_Mage_SurfaceCatalyst_01"), Action)))
        return false;
    TestTrue(TEXT("N1 catalyst does not require Mage N2 talent"),
        Action.Requirements.IsEmpty());
    TestEqual(TEXT("Catalyst is a QuickItem"),
        Action.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);

    const int32 OriginalKitCount = Mage->StartingEquipmentKits.Num();
    const FName OriginalDefault = Mage->DefaultStartingEquipmentKitId;
    URPGClassAsset* Preview = DuplicateObject<URPGClassAsset>(Mage, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Approved Mage kits materialize from canonical ItemSystem"),
        Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *Preview, Error, Fixture.Registry)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Production Mage asset is unchanged"),
        Mage->StartingEquipmentKits.Num(), OriginalKitCount);
    TestEqual(TEXT("Mage default unchanged"),
        Mage->DefaultStartingEquipmentKitId, OriginalDefault);
    if (!TestEqual(TEXT("Exactly three Mage N1 orientations"),
        Preview->StartingEquipmentKits.Num(), 3))
        return false;

    for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
    {
        const bool bWeaver = Kit.KitId == ARMEMENT01172B::SurfaceWeaverKit;
        int32 CatalystEntries = 0;
        for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
        {
            if (Entry.ItemId == ARMEMENT01172B::CatalystId)
            {
                ++CatalystEntries;
                TestEqual(TEXT("One catalyst in the kit"), Entry.Quantity, 1);
                TestEqual(TEXT("Catalyst goes into the bag"),
                    Entry.TargetSlot, EGridEquipmentSlot::None);
                TestEqual(TEXT("No weapon level on consumable"), Entry.ItemLevel, 0);
            }
        }
        TestEqual(TEXT("Only SurfaceWeaver has catalyst"),
            CatalystEntries, bWeaver ? 1 : 0);
        if (bWeaver)
        {
            TestEqual(TEXT("Staff plus one catalyst"), Kit.Items.Num(), 2);
            TestEqual(TEXT("Elemental staff equipped in main hand"),
                Kit.Items[0].ItemId, FName(TEXT("Weapon_ElementalStaff_01")));
            TestEqual(TEXT("Staff retains main-hand ownership"),
                Kit.Items[0].TargetSlot, EGridEquipmentSlot::MainHand);
        }
    }

    UGridPartyInventoryComponent* Inventory = Fixture.NewInventory();
    if (!TestNotNull(TEXT("Real frontend Mage inventory"), Inventory))
        return false;
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Mage Surface Weaver N1"));
    Request.RaceDefinition = Human;
    Request.ClassDefinition = Mage;
    Request.CombatActionSourceClassDefinition = Mage;
    Request.StartingEquipmentKitId = ARMEMENT01172B::SurfaceWeaverKit;
    FText Failure;
    if (!TestTrue(TEXT("Real New Game creates Mage with canonical catalyst"),
        Inventory->CreateInitialCharacter(Request, Failure)))
    {
        AddError(Failure.ToString());
        return false;
    }
    const FGridCharacterInventoryState& Hero =
        Inventory->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* CatalystItem = ARMEMENT01172B::FindInBag(Hero);
    if (!TestNotNull(TEXT("Physical catalyst in Mage bag"), CatalystItem))
        return false;
    const FGuid RuntimeId = CatalystItem->RuntimeObjectId;
    TestTrue(TEXT("Catalyst runtime GUID exists"), RuntimeId.IsValid());
    TestEqual(TEXT("One consumable for N1"), CatalystItem->Quantity, 1);
    TestEqual(TEXT("Exactly owned by starter Mage"),
        CatalystItem->OwnerGuid, Hero.CharacterId);
    TestEqual(TEXT("Inventory ownership"),
        CatalystItem->OwnerType, EGridItemOwnerType::CharacterInventory);
    TestEqual(TEXT("Kit identity"),
        Hero.StartingEquipmentKitId, ARMEMENT01172B::SurfaceWeaverKit);
    TestEqual(TEXT("Mage starts level one without unlocked talents"), Hero.Level, 1);
    TestEqual(TEXT("Elemental staff still equipped"),
        Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.ItemDefinitionId,
        FName(TEXT("Weapon_ElementalStaff_01")));

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->PartyInventoryState = Inventory->PartyInventoryState;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("SaveGame in-memory snapshot succeeds"),
        UGameplayStatics::SaveGameToMemory(Save, Bytes)))
        return false;
    UGrimrockPartySaveGame* Loaded =
        Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("SaveGame binary reload"), Loaded))
        return false;
    const FGridCharacterInventoryState& LoadedHero =
        Loaded->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* LoadedCatalyst =
        ARMEMENT01172B::FindInBag(LoadedHero);
    if (!TestNotNull(TEXT("Catalyst survives binary reload"), LoadedCatalyst))
        return false;
    TestEqual(TEXT("Exact persistent catalyst GUID"),
        LoadedCatalyst->RuntimeObjectId, RuntimeId);
    TestEqual(TEXT("Exact persistent N1 kit id"),
        LoadedHero.StartingEquipmentKitId, ARMEMENT01172B::SurfaceWeaverKit);

    UGridPartyInventoryComponent* Continued = Fixture.NewInventory();
    if (!TestNotNull(TEXT("Real dungeon inventory"), Continued))
        return false;
    Continued->InitializeDefaultPartyIfNeeded();
    FText RestoreError;
    if (!TestTrue(TEXT("Continue restores canonical owned inventory"),
        Continued->RestorePartyInventoryState(Loaded->PartyInventoryState, RestoreError)))
    {
        AddError(RestoreError.ToString());
        return false;
    }
    TestEqual(TEXT("Restored Mage has one catalyst"),
        Continued->CountItemDefinitionInCharacterInventory(0,
            ARMEMENT01172B::CatalystId), 1);
    FString OwnershipError;
    TestTrue(TEXT("Character and item ownership valid"),
        Continued->ValidateInventoryOwnership(OwnershipError));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
