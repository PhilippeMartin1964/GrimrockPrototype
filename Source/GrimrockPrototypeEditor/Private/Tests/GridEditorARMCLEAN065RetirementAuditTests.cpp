#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/AssetManager.h"
#include "Misc/PackageName.h"
#include "UObject/CoreRedirects.h"
#include "UObject/SoftObjectPath.h"

#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorRetirementAudit.h"
#include "../ARMEMENT/GridEditorInstanceCacheEvidence.h"
#include "Serialization/MemoryWriter.h"
#include "../ARMEMENT/GridEditorInstanceSchemaProjection.h"
#include "Runtime/Items/GridWorldSnapshotReadModel.h"
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
    struct FRedirectCase
    {
        const TCHAR* FormerName;
        const TCHAR* FinalName;
        UClass* NativeClass;
    };
    const FRedirectCase Classes[] = {
        {TEXT("GridCanonicalItemDefinitionAsset"), TEXT("GridItemDefinitionAsset"), UGridItemDefinitionAsset::StaticClass()},
        {TEXT("GridCanonicalWeaponCatalogueAsset"), TEXT("GridWeaponCatalogueAsset"), UGridWeaponCatalogueAsset::StaticClass()},
        {TEXT("GridCanonicalWeaponVisualCatalogueAsset"), TEXT("GridWeaponVisualCatalogueAsset"), UGridWeaponVisualCatalogueAsset::StaticClass()}
    };
    for (const auto& Entry : Classes)
    {
        UClass* Native = Entry.NativeClass;
        const FString FormerPath = FString(TEXT("/Script/GrimrockPrototype.")) + Entry.FormerName;
        const FString FinalPath = FString(TEXT("/Script/GrimrockPrototype.")) + Entry.FinalName;
        TestEqual(TEXT("The live class has its final reflected path"), Native->GetPathName(), FinalPath);
        TestNull(TEXT("No registered class object at the former name"),
            FindObjectFast<UClass>(Native->GetOutermost(), FName(Entry.FormerName)));
        TestTrue(TEXT("The final class owns its CDO"), Native->GetDefaultObject()->GetClass() == Native);
        TestEqual(TEXT("Core Redirect resolves once to the live reflected name"),
            FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class,
                FCoreRedirectObjectName(FormerPath)).ToString(), FinalPath);
        TestEqual(TEXT("The final reflected name does not redirect"),
            FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Class,
                FCoreRedirectObjectName(FinalPath)).ToString(), FinalPath);
        FSoftClassPath SerializedPath(FormerPath);
        TestTrue(TEXT("The serialized soft class path accepts the Core Redirect"),
            SerializedPath.FixupCoreRedirects());
        TestTrue(TEXT("The redirected soft class path loads the live class"),
            SerializedPath.TryLoadClass<UObject>() == Native);
    }

    const UClass* ItemBase = UGridItemDefinitionAsset::StaticClass();
    TestTrue(TEXT("Item definition base remains abstract"), ItemBase->HasAnyClassFlags(CLASS_Abstract));
    for (const FName Field : {FName(TEXT("ItemId")), FName(TEXT("DisplayName")), FName(TEXT("Description")),
        FName(TEXT("WeightKg")), FName(TEXT("Tags"))})
        TestNotNull(TEXT("Current definition field is reflected"), ItemBase->FindPropertyByName(Field));
    for (const FName Field : {FName(TEXT("ItemDefinitionId")), FName(TEXT("Weight")), FName(TEXT("CombatActions")),
        FName(TEXT("bUseWeaponCatalogue")), FName(TEXT("bUseWeaponVisualCatalogue"))})
        TestNull(TEXT("Retired definition field stays absent"), ItemBase->FindPropertyByName(Field));
    TestEqual(TEXT("Dungeon and inventory category enum preserves its script identity"),
        StaticEnum<EGridItemType>()->GetPathName(), FString(TEXT("/Script/GrimrockPrototype.EGridItemType")));

    const auto* ItemSystem = LoadObject<UGridItemSystemAsset>(nullptr, RootPath());
    if (!TestNotNull(TEXT("Live ItemSystem root"), ItemSystem)) return false;
    TestEqual(TEXT("Numeric catalogue preserves the complete Primary Asset ID"),
        ItemSystem->WeaponCatalogue->GetPrimaryAssetId().ToString(),
        FString(TEXT("GridCanonicalWeaponCatalogueAsset:DA_WeaponCatalogue")));
    TestEqual(TEXT("Visual catalogue preserves the complete Primary Asset ID"),
        ItemSystem->WeaponVisualCatalogue->GetPrimaryAssetId().ToString(),
        FString(TEXT("GridCanonicalWeaponVisualCatalogueAsset:DA_WeaponVisualCatalogue")));
    for (const auto& Definition : ItemSystem->ItemDefinitions)
    {
        const UClass* Concrete = Definition->GetClass();
        TestTrue(TEXT("Registered item is a concrete child of the final definition base"),
            Concrete->IsChildOf(ItemBase) && !Concrete->HasAnyClassFlags(CLASS_Abstract));
        TestEqual(TEXT("Concrete item keeps its historic Primary Asset Type and name"),
            Definition->GetPrimaryAssetId(), FPrimaryAssetId(Concrete->GetFName(), Definition->GetFName()));
    }

    TestEqual(TEXT("Final canonical count"), Audit.ItemIds.Num(), 48);
    TestEqual(TEXT("Historical definitions fully retired"), int32(Audit.Report->GetNumberField(TEXT("legacy_definition_count"))), 0);
    TestEqual(TEXT("Canonical resolutions: 22 x 20 + 26 N0"),
        int32(Audit.Report->GetNumberField(TEXT("resolved_item_levels"))), 466);
    TestEqual(TEXT("All historical identities are registered"),
        Audit.Report->GetArrayField(TEXT("legacy_ids_without_canonical")).Num(), 0);
    TestEqual(TEXT("Canonical identities no longer require old package sources"),
        Audit.Report->GetArrayField(TEXT("canonical_ids_without_legacy_source")).Num(), 48);
    TestEqual(TEXT("No external package referencer remains after native 5F"),
        int32(Audit.Report->GetNumberField(TEXT("external_referencer_edges"))), 0);
    // Following the two atomic package retirements, ONLY the two SaveGame
    // Readables may remain in the candidate inventory. Never silently
    // reintroduce a retired weapon source, catalogue or redirector.
    const TSharedPtr<FJsonObject> Roles = Audit.Report->GetObjectField(TEXT("candidate_role_counts"));
    if (!TestTrue(TEXT("Candidate roles are reported"), Roles.IsValid())) return false;
    const int32 LegacyIconPalettes =
        int32(Roles->GetNumberField(TEXT("historical_action_icon_palette")));
    TestEqual(TEXT("No historical icon palette remains"),
        LegacyIconPalettes, 0);
    TestEqual(TEXT("Only the two SavedGame Readable fixtures remain"),
        int32(Audit.Report->GetNumberField(TEXT("candidate_asset_count"))), 2);
    TestEqual(TEXT("Zero obsolete item-definition packages"),
        int32(Roles->GetNumberField(TEXT("historical_item_definition"))), 0);
    TestEqual(TEXT("No obsolete authoring catalogue source survives"),
        int32(Roles->GetNumberField(TEXT("historical_catalogue_source"))), 0);
    TestEqual(TEXT("Two saved readable fixtures protected"),
        int32(Roles->GetNumberField(TEXT("save_readable_fixture"))), 2);
    TestEqual(TEXT("No dangling ThrowingKnife redirector"),
        int32(Roles->GetNumberField(TEXT("historical_throwing_knife_redirector"))), 0);
    // Icon canary count checked above; never infer general deletion approval.
    TestEqual(TEXT("No unclassified paths in current sample"),
        int32(Roles->GetNumberField(TEXT("unclassified_keep_and_investigate"))), 0);
    TestEqual(TEXT("Every candidate is covered by an explicit non-deleting role"),
        int32(Audit.Report->GetNumberField(TEXT("unclassified_candidate_count"))), 0);
    for (const TSharedPtr<FJsonValue>& Candidate : Audit.Report->GetArrayField(TEXT("candidates")))
    {
        const auto Row = Candidate->AsObject();
        TestTrue(TEXT("Every candidate has a declared role"),
            Row.IsValid() && !Row->GetStringField(TEXT("retirement_role")).IsEmpty());
    }
    const auto& ActivePackages = Audit.Report->GetArrayField(TEXT("active_packages"));
    TestEqual(TEXT("Root owns 53 distinct live packages"), ActivePackages.Num(), 53);
    for (const auto& Package : ActivePackages)
    {
        TestFalse(TEXT("Active package remains active independently of its reflected class name"),
            IsRetiredItemPackage(FName(*Package->AsString())));
    }
    TestTrue(TEXT("The exact old dagger package remains a retired source"),
        IsRetiredItemPackage(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01")));
    const auto& Aliases = Audit.Report->GetArrayField(TEXT("active_compatibility_redirectors"));
    TestEqual(TEXT("All 53 former paths remain live compatibility aliases"), Aliases.Num(), 53);
    int32 RedirectedIds = 0;
    for (const auto& Value : Aliases)
    {
        const auto Row = Value->AsObject();
        const FString Package = Row->GetStringField(TEXT("package"));
        const FString Target = Row->GetStringField(TEXT("target"));
        TestTrue(TEXT("Alias target belongs to the live Items tree"),
            Target.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/")));
        const FString OldPath = Package + TEXT(".") + FPackageName::GetShortName(Package);
        // Cook omits UObjectRedirector packages. Config-backed exact object
        // redirects preserve runtime soft paths; native aliases serve Editor loads.
        TestEqual(TEXT("Former object path has a runtime core asset redirect"),
            FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Object,
                FCoreRedirectObjectName(OldPath)).ToString(),
            Target + TEXT(".") + FPackageName::GetShortName(Target));
        // In Editor, a loaded UObjectRedirector deliberately prevents an
        // in-place Core fixup. Verify its native path and the mapped runtime
        // path separately, without unloading or modifying any package.
        const FSoftObjectPath RuntimePath(FCoreRedirects::GetRedirectedName(
            ECoreRedirectFlags::Type_Object, FCoreRedirectObjectName(OldPath)).ToString());
        const auto* Asset = LoadObject<UPrimaryDataAsset>(nullptr, *OldPath);
        if (!TestNotNull(TEXT("Former object path resolves to a primary data asset"), Asset)) continue;
        TestTrue(TEXT("Mapped runtime soft path loads the same asset as its native alias"), RuntimePath.TryLoad() == Asset);
        TestTrue(TEXT("Former soft path loads through its native Editor alias"), FSoftObjectPath(OldPath).TryLoad() == Asset);
        const FPrimaryAssetId CurrentId = Asset->GetPrimaryAssetId();
        const FPrimaryAssetId OldId(CurrentId.PrimaryAssetType, FName(*FPackageName::GetShortName(Package)));
        if (OldId == CurrentId)
        {
            // UE returns an invalid ID when no redirect is required.
            TestFalse(TEXT("Unchanged PrimaryAssetId has no artificial self-redirect"),
                UAssetManager::Get().GetRedirectedPrimaryAssetId(OldId).IsValid());
        }
        else
        {
            TestEqual(TEXT("Derived former PrimaryAssetId redirects to current identity"),
                UAssetManager::Get().GetRedirectedPrimaryAssetId(OldId), CurrentId);
            ++RedirectedIds;
        }
    }
    TestEqual(TEXT("Exactly 50 derived PrimaryAssetIds were renamed"), RedirectedIds, 50);
    const auto& Categories = Audit.Report->GetArrayField(TEXT("puzzle_category_sources"));
    TestEqual(TEXT("No obsolete type source remains after cutover"), Categories.Num(), 0);
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
    TestEqual(TEXT("No retired ARM type evidence remains in live packages"), ArmCategories, 0);
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
                EGridItemResolveError::InvalidItemLevel);
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
    TestTrue(TEXT("Instance cache fields are absent from saved schema"), GridEditorInstanceCacheEvidence::HasRetiredSchema());
    FJsonObject Report;
    Audit.Write(Report);
    TestEqual(TEXT("Retired cache count in current schema"), int32(Report.GetNumberField(TEXT("instance_legacy_name_or_weight"))), 0);
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
        FGridWorldSnapshotReadModel::ValidateDungeon(Registry.Get(), Save->DungeonRuntimeState, Error));
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
        FGridWorldSnapshotReadModel::ValidateDungeon(Registry.Get(), Expected->DungeonRuntimeState, Error));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065G88CacheEvidence,
    "Grimrock.Editor.ARM.CLEAN06.5G88.InstanceCacheEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN065G88CacheEvidence::RunTest(const FString&)
{
    using namespace GridEditorInstanceCacheEvidence;
    TestNull(TEXT("Item name is no longer reflected or persisted"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
    TestNull(TEXT("Item weight is no longer reflected or persisted"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));
    TestNotNull(TEXT("Character name is still reflected"),
        FGridCharacterInventoryState::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
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
    Item.Quantity = 2;
    Item.AbsorbedEquipmentPhysicalArmor = 3;
    Item.ReadTitleOverride = FText::FromString(TEXT("Fixture narrative"));
    Party.ActiveCharacters.AddDefaulted_GetRef().InventorySlots.AddDefaulted_GetRef().Item = Item;
    Party.CharacterPool.AddDefaulted_GetRef().InventorySlots.AddDefaulted_GetRef().Item = Item;
    Party.ActiveEquipment.AddDefaulted_GetRef().MainHand = Item;
    Party.CursorItem = Item;
    const FCounts Before = Inspect(Party, *Registry.Get());
    TestEqual(TEXT("All four instance locations inspected"), Before.Instances, 4);
    TestTrue(TEXT("Canonical reader projections agree with definitions"), Before.IsValid());
    Party.CursorItem.ItemLevel = 1;
    TestFalse(TEXT("Wrong non-ARM level refuses audit"), Inspect(Party, *Registry.Get()).IsValid());
    Party.CursorItem = Item;
    Party.CursorItem.Quantity = FMath::Max(1, Stone.MaxStack) + 1;
    TestFalse(TEXT("Oversized stack refuses audit"), Inspect(Party, *Registry.Get()).IsValid());
    Party.CursorItem = Item;
    Party.CursorItem.RuntimeObjectId.Invalidate();
    TestFalse(TEXT("Invalid physical GUID refuses audit"), Inspect(Party, *Registry.Get()).IsValid());
    Party.CursorItem = Item;
    TestTrue(TEXT("Restored canonical state passes"), Inspect(Party, *Registry.Get()).IsValid());
    TestEqual(TEXT("Physical GUID remains"), Party.CursorItem.RuntimeObjectId, Item.RuntimeObjectId);
    TestEqual(TEXT("Physical debt remains"), Party.CursorItem.AbsorbedEquipmentPhysicalArmor, 3);
    TestTrue(TEXT("Narrative title remains"), Party.CursorItem.ReadTitleOverride.EqualTo(Item.ReadTitleOverride));

    int32 Resolved = 0;
    for (const auto& Def : Root->ItemDefinitions)
    {
        const bool bArm = Def->IsA<UGridArmamentDefinitionAsset>();
        for (int32 Level = bArm ? 1 : 0; Level <= (bArm ? 20 : 0); ++Level)
        {
            FGridItemInstance Probe;
            Probe.RuntimeObjectId = FGuid::NewGuid();
            Probe.ItemDefinitionId = Def->ItemId;
            Probe.ItemLevel = Level;
            Probe.Quantity = 1;
            if (TestTrue(TEXT("Canonical weight, metadata, sort, physics and throw readers"),
                GridItemCacheParityAudit::HasCanonicalProjection(*Registry.Get(), Probe))) ++Resolved;
        }
    }
    TestEqual(TEXT("All canonical physical profiles verified"), Resolved, 466);
    // Historical 5G92/5G93 proved cache exclusion across the cutover.
    // The current fingerprint must still detect distinct durable state changes.
    TStrongObjectPtr<UGrimrockPartySaveGame> Saved(NewObject<UGrimrockPartySaveGame>());
    Saved->PartyInventoryState = Party;
    const auto Baseline = GridEditorInstanceSchemaProjection::Build(*Saved.Get());
    auto& Cursor = Saved->PartyInventoryState.CursorItem;
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
    TestFalse(TEXT("Unknown ID refuses evidence"), Inspect(Party, *Registry.Get()).IsValid());
    return !HasAnyErrors();
}

#endif
