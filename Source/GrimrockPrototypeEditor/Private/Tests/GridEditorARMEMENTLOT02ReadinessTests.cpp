#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "Serialization/Csv/CsvParser.h"
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
        UGridItemDefinitionAsset* Item = nullptr;
        FString Reason;
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

    FResolved ResolveWeapon(
        FName Archetype, EGridEquipmentSlot Slot,
        const TArray<FGridWeaponPipelineRow>& Manifest)
    {
        FResolved Result;
        const FGridWeaponPipelineRow* Spec =
            FGridWeaponPipelineManifest::Find(Manifest, Archetype.ToString());
        if (!Spec || Spec->ProductionState != TEXT("Active"))
        {
            Result.Reason = TEXT("NOT_ACTIVE:") + Archetype.ToString();
            return Result;
        }
        UGridItemDefinitionAsset* Item =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Spec->ItemPath);
        if (!IsValid(Item) || !Item->IsValidDefinition() ||
            Item->GetPathName() != Spec->ItemPath ||
            Item->ItemDefinitionId != FName(*Spec->ItemDefinitionId) ||
            Item->WeaponArchetypeId != Archetype ||
            !Item->bUseWeaponCatalogue || Item->DefaultWeaponItemLevel != 1)
        {
            Result.Reason = TEXT("INVALID_ITEM:") + Archetype.ToString();
            return Result;
        }
        const FGridWeaponLevelProfile* N1 = nullptr;
        if (!Item->TryGetWeaponCatalogueProfile(1, N1) || !N1 ||
            !N1->IsValidProfile())
        {
            Result.Reason = TEXT("INVALID_N1:") + Archetype.ToString();
            return Result;
        }
        if (Slot != EGridEquipmentSlot::None &&
            (!Item->CanEquipToSlot(Slot) ||
             (!Spec->IsPassive() && !Item->CanProvideAttackFromSlot(Slot))))
        {
            Result.Reason = TEXT("INVALID_HAND_ACTION:") + Archetype.ToString();
            return Result;
        }
        Result.Item = Item;
        return Result;
    }

    void AddEntry(FRPGStartingEquipmentKit& Kit,
        UGridItemDefinitionAsset* Definition, EGridEquipmentSlot Slot,
        int32 Level, int32 Quantity)
    {
        FRPGStartingEquipmentEntry& Entry = Kit.Items.AddDefaulted_GetRef();
        Entry.ItemDefinition = Definition;
        Entry.TargetSlot = Slot;
        Entry.ItemLevel = Level;
        Entry.Quantity = Quantity;
    }

    bool SimulateKit(const FRecipe& Recipe,
        UGridItemDefinitionAsset* Main,
        UGridItemDefinitionAsset* Second,
        UGridItemDefinitionAsset* Support,
        UGridItemDefinitionAsset* Ammunition,
        FString& Error)
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
        // Never mutate the six production class DataAssets or legacy kits.
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
        Kit.DisplayName = FText::FromString(Recipe.KitId); // Test-only, not UI localization.
        AddEntry(Kit, Main, EGridEquipmentSlot::MainHand, 1, 1);
        if (Second)
        {
            const EGridEquipmentSlot Slot = Recipe.SecondSlot == TEXT("OffHand")
                ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::None;
            AddEntry(Kit, Second, Slot, 1, 1);
        }
        if (Support)
        {
            AddEntry(Kit, Support, EGridEquipmentSlot::None, 0,
                Recipe.SupportQuantity);
        }
        if (Ammunition)
        {
            AddEntry(Kit, Ammunition, EGridEquipmentSlot::None, 0, 12);
        }

        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid();
        Character.InventorySlots.SetNum(24);
        FGridCharacterEquipmentState Equipment;
        TArray<UGridItemDefinitionAsset*> Definitions;
        if (!FRPGStartingEquipmentService::TryBuildKit(
                TransientClass, Kit.KitId, Character, Equipment,
                Definitions, Error))
        {
            return false;
        }
        if (Character.StartingEquipmentKitId != Kit.KitId ||
            Equipment.MainHand.ItemDefinitionId != Main->ItemDefinitionId)
        {
            Error = TEXT("GRANTED_MAIN_OR_KIT_ID_MISMATCH");
            return false;
        }
        if (Second && Recipe.SecondSlot == TEXT("OffHand") &&
            Equipment.OffHand.ItemDefinitionId != Second->ItemDefinitionId)
        {
            Error = TEXT("GRANTED_OFFHAND_MISMATCH");
            return false;
        }
        if (Ammunition &&
            !Character.InventorySlots.ContainsByPredicate([Ammunition](const FGridInventorySlot& Slot)
            {
                return !Slot.IsEmpty() &&
                    Slot.Item.ItemDefinitionId == Ammunition->ItemDefinitionId &&
                    Slot.Item.Quantity == 12;
            }))
        {
            Error = TEXT("STARTER_AMMUNITION_NOT_GRANTED");
            return false;
        }
        const int32 ExpectedDefinitions = 1 + (Second ? 1 : 0) + (Support ? 1 : 0) +
            (Ammunition ? 1 : 0);
        if (Definitions.Num() != ExpectedDefinitions)
        {
            Error = TEXT("GRANTED_DEFINITION_COUNT_MISMATCH");
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
    if (!TestTrue(TEXT("18 approved recipes read without mutation"),
        ReadRecipes(Recipes, Error)))
    {
        AddError(Error);
        return false;
    }
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
            EGridEquipmentSlot::MainHand, Manifest);
        const EGridEquipmentSlot SecondSlot =
            Recipe.SecondSlot == TEXT("OffHand")
            ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::None;
        FResolved Second = Recipe.SecondArchetype.IsNone()
            ? FResolved() : ResolveWeapon(Recipe.SecondArchetype, SecondSlot, Manifest);
        UGridItemDefinitionAsset* Support = nullptr;
        UGridItemDefinitionAsset* Ammunition = nullptr;
        FString Reason;
        if (!Main.Item) Reason = Main.Reason;
        else if (!Recipe.SecondArchetype.IsNone() && !Second.Item) Reason = Second.Reason;
        else if (Recipe.PendingSupport != TEXT("None"))
            Reason = TEXT("PENDING_SUPPORT:") + Recipe.PendingSupport;
        else if (Recipe.DesignNote == TEXT("AmmoSupplyMustBeVerified"))
        {
            Ammunition = FRPGStartingEquipmentPresetCatalog::
                ResolveKnownStartingItemDefinition(TEXT("Item_Arrows_01"));
            if (!IsValid(Ammunition) || !Ammunition->IsValidDefinition() ||
                !Ammunition->bStackable || Ammunition->MaxStackSize < 12 ||
                !IsValid(Main.Item) ||
                Main.Item->RequiredAmmunitionItemDefinitionId != Ammunition->ItemDefinitionId)
            {
                Reason = TEXT("AMMO_NOT_VERIFIED");
            }
        }
        else if (!Recipe.SupportId.IsNone())
        {
            Support = FRPGStartingEquipmentPresetCatalog::
                ResolveKnownStartingItemDefinition(Recipe.SupportId);
            if (!IsValid(Support) || !Support->IsValidDefinition())
                Reason = TEXT("SUPPORT_ITEM_UNAVAILABLE:") + Recipe.SupportId.ToString();
        }
        if (Reason.IsEmpty() &&
            !SimulateKit(Recipe, Main.Item, Second.Item, Support, Ammunition, Reason))
        {
            Reason = TEXT("TRANSACTION_REJECTED:") + Reason;
        }

        const FString Status = Reason.IsEmpty()
            ? TEXT("CANDIDATE_FOR_PIE") : TEXT("BLOCKED");
        if (Reason.IsEmpty())
        {
            ++Simulated;
            Candidates.Add(Recipe.KitId);
            Reason = TEXT("TransientTransactionPassed;RuntimeNotEnabled");
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

    // LOT02.5: five pre-existing candidates plus Marksman with real arrows.
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
        TestTrue(*FString::Printf(TEXT("%s is a transient playtest candidate"), Id),
            Candidates.Contains(Id));
    }
    TestFalse(TEXT("Saboteur cannot silently omit its planned disarming tool"),
        Candidates.Contains(TEXT("Kit_Rogue_Saboteur")));
    TestTrue(TEXT("Marksman ammo package and bow link explicitly verified"),
        Candidates.Contains(TEXT("Kit_Ranger_Marksman")));
    TestEqual(TEXT("Exactly six approved playable-kit candidates"), Candidates.Num(), 6);

    const FString Folder = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("ARMEMENT-LOT02"));
    IFileManager::Get().MakeDirectory(*Folder, true);
    const FString Path = FPaths::Combine(Folder, TEXT("LOT02_ApprovedKitsReadiness.csv"));
    TestTrue(TEXT("Write read-only 18-kit matrix under Saved"),
        FFileHelper::SaveStringArrayToFile(Report, *Path,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));

    TestEqual(TEXT("Exactly 18 kits evaluated"), Simulated + Blocked, 18);
    AddInfo(FString::Printf(
        TEXT("[LOT02] Recipes=18 TransientCandidates=%d Blocked=%d RuntimeActivated=0 Report=%s"),
        Simulated, Blocked, *Path));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
