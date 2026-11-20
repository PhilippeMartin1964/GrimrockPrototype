#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorRetirementAudit.h"
#include "../ARMEMENT/GridEditorInstanceCacheEvidence.h"
#include "Serialization/MemoryWriter.h"
#include "../ARMEMENT/GridEditorInstanceSchemaProjection.h"
#include "Runtime/Items/GridCanonicalWorldSnapshotReadModel.h"
#include "../ARMEMENT/GridEditorAlchemySaveCopy.h"
#include "../ARMEMENT/GridEditorWorldSaveProjection.h"
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
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065SaveAudit,
    "Grimrock.Editor.ARM.CLEAN06_5A.SaveRetirementEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065SaveAudit::RunTest(const FString&)
{
    using namespace GridEditorRetirementAudit;
    TStrongObjectPtr<UGrimrockPartySaveGame> Save(NewObject<UGrimrockPartySaveGame>());
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
    FGridLevelRuntimeState Level;
    Level.Items.Add(World.ObjectId, World);
    Level.Receptacles.Add(FGuid::NewGuid()).ContainedItems.Add(World);
    auto& Pending = Level.PendingInboundItems.Add(FGuid::NewGuid());
    Pending.ItemState = World;
    Pending.ItemState.ItemDefinitionId = TEXT("UnknownFixture");
    Pending.ItemState.ReadableContentAsset = Readable.Get();
    Pending.ItemState.ReadableContentId = TEXT("ConflictingReadableFixture");
    Save->DungeonRuntimeState.LevelStates.Add(TEXT("Fixture"), Level);

    TSet<FName> Canonical = {FName(TEXT("Stone"))};
    FSaveAudit Audit;
    Audit.Collect(*Save.Get(), Canonical);
    TestEqual(TEXT("Active/reserve/equipment/cursor/world/receptacle/pending rows"), Audit.Items, 7);
    TestEqual(TEXT("Legacy item caches counted"), Audit.InstanceCaches, 4);
    TestEqual(TEXT("Readable pointers counted separately"), Audit.ReadablePointers, 5);
    TestNull(TEXT("Projectile definition schema removed"), FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ProjectileDefinitionAsset")));
    TestNull(TEXT("Pending definition schema removed"), FGridPendingWorldItemState::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
    TestTrue(TEXT("Unknown ID is reported, not substituted"), Audit.UnknownItemIds.Contains(TEXT("UnknownFixture")));
    int32 Conflicts = 0;
    for (const auto& Row : Audit.References) if (Row->AsObject()->GetBoolField(TEXT("identity_conflict"))) ++Conflicts;
    TestEqual(TEXT("Narrative identity conflict remains detected"), Conflicts, 1);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065G85WorldPreflight,
    "Grimrock.Editor.ARM.CLEAN06.5G85.WorldPointerPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065G85WorldPreflight::RunTest(const FString&)
{
    using namespace GridEditorRetirementAudit;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath()));
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!Root.IsValid() || !Registry->RegisterItemSystem(Root.Get(), Errors)) return false;
    TStrongObjectPtr<UGrimrockPartySaveGame> Save(NewObject<UGrimrockPartySaveGame>());
    auto& Level = Save->DungeonRuntimeState.LevelStates.Add(TEXT("Fixture"));
    Level.LevelId = TEXT("Fixture");
    FGridRuntimeItemState Item;
    Item.ObjectId = FGuid::NewGuid();
    Item.ItemDefinitionId = TEXT("Stone");
    Item.ItemLevel = 0;
    Item.Quantity = 1;
    Item.CellX = 1;
    Item.CellY = 1;
    Level.Items.Add(Item.ObjectId, Item);
    FGridPendingWorldItemState Pending;
    Pending.ItemState = Item;
    Pending.ItemState.ObjectId = FGuid::NewGuid();
    Pending.ItemState.ReadTitleOverride = FText::FromString(TEXT("Fixture narrative"));
    Level.PendingInboundItems.Add(Pending.ItemState.ObjectId, Pending);
    FString Error;
    TestTrue(TEXT("Canonical world preflight accepts actual physical identities"),
        FGridCanonicalWorldSnapshotReadModel::ValidateDungeon(Registry.Get(), Save->DungeonRuntimeState, Error));
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Transient archive encodes"), GridEditorAlchemySaveCopy::EncodeChecked(*Save.Get(), Bytes, Error))) return false;
    // The parity helper explicitly compares objects originating from UE
    // deserialization, not a fresh transient object's FText/derived caches.
    // Exercise the same double-decode/re-encode sequence as the commandlet.
    TStrongObjectPtr<UGrimrockPartySaveGame> Expected(GridEditorAlchemySaveCopy::LoadExact(Bytes, Error));
    TStrongObjectPtr<UGrimrockPartySaveGame> Working(GridEditorAlchemySaveCopy::LoadExact(Bytes, Error));
    if (!TestTrue(TEXT("Independent transient archive decodes"), Expected.IsValid() && Working.IsValid())) return false;
    TArray<uint8> Reencoded;
    if (!TestTrue(TEXT("Decoded archive re-encodes"), GridEditorAlchemySaveCopy::EncodeChecked(*Working.Get(), Reencoded, Error))) return false;
    TStrongObjectPtr<UGrimrockPartySaveGame> Loaded(GridEditorAlchemySaveCopy::LoadExact(Reencoded, Error));
    if (!TestTrue(TEXT("Reencoded archive decodes"), Loaded.IsValid())) return false;
    TestTrue(TEXT("All current saved properties round-trip"), GridEditorAlchemySaveCopy::SameSaveProperties(*Expected.Get(), *Loaded.Get(), Error));
    const auto Projection = GridEditorWorldSaveProjection::Build(Expected->DungeonRuntimeState);
    TestEqual(TEXT("Projection covers both physical rows"), Projection.Num(), 2);
    TestTrue(TEXT("Private physical projection survives independent decode"),
        Projection == GridEditorWorldSaveProjection::Build(Loaded->DungeonRuntimeState));
    auto& Invalid = Expected->DungeonRuntimeState.LevelStates.FindChecked(TEXT("Fixture")).PendingInboundItems.FindChecked(Pending.ItemState.ObjectId);
    TestNull(TEXT("Projection requires no old projectile pointer"), FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ProjectileDefinitionAsset")));
    TestNull(TEXT("Projection requires no old pending pointer"), FGridPendingWorldItemState::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
    Invalid.ItemState.ItemLevel = 1;
    TestFalse(TEXT("Physical level change alters the projection"), Projection ==
        GridEditorWorldSaveProjection::Build(Expected->DungeonRuntimeState));
    TestFalse(TEXT("Invalid non-ARM level refuses preflight"),
        FGridCanonicalWorldSnapshotReadModel::ValidateDungeon(Registry.Get(), Expected->DungeonRuntimeState, Error));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065G88CacheEvidence,
    "Grimrock.Editor.ARM.CLEAN06.5G88.InstanceCacheEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065G88CacheEvidence::RunTest(const FString&)
{
    using namespace GridEditorInstanceCacheEvidence;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, GridEditorRetirementAudit::RootPath()));
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Production root"), Root.IsValid() && Registry->RegisterItemSystem(Root.Get(), Errors))) return false;
    FGridResolvedItem Stone;
    if (!TestTrue(TEXT("Canonical N0 stone"), Registry->ResolveItem(TEXT("Stone"), 0, Stone))) return false;
    FGridPartyInventoryState Party;
    FGridItemInstance Item;
    Item.RuntimeObjectId = FGuid::NewGuid();
    Item.ItemDefinitionId = TEXT("Stone");
    Item.DisplayName = Stone.DisplayName;
    Item.Weight = Stone.WeightKg;
    Item.Quantity = 2;
    Item.AbsorbedEquipmentPhysicalArmor = 3;
    Item.ReadTitleOverride = FText::FromString(TEXT("Fixture narrative"));
    Party.ActiveCharacters.AddDefaulted_GetRef().InventorySlots.AddDefaulted_GetRef().Item = Item;
    Party.CharacterPool.AddDefaulted_GetRef().InventorySlots.AddDefaulted_GetRef().Item = Item;
    Party.ActiveEquipment.AddDefaulted_GetRef().MainHand = Item;
    Party.CursorItem = Item;
    const FCounts Before = Inspect(Party, *Registry.Get());
    TestEqual(TEXT("All four instance locations inspected"), Before.Instances, 4);
    TestTrue(TEXT("Matching cache copies can clear"), Before.CanClear());
    Party.CursorItem.DisplayName = FText::FromString(TEXT("Unexpected alias"));
    Party.CursorItem.Weight += 1.f;
    const FCounts Conflicts = Inspect(Party, *Registry.Get());
    TestEqual(TEXT("Name conflict is distinguished"), Conflicts.NameConflicts, 1);
    TestEqual(TEXT("Weight conflict is distinguished"), Conflicts.WeightConflicts, 1);
    TestFalse(TEXT("Conflict aborts the entire operation"), ClearVerifiedCopy(Party, *Registry.Get()));
    TestTrue(TEXT("Earlier bags remain unchanged"), Party.ActiveCharacters[0].InventorySlots[0].Item.DisplayName.EqualTo(Item.DisplayName));
    Party.CursorItem = Item;
    TStrongObjectPtr<UGridItemDefinitionAsset> HistoricalStone(NewObject<UGridItemDefinitionAsset>());
    HistoricalStone->ItemDefinitionId = Item.ItemDefinitionId;
    HistoricalStone->Weight = Stone.WeightKg + 2.f;
    FLegacySources Historical;
    Historical.Add(Item.ItemDefinitionId, HistoricalStone.Get());
    Party.CursorItem.Weight = HistoricalStone->Weight;
    const auto Explained = Inspect(Party, *Registry.Get(), &Historical);
    TestEqual(TEXT("Exact historical unit weight explained"), Explained.ExplainedWeightConflicts, 1);
    TestTrue(TEXT("Known stale cache may clear only in explicit Editor proof"), Explained.CanClear());
    TestFalse(TEXT("Old strict policy still rejects stale weight"), Inspect(Party, *Registry.Get()).CanClear());
    Party.CursorItem.Weight = HistoricalStone->Weight * Item.Quantity;
    TestTrue(TEXT("Historical stack mass explained"), Inspect(Party, *Registry.Get(), &Historical).CanClear());
    Party.CursorItem.Weight = -1.f;
    TestFalse(TEXT("Negative weight never explained"), Inspect(Party, *Registry.Get(), &Historical).CanClear());
    Party.CursorItem.Weight = HistoricalStone->Weight + 999.f;
    TestFalse(TEXT("Unexplained weight rejects without mutation"), ClearVerifiedCopy(Party, *Registry.Get(), &Historical));
    Party.CursorItem.Weight = HistoricalStone->Weight + 999.f;
    const auto Semantic = Inspect(Party, *Registry.Get(), nullptr, true);
    TestEqual(TEXT("Arbitrary stale positive mass is not provenance"), Semantic.ExplainedWeightConflicts, 0);
    TestEqual(TEXT("Real runtime readers prove same outputs"), Semantic.SemanticWeightConflicts, 1);
    TestEqual(TEXT("All physical rows have equal projections"), Semantic.ProjectionFailures, 0);
    TestTrue(TEXT("Explicit semantic proof permits clearing static cache in memory"), Semantic.CanClear());
    Party.CursorItem.Weight = -1.f;
    TestFalse(TEXT("Negative mass refuses even semantic proof"), ClearVerifiedCopy(Party, *Registry.Get(), nullptr, true));
    Party.CursorItem.Weight = HistoricalStone->Weight + 999.f;
    Party.CursorItem.DisplayName = FText::FromString(TEXT("Unexpected name"));
    TestFalse(TEXT("Semantic proof does not authorize name conflict"), ClearVerifiedCopy(Party, *Registry.Get(), nullptr, true));
    Party.CursorItem = Item;
    Party.CursorItem.Weight = HistoricalStone->Weight + 999.f;
    TestTrue(TEXT("Verified semantic copy may clear"), ClearVerifiedCopy(Party, *Registry.Get(), nullptr, true));
    const FCounts After = Inspect(Party, *Registry.Get());
    TestEqual(TEXT("No name cache remains"), After.CachedNames, 0);
    TestEqual(TEXT("No weight cache remains"), After.CachedWeights, 0);
    TestEqual(TEXT("Physical GUID remains"), Party.CursorItem.RuntimeObjectId, Item.RuntimeObjectId);
    TestEqual(TEXT("Physical debt remains"), Party.CursorItem.AbsorbedEquipmentPhysicalArmor, 3);
    TestTrue(TEXT("Narrative title remains"), Party.CursorItem.ReadTitleOverride.EqualTo(Item.ReadTitleOverride));
    // The private cross-schema fingerprint ignores exactly the two item
    // cache fields. It must detect all distinct durable state changes.
    TStrongObjectPtr<UGrimrockPartySaveGame> Saved(NewObject<UGrimrockPartySaveGame>());
    Saved->PartyInventoryState = Party;
    const auto Baseline = GridEditorInstanceSchemaProjection::Build(*Saved.Get());
    auto& Cursor = Saved->PartyInventoryState.CursorItem;
    Cursor.Weight = 765.f;
    Cursor.DisplayName = FText::FromString(TEXT("Cached alias"));
    TestTrue(TEXT("Only caches do not change fingerprint"),
        Baseline == GridEditorInstanceSchemaProjection::Build(*Saved.Get()));
    Cursor.Quantity++;
    TestFalse(TEXT("Quantity change detected"), Baseline == GridEditorInstanceSchemaProjection::Build(*Saved.Get()));
    Cursor.Quantity--;
    Cursor.AbsorbedEquipmentPhysicalArmor++;
    TestFalse(TEXT("Armor debt change detected"), Baseline == GridEditorInstanceSchemaProjection::Build(*Saved.Get()));
    Cursor.AbsorbedEquipmentPhysicalArmor--;
    Cursor.ReadTitleOverride = FText::FromString(TEXT("Different actual readable title"));
    TestFalse(TEXT("Readable title change detected"), Baseline == GridEditorInstanceSchemaProjection::Build(*Saved.Get()));
    Cursor.ReadTitleOverride = Party.CursorItem.ReadTitleOverride;
    Saved->PartyInventoryState.ActiveCharacters[0].DisplayName = FText::FromString(TEXT("Actual hero name"));
    TestFalse(TEXT("Character name is not an item cache"), Baseline == GridEditorInstanceSchemaProjection::Build(*Saved.Get()));
    Saved->PartyInventoryState.ActiveCharacters[0].DisplayName = Party.ActiveCharacters[0].DisplayName;
    Saved->PartyCellX++;
    TestFalse(TEXT("Position change detected"), Baseline == GridEditorInstanceSchemaProjection::Build(*Saved.Get()));
    Saved->PartyCellX--;
    TestTrue(TEXT("Restored durable values match"), Baseline == GridEditorInstanceSchemaProjection::Build(*Saved.Get()));
    Party.CursorItem.ItemDefinitionId = TEXT("UnknownCacheFixture");
    TestFalse(TEXT("Unknown ID cannot clear"), ClearVerifiedCopy(Party, *Registry.Get()));
    return !HasAnyErrors();
}

#endif
