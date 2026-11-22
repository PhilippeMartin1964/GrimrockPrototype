#pragma once

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "Core/GridLevelAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Misc/AssetRegistryInterface.h"
#include "Modules/ModuleManager.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

// Read-only evidence, NOT a deletion authorizer. Registry edges, decoded
// save fields and raw path candidates answer three different questions.
namespace GridEditorRetirementAudit
{
    inline const TCHAR* RootPath()
    {
        return TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem");
    }

    inline TArray<TSharedPtr<FJsonValue>> Names(TArray<FName> Values)
    {
        Values.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });
        TArray<TSharedPtr<FJsonValue>> Out;
        for (FName Name : Values) Out.Add(MakeShared<FJsonValueString>(Name.ToString()));
        return Out;
    }

    inline TArray<FString> MovedReadablePaths()
    {
        return {
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Readables/DA_Scroll_Test.DA_Scroll_Test"),
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Readables/DA_BookPage_Test.DA_BookPage_Test")
        };
    }

    // This is a disposition taxonomy, never an automatic deletion list.
    // In particular the legacy DA_WeaponCatalogue and DA_WeaponVisualCatalogue
    // are NOT the active catalogues owned by DA_ItemSystem in Items/Catalogues.
    inline FString CandidateRetirementRole(const FAssetData& Data)
    {
        const FString Package = Data.PackageName.ToString();
        const FName Class = Data.AssetClassPath.GetAssetName();
        if (Class == UGridItemDefinitionAsset::StaticClass()->GetFName())
            return TEXT("historical_item_definition");
        if ((Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue") &&
             Class == FName(TEXT("GridWeaponCatalogueAsset"))) ||
            (Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue") &&
             Class == FName(TEXT("GridWeaponVisualCatalogueAsset"))))
            return TEXT("historical_catalogue_source");
        if ((Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_BookPage_Test") ||
             Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_Scroll_Test")) &&
            Class == UGridReadableContentAsset::StaticClass()->GetFName())
            return TEXT("save_readable_fixture");
        if (Package == TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ThrowingKnife") &&
            Class == FName(TEXT("ObjectRedirector")))
            return TEXT("historical_throwing_knife_redirector");
        if (Package == TEXT("/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons") &&
            Class == FName(TEXT("GridEquippedWeaponActionIconPalette")))
            return TEXT("historical_action_icon_palette");
        return TEXT("unclassified_keep_and_investigate");
    }

    struct FNativeAudit
    {
        TSet<FName> ItemIds;
        TArray<FString> CandidatePaths;
        TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    };

    inline bool CollectNative(FNativeAudit& Out, FString& Error)
    {
        Error.Reset();
        Out.ItemIds.Reset();
        Out.CandidatePaths = MovedReadablePaths();
        Out.Report = MakeShared<FJsonObject>();
        TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, RootPath()));
        TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
        TStrongObjectPtr<UGridItemSubsystem> Items(NewObject<UGridItemSubsystem>(GI.Get()));
        TArray<FString> Errors;
        if (!Root.IsValid() || !Root->ActionCatalogue || Root->ItemDefinitions.Num() != 48 ||
            Root->ActionCatalogue->DetailedActions.Num() != 14 ||
            !Items->RegisterItemSystem(Root.Get(), Errors))
        {
            Error = TEXT("Expected validated post-X66 native root (48 / 14): ") + FString::Join(Errors, TEXT("; "));
            return false;
        }

        int32 ArmCount = 0;
        int32 ResolveCount = 0;
        for (const auto& Def : Root->ItemDefinitions)
        {
            if (!IsValid(Def.Get()) || Def->ItemId.IsNone() || Out.ItemIds.Contains(Def->ItemId))
            {
                Error = TEXT("Invalid or duplicate canonical item identity");
                return false;
            }
            Out.ItemIds.Add(Def->ItemId);
            const bool bArm = Def->IsA<UGridArmamentDefinitionAsset>();
            ArmCount += bArm ? 1 : 0;
            for (int32 Level = bArm ? 1 : 0; Level <= (bArm ? 20 : 0); ++Level)
            {
                FGridResolvedItem Resolved;
                if (!Items->ResolveItem(Def->ItemId, Level, Resolved) || !Resolved.IsValid())
                {
                    Error = TEXT("Cannot resolve persisted canonical identity: ") + Def->ItemId.ToString();
                    return false;
                }
                ++ResolveCount;
            }
        }
        Out.Report->SetNumberField(TEXT("canonical_count"), Out.ItemIds.Num());
        Out.Report->SetNumberField(TEXT("armament_count"), ArmCount);
        Out.Report->SetNumberField(TEXT("resolved_item_levels"), ResolveCount);
        Out.Report->SetNumberField(TEXT("detailed_actions"), Root->ActionCatalogue->DetailedActions.Num());

        // Derive the live package set from the validated root, never from a
        // directory exemption: an unrelated legacy asset in Items still fails.
        TSet<FName> ActivePackages;
        const auto AddActive = [&](const UObject* Object)
        {
            if (IsValid(Object)) ActivePackages.Add(Object->GetOutermost()->GetFName());
        };
        AddActive(Root.Get());
        AddActive(Root->WeaponCatalogue.Get());
        AddActive(Root->WeaponVisualCatalogue.Get());
        AddActive(Root->ActionCatalogue.Get());
        AddActive(Root->ActionPresentationCatalogue.Get());
        for (const auto& Definition : Root->ItemDefinitions) AddActive(Definition.Get());
        if (ActivePackages.Num() != 53)
        {
            Error = TEXT("Expected 53 distinct packages owned by the active root");
            return false;
        }
        TArray<TSharedPtr<FJsonValue>> CompatibilityRedirectors;
        Out.Report->SetArrayField(TEXT("active_packages"), Names(ActivePackages.Array()));

        using namespace UE::AssetRegistry;
        IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
        Registry.SearchAllAssets(true);
        TArray<FAssetData> Assets;
        // Enumerate classes across the whole /Game mount, not just the old
        // Items directory. Also include redirectors/catalogues and the
        // readable fixtures, which are not legacy item-definition classes.
        TArray<FAssetData> All;
        Registry.GetAssetsByPath(FName(TEXT("/Game")), All, true, true);
        TSet<FName> LegacyPackages;
        TArray<TSharedPtr<FJsonValue>> Rows;
        const FString Base = TEXT("/Game/GrimrockPrototype/Core/DataAssets/");
        const auto IsCandidate = [&](const FAssetData& Data)
        {
            const FString Package = Data.PackageName.ToString();
            const FName Class = Data.AssetClassPath.GetAssetName();
            if (Class == UGridItemDefinitionAsset::StaticClass()->GetFName()) return true;
            if (ActivePackages.Contains(Data.PackageName)) return false;
            if (Class == FName(TEXT("ObjectRedirector")) &&
                Package.StartsWith(Base + TEXT("CanonicalStage/")))
            {
                // Only a redirector resolving to a current owned package is an
                // active compatibility alias. Dangling/unrelated ones stay candidates.
                const UPrimaryDataAsset* Target = LoadObject<UPrimaryDataAsset>(nullptr, *Data.GetSoftObjectPath().ToString());
                if (IsValid(Target) && Target->GetOutermost()->GetFName() != Data.PackageName &&
                    ActivePackages.Contains(Target->GetOutermost()->GetFName()))
                {
                    auto Row = MakeShared<FJsonObject>();
                    Row->SetStringField(TEXT("package"), Package);
                    Row->SetStringField(TEXT("target"), Target->GetOutermost()->GetName());
                    CompatibilityRedirectors.Add(MakeShared<FJsonValueObject>(Row));
                    return false;
                }
            }
            return Package.StartsWith(Base + TEXT("Weapons/")) ||
                Package.StartsWith(Base + TEXT("Items/")) ||
                Package.StartsWith(Base + TEXT("Tests/")) ||
                Package.EndsWith(TEXT("/DA_EquippedWeaponActionIcons")) ||
                (Class == FName(TEXT("ObjectRedirector")) &&
                    (Package.Contains(TEXT("/Readables/")) || Package.Contains(TEXT("/CanonicalStage/"))));
        };
        for (const FAssetData& Data : All)
        {
            if (!IsCandidate(Data)) continue;
            Assets.Add(Data);
            LegacyPackages.Add(Data.PackageName);
        }
        Out.Report->SetArrayField(TEXT("active_compatibility_redirectors"), CompatibilityRedirectors);
        if (Assets.IsEmpty())
        {
            Error = TEXT("No historical candidates returned by AssetRegistry");
            return false;
        }
        Assets.Sort([](const FAssetData& A, const FAssetData& B)
            { return A.GetSoftObjectPath().ToString() < B.GetSoftObjectPath().ToString(); });

        int32 LegacyCount = 0;
        TMap<FString, int32> CandidateRoleCounts;
        int32 ExternalEdges = 0;
        TMap<EGridItemType, int32> LegacyCategoryCounts;
        TArray<TSharedPtr<FJsonValue>> CategoryRows;
        const auto CategoryName = [](EGridItemType Type)
        {
            return StaticEnum<EGridItemType>()->GetNameStringByValue(static_cast<int64>(Type));
        };
        TSet<FName> CoveredIds;
        TArray<FName> UnmappedIds;
        for (const FAssetData& Data : Assets)
        {
            const FString ObjectPath = Data.GetSoftObjectPath().ToString();
            Out.CandidatePaths.AddUnique(ObjectPath);
            TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("package"), Data.PackageName.ToString());
            Row->SetStringField(TEXT("object"), ObjectPath);
            Row->SetStringField(TEXT("class"), Data.AssetClassPath.ToString());
            const FString RetirementRole = CandidateRetirementRole(Data);
            Row->SetStringField(TEXT("retirement_role"), RetirementRole);
            ++CandidateRoleCounts.FindOrAdd(RetirementRole);
            if (Data.AssetClassPath.GetAssetName() == UGridItemDefinitionAsset::StaticClass()->GetFName())
            {
                const auto* Legacy = Cast<UGridItemDefinitionAsset>(Data.GetAsset());
                if (!IsValid(Legacy))
                {
                    Error = TEXT("Historical definition failed to load: ") + ObjectPath;
                    return false;
                }
                ++LegacyCount;
                Row->SetStringField(TEXT("item_id"), Legacy->ItemDefinitionId.ToString());
                const bool bCovered = Out.ItemIds.Contains(Legacy->ItemDefinitionId);
                Row->SetBoolField(TEXT("canonical_id_exists"), bCovered);
                if (bCovered) CoveredIds.Add(Legacy->ItemDefinitionId);
                else UnmappedIds.AddUnique(Legacy->ItemDefinitionId);

                // The old category is evidence of the authored puzzle contract,
                // not a new runtime authority. Record tags without inferring types.
                ++LegacyCategoryCounts.FindOrAdd(Legacy->ItemType);
                auto Category = MakeShared<FJsonObject>();
                Category->SetStringField(TEXT("source"), ObjectPath);
                Category->SetStringField(TEXT("item_id"), Legacy->ItemDefinitionId.ToString());
                Category->SetStringField(TEXT("legacy_type"), CategoryName(Legacy->ItemType));
                Category->SetBoolField(TEXT("canonical_id_exists"), bCovered);
                if (bCovered)
                {
                    const auto* Definition = Root->ItemDefinitions.FindByPredicate(
                        [&](const auto& Def) { return IsValid(Def.Get()) && Def->ItemId == Legacy->ItemDefinitionId; });
                    if (!Definition)
                    {
                        Error = TEXT("Missing canonical definition for category evidence: ") + ObjectPath;
                        return false;
                    }
                    const bool bArm = (*Definition)->IsA<UGridArmamentDefinitionAsset>();
                    FGridResolvedItem Resolved;
                    if (!Items->ResolveItem(Legacy->ItemDefinitionId, bArm ? 1 : 0, Resolved))
                    {
                        Error = TEXT("Unresolved category evidence: ") + ObjectPath;
                        return false;
                    }
                    Category->SetBoolField(TEXT("armament"), bArm);
                    Category->SetArrayField(TEXT("canonical_tags"), Names(Resolved.Tags));
                    if (bArm)
                    {
                        const EGridItemType Type = Resolved.Weapon.Kind == EGridArmamentKind::Shield
                            ? EGridItemType::Shield : EGridItemType::Weapon;
                        Category->SetStringField(TEXT("canonical_arm_type"), CategoryName(Type));
                        Category->SetBoolField(TEXT("arm_type_matches"), Type == Legacy->ItemType);
                    }
                }
                CategoryRows.Add(MakeShared<FJsonValueObject>(Category));
            }
            TArray<FName> AllRefs, Hard, Soft, External;
            const bool bQuery = Registry.GetReferencers(Data.PackageName, AllRefs, EDependencyCategory::All, FDependencyQuery());
            Registry.GetReferencers(Data.PackageName, Hard, EDependencyCategory::Package, FDependencyQuery(EDependencyQuery::Hard));
            Registry.GetReferencers(Data.PackageName, Soft, EDependencyCategory::Package, FDependencyQuery(EDependencyQuery::Soft));
            for (FName Referencer : AllRefs)
                if (!LegacyPackages.Contains(Referencer)) External.AddUnique(Referencer);
            ExternalEdges += External.Num();
            Row->SetBoolField(TEXT("referencer_query"), bQuery);
            Row->SetArrayField(TEXT("all_referencers"), Names(AllRefs));
            Row->SetArrayField(TEXT("hard_referencers"), Names(Hard));
            Row->SetArrayField(TEXT("soft_referencers"), Names(Soft));
            Row->SetArrayField(TEXT("external_referencers"), Names(External));
            Rows.Add(MakeShared<FJsonValueObject>(Row));
        }
        TArray<FName> MissingLegacyIds;
        for (FName Id : Out.ItemIds)
            if (!CoveredIds.Contains(Id)) MissingLegacyIds.Add(Id);
        Out.CandidatePaths.Sort();
        Out.Report->SetNumberField(TEXT("legacy_definition_count"), LegacyCount);
        Out.Report->SetNumberField(TEXT("candidate_asset_count"), Assets.Num());
        TSharedRef<FJsonObject> Roles = MakeShared<FJsonObject>();
        for (const TCHAR* Name : {
            TEXT("historical_item_definition"),
            TEXT("historical_catalogue_source"),
            TEXT("save_readable_fixture"),
            TEXT("historical_throwing_knife_redirector"),
            TEXT("historical_action_icon_palette"),
            TEXT("unclassified_keep_and_investigate")})
        {
            Roles->SetNumberField(Name, CandidateRoleCounts.FindRef(Name));
        }
        Out.Report->SetObjectField(TEXT("candidate_role_counts"), Roles);
        Out.Report->SetNumberField(TEXT("unclassified_candidate_count"),
            CandidateRoleCounts.FindRef(TEXT("unclassified_keep_and_investigate")));
        Out.Report->SetNumberField(TEXT("external_referencer_edges"), ExternalEdges);
        Out.Report->SetArrayField(TEXT("legacy_ids_without_canonical"), Names(UnmappedIds));
        Out.Report->SetArrayField(TEXT("canonical_ids_without_legacy_source"), Names(MissingLegacyIds));
        Out.Report->SetArrayField(TEXT("candidates"), Rows);
        Out.Report->SetArrayField(TEXT("puzzle_category_sources"), CategoryRows);

        // Native level assets, including subclasses. Do not rewrite links,
        // reverse conditions, Blueprint defaults or save data in this audit.
        FARFilter LevelFilter;
        LevelFilter.PackagePaths.Add(FName(TEXT("/Game")));
        LevelFilter.bRecursivePaths = true;
        LevelFilter.ClassPaths.Add(UGridLevelAsset::StaticClass()->GetClassPathName());
        LevelFilter.bRecursiveClasses = true;
        TArray<FAssetData> LevelAssets;
        Registry.GetAssets(LevelFilter, LevelAssets);
        LevelAssets.Sort([](const FAssetData& A, const FAssetData& B)
            { return A.GetSoftObjectPath().ToString() < B.GetSoftObjectPath().ToString(); });
        TArray<TSharedPtr<FJsonValue>> TypeLinks;
        for (const FAssetData& Data : LevelAssets)
        {
            const auto* Level = Cast<UGridLevelAsset>(Data.GetAsset());
            if (!IsValid(Level))
            {
                Error = TEXT("Level failed to load for puzzle category evidence: ") +
                    Data.GetSoftObjectPath().ToString();
                return false;
            }
            for (const FGridObjectLink& Link : Level->Links)
            {
                if (Link.Condition != EGridObjectCondition::ReceptacleContainsItemType) continue;
                auto Row = MakeShared<FJsonObject>();
                Row->SetStringField(TEXT("level"), Data.GetSoftObjectPath().ToString());
                Row->SetStringField(TEXT("source_id"), Link.SourceObjectId.ToString());
                Row->SetStringField(TEXT("target_id"), Link.TargetObjectId.ToString());
                Row->SetStringField(TEXT("type"), CategoryName(Link.ConditionItemType));
                Row->SetBoolField(TEXT("inverted"), Link.bInvertCondition);
                Row->SetNumberField(TEXT("historical_matching_definitions"),
                    LegacyCategoryCounts.FindRef(Link.ConditionItemType));
                TypeLinks.Add(MakeShared<FJsonValueObject>(Row));
            }
        }
        Out.Report->SetNumberField(TEXT("puzzle_level_assets_scanned"), LevelAssets.Num());
        Out.Report->SetArrayField(TEXT("puzzle_type_links"), TypeLinks);
        Out.Report->SetBoolField(TEXT("native_root_dirty"), Root->GetOutermost()->IsDirty());
        return !Root->GetOutermost()->IsDirty();
    }

    // SaveGame proxy archives may replace missing object references with null.
    // Search the unmodified bytes as well, in ASCII and UTF-16LE, only for
    // exact known full object paths. Matches are evidence candidates, not
    // a complete parser or proof that the occurrence belongs to a field.
    inline bool HasPathBytes(const TArray<uint8>& Bytes, const FString& Path, bool bWide)
    {
        if (Path.IsEmpty()) return false;
        const int32 Width = bWide ? 2 : 1;
        const int32 Length = Path.Len() * Width;
        for (int32 Start = 0; Start <= Bytes.Num() - Length; ++Start)
        {
            bool bMatches = true;
            for (int32 I = 0; I < Path.Len(); ++I)
            {
                const int32 At = Start + I * Width;
                if (FChar::ToLower(TCHAR(Bytes[At])) != FChar::ToLower(Path[I]) ||
                    (bWide && Bytes[At+1] != 0))
                {
                    bMatches = false;
                    break;
                }
            }
            if (bMatches) return true;
        }
        return false;
    }

    inline TArray<TSharedPtr<FJsonValue>> RawPaths(const TArray<uint8>& Bytes, const TArray<FString>& Candidates)
    {
        TArray<TSharedPtr<FJsonValue>> Matches;
        for (const FString& Path : Candidates)
        {
            const bool bAscii = HasPathBytes(Bytes, Path, false);
            const bool bWide = HasPathBytes(Bytes, Path, true);
            if (bAscii || bWide)
            {
                auto Row = MakeShared<FJsonObject>();
                Row->SetStringField(TEXT("object"), Path);
                Row->SetBoolField(TEXT("ascii"), bAscii);
                Row->SetBoolField(TEXT("utf16le"), bWide);
                Matches.Add(MakeShared<FJsonValueObject>(Row));
            }
        }
        return Matches;
    }

    struct FSaveAudit
    {
        int32 Items = 0;
        int32 ReadablePointers = 0;
        int32 ReadableIdsWithoutPointer = 0;
        TArray<FName> UnknownItemIds;
        TArray<TSharedPtr<FJsonValue>> References;

        void Ref(const TCHAR* Field, FName ItemId, const UObject* Object, bool bConflict = false)
        {
            auto Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("field"), Field);
            Row->SetStringField(TEXT("item_id"), ItemId.ToString());
            Row->SetStringField(TEXT("object"), IsValid(Object) ? Object->GetPathName() : TEXT(""));
            Row->SetBoolField(TEXT("identity_conflict"), bConflict);
            References.Add(MakeShared<FJsonValueObject>(Row));
        }

        void Id(FName ItemId, const TSet<FName>& Canonical)
        {
            ++Items;
            if (!Canonical.Contains(ItemId)) UnknownItemIds.AddUnique(ItemId);
        }

        void Readable(FName ItemId, const UGridReadableContentAsset* Object, FName ReadableId)
        {
            if (IsValid(Object))
            {
                ++ReadablePointers;
                Ref(TEXT("ReadableContentAsset"), ItemId, Object,
                    !ReadableId.IsNone() && ReadableId != Object->ReadableContentId);
            }
            else if (!ReadableId.IsNone())
            {
                ++ReadableIdsWithoutPointer;
            }
        }

        void Instance(const FGridItemInstance& Value, const TSet<FName>& Canonical)
        {
            if (!Value.RuntimeObjectId.IsValid() && Value.ItemDefinitionId.IsNone() &&
                !Value.ReadableContentAsset && Value.ReadableContentId.IsNone()) return;
            Id(Value.ItemDefinitionId, Canonical);
            Readable(Value.ItemDefinitionId, Value.ReadableContentAsset.Get(), Value.ReadableContentId);
        }

        void World(const FGridRuntimeItemState& Value, const TSet<FName>& Canonical)
        {
            Id(Value.ItemDefinitionId, Canonical);
            Readable(Value.ItemDefinitionId, Value.ReadableContentAsset.Get(), Value.ReadableContentId);
        }

        void Collect(const UGrimrockPartySaveGame& Save, const TSet<FName>& Canonical)
        {
            const auto& Party = Save.PartyInventoryState;
            for (const auto* Group : {&Party.ActiveCharacters, &Party.CharacterPool})
                for (const auto& Character : *Group)
                    for (const auto& Slot : Character.InventorySlots)
                        Instance(Slot.Item, Canonical);
            for (const auto& Equipment : Party.ActiveEquipment)
                for (TFieldIterator<FStructProperty> Field(FGridCharacterEquipmentState::StaticStruct()); Field; ++Field)
                    if (Field->Struct == FGridItemInstance::StaticStruct())
                        Instance(*Field->ContainerPtrToValuePtr<FGridItemInstance>(&Equipment), Canonical);
            Instance(Party.CursorItem, Canonical);
            for (const auto& Level : Save.DungeonRuntimeState.LevelStates)
            {
                for (const auto& Entry : Level.Value.Items) World(Entry.Value, Canonical);
                for (const auto& Entry : Level.Value.Receptacles)
                    for (const auto& Contained : Entry.Value.ContainedItems) World(Contained, Canonical);
                for (const auto& Entry : Level.Value.PendingInboundItems)
                {
                    const auto& Value = Entry.Value;
                    World(Value.ItemState, Canonical);
                }
            }
        }

        void Write(FJsonObject& Row) const
        {
            Row.SetNumberField(TEXT("item_instances_and_world_rows"), Items);
            // The commandlet refuses a schema containing either property.
            // Retain the post-cutover compatibility key without reading C++ fixture members.
            Row.SetNumberField(TEXT("instance_legacy_name_or_weight"), 0);
            // 5G86: compatibility keys for the private 5A report readers.
            // The schema lacks these fields; zero here is not a decoder
            // proof about a pre-cutover archive. That proof is 5G85.
            Row.SetNumberField(TEXT("projectile_definition_pointers"), 0);
            Row.SetNumberField(TEXT("pending_definition_pointers"), 0);
            Row.SetNumberField(TEXT("readable_content_pointers"), ReadablePointers);
            Row.SetNumberField(TEXT("readable_ids_without_pointer"), ReadableIdsWithoutPointer);
            Row.SetArrayField(TEXT("unknown_item_ids"), Names(UnknownItemIds));
            Row.SetArrayField(TEXT("decoded_references"), References);
        }
    };
}
