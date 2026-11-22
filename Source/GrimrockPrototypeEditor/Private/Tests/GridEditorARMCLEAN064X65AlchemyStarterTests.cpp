#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "RPG/RPGStartingEquipmentService.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X65AlchemyStarterAuthoring,
    "Grimrock.RPG.ARM.CLEAN06_4X65.CanonicalAlchemyStartersAndAuthoringGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorARMCLEAN064X65AlchemyStarterAuthoring::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real installed canonical ItemSystem"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real 48-item canonical root"), Root)) return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    TArray<FName> Pending;
    FString Error;
    TestTrue(TEXT("Authoring guard accepts complete single registry"),
        FRPGAlchemistAuthoring::PlanUnregisteredItems(*Fixture.Registry.Get(), Pending, Error));
    TestTrue(TEXT("Nothing historical may be reauthored after cutover"), Pending.IsEmpty());

    UGridItemSystemAsset* Partial =
        DuplicateObject<UGridItemSystemAsset>(Root, GetTransientPackage());
    if (!TestNotNull(TEXT("Private incomplete-registry fixture"), Partial)) return false;
    Partial->ItemDefinitions.Pop();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> PartialRegistry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (TestTrue(TEXT("Partial root can be inspected without modifying production"),
        PartialRegistry->RegisterItemSystem(Partial, Errors)))
    {
        TestFalse(TEXT("Nine-of-ten alchemy item state fails closed"),
            FRPGAlchemistAuthoring::PlanUnregisteredItems(
                *PartialRegistry.Get(), Pending, Error));
        TestTrue(TEXT("No partial historical writer plan emitted"), Pending.IsEmpty());
    }

    URPGClassAsset* NativeClass = LoadObject<URPGClassAsset>(
        nullptr, FRPGAlchemistAuthoring::AlchemistAssetPath());
    if (!TestNotNull(TEXT("Real Alchemist class"), NativeClass) ||
        !TestTrue(TEXT("Class does not store duplicate starting kit instances"),
            NativeClass->StartingEquipmentKits.IsEmpty()))
        return false;
    const bool bClassDirty = NativeClass->GetOutermost()->IsDirty();
    URPGClassAsset* Class = DuplicateObject<URPGClassAsset>(
        NativeClass, GetTransientPackage());
    if (!TestNotNull(TEXT("Transient class copy"), Class) ||
        !TestTrue(TEXT("All alchemy starter kits from canonical registry"),
            FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
                *Class, Error, Fixture.Registry.Get())))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Three approved alchemist orientations"), Class->StartingEquipmentKits.Num(), 3);
    const FName KitIds[] = {TEXT("Kit_Alchemist_Grenadier"), TEXT("Kit_Alchemist_Transmuter")};
    const FName ItemIds[] = {TEXT("Item_Bomb_Fire"), TEXT("Item_Flask_Oil")};
    for (int32 I = 0; I < 2; ++I)
    {
        const FRPGStartingEquipmentKit* Kit = Class->FindStartingEquipmentKit(KitIds[I]);
        if (!TestNotNull(TEXT("Canonical approved kit"), Kit)) continue;
        const FRPGStartingEquipmentEntry* Entry = Kit->Items.FindByPredicate(
            [&](const FRPGStartingEquipmentEntry& Row)
            {
                return Row.CanonicalItemId == ItemIds[I];
            });
        if (!TestNotNull(TEXT("Explicit canonical consumable identity"), Entry)) continue;
        TestNull(TEXT("No historical pointer in approved starter"), Entry->ItemDefinition.Get());
        TestEqual(TEXT("Physical N0 starter"), Entry->ItemLevel, 0);
        TestEqual(TEXT("One starter consumable"), Entry->Quantity, 1);
        TestEqual(TEXT("Bag target only"), Entry->TargetSlot, EGridEquipmentSlot::None);
        FGridCharacterInventoryState Character;
        Character.CharacterId = FGuid::NewGuid();
        Character.InventorySlots.SetNum(12);
        FGridCharacterEquipmentState Equipment;
        TArray<UGridItemDefinitionAsset*> Definitions;
        if (!TestTrue(TEXT("Native kit builder uses item identity"),
            FRPGStartingEquipmentService::TryBuildKit(
                Class, KitIds[I], Character, Equipment,
                Definitions, Error, Fixture.Registry.Get())))
        {
            AddError(Error);
            continue;
        }
        const FGridInventorySlot* Slot = Character.InventorySlots.FindByPredicate(
            [&](const FGridInventorySlot& Row)
            {
                return !Row.IsEmpty() && Row.Item.ItemDefinitionId == ItemIds[I];
            });
        if (TestNotNull(TEXT("Real canonical N0 in starter bag"), Slot))
        {
            TestEqual(TEXT("Quantity persisted in physical item"), Slot->Item.Quantity, 1);
            TestEqual(TEXT("N0 level retained"), Slot->Item.ItemLevel, 0);
            TestEqual(TEXT("Correct character owns starter"), Slot->Item.OwnerGuid, Character.CharacterId);
        }
        TestFalse(TEXT("No historical definition for this canonical consumable"),
            Definitions.ContainsByPredicate([&](const UGridItemDefinitionAsset* Definition)
            {
                return Definition && Definition->ItemDefinitionId == ItemIds[I];
            }));
    }
    TestEqual(TEXT("No production class asset mutation"),
        NativeClass->GetOutermost()->IsDirty(), bClassDirty);
    TestEqual(TEXT("No production registry mutation"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
