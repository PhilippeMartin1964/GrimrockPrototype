#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorPuzzleCategoryMigrationPlan.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorPuzzleCategoryPreviewTest,
    "Grimrock.Editor.ARM.CLEAN06.5G69.PuzzleCategoryPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorPuzzleCategoryPreviewTest::RunTest(const FString& Parameters)
{
    using namespace GridEditorPuzzleCategoryMigration;
    using namespace GridCanonicalPuzzleCategoryTags;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, GridEditorRetirementAudit::RootPath()));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    FString Error;
    if (!Root.IsValid() || !BuildPreview(*Root.Get(), *Preview.Get(), Error))
    {
        AddError(TEXT("Native category preview rejected: ") + Error);
        return false;
    }
    TStrongObjectPtr<UGameInstance> BeforeGI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGameInstance> AfterGI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Before(NewObject<UGridItemSubsystem>(BeforeGI.Get()));
    TStrongObjectPtr<UGridItemSubsystem> After(NewObject<UGridItemSubsystem>(AfterGI.Get()));
    TArray<FString> Errors;
    if (!Before->RegisterItemSystem(Root.Get(), Errors) || !After->RegisterItemSystem(Preview.Get(), Errors))
    {
        AddError(FString::Join(Errors, TEXT("; ")));
        return false;
    }
    int32 Checked = 0;
    const TArray<FName> Types = { TEXT("Torch"), TEXT("Weapon"), TEXT("Shield"), TEXT("Armor"),
        TEXT("Jewelry"), TEXT("Key"), TEXT("Gem"), TEXT("Potion"), TEXT("Scroll"), TEXT("Book"),
        TEXT("Food"), TEXT("Component"), TEXT("Quest"), TEXT("Misc"), TEXT("None") };
    for (int32 Index = 0; Index < Root->ItemDefinitions.Num(); ++Index)
    {
        const auto* Source = Root->ItemDefinitions[Index].Get();
        const auto* Target = Preview->ItemDefinitions[Index].Get();
        const FName* Type = Manifest().Find(Source->ItemId);
        TestEqual(TEXT("Identical definition order"), Target->ItemId, Source->ItemId);
        TestFalse(TEXT("Native package remains clean"), Source->GetOutermost()->IsDirty());
        TestEqual(TEXT("Native reserved tags remain absent"), CountReserved(Source->Tags), 0);
        TArray<FName> Expected = Source->Tags;
        if (Type) Expected.Add(ForType(*Type));
        TestTrue(TEXT("Only exact authored tag appended"), Target->Tags == Expected);
        for (int32 Level = Type ? 0 : 1; Level <= (Type ? 0 : 20); ++Level)
        {
            FGridResolvedItem A, B;
            if (!Before->ResolveItem(Source->ItemId, Level, A) || !After->ResolveItem(Source->ItemId, Level, B))
            {
                AddError(TEXT("Resolution failed: ") + Source->ItemId.ToString());
                return false;
            }
            const FName ExpectedType = Type ? *Type :
                (A.Weapon.Kind == EGridCanonicalArmamentKind::Shield ? FName(TEXT("Shield")) : FName(TEXT("Weapon")));
            TestTrue(TEXT("Resolved tags match preview"), B.Tags == Expected);
            for (FName Candidate : Types)
                TestEqual(TEXT("Exact category predicate"), Matches(B, Candidate), Candidate == ExpectedType);
            for (TFieldIterator<FProperty> Field(FGridResolvedItem::StaticStruct()); Field; ++Field)
                if (Field->GetFName() != FName(TEXT("Tags")))
                    TestTrue(FString::Printf(TEXT("%s N%d unchanged: %s"), *Source->ItemId.ToString(), Level, *Field->GetName()),
                        Field->Identical_InContainer(&A, &B));
            ++Checked;
        }
    }
    TestEqual(TEXT("All 466 physical item/level resolutions"), Checked, 466);
    TArray<FName> Candidate;
    TestFalse(TEXT("No unknown category"), PlanTags({}, TEXT("Unknown"), Candidate));
    TestFalse(TEXT("No existing or partial migration"), PlanTags({TEXT("PuzzleCategory.Key")}, TEXT("Key"), Candidate));
    FGridResolvedItem Invalid;
    Invalid.Error = EGridCanonicalItemResolveError::None;
    Invalid.Tags = {TEXT("PuzzleCategory.Key"), TEXT("PuzzleCategory.Gem")};
    TestFalse(TEXT("Conflicting category fails closed"), Matches(Invalid, TEXT("Key")));
    Invalid.Tags = {TEXT("PuzzleCategory.Key"), TEXT("PuzzleCategory.Key")};
    TestFalse(TEXT("Duplicate category fails closed"), Matches(Invalid, TEXT("Key")));
    Invalid.Tags = {TEXT("InventoryCategory.Key")};
    TestFalse(TEXT("Presentation tags not category authority"), Matches(Invalid, TEXT("Key")));
    return !HasAnyErrors();
}
#endif
