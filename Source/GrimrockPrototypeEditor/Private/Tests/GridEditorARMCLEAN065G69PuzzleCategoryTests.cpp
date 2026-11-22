#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridPuzzleCategoryTags.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
    // Frozen parity contract from native report #208 and binary commit
    // b8a556dc. Test expectation only: never a runtime category source.
    const TMap<FName, FName>& ExpectedCategories()
    {
        static const TMap<FName, FName> Rows = {
            { TEXT("EmptyVial"), TEXT("Component") },
            { TEXT("Gem_Blue"), TEXT("Gem") },
            { TEXT("GoblinKnife"), TEXT("Weapon") },
            { TEXT("Item_Antidote"), TEXT("Potion") },
            { TEXT("Item_Arrows_01"), TEXT("Misc") },
            { TEXT("Item_Bomb_Fire"), TEXT("Misc") },
            { TEXT("Item_Bomb_Toxic"), TEXT("Misc") },
            { TEXT("Item_DefensiveElixir_Fire"), TEXT("Potion") },
            { TEXT("Item_DefensiveElixir_Ice"), TEXT("Potion") },
            { TEXT("Item_DefensiveElixir_Lightning"), TEXT("Potion") },
            { TEXT("Item_DefensiveElixir_Poison"), TEXT("Potion") },
            { TEXT("Item_DisarmTools_01"), TEXT("Misc") },
            { TEXT("Item_Flask_Acid"), TEXT("Misc") },
            { TEXT("Item_Flask_CorrosiveCloud"), TEXT("Misc") },
            { TEXT("Item_Flask_Oil"), TEXT("Misc") },
            { TEXT("Item_Panacea"), TEXT("Potion") },
            { TEXT("Item_Potion_Healing_01"), TEXT("Potion") },
            { TEXT("Item_RatMeat"), TEXT("Food") },
            { TEXT("Item_RatTooth"), TEXT("Component") },
            { TEXT("Item_SurfaceCatalyst_01"), TEXT("Misc") },
            { TEXT("Item_TestNote"), TEXT("Scroll") },
            { TEXT("Item_Torch"), TEXT("Torch") },
            { TEXT("Key_Copper"), TEXT("Key") },
            { TEXT("Key_Iron"), TEXT("Key") },
            { TEXT("Shuriken"), TEXT("Weapon") },
            { TEXT("Stone"), TEXT("Misc") },
        };
        return Rows;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorPuzzleCategoryPreviewTest,
    "Grimrock.Editor.ARM.CLEAN06.5G69.PuzzleCategoryPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorPuzzleCategoryPreviewTest::RunTest(const FString& Parameters)
{
    using namespace GridPuzzleCategoryTags;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Root.IsValid() || !Registry->RegisterItemSystem(Root.Get(), Errors))
    {
        AddError(FString::Join(Errors, TEXT("; ")));
        return false;
    }

    TestEqual(TEXT("All 48 native definitions"), Root->ItemDefinitions.Num(), 48);
    int32 Checked = 0, NativeTagged = 0, Arms = 0;
    TSet<FName> Seen;
    const TArray<FName> Types = { TEXT("Torch"), TEXT("Weapon"), TEXT("Shield"), TEXT("Armor"),
        TEXT("Jewelry"), TEXT("Key"), TEXT("Gem"), TEXT("Potion"), TEXT("Scroll"), TEXT("Book"),
        TEXT("Food"), TEXT("Component"), TEXT("Quest"), TEXT("Misc"), TEXT("None") };
    for (const auto& Entry : Root->ItemDefinitions)
    {
        if (!TestNotNull(TEXT("Native definition"), Entry.Get())) return false;
        const auto* Source = Entry.Get();
        TestFalse(TEXT("Native identity unique"), Seen.Contains(Source->ItemId));
        Seen.Add(Source->ItemId);
        const bool bArm = Source->IsA<UGridArmamentDefinitionAsset>();
        const FName* Type = ExpectedCategories().Find(Source->ItemId);
        if (!TestTrue(TEXT("Mapping exactly separates ARM and non-ARM"), bArm != (Type != nullptr))) return false;
        TestFalse(TEXT("Native package remains clean"), Source->GetOutermost()->IsDirty());
        const int32 Count = CountReserved(Source->Tags);
        TestEqual(TEXT("Every non-ARM has exactly one category, ARM none"), Count, bArm ? 0 : 1);
        NativeTagged += Count;
        if (bArm) ++Arms;
        if (Type)
            TestTrue(TEXT("Exact category from proven binary commit"), Source->Tags.Contains(ForType(*Type)));
        for (int32 Level = bArm ? 1 : 0; Level <= (bArm ? 20 : 0); ++Level)
        {
            FGridResolvedItem Item;
            if (!Registry->ResolveItem(Source->ItemId, Level, Item) || !Item.IsValid())
            {
                AddError(TEXT("Resolution failed: ") + Source->ItemId.ToString());
                return false;
            }
            const FName ExpectedType = Type ? *Type :
                (Item.Weapon.Kind == EGridArmamentKind::Shield ? FName(TEXT("Shield")) : FName(TEXT("Weapon")));
            TestTrue(TEXT("Resolved tags match native asset"), Item.Tags == Source->Tags);
            for (FName Candidate : Types)
                TestEqual(TEXT("Exact native category predicate"), Matches(Item, Candidate), Candidate == ExpectedType);
            ++Checked;
        }
    }
    TestEqual(TEXT("All 466 item/level resolutions"), Checked, 466);
    TestEqual(TEXT("22 ARM"), Arms, 22);
    TestEqual(TEXT("Native category migration must remain complete"), NativeTagged, 26);
    for (const auto& Pair : ExpectedCategories())
        TestTrue(TEXT("Every historically proved non-ARM ID remains present"), Seen.Contains(Pair.Key));

    const TArray<FName> Original = { TEXT("Tool.Disarm"), TEXT("QuickItem.Potion") };
    TArray<FName> Migrated = Original;
    Migrated.Add(TEXT("PuzzleCategory.Potion"));
    TestTrue(TEXT("Combat tags preserved in order"), WithoutReserved(Migrated) == Original);
    TestFalse(TEXT("Puzzle metadata forbidden on ARM"), IsValidMetadata(Migrated, true));
    TestTrue(TEXT("One legal non-ARM tag"), IsValidMetadata(Migrated, false));
    Migrated.Add(TEXT("PuzzleCategory.Unknown"));
    TestFalse(TEXT("Unknown or conflicting metadata refused"), IsValidMetadata(Migrated, false));
    TestFalse(TEXT("Unknown tag refused"), IsValidMetadata({TEXT("PuzzleCategory.Unknown")}, false));
    FGridResolvedItem Malformed;
    if (!TestTrue(TEXT("Actual N0 key for malformed tag cases"),
        Registry->ResolveItem(TEXT("Key_Copper"), 0, Malformed))) return false;
    Malformed.Tags = {TEXT("PuzzleCategory.Key"), TEXT("PuzzleCategory.Gem")};
    TestFalse(TEXT("Conflicting category fails closed"), Matches(Malformed, TEXT("Key")));
    Malformed.Tags = {TEXT("PuzzleCategory.Key"), TEXT("PuzzleCategory.Key")};
    TestFalse(TEXT("Duplicate category fails closed"), Matches(Malformed, TEXT("Key")));
    Malformed.Tags = {TEXT("InventoryCategory.Key")};
    TestFalse(TEXT("Presentation tags not category authority"), Matches(Malformed, TEXT("Key")));
    return !HasAnyErrors();
}
#endif
