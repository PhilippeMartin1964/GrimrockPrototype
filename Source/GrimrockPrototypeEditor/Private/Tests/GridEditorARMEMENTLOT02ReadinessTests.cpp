#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Serialization/Csv/CsvParser.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENTLOT02
{
    struct FRecipe
    {
        FString ClassId;
        FString KitId;
        FName MainArchetype;
        FName SecondArchetype;
        FString SecondSlot;
        FName SupportId;
        int32 SupportQuantity = 0;
        FString PendingSupport;
        FString DesignNote;
    };

    struct FResolved
    {
        FName ItemId = NAME_None;
        FGridResolvedItem Profile;
        FString Reason;
        bool IsValid() const { return !ItemId.IsNone() && Profile.IsValid(); }
    };

    FString Csv(const FString& Value)
    {
        return TEXT("\"") + Value.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
    }

    bool ReadRecipes(TArray<FRecipe>& Out, FString& Error)
    {
        Out.Reset();
        FString Content;
        const FString Path = FPaths::Combine(
            FPaths::ProjectDir(),
            TEXT("docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv"));
        if (!FFileHelper::LoadFileToString(Content, *Path))
        {
            Error = TEXT("Approved starter-kit recipe CSV is missing.");
            return false;
        }
        FCsvParser Parser(Content);
        const auto& Lines = Parser.GetRows();
        const TCHAR* Headers[] = {
            TEXT("ClassId"), TEXT("BranchId"), TEXT("KitId"),
            TEXT("Weapon1ArchetypeId"), TEXT("Weapon1Level"),
            TEXT("Weapon1Slot"), TEXT("Weapon2ArchetypeId"),
            TEXT("Weapon2Level"), TEXT("Weapon2Slot"),
            TEXT("KnownSupportItemId"), TEXT("KnownSupportQuantity"),
            TEXT("PlannedSupportNeed"), TEXT("ProductionReadiness"),
            TEXT("ApprovalState"), TEXT("DesignNoteCode")
        };
        if (Lines.IsEmpty() || Lines[0].Num() != UE_ARRAY_COUNT(Headers))
        {
            Error = TEXT("Unexpected recipe header width.");
            return false;
        }
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Headers); ++Index)
        {
            if (FCString::Strcmp(Lines[0][Index], Headers[Index]) != 0)
            {
                Error = FString::Printf(TEXT("Recipe header mismatch at %d."), Index);
                return false;
            }
        }
        TSet<FString> UniqueKitIds;
        for (int32 Index = 1; Index < Lines.Num(); ++Index)
        {
            const auto& Cells = Lines[Index];
            if (Cells.IsEmpty() || !Cells[0] || !*Cells[0]) continue;
            if (Cells.Num() != UE_ARRAY_COUNT(Headers))
            {
                Error = FString::Printf(TEXT("Bad recipe field count at row %d."), Index + 1);
                return false;
            }
            FRecipe& R = Out.AddDefaulted_GetRef();
            R.ClassId = Cells[0];
            R.KitId = Cells[2];
            R.MainArchetype = FName(Cells[3]);
            R.SecondArchetype = FCString::Strlen(Cells[6]) ? FName(Cells[6]) : NAME_None;
            R.SecondSlot = Cells[8];
            R.SupportId = FName(Cells[9]);
            R.SupportQuantity = FCString::Atoi(Cells[10]);
            R.PendingSupport = Cells[11];
            R.DesignNote = Cells[14];
            if (R.KitId != FString::Printf(TEXT("Kit_%s_%s"), Cells[0], Cells[1]) ||
                UniqueKitIds.Contains(R.KitId) ||
                R.MainArchetype.IsNone() || FCString::Atoi(Cells[4]) != 1 ||
                FCString::Strcmp(Cells[5], TEXT("MainHand")) != 0 ||
                (R.SecondArchetype.IsNone() != (FCString::Atoi(Cells[7]) == 0)) ||
                (R.SecondArchetype.IsNone() != (R.SecondSlot == TEXT("None"))) ||
                (R.SecondSlot != TEXT("None") && R.SecondSlot != TEXT("Bag") &&
                 R.SecondSlot != TEXT("OffHand")) ||
                (R.SupportId.IsNone() != (R.SupportQuantity == 0)) ||
                FCString::Strcmp(Cells[13], TEXT("ApprovedDesign")) != 0)
            {
                Error = FString::Printf(TEXT("Invalid approved recipe %s."), *R.KitId);
                return false;
            }
            UniqueKitIds.Add(R.KitId);
        }
        if (Out.Num() != 18)
        {
            Error = FString::Printf(TEXT("Expected 18 recipes, found %d."), Out.Num());
            return false;
        }
        return true;
    }

    FResolved ResolveWeapon(FName Archetype, EGridEquipmentSlot Slot,
        const TArray<FGridWeaponPipelineRow>& Manifest,
        const FGridCanonicalRegistryFixture& Fixture)
    {
        FResolved Result;
        const FGridWeaponPipelineRow* Spec =
            FGridWeaponPipelineManifest::Find(Manifest, Archetype.ToString());
        if (!Spec || Spec->ProductionState != TEXT("Active"))
        {
            Result.Reason = TEXT("NOT_ACTIVE:") + Archetype.ToString();
            return Result;
        }
        const FName CanonicalId(*Spec->ItemDefinitionId);
        if (!Fixture.Resolve(CanonicalId, 1, Result.Profile) ||
            !Result.Profile.bIsArmament ||
            Result.Profile.ArchetypeId != Archetype)
        {
            Result.Reason = TEXT("INVALID_CANONICAL_N1:") + Archetype.ToString();
            return Result;
        }
        if (Slot != EGridEquipmentSlot::None &&
            (!Result.Profile.Weapon.CanEquipInHand(Slot) ||
             (!Spec->IsPassive() && !Result.Profile.Weapon.HasAttackSourceInHand(Slot))))
        {
            Result.Reason = TEXT("INVALID_CANONICAL_HAND_ACTION:") + Archetype.ToString();
            return Result;
        }
        Result.ItemId = CanonicalId;
        return Result;
    }

    bool ResolveSupport(FName ItemId, int32 MinimumStack,
        const FGridCanonicalRegistryFixture& Fixture, FResolved& Out)
    {
        Out = FResolved();
        if (ItemId.IsNone() || !Fixture.Resolve(ItemId, 0, Out.Profile) ||
            Out.Profile.bIsArmament || Out.Profile.MaxStack < MinimumStack)
        {
            return false;
        }
        Out.ItemId = ItemId;
        return true;
    }

    void AddEntry(FRPGStartingEquipmentKit& Kit, FName ItemId,
        EGridEquipmentSlot Slot, int32 Level, int32 Quantity)
    {
        FRPGStartingEquipmentEntry& Entry = Kit.Items.AddDefaulted_GetRef();
        Entry.ItemId = ItemId;
        Entry.TargetSlot = Slot;
        Entry.ItemLevel = Level;
        Entry.Quantity = Quantity;
        // Never load or duplicate DA_Weapon_*/DA_Item_*.
        // The real physical identity is the canonical ItemId + ItemLevel.
    }

    bool HasBagItem(const FGridCharacterInventoryState& Character,
        FName ItemId, int32 Quantity, int32 Level)
    {
        return Character.InventorySlots.ContainsByPredicate(
            [ItemId, Quantity, Level](const FGridInventorySlot& Slot)
            {
                return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemId &&
                    Slot.Item.Quantity == Quantity && Slot.Item.ItemLevel == Level;
            });
    }

    bool SimulateKit(const FRecipe& Recipe, const FResolved& Main,
        const FResolved& Second, const FResolved& Support,
        const FResolved& Ammunition,
        const FGridCanonicalRegistryFixture& Fixture, FString& Error)
    {
        const FString ClassPath = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            *Recipe.ClassId, *Recipe.ClassId);
        URPGClassAsset* ProductionClass = LoadObject<URPGClassAsset>(nullptr, *ClassPath);
        if (!IsValid(ProductionClass))
        {
            Error = TEXT("MISSING_CLASS");
            return false;
        }
        // No mutation of any production DataAsset: a transient test class
        // receives strictly physical ItemIds, never historical pointers.
        URPGClassAsset* TransientClass =
            DuplicateObject<URPGClassAsset>(ProductionClass, GetTransientPackage());
        if (!IsValid(TransientClass))
        {
            Error = TEXT("CLASS_DUPLICATE_FAILED");
            return false;
        }
        FRPGStartingEquipmentKit& Kit =
            TransientClass->StartingEquipmentKits.AddDefaulted_GetRef();
        Kit.KitId = FName(*Recipe.KitId);
        Kit.DisplayName = FText::FromString(Recipe.KitId);
        AddEntry(Kit, Main.ItemId, EGridEquipmentSlot::MainHand, 1, 1);
        if (Second.IsValid())
        {
            const EGridEquipmentSlot Slot = Recipe.SecondSlot == TEXT("OffHand")
                ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::None;
            AddEntry(Kit, Second.ItemId, Slot, 1, 1);
        }
        if (Support.IsValid())
            AddEntry(Kit, Support.ItemId, EGridEquipmentSlot::None, 0, Recipe.SupportQuantity);
        if (Ammunition.IsValid())
            AddEntry(Kit, Ammunition.ItemId, EGridEquipmentSlot::None, 0, 12);

        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid();
        Character.InventorySlots.SetNum(24);
        FGridCharacterEquipmentState Equipment;
        TArray<UGridItemDefinitionAsset*> HistoricalDefinitions;
        if (!FRPGStartingEquipmentService::TryBuildKit(
                TransientClass, Kit.KitId, Character, Equipment,
                HistoricalDefinitions, Error, Fixture.Registry.Get()))
        {
            return false;
        }
        if (Character.StartingEquipmentKitId != Kit.KitId ||
            Equipment.MainHand.ItemDefinitionId != Main.ItemId ||
            Equipment.MainHand.ItemLevel != 1)
        {
            Error = TEXT("GRANTED_CANONICAL_MAIN_OR_KIT_MISMATCH");
            return false;
        }
        if (Second.IsValid())
        {
            if (Recipe.SecondSlot == TEXT("OffHand"))
            {
                if (Equipment.OffHand.ItemDefinitionId != Second.ItemId ||
                    Equipment.OffHand.ItemLevel != 1)
                {
                    Error = TEXT("GRANTED_CANONICAL_OFFHAND_MISMATCH");
                    return false;
                }
            }
            else if (!HasBagItem(Character, Second.ItemId, 1, 1))
            {
                Error = TEXT("GRANTED_CANONICAL_BAG_WEAPON_MISMATCH");
                return false;
            }
        }
        if ((Support.IsValid() &&
             !HasBagItem(Character, Support.ItemId, Recipe.SupportQuantity, 0)) ||
            (Ammunition.IsValid() &&
             !HasBagItem(Character, Ammunition.ItemId, 12, 0)))
        {
            Error = TEXT("GRANTED_CANONICAL_SUPPORT_OR_AMMO_MISMATCH");
            return false;
        }
        if (!HistoricalDefinitions.IsEmpty())
        {
            Error = TEXT("CANONICAL_KIT_RESURRECTED_LEGACY_DEFINITIONS");
            return false;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT02Readiness,
    "Grimrock.RPG.ARMEMENT.LOT02.ApprovedKitsProductionGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT02Readiness::RunTest(const FString&)
{
    using namespace ARMEMENTLOT02;
    TArray<FRecipe> Recipes;
    FString Error;
    if (!TestTrue(TEXT("18 approved recipes load without mutation"),
        ReadRecipes(Recipes, Error)))
    {
        AddError(Error);
        return false;
    }
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production ItemSystem is available"), Fixture.bReady))
        return false;
    TArray<FGridWeaponPipelineRow> Manifest;
    if (!TestTrue(TEXT("Production manifest loaded"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
    {
        AddError(Error);
        return false;
    }

    TArray<FString> Report;
    Report.Add(TEXT("ClassId,KitId,Primary,Secondary,Support,Status,Reason"));
    TSet<FString> Candidates;
    int32 Blocked = 0, Simulated = 0;

    for (const FRecipe& Recipe : Recipes)
    {
        FResolved Main = ResolveWeapon(Recipe.MainArchetype,
            EGridEquipmentSlot::MainHand, Manifest, Fixture);
        const EGridEquipmentSlot SecondSlot =
            Recipe.SecondSlot == TEXT("OffHand")
            ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::None;
        FResolved Second = Recipe.SecondArchetype.IsNone()
            ? FResolved() : ResolveWeapon(
                Recipe.SecondArchetype, SecondSlot, Manifest, Fixture);
        FResolved Support, Ammunition;
        FString Reason;
        if (!Main.IsValid()) Reason = Main.Reason;
        else if (!Recipe.SecondArchetype.IsNone() && !Second.IsValid()) Reason = Second.Reason;
        else if (Recipe.PendingSupport != TEXT("None"))
            Reason = TEXT("PENDING_SUPPORT:") + Recipe.PendingSupport;
        else if (Recipe.DesignNote == TEXT("AmmoSupplyMustBeVerified"))
        {
            if (!ResolveSupport(TEXT("Item_Arrows_01"), 12, Fixture, Ammunition) ||
                Main.Profile.Weapon.RequiredAmmoKindId != Ammunition.Profile.AmmoKindId)
            {
                Reason = TEXT("CANONICAL_AMMO_NOT_VERIFIED");
            }
        }
        else if (!Recipe.SupportId.IsNone() &&
                 !ResolveSupport(Recipe.SupportId, Recipe.SupportQuantity, Fixture, Support))
        {
            Reason = TEXT("CANONICAL_SUPPORT_UNAVAILABLE:") + Recipe.SupportId.ToString();
        }
        if (Reason.IsEmpty() &&
            !SimulateKit(Recipe, Main, Second, Support, Ammunition, Fixture, Reason))
        {
            Reason = TEXT("CANONICAL_TRANSACTION_REJECTED:") + Reason;
        }

        const FString Status = Reason.IsEmpty()
            ? TEXT("CANDIDATE_FOR_PIE") : TEXT("BLOCKED");
        if (Reason.IsEmpty())
        {
            ++Simulated;
            Candidates.Add(Recipe.KitId);
            Reason = TEXT("CanonicalTransientTransactionPassed;RuntimeNotEnabled");
        }
        else
        {
            ++Blocked;
        }
        const TArray<FString> Columns = {
            Csv(Recipe.ClassId), Csv(Recipe.KitId),
            Csv(Recipe.MainArchetype.ToString()),
            Csv(Recipe.SecondArchetype.ToString()),
            Csv(Recipe.SupportId.ToString()), Csv(Status), Csv(Reason)
        };
        Report.Add(FString::Join(Columns, TEXT(",")));
        AddInfo(FString::Printf(TEXT("[LOT02] %s Status=%s Reason=%s"),
            *Recipe.KitId, *Status, *Reason));
    }

    const TCHAR* ExpectedCandidates[] = {
        TEXT("Kit_Warrior_Guardian"),
        TEXT("Kit_Warrior_Breaker"),
        TEXT("Kit_Warrior_WeaponMaster"),
        TEXT("Kit_Rogue_Shadow"),
        TEXT("Kit_Priest_Protection"),
        TEXT("Kit_Ranger_Marksman")
    };
    for (const TCHAR* Id : ExpectedCandidates)
    {
        TestTrue(*FString::Printf(TEXT("%s retains a native transient playtest candidate"), Id),
            Candidates.Contains(Id));
    }
    // The CSV still records the Saboteur disarm-tool design dependency:
    // native tooling does not silently rewrite an approved design manifest.
    TestFalse(TEXT("Saboteur pending support must not silently disappear"),
        Candidates.Contains(TEXT("Kit_Rogue_Saboteur")));
    TestTrue(TEXT("Marksman arrow kind and real bow link are resolved from ItemSystem"),
        Candidates.Contains(TEXT("Kit_Ranger_Marksman")));
    TestTrue(TEXT("At least the six approved baseline kits remain playable"), Candidates.Num() >= 6);
    TestTrue(TEXT("Unresolved design needs still block some of the eighteen"), Candidates.Num() < 18);

    const FString Folder = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("ARMEMENT-LOT02"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Path = FPaths::Combine(Folder, TEXT("LOT02_ApprovedKitsReadiness.csv"));
    TestTrue(TEXT("Write canonical 18-kit matrix under Saved"),
        FFileHelper::SaveStringArrayToFile(Report, *Path,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    TestEqual(TEXT("Exactly 18 kits evaluated"), Simulated + Blocked, 18);
    AddInfo(FString::Printf(
        TEXT("[LOT02] Recipes=18 CanonicalCandidates=%d Blocked=%d RuntimeActivated=0 Report=%s"),
        Simulated, Blocked, *Path));
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
