#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/RPGCharacterCreationWizardWidget.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01132
{
    const TCHAR* const Classes[] = {
        TEXT("Warrior"), TEXT("Rogue"), TEXT("Ranger"),
        TEXT("Mage"), TEXT("Priest"), TEXT("Alchemist")
    };

    URPGClassAsset* LoadClass(const TCHAR* Id)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"), Id, Id);
        return LoadObject<URPGClassAsset>(nullptr, *Path);
    }

    URPGRaceAsset* Human()
    {
        return LoadObject<URPGRaceAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    }

    URPGClassAsset* Approved(URPGClassAsset* Original, FString& Error)
    {
        URPGClassAsset* Preview = Original
            ? DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()) : nullptr;
        return Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Preview, Error)
            ? Preview : nullptr;
    }

    FRPGCharacterCreationRequest Request(URPGClassAsset* Class, URPGRaceAsset* Race, FName Kit)
    {
        FRPGCharacterCreationRequest Result;
        Result.DisplayName = FText::FromString(TEXT("Starting Kit Test"));
        Result.ClassDefinition = Class;
        Result.CombatActionSourceClassDefinition = Class;
        Result.RaceDefinition = Race;
        Result.StartingEquipmentKitId = Kit;
        return Result;
    }

    const FGridItemInstance* Find(const FGridCharacterInventoryState& Character,
        const FGridCharacterEquipmentState& Equipment, const FRPGStartingEquipmentEntry& Entry)
    {
        const FName Id = Entry.ItemDefinition->ItemDefinitionId;
        if (Entry.TargetSlot != EGridEquipmentSlot::None)
        {
            const FGridItemInstance* Item = Equipment.GetSlot(Entry.TargetSlot);
            return Item && Item->IsValid() && Item->ItemDefinitionId == Id ? Item : nullptr;
        }
        for (const FGridInventorySlot& Slot : Character.InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == Id) return &Slot.Item;
        }
        return nullptr;
    }

    int32 CountPhysical(const FGridCharacterInventoryState& Character,
        const FGridCharacterEquipmentState& Equipment, TSet<FGuid>& Guids)
    {
        int32 Count = 0;
        const auto CountOne = [&Count, &Guids](const FGridItemInstance& Item)
        {
            if (Item.IsValid())
            {
                ++Count;
                Guids.Add(Item.RuntimeObjectId);
            }
        };
        for (const FGridInventorySlot& Slot : Character.InventorySlots)
        {
            if (!Slot.IsEmpty()) CountOne(Slot.Item);
        }
        for (int32 Index = int32(EGridEquipmentSlot::MainHand); Index <= int32(EGridEquipmentSlot::Bracers); ++Index)
        {
            if (const FGridItemInstance* Item = Equipment.GetSlot(static_cast<EGridEquipmentSlot>(Index)))
            {
                CountOne(*Item);
            }
        }
        return Count;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01132Projection,
    "Grimrock.RPG.ARMEMENT01.13.ApprovedKits.ThreeOptionsPerClass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01132Projection::RunTest(const FString&)
{
    TSet<FName> GlobalIds;
    for (const TCHAR* Id : ARMEMENT01132::Classes)
    {
        URPGClassAsset* Original = ARMEMENT01132::LoadClass(Id);
        if (!TestNotNull(TEXT("Production class exists"), Original)) return false;
        const int32 PreviousCount = Original->StartingEquipmentKits.Num();
        const FName PreviousDefault = Original->DefaultStartingEquipmentKitId;
        FString Error;
        URPGClassAsset* Preview = ARMEMENT01132::Approved(Original, Error);
        if (!TestNotNull(TEXT("Approved preview builds"), Preview))
        {
            AddError(Error);
            return false;
        }
        if (!TestEqual(TEXT("Three N1 kits per class"), Preview->StartingEquipmentKits.Num(), 3)) return false;
        TestEqual(TEXT("First approved kit is default"),
            Preview->DefaultStartingEquipmentKitId, Preview->StartingEquipmentKits[0].KitId);
        TSet<FString> Labels;
        for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
        {
            TestFalse(TEXT("Globally unique approved kit"), GlobalIds.Contains(Kit.KitId));
            GlobalIds.Add(Kit.KitId);
            TestTrue(TEXT("Kit belongs to class"),
                Kit.KitId.ToString().StartsWith(FString::Printf(TEXT("Kit_%s_"), Id)));
            TestFalse(TEXT("Localized title provided"), Kit.DisplayName.IsEmpty());
            TestFalse(TEXT("Distinct labels in class"), Labels.Contains(Kit.DisplayName.ToString()));
            Labels.Add(Kit.DisplayName.ToString());
        }
        TestEqual(TEXT("Production class not mutated"), Original->StartingEquipmentKits.Num(), PreviousCount);
        TestEqual(TEXT("Production default not mutated"), Original->DefaultStartingEquipmentKitId, PreviousDefault);

        URPGCharacterCreationWizardWidget* Wizard = NewObject<URPGCharacterCreationWizardWidget>();
        Wizard->ClassDefinition = Original;
        const TArray<FRPGStartingEquipmentKit> Options = Wizard->GetAvailableStartingEquipmentKits();
        if (!TestEqual(TEXT("Wizard exposes three options"), Options.Num(), 3)) return false;
        for (const FRPGStartingEquipmentKit& Kit : Options)
        {
            TestTrue(TEXT("Wizard accepts new kit ID"), Wizard->SelectStartingEquipmentKit(Kit.KitId));
            TestEqual(TEXT("Selected kit preserved"), Wizard->GetSelectedStartingEquipmentKitId(), Kit.KitId);
            TestFalse(TEXT("Summary is populated"), Wizard->GetSelectedStartingEquipmentKitSummary().IsEmpty());
        }
        TestFalse(TEXT("Historic kit no longer shown in New Game"),
            Wizard->SelectStartingEquipmentKit(TEXT("Warrior_Defender")));
    }
    TestEqual(TEXT("Eighteen distinct kit IDs"), GlobalIds.Num(), 18);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01132Creation,
    "Grimrock.RPG.ARMEMENT01.13.ApprovedKits.CreateAllEighteen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01132Creation::RunTest(const FString&)
{
    URPGRaceAsset* Race = ARMEMENT01132::Human();
    if (!TestNotNull(TEXT("Production race exists"), Race)) return false;
    int32 Created = 0;
    bool bMarksmanArrows = false;
    bool bScoutArrows = false;
    for (const TCHAR* Id : ARMEMENT01132::Classes)
    {
        URPGClassAsset* Class = ARMEMENT01132::LoadClass(Id);
        if (!TestNotNull(TEXT("Production class exists"), Class)) return false;
        FString Error;
        URPGClassAsset* Preview = ARMEMENT01132::Approved(Class, Error);
        if (!TestNotNull(TEXT("Approved class copy available"), Preview))
        {
            AddError(Error);
            return false;
        }
        for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
        {
            UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
            Party->InitializeDefaultPartyIfNeeded();
            FText Failure;
            if (!Party->CreateInitialCharacter(ARMEMENT01132::Request(Class, Race, Kit.KitId), Failure))
            {
                AddError(FString::Printf(TEXT("CreateInitialCharacter %s: %s"),
                    *Kit.KitId.ToString(), *Failure.ToString()));
                return false;
            }
            ++Created;
            const FGridCharacterInventoryState& Character = Party->PartyInventoryState.ActiveCharacters[0];
            const FGridCharacterEquipmentState& Equipment = Party->PartyInventoryState.ActiveEquipment[0];
            TestTrue(TEXT("Hero has GUID"), Character.CharacterId.IsValid());
            TestEqual(TEXT("Exact kit ID stored"), Character.StartingEquipmentKitId, Kit.KitId);
            TestTrue(TEXT("Creation completed"), Party->HasCompletedInitialCharacterCreation());
            TSet<FGuid> Guids;
            const int32 Count = ARMEMENT01132::CountPhysical(Character, Equipment, Guids);
            TestEqual(TEXT("Physical count equals recipe"), Count, Kit.Items.Num());
            TestEqual(TEXT("All physical GUIDs unique"), Guids.Num(), Count);
            for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
            {
                const FGridItemInstance* Item = ARMEMENT01132::Find(Character, Equipment, Entry);
                if (!TestNotNull(TEXT("Correct item in correct slot/bag"), Item)) return false;
                TestTrue(TEXT("Instance GUID valid"), Item->RuntimeObjectId.IsValid());
                TestEqual(TEXT("Quantity matches recipe"), Item->Quantity, Entry.Quantity);
                TestEqual(TEXT("Owner GUID matches hero"), Item->OwnerGuid, Character.CharacterId);
                TestEqual(TEXT("Owner index is first hero"), Item->OwnerCharacterIndex, 0);
                const bool bBag = Entry.TargetSlot == EGridEquipmentSlot::None;
                TestEqual(TEXT("Owner type correct"), Item->OwnerType,
                    bBag ? EGridItemOwnerType::CharacterInventory : EGridItemOwnerType::EquipmentSlot);
                TestEqual(TEXT("Equipment destination correct"), Item->EquipmentSlot, Entry.TargetSlot);
                const UGridItemDefinitionAsset* Definition = Entry.ItemDefinition.Get();
                if (!TestNotNull(TEXT("Item definition resolves"), Definition)) return false;
                TestEqual(TEXT("Catalogue N1 level"), Item->ItemLevel,
                    Definition->bUseWeaponCatalogue ? Definition->DefaultWeaponItemLevel : 0);
                TestTrue(TEXT("Item definition registered"), Party->FindItemDefinition(Definition->ItemDefinitionId) == Definition);
                if (Item->ItemDefinitionId == FName(TEXT("Item_Arrows_01")) && bBag)
                {
                    if (Kit.KitId == FName(TEXT("Kit_Ranger_Marksman"))) bMarksmanArrows = Item->Quantity == 12;
                    if (Kit.KitId == FName(TEXT("Kit_Ranger_Scout"))) bScoutArrows = Item->Quantity == 12;
                }
            }
            FString OwnershipError;
            TestTrue(TEXT("Exclusive ownership invariant"), Party->ValidateInventoryOwnership(OwnershipError));
            UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
            Save->PartyInventoryState = Party->PartyInventoryState;
            TestEqual(TEXT("Kit ID persists in SaveGame snapshot"),
                Save->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, Kit.KitId);
            TSet<FGuid> SavedGuids;
            ARMEMENT01132::CountPhysical(Save->PartyInventoryState.ActiveCharacters[0],
                Save->PartyInventoryState.ActiveEquipment[0], SavedGuids);
            TestEqual(TEXT("Item GUIDs persist in SaveGame snapshot"), SavedGuids.Num(), Guids.Num());

            // ARMEMENT-LOT02.17: write every real New Game kit to an isolated
            // disk slot. Restore and rehydrate from disk, not from the
            // previous in-memory SaveGame object.
            Save->SaveVersion = UGrimrockPartySaveGame::CurrentSaveVersion;
            const FString DiskSlot = FString::Printf(TEXT("ARMLOT0217_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits));
            struct FDeleteTestSlot
            {
                FString Name;
                ~FDeleteTestSlot() { UGameplayStatics::DeleteGameInSlot(Name, 0); }
            } SlotCleanup{DiskSlot};
            if (!TestTrue(*FString::Printf(TEXT("%s: SaveGame written to disk"), *Kit.KitId.ToString()),
                UGameplayStatics::SaveGameToSlot(Save, DiskSlot, 0)))
            {
                return false;
            }
            const UGrimrockPartySaveGame* Disk = Cast<UGrimrockPartySaveGame>(
                UGameplayStatics::LoadGameFromSlot(DiskSlot, 0));
            if (!TestNotNull(*FString::Printf(TEXT("%s: disk SaveGame readable"), *Kit.KitId.ToString()), Disk) ||
                !TestTrue(TEXT("Disk save version is compatible"), Disk->IsCompatible()))
            {
                return false;
            }
            UGridPartyInventoryComponent* Continued = NewObject<UGridPartyInventoryComponent>();
            Continued->DefaultInventorySlotCountPerCharacter = 48;
            Continued->InitializeDefaultPartyIfNeeded();
            FText RestoreError;
            if (!TestTrue(*FString::Printf(TEXT("%s: restore into fresh inventory"), *Kit.KitId.ToString()),
                Continued->RestorePartyInventoryState(Disk->PartyInventoryState, RestoreError)))
            {
                AddError(RestoreError.ToString());
                return false;
            }
            FName MissingDefinitionId = NAME_None;
            if (!TestTrue(*FString::Printf(TEXT("%s: rehydrate starting item definitions"), *Kit.KitId.ToString()),
                Continued->RehydrateOwnedItemDefinitions(
                    [](FName ItemId)
                    {
                        return FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(ItemId);
                    }, MissingDefinitionId)))
            {
                AddError(FString::Printf(TEXT("%s: missing saved item %s"),
                    *Kit.KitId.ToString(), *MissingDefinitionId.ToString()));
                return false;
            }
            const FGridPartyInventoryState& Loaded = Continued->PartyInventoryState;
            if (!TestTrue(TEXT("One hero and one equipment state restored"),
                    Loaded.ActiveCharacters.Num() == 1 && Loaded.ActiveEquipment.Num() == 1) ||
                !TestEqual(TEXT("Kit ID round-trips through disk"),
                    Loaded.ActiveCharacters[0].StartingEquipmentKitId, Kit.KitId) ||
                !TestEqual(TEXT("Hero identity round-trips through disk"),
                    Loaded.ActiveCharacters[0].CharacterId, Character.CharacterId))
            {
                return false;
            }
            TSet<FGuid> LoadedGuids;
            const int32 LoadedCount = ARMEMENT01132::CountPhysical(
                Loaded.ActiveCharacters[0], Loaded.ActiveEquipment[0], LoadedGuids);
            bool bSameGuids = LoadedGuids.Num() == Guids.Num();
            for (const FGuid& OriginalGuid : Guids)
            {
                bSameGuids &= LoadedGuids.Contains(OriginalGuid);
            }
            if (!TestEqual(TEXT("Disk kit has exact physical item count"), LoadedCount, Kit.Items.Num()) ||
                !TestEqual(TEXT("Disk kit has distinct physical GUIDs"), LoadedGuids.Num(), LoadedCount) ||
                !TestTrue(TEXT("All original GUIDs survived disk"), bSameGuids))
            {
                return false;
            }
            for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
            {
                const FGridItemInstance* Before = ARMEMENT01132::Find(Character, Equipment, Entry);
                const FGridItemInstance* After = ARMEMENT01132::Find(
                    Loaded.ActiveCharacters[0], Loaded.ActiveEquipment[0], Entry);
                if (!TestTrue(*FString::Printf(TEXT("%s: item/slot/GUID/quantity retained"),
                        *Kit.KitId.ToString()),
                    Before && After && Before->RuntimeObjectId == After->RuntimeObjectId &&
                    Before->ItemDefinitionId == After->ItemDefinitionId &&
                    Before->ItemLevel == After->ItemLevel &&
                    Before->Quantity == After->Quantity &&
                    After->OwnerGuid == Loaded.ActiveCharacters[0].CharacterId &&
                    After->OwnerCharacterIndex == 0 &&
                    After->EquipmentSlot == Entry.TargetSlot &&
                    After->OwnerType == (Entry.TargetSlot == EGridEquipmentSlot::None
                        ? EGridItemOwnerType::CharacterInventory : EGridItemOwnerType::EquipmentSlot)) ||
                    !TestTrue(TEXT("Exact item definition available after disk load"),
                        Continued->FindItemDefinition(Entry.ItemDefinition->ItemDefinitionId) == Entry.ItemDefinition.Get()))
                {
                    return false;
                }
            }
            FString LoadedOwnershipError;
            if (!TestTrue(TEXT("Continued party ownership invariant"),
                Continued->ValidateInventoryOwnership(LoadedOwnershipError)))
            {
                AddError(LoadedOwnershipError);
                return false;
            }
            AddInfo(FString::Printf(TEXT("[ARMEMENT-LOT02.17] Disk kit PASS %s"),
                *Kit.KitId.ToString()));
        }
    }
    TestEqual(TEXT("All 18 approved kits create heroes"), Created, 18);
    TestTrue(TEXT("Marksman has 12 arrows in bag"), bMarksmanArrows);
    TestTrue(TEXT("Scout has 12 arrows in bag"), bScoutArrows);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01132Legacy,
    "Grimrock.RPG.ARMEMENT01.13.ApprovedKits.LegacyAndAtomicRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01132Legacy::RunTest(const FString&)
{
    int32 Count = 0;
    for (const TCHAR* Id : ARMEMENT01132::Classes)
    {
        URPGClassAsset* Class = ARMEMENT01132::LoadClass(Id);
        if (!TestNotNull(TEXT("Class exists"), Class)) return false;
        URPGClassAsset* Legacy = DuplicateObject<URPGClassAsset>(Class, GetTransientPackage());
        FString Error;
        if (!TestTrue(TEXT("Twelve historical kits remain materializable"),
            Legacy && FRPGStartingEquipmentPresetCatalog::TryPopulateTransientClass(*Legacy, Error)))
        {
            AddError(Error);
            return false;
        }
        for (const FRPGStartingEquipmentKit& Kit : Legacy->StartingEquipmentKits)
        {
            ++Count;
            for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
            {
                UGridItemDefinitionAsset* Definition = Entry.ItemDefinition.Get();
                if (!TestNotNull(TEXT("Legacy definition exists"), Definition)) return false;
                TestTrue(TEXT("Exact legacy item identity can be rehydrated"),
                    FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(
                        Definition->ItemDefinitionId) == Definition);
            }
        }
    }
    TestEqual(TEXT("All twelve legacy recipes preserved"), Count, 12);
    TestNull(TEXT("Unknown saved ID is not substituted"),
        FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(TEXT("Unknown_01132")));

    URPGClassAsset* Warrior = ARMEMENT01132::LoadClass(TEXT("Warrior"));
    URPGRaceAsset* Race = ARMEMENT01132::Human();
    if (!TestNotNull(TEXT("Warrior exists"), Warrior) || !TestNotNull(TEXT("Race exists"), Race)) return false;
    UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
    Party->InitializeDefaultPartyIfNeeded();
    const FGuid OriginalId = Party->PartyInventoryState.ActiveCharacters[0].CharacterId;
    FText Error;
    TestFalse(TEXT("Old kit cannot be selected for new hero"),
        Party->CreateInitialCharacter(ARMEMENT01132::Request(Warrior, Race, TEXT("Warrior_Defender")), Error));
    TestFalse(TEXT("Invalid ID reports error"), Error.IsEmpty());
    TestFalse(TEXT("Rejected creation incomplete"), Party->HasCompletedInitialCharacterCreation());
    TestEqual(TEXT("No hero mutation after failure"),
        Party->PartyInventoryState.ActiveCharacters[0].CharacterId, OriginalId);
    TestFalse(TEXT("No weapon registered on failed creation"),
        Party->FindItemDefinition(TEXT("Weapon_Dagger_01")) != nullptr);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
