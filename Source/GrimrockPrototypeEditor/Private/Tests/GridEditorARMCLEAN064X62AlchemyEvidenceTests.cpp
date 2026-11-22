#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

// Final-state alchemy evidence. No old source DataAssets are required.
// The real 48-item root plus the action catalogue is the only authority.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X62AlchemyEvidence,
    "Grimrock.RPG.ARM.CLEAN06_4X62.NativeAlchemyEvidenceReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorARMCLEAN064X62AlchemyEvidence::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical ItemSystem available"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Native production root"), Root) ||
        !TestNotNull(TEXT("Native detailed action catalogue"), Root->ActionCatalogue.Get()) ||
        !TestNotNull(TEXT("Native presentation catalogue"), Root->ActionPresentationCatalogue.Get()))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bActionsDirty = Root->ActionCatalogue->GetOutermost()->IsDirty();
    const bool bPresentationDirty = Root->ActionPresentationCatalogue->GetOutermost()->IsDirty();
    const FName Ids[] = {
        TEXT("Item_Antidote"), TEXT("Item_Bomb_Fire"), TEXT("Item_Bomb_Toxic"),
        TEXT("Item_DefensiveElixir_Fire"), TEXT("Item_DefensiveElixir_Ice"),
        TEXT("Item_DefensiveElixir_Lightning"), TEXT("Item_DefensiveElixir_Poison"),
        TEXT("Item_Flask_Acid"), TEXT("Item_Flask_CorrosiveCloud"), TEXT("Item_Panacea"),
        TEXT("Item_Flask_Oil")
    };
    TSet<FName> SeenActionIds;
    int32 Resolved = 0;
    for (FName ItemId : Ids)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Exactly one real N0 owner identity"),
            Fixture.Resolve(ItemId, 0, Item))) continue;
        TestTrue(TEXT("Alchemy item is not an armament"),
            !Item.bIsArmament && Item.ItemId == ItemId);
        if (!TestEqual(TEXT("One native explicit QuickItem action binding"),
            Item.AvailableActions.Num(), 1)) continue;
        const FName ActionId = Item.AvailableActions[0].ActionId;
        TestFalse(TEXT("One source-specific action ID per alchemy item"),
            SeenActionIds.Contains(ActionId));
        SeenActionIds.Add(ActionId);
        FGridCombatActionDefinition Detailed;
        if (!TestTrue(TEXT("Native QuickItem mechanics resolve from bound ActionId"),
            Fixture.Registry->ResolveDetailedNonArmAction(ItemId, 0, ActionId, Detailed)))
            continue;
        ++Resolved;
        TestEqual(TEXT("Exact persisted N0 source action identity"),
            Detailed.ActionId, ActionId);
        TestEqual(TEXT("QuickItem execution policy retained"),
            Detailed.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
        TestEqual(TEXT("One physical alchemy source consumed"),
            Detailed.ResourceCosts.SourceItemQuantityCost, 1);
        TestEqual(TEXT("No invented N1 progression"), Item.ItemLevel, 0);
        FGridCombatActionDefinition Wrong;
        TestFalse(TEXT("Item cannot borrow an unrelated action"),
            Fixture.Registry->ResolveDetailedNonArmAction(
                ItemId, 0, TEXT("Not_An_Alchemy_Action"), Wrong));
    }
    TestEqual(TEXT("All eleven source-specific action bindings"), Resolved, 11);
    TestEqual(TEXT("No shared collision remains for eleven alchemy items"), SeenActionIds.Num(), 11);
    TestEqual(TEXT("Exactly fourteen native detailed actions"), Root->ActionCatalogue->DetailedActions.Num(), 14);

    UGridActionCatalogueAsset* Duplicate = DuplicateObject<UGridActionCatalogueAsset>(
        Root->ActionCatalogue.Get(), GetTransientPackage());
    if (!TestNotNull(TEXT("Private action catalogue clone"), Duplicate)) return false;
    FGridRuntimeAction Conflict = Duplicate->DetailedActions[0];
    Conflict.Definition.ActionPointCost += 1;
    Duplicate->DetailedActions.Add(Conflict);
    TArray<FString> Errors;
    TestFalse(TEXT("Same ActionId with changed mechanics is rejected"),
        Duplicate->Validate(Errors));
    TestTrue(TEXT("Action collision generates a validation reason"),
        !Errors.IsEmpty());
    TestEqual(TEXT("Production root never dirtied"), Root->GetOutermost()->IsDirty(), bRootDirty);
    TestEqual(TEXT("Production action catalogue never dirtied"),
        Root->ActionCatalogue->GetOutermost()->IsDirty(), bActionsDirty);
    TestEqual(TEXT("Production presentation catalogue never dirtied"),
        Root->ActionPresentationCatalogue->GetOutermost()->IsDirty(), bPresentationDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
