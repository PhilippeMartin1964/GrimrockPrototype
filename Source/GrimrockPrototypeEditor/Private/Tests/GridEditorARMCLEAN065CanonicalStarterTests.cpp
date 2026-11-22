#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065CanonicalStarterKits,
    "Grimrock.RPG.ARM.CLEAN06_5C.AllApprovedKitsCanonical",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065CanonicalStarterKits::RunTest(const FString&)
{
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    TArray<FString> Errors;
    if (!TestNotNull(TEXT("Persisted item system"), Root.Get()) ||
        !TestTrue(TEXT("All 48 definitions registered"), Root->ItemDefinitions.Num() == 48 &&
            Registry->RegisterItemSystem(Root.Get(), Errors))) return false;
    TStrongObjectPtr<UGridItemSubsystem> Empty(NewObject<UGridItemSubsystem>(GI.Get()));
    const TCHAR* Classes[] = {TEXT("Warrior"), TEXT("Rogue"), TEXT("Ranger"), TEXT("Mage"), TEXT("Priest"), TEXT("Alchemist")};
    int32 NumKits = 0;
    for (const TCHAR* ClassId : Classes)
    {
        const FString Path = FString::Printf(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"), ClassId, ClassId);
        auto* Original = LoadObject<URPGClassAsset>(nullptr, *Path);
        if (!TestNotNull(TEXT("Production class"), Original)) return false;
        TStrongObjectPtr<URPGClassAsset> Reference(DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()));
        TStrongObjectPtr<URPGClassAsset> Canonical(DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()));
        TStrongObjectPtr<URPGClassAsset> Missing(DuplicateObject<URPGClassAsset>(Original, GetTransientPackage()));
        FString Error;
        if (!TestTrue(TEXT("Independent native reference recipes resolve"),
                FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Reference.Get(), Error, Registry.Get())) ||
            !TestTrue(TEXT("Canonical recipes resolve"), 
                FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Canonical.Get(), Error, Registry.Get())))
        { AddError(Error); return false; }
        TestFalse(TEXT("Unconfigured registry cannot borrow historical assets"),
            FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Missing.Get(), Error, Empty.Get()));
        TestTrue(TEXT("Failed materialization remains atomic"), Missing->StartingEquipmentKits.IsEmpty() &&
            Missing->DefaultStartingEquipmentKitId.IsNone());
        TestEqual(TEXT("Same default kit"), Canonical->DefaultStartingEquipmentKitId, Reference->DefaultStartingEquipmentKitId);
        if (!TestEqual(TEXT("Three approved kits"), Canonical->StartingEquipmentKits.Num(), 3)) return false;
        for (const auto& Kit : Canonical->StartingEquipmentKits)
        {
            const auto* Old = Reference->FindStartingEquipmentKit(Kit.KitId);
            if (!TestNotNull(TEXT("Same approved kit ID"), Old) ||
                !TestEqual(TEXT("Same number of entries"), Kit.Items.Num(), Old->Items.Num())) return false;
            FGridCharacterInventoryState Character;
            Character.CharacterId = FGuid::NewGuid();
            Character.InventorySlots.SetNum(12);
            FGridCharacterEquipmentState Equipment;
            TArray<UGridItemDefinitionAsset*> Definitions;
            if (!TestTrue(TEXT("Build actual canonical kit"), FRPGStartingEquipmentService::TryBuildKit(
                Canonical.Get(), Kit.KitId, Character, Equipment, Definitions, Error, Registry.Get())))
            { AddError(Error); return false; }
            TestTrue(TEXT("No legacy definitions returned or needed"), Definitions.IsEmpty());
            for (int32 I = 0; I < Kit.Items.Num(); ++I)
            {
                const auto& Entry = Kit.Items[I];
                const auto& Source = Old->Items[I];
                TestNull(TEXT("Independent native reference contains no old ItemDefinition"),
                    Source.ItemDefinition.Get());
                TestNull(TEXT("Native test entry has no obsolete ItemDefinition"),
                    Entry.ItemDefinition.Get());
                TestFalse(TEXT("Stable native ItemId is explicit"),
                    Entry.ItemId.IsNone());
                TestEqual(TEXT("Reference and native ItemId are identical"),
                    Entry.ItemId, Source.ItemId);
                TestEqual(TEXT("Both native projections preserve the same physical level"),
                    Entry.ItemLevel, Source.ItemLevel);
                TestEqual(TEXT("Identical quantity"), Entry.Quantity, Source.Quantity);
                TestTrue(TEXT("Identical equipment slot"), Entry.TargetSlot == Source.TargetSlot);
                const FGridItemInstance* Granted = nullptr;
                if (Entry.TargetSlot != EGridEquipmentSlot::None)
                    Granted = Equipment.GetSlot(Entry.TargetSlot);
                else
                {
                    const auto* Slot = Character.InventorySlots.FindByPredicate([&](const FGridInventorySlot& Row)
                        { return !Row.IsEmpty() && Row.Item.ItemDefinitionId == Entry.ItemId; });
                    if (Slot) Granted = &Slot->Item;
                }
                if (!TestNotNull(TEXT("Item actually granted"), Granted)) return false;
                TestTrue(TEXT("Grant preserves identity, owner, physical level and quantity"), Granted->IsValid() &&
                    Granted->ItemDefinitionId == Entry.ItemId && Granted->OwnerGuid == Character.CharacterId &&
                    Granted->ItemLevel == Entry.ItemLevel && Granted->Quantity == Entry.Quantity);
                FGridResolvedItem Resolved;
                if (!TestTrue(TEXT("Canonical display and combat profile resolve"),
                    Registry->ResolveItem(Entry.ItemId, Entry.ItemLevel, Resolved))) return false;
                if (Resolved.bIsArmament)
                    TestFalse(TEXT("Canonical weapon icon available"), Resolved.ItemIcon.IsNull());
            }
            ++NumKits;
        }
        TestTrue(TEXT("No production class mutation"), Original->StartingEquipmentKits.IsEmpty() && !Original->GetOutermost()->IsDirty());
    }
    TestEqual(TEXT("All eighteen approved kits"), NumKits, 18);
    TestFalse(TEXT("No root mutation"), Root->GetOutermost()->IsDirty());
    return !HasAnyErrors();
}
#endif
