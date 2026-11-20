#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorRetirementAudit.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"

namespace
{
    // Minimal project metadata only; never export the full audit or saves.
    bool WritePuzzleCategoryEvidence(const GridEditorRetirementAudit::FNativeAudit& Audit)
    {
        auto Result = MakeShared<FJsonObject>();
        TArray<TSharedPtr<FJsonValue>> Sources;
        for (const auto& Value : Audit.Report->GetArrayField(TEXT("puzzle_category_sources")))
        {
            const auto Original = Value->AsObject();
            auto Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("item_id"), Original->GetStringField(TEXT("item_id")));
            Row->SetStringField(TEXT("legacy_type"), Original->GetStringField(TEXT("legacy_type")));
            const bool bArm = Original->GetBoolField(TEXT("armament"));
            Row->SetBoolField(TEXT("armament"), bArm);
            Row->SetStringField(TEXT("canonical_arm_type"), bArm
                ? Original->GetStringField(TEXT("canonical_arm_type")) : TEXT("None"));
            int32 ExistingPuzzleTags = 0;
            for (const auto& Tag : Original->GetArrayField(TEXT("canonical_tags")))
                if (Tag->AsString().StartsWith(TEXT("PuzzleCategory."), ESearchCase::IgnoreCase))
                    ++ExistingPuzzleTags;
            Row->SetNumberField(TEXT("existing_puzzle_category_tags"), ExistingPuzzleTags);
            Sources.Add(MakeShared<FJsonValueObject>(Row));
        }
        auto Counts = MakeShared<FJsonObject>();
        TMap<FString, int32> ByType;
        for (const auto& Value : Audit.Report->GetArrayField(TEXT("puzzle_type_links")))
            ++ByType.FindOrAdd(Value->AsObject()->GetStringField(TEXT("type")));
        for (const auto& Pair : ByType) Counts->SetNumberField(Pair.Key, Pair.Value);
        Result->SetNumberField(TEXT("schema"), 1);
        Result->SetNumberField(TEXT("level_assets_scanned"),
            Audit.Report->GetNumberField(TEXT("puzzle_level_assets_scanned")));
        Result->SetArrayField(TEXT("sources"), Sources);
        Result->SetObjectField(TEXT("link_counts"), Counts);
        FString Json;
        if (!FJsonSerializer::Serialize(Result, TJsonWriterFactory<>::Create(&Json))) return false;
        FString Directory;
        // ValidateUE uses a unique report directory even when -UserDir
        // redirects Saved; do not rely on the physical project Saved path.
        if (!FParse::Value(FCommandLine::Get(), TEXT("ReportExportPath="), Directory) ||
            Directory.IsEmpty())
        {
            Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Audits/ARM_CLEAN06_5G68"));
        }
        IFileManager::Get().MakeDirectory(*Directory, true);
        const FString Path = FPaths::Combine(Directory, TEXT("PuzzleCategoryEvidence.json"));
        return FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065NativeAudit,
    "Grimrock.Editor.ARM.CLEAN06_5A.NativeRetirementEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065NativeAudit::RunTest(const FString&)
{
    using namespace GridEditorRetirementAudit;
    FNativeAudit Audit;
    FString Error;
    if (!CollectNative(Audit, Error))
    {
        AddError(Error.IsEmpty() ? TEXT("Native read-only audit failed") : Error);
        return false;
    }
    TestEqual(TEXT("Final canonical count"), Audit.ItemIds.Num(), 48);
    TestEqual(TEXT("Actual historical definitions"), int32(Audit.Report->GetNumberField(TEXT("legacy_definition_count"))), 48);
    TestEqual(TEXT("Canonical resolutions: 22 x 20 + 26 N0"),
        int32(Audit.Report->GetNumberField(TEXT("resolved_item_levels"))), 466);
    TestEqual(TEXT("All historical identities are registered"),
        Audit.Report->GetArrayField(TEXT("legacy_ids_without_canonical")).Num(), 0);
    TestEqual(TEXT("All canonical identities have historical evidence"),
        Audit.Report->GetArrayField(TEXT("canonical_ids_without_legacy_source")).Num(), 0);
    TestEqual(TEXT("No external package referencer remains after native 5F"),
        int32(Audit.Report->GetNumberField(TEXT("external_referencer_edges"))), 0);
    const auto& Categories = Audit.Report->GetArrayField(TEXT("puzzle_category_sources"));
    TestEqual(TEXT("Every native historical source has explicit category evidence"), Categories.Num(), 48);
    TSet<FName> CategoryIds;
    int32 ArmCategories = 0;
    for (const auto& Value : Categories)
    {
        const auto Row = Value->AsObject();
        const FName Id(*Row->GetStringField(TEXT("item_id")));
        TestFalse(TEXT("Category evidence has no duplicate identity"), CategoryIds.Contains(Id));
        CategoryIds.Add(Id);
        TestTrue(TEXT("Category evidence retains exact source category"), !Row->GetStringField(TEXT("legacy_type")).IsEmpty());
        TestTrue(TEXT("Category evidence has canonical match"), Row->GetBoolField(TEXT("canonical_id_exists")));
        TestTrue(TEXT("Canonical tags captured independently"), Row->HasTypedField<EJson::Array>(TEXT("canonical_tags")));
        if (Row->GetBoolField(TEXT("armament"))) ++ArmCategories;
    }
    TestEqual(TEXT("ARM category evidence"), ArmCategories, 22);
    TestTrue(TEXT("Native level assets scanned for category links"),
        Audit.Report->GetNumberField(TEXT("puzzle_level_assets_scanned")) > 0);
    for (const auto& Value : Audit.Report->GetArrayField(TEXT("puzzle_type_links")))
    {
        const auto Row = Value->AsObject();
        TestTrue(TEXT("Link evidence retains level and endpoints"),
            !Row->GetStringField(TEXT("level")).IsEmpty() &&
            !Row->GetStringField(TEXT("source_id")).IsEmpty() &&
            !Row->GetStringField(TEXT("target_id")).IsEmpty());
    }

    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath()));
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Configured root for full physical level rejection audit"),
        Root.IsValid() && Registry->RegisterItemSystem(Root.Get(), Errors))) return false;
    int32 Rejections = 0;
    for (const auto& Def : Root->ItemDefinitions)
    {
        const bool bArm = Def->IsA<UGridArmamentDefinitionAsset>();
        for (int32 Level = bArm ? 0 : 1; Level <= (bArm ? 0 : 20); ++Level)
        {
            FGridResolvedItem Item;
            TestFalse(TEXT("Wrong physical family level rejected"), Registry->ResolveItem(Def->ItemId, Level, Item));
            TestEqual(TEXT("Exact invalid physical level diagnostic"), Item.Error,
                EGridCanonicalItemResolveError::InvalidItemLevel);
            TestTrue(TEXT("Invalid level exposes no metadata"), Item.ItemId.IsNone());
            ++Rejections;
        }
    }
    TestEqual(TEXT("22 ARM N0 and 26 non-ARM x 20 invalid levels audited"), Rejections, 542);
    if (HasAnyErrors()) return false;
    return TestTrue(TEXT("Minimal project-only category evidence written"),
        WritePuzzleCategoryEvidence(Audit));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065SaveAudit,
    "Grimrock.Editor.ARM.CLEAN06_5A.SaveRetirementEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065SaveAudit::RunTest(const FString&)
{
    using namespace GridEditorRetirementAudit;
    TStrongObjectPtr<UGrimrockPartySaveGame> Save(NewObject<UGrimrockPartySaveGame>());
    TStrongObjectPtr<UGridItemDefinitionAsset> Legacy(NewObject<UGridItemDefinitionAsset>());
    Legacy->ItemDefinitionId = TEXT("Stone");
    TStrongObjectPtr<UGridReadableContentAsset> Readable(NewObject<UGridReadableContentAsset>());
    Readable->ReadableContentId = TEXT("FixtureReadable");
    FGridItemInstance Item;
    Item.RuntimeObjectId = FGuid::NewGuid();
    Item.ItemDefinitionId = TEXT("Stone");
    Item.DisplayName = FText::FromString(TEXT("Fixture"));
    Item.Weight = 1.f;
    Item.ReadableContentAsset = Readable.Get();
    Item.ReadableContentId = Readable->ReadableContentId;
    Save->PartyInventoryState.ActiveCharacters.AddDefaulted_GetRef().InventorySlots.AddDefaulted_GetRef().Item = Item;
    Save->PartyInventoryState.CharacterPool.AddDefaulted_GetRef().InventorySlots.AddDefaulted_GetRef().Item = Item;
    Save->PartyInventoryState.ActiveEquipment.AddDefaulted_GetRef().OffHand = Item;
    Save->PartyInventoryState.CursorItem = Item;
    FGridRuntimeItemState World;
    World.ObjectId = FGuid::NewGuid();
    World.ItemDefinitionId = TEXT("Stone");
    World.ProjectileDefinitionAsset = Legacy.Get();
    FGridLevelRuntimeState Level;
    Level.Items.Add(World.ObjectId, World);
    Level.Receptacles.Add(FGuid::NewGuid()).ContainedItems.Add(World);
    auto& Pending = Level.PendingInboundItems.Add(FGuid::NewGuid());
    Pending.ItemState = World;
    Pending.ItemDefinitionAsset = Legacy.Get();
    Pending.ItemState.ItemDefinitionId = TEXT("UnknownFixture");
    Save->DungeonRuntimeState.LevelStates.Add(TEXT("Fixture"), Level);

    TSet<FName> Canonical = {FName(TEXT("Stone"))};
    FSaveAudit Audit;
    Audit.Collect(*Save.Get(), Canonical);
    TestEqual(TEXT("Active/reserve/equipment/cursor/world/receptacle/pending rows"), Audit.Items, 7);
    TestEqual(TEXT("Legacy item caches counted"), Audit.InstanceCaches, 4);
    TestEqual(TEXT("Readable pointers counted separately"), Audit.ReadablePointers, 4);
    TestEqual(TEXT("Projectile pointers in all world containers"), Audit.ProjectilePointers, 3);
    TestEqual(TEXT("Pending item pointer"), Audit.PendingPointers, 1);
    TestTrue(TEXT("Unknown ID is reported, not substituted"), Audit.UnknownItemIds.Contains(TEXT("UnknownFixture")));
    int32 Conflicts = 0;
    for (const auto& Row : Audit.References) if (Row->AsObject()->GetBoolField(TEXT("identity_conflict"))) ++Conflicts;
    TestEqual(TEXT("Both mismatched world pointers detected"), Conflicts, 2);
    TestTrue(TEXT("Source pointer remains unchanged"), Save->PartyInventoryState.CursorItem.ReadableContentAsset.Get() == Readable.Get());

    // Actual FString serialization covers an ASCII path that can be lost
    // during object load. Also exercise UTF-16LE matches independently.
    const auto Paths = MovedReadablePaths();
    FString OldPath = Paths[0];
    TArray<uint8> Bytes;
    FMemoryWriter Writer(Bytes);
    Writer << OldPath;
    TestTrue(TEXT("Old full path detected in serialized FString"), HasPathBytes(Bytes, OldPath, false));
    TestFalse(TEXT("Different readable not falsely reported"), HasPathBytes(Bytes, Paths[1], false));
    TArray<uint8> Wide;
    for (TCHAR Ch : Paths[1]) { Wide.Add(uint8(Ch)); Wide.Add(0); }
    TestTrue(TEXT("UTF-16LE old path detected"), HasPathBytes(Wide, Paths[1], true));
    TestEqual(TEXT("Raw path audit reports one exact candidate"), RawPaths(Wide, Paths).Num(), 1);
    return !HasAnyErrors();
}

#endif
