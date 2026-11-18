#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorAlchemyCutoverPlan.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "RPG/RPGStartingEquipmentService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X65AlchemyStarterAuthoring,
    "Grimrock.RPG.ARM.CLEAN06_4X65.CanonicalAlchemyStartersAndAuthoringGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X65AlchemyStarterAuthoring::RunTest(const FString&)
{
    namespace Plan = GridEditorAlchemyCutoverPlan;
    namespace Audit = GridEditorAlchemyMigrationAudit;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, Audit::RootPath()));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    if (!TestNotNull(TEXT("Native baseline"), Root.Get())) return false;
    FString Error;
    if (!TestTrue(TEXT("Joint alchemy preview"), Plan::Build(*Root.Get(), *Preview.Get(), Error)))
    {
        AddError(Error);
        return false;
    }
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Before(NewObject<UGridItemSubsystem>(GI.Get()));
    TStrongObjectPtr<UGridItemSubsystem> After(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Real pre-cutover registry"), Before->RegisterItemSystem(Root.Get(), Errors)) ||
        !TestTrue(TEXT("Isolated post-cutover registry"), After->RegisterItemSystem(Preview.Get(), Errors)))
        return false;

    TArray<FName> Pending;
    TestTrue(TEXT("Historical authoring guard accepts source state"), FRPGAlchemistAuthoring::PlanUnregisteredItems(*Before.Get(), Pending, Error));
    TestEqual(TEXT("Exactly ten unregistered items before native cutover"), Pending.Num(), 10);
    TestFalse(TEXT("Already canonical Oil must never be reauthored as legacy"), Pending.Contains(TEXT("Item_Flask_Oil")));
    TestTrue(TEXT("Guard accepts complete target state"), FRPGAlchemistAuthoring::PlanUnregisteredItems(*After.Get(), Pending, Error));
    TestTrue(TEXT("No legacy item authoring after cutover"), Pending.IsEmpty());

    TStrongObjectPtr<UGridItemSystemAsset> Partial(NewObject<UGridItemSystemAsset>());
    Audit::CopyRoot(*Preview.Get(), *Partial.Get());
    Partial->ItemDefinitions.Pop(); // Valid resolver, but nine of ten new alchemy items.
    TStrongObjectPtr<UGridItemSubsystem> PartialRegistry(NewObject<UGridItemSubsystem>(GI.Get()));
    if (!TestTrue(TEXT("Partial root can register mechanically"), PartialRegistry->RegisterItemSystem(Partial.Get(), Errors))) return false;
    TestFalse(TEXT("Legacy authoring refuses mixed state"), FRPGAlchemistAuthoring::PlanUnregisteredItems(*PartialRegistry.Get(), Pending, Error));
    TestTrue(TEXT("No partial authoring plan returned"), Pending.IsEmpty());

    auto* NativeClass = LoadObject<URPGClassAsset>(nullptr, FRPGAlchemistAuthoring::AlchemistAssetPath());
    if (!TestNotNull(TEXT("Real alchemist class"), NativeClass) ||
        !TestTrue(TEXT("Production presets use an unconfigured class"), NativeClass->StartingEquipmentKits.IsEmpty()))
        return false;
    TStrongObjectPtr<URPGClassAsset> Class(DuplicateObject<URPGClassAsset>(NativeClass, GetTransientPackage()));
    if (!TestTrue(TEXT("Approved transient kits materialize from joint registry"),
        FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Class.Get(), Error, After.Get())))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Three approved orientations"), Class->StartingEquipmentKits.Num(), 3);
    const FName KitIds[] = {TEXT("Kit_Alchemist_Grenadier"), TEXT("Kit_Alchemist_Transmuter")};
    const FName ItemIds[] = {TEXT("Item_Bomb_Fire"), TEXT("Item_Flask_Oil")};
    for (int32 I = 0; I < 2; ++I)
    {
        const auto* Kit = Class->FindStartingEquipmentKit(KitIds[I]);
        if (!TestNotNull(TEXT("Approved kit"), Kit)) return false;
        const auto* Entry = Kit->Items.FindByPredicate([&](const FRPGStartingEquipmentEntry& Row)
            { return Row.CanonicalItemId == ItemIds[I]; });
        if (!TestNotNull(TEXT("Explicit canonical alchemy entry"), Entry)) return false;
        TestNull(TEXT("No historical item pointer"), Entry->ItemDefinition.Get());
        TestEqual(TEXT("Exact physical N0"), Entry->ItemLevel, 0);
        TestEqual(TEXT("One starter consumable"), Entry->Quantity, 1);
        TestTrue(TEXT("Consumable targets bag"), Entry->TargetSlot == EGridEquipmentSlot::None);

        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid();
        Character.InventorySlots.SetNum(12);
        FGridCharacterEquipmentState Equipment;
        TArray<UGridItemDefinitionAsset*> Definitions;
        if (!TestTrue(TEXT("Production kit builder accepts pointerless consumable"),
                FRPGStartingEquipmentService::TryBuildKit(Class.Get(), KitIds[I], Character, Equipment, Definitions, Error, After.Get())))
        {
            AddError(Error);
            return false;
        }
        const auto* Slot = Character.InventorySlots.FindByPredicate([&](const FGridInventorySlot& Row)
            { return !Row.IsEmpty() && Row.Item.ItemDefinitionId == ItemIds[I]; });
        if (!TestNotNull(TEXT("Actual new item in bag"), Slot)) return false;
        TestEqual(TEXT("Quantity retained"), Slot->Item.Quantity, 1);
        TestEqual(TEXT("N0 retained"), Slot->Item.ItemLevel, 0);
        TestEqual(TEXT("Owned by correct character"), Slot->Item.OwnerGuid, Character.CharacterId);
        TestFalse(TEXT("No historical definition registered for the canonical starter"), Definitions.ContainsByPredicate(
            [&](const UGridItemDefinitionAsset* D) { return D && D->ItemDefinitionId == ItemIds[I]; }));
    }
    TestTrue(TEXT("Real class remains unchanged"), NativeClass->StartingEquipmentKits.IsEmpty() && !NativeClass->GetOutermost()->IsDirty());
    TestFalse(TEXT("Native canonical root not modified"), Root->GetOutermost()->IsDirty());
    return !HasAnyErrors();
}
#endif
