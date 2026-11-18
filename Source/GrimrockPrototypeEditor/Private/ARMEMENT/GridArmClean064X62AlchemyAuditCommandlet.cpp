#include "ARMEMENT/GridArmClean064X62AlchemyAuditCommandlet.h"

#include "GridEditorAlchemyMigrationAudit.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    namespace Audit = GridEditorAlchemyMigrationAudit;

    FString FullPath(FString Path)
    {
        Path = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeFilename(Path);
        FPaths::CollapseRelativeDirectories(Path);
        return Path;
    }

    bool SafeReport(const FString& Path)
    {
        if (Path.IsEmpty() || FPaths::IsRelative(Path) ||
            FPaths::GetExtension(Path) != TEXT("json") || FPaths::FileExists(Path))
            return false;
        FString Project = FullPath(FPaths::ProjectDir());
        FPaths::NormalizeDirectoryName(Project);
        const FString Absolute = FullPath(Path);
        return !Absolute.Equals(Project, ESearchCase::IgnoreCase) &&
            !Absolute.StartsWith(Project + TEXT("/"), ESearchCase::IgnoreCase) &&
            FPaths::DirectoryExists(FPaths::GetPath(Absolute));
    }

    TArray<TSharedPtr<FJsonValue>> Strings(const TArray<FString>& Values)
    {
        TArray<TSharedPtr<FJsonValue>> Json;
        for (const FString& Value : Values)
            Json.Add(MakeShared<FJsonValueString>(Value));
        return Json;
    }

    TArray<TSharedPtr<FJsonValue>> Names(const TArray<FName>& Values)
    {
        TArray<FString> Text;
        for (FName Value : Values) Text.Add(Value.ToString());
        Text.Sort();
        return Strings(Text);
    }

    void EffectiveStatusIds(const FGridCombatActionDefinition& Action, TSet<FName>& Ids)
    {
        for (const auto& Status : Action.StatusApplications)
            if (!Status.StatusEffectId.IsNone()) Ids.Add(Status.StatusEffectId);
        for (const auto& Surface : Action.SurfaceEffects)
            for (const auto& Status : Surface.PeriodicStatusApplications)
                if (!Status.StatusEffectId.IsNone()) Ids.Add(Status.StatusEffectId);
        for (const auto& Removal : Action.StatusRemovals)
            for (FName Id : Removal.EffectIds)
                if (!Id.IsNone()) Ids.Add(Id);
    }
}

UGridArmClean064X62AlchemyAuditCommandlet::UGridArmClean064X62AlchemyAuditCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064X62AlchemyAuditCommandlet::Main(const FString& Params)
{
    FString Output;
    if (!FParse::Param(*Params, TEXT("ReadOnly")) ||
        FParse::Param(*Params, TEXT("Apply")) ||
        FParse::Param(*Params, TEXT("Publish")) ||
        !FParse::Value(*Params, TEXT("Output="), Output) || !SafeReport(Output))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X62] Requires -ReadOnly and a new absolute .json outside the project."));
        return 1;
    }

    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, Audit::RootPath()));
    if (!Root.IsValid()) return 2;
    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    AssetRegistry.SearchAllAssets(true);
    TArray<Audit::FRow> Rows;
    FString Error;
    if (!Audit::Build(*Root.Get(), Rows, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4X62] Audit refused: %s"), *Error);
        return 3;
    }

    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4X62/alchemy-source-audit/v1"));
    Result->SetNumberField(TEXT("registry_count"), Root->ItemDefinitions.Num());
    Result->SetNumberField(TEXT("compact_action_count"), Root->ActionCatalogue->Actions.Num());
    Result->SetNumberField(TEXT("detailed_action_count"), Root->ActionCatalogue->DetailedActions.Num());
    Result->SetNumberField(TEXT("compact_presentation_count"), Root->ActionPresentationCatalogue->Presentations.Num());
    Result->SetNumberField(TEXT("detailed_presentation_count"), Root->ActionPresentationCatalogue->DetailedPresentations.Num());
    Result->SetNumberField(TEXT("unregistered_count"), Audit::LegacyCount);
    Result->SetBoolField(TEXT("migration_authorized"), false);
    Result->SetBoolField(TEXT("packages_saved"), false);
    Result->SetBoolField(TEXT("savegames_touched"), false);
    Result->SetStringField(TEXT("hotbar_identity_fields"), TEXT("SourcePolicy,SourceDefinitionId,ActionId,PreferredSourceRuntimeId"));
    TArray<TSharedPtr<FJsonValue>> Entries;
    TSet<FName> CollidingIds;
    TSet<FName> ReferencedStatuses;
    int32 IsolatedParity = 0;
    for (const Audit::FRow& Row : Rows)
    {
        const auto& Old = *Row.Source;
        const auto& Action = Row.Action;
        TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
        Record->SetStringField(TEXT("item_id"), Old.ItemDefinitionId.ToString());
        Record->SetStringField(TEXT("source_path"), Old.GetPathName());
        Record->SetBoolField(TEXT("already_registered"), Row.bRegistered);
        Record->SetNumberField(TEXT("source_item_type"), int32(Old.ItemType));
        Record->SetStringField(TEXT("source_item_type_name"), StaticEnum<EGridItemType>()->GetNameStringByValue(int64(Old.ItemType)));
        Record->SetNumberField(TEXT("weight_kg"), Old.Weight);
        Record->SetNumberField(TEXT("effective_max_stack"), Old.bStackable ? Old.MaxStackSize : 1);
        Record->SetArrayField(TEXT("item_tags"), Names(Old.ItemTags));
        Record->SetStringField(TEXT("action_id"), Action.ActionId.ToString());
        Record->SetStringField(TEXT("action_presentation_id"), Action.PresentationProfileId.ToString());
        Record->SetBoolField(TEXT("source_has_attack_presentation"), Old.bProvidesAttackPresentation);
        Record->SetNumberField(TEXT("action_point_cost"), Action.ActionPointCost);
        Record->SetNumberField(TEXT("range_cells"), Action.RangeCells);
        Record->SetNumberField(TEXT("cooldown_rounds"), Action.CooldownRounds);
        Record->SetNumberField(TEXT("quantity_cost"), Action.ResourceCosts.SourceItemQuantityCost);
        Record->SetNumberField(TEXT("targeting_policy"), int32(Action.TargetingPolicy));
        Record->SetNumberField(TEXT("resolution_profile"), int32(Action.ResolutionProfile));
        Record->SetNumberField(TEXT("status_applications"), Action.StatusApplications.Num());
        Record->SetNumberField(TEXT("status_removals"), Action.StatusRemovals.Num());
        Record->SetNumberField(TEXT("armor_effects"), Action.ArmorEffects.Num());
        Record->SetNumberField(TEXT("surface_effects"), Action.SurfaceEffects.Num());
        Record->SetBoolField(TEXT("existing_catalogue_collision"), Row.bExistingCatalogueCollision);
        Record->SetArrayField(TEXT("existing_action_differences"), Strings(Row.ExistingActionDifferences));
        Record->SetBoolField(TEXT("isolated_projection_exact"), Row.bIsolatedParity);
        Record->SetStringField(TEXT("projection_reason"), Row.ProjectionReason);
        Record->SetArrayField(TEXT("same_action_id_different_fields"), Strings(Row.PeerDifferences));
        TArray<FName> Peers;
        for (int32 Peer : Row.SameIdPeers)
            Peers.Add(Rows[Peer].Source->ItemDefinitionId);
        Record->SetArrayField(TEXT("same_action_id_items"), Names(Peers));
        if (!Peers.IsEmpty()) CollidingIds.Add(Action.ActionId);
        if (Row.bIsolatedParity) ++IsolatedParity;

        TArray<FName> Referencers;
        TArray<FName> Dependencies;
        AssetRegistry.GetReferencers(Old.GetOutermost()->GetFName(), Referencers,
            UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery());
        AssetRegistry.GetDependencies(Old.GetOutermost()->GetFName(), Dependencies,
            UE::AssetRegistry::EDependencyCategory::All, UE::AssetRegistry::FDependencyQuery());
        Record->SetArrayField(TEXT("package_referencers"), Names(Referencers));
        Record->SetArrayField(TEXT("package_dependencies"), Names(Dependencies));
        Record->SetStringField(TEXT("source_authored_properties"), FGridEditorNativeStoneLootPlan::Export(Old));
        Record->SetStringField(TEXT("effective_action_properties"), Audit::ActionExport(Action));
        TSet<FName> StatusIds;
        EffectiveStatusIds(Action, StatusIds);
        Record->SetArrayField(TEXT("explicit_status_ids"), Names(StatusIds.Array()));
        ReferencedStatuses.Append(StatusIds);
        Entries.Add(MakeShared<FJsonValueObject>(Record));
        UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4X62] %s Action=%s PA=%d cooldown=%d peers=%d isolated=%s reason=%s"),
            *Old.ItemDefinitionId.ToString(), *Action.ActionId.ToString(),
            Action.ActionPointCost, Action.CooldownRounds, Peers.Num(),
            Row.bIsolatedParity ? TEXT("EXACT") : (Row.bRegistered ? TEXT("ALREADY_REGISTERED") : TEXT("REVIEW")),
            *Row.ProjectionReason);
    }
    Result->SetArrayField(TEXT("rows"), Entries);
    Result->SetArrayField(TEXT("shared_action_ids"), Names(CollidingIds.Array()));
    Result->SetNumberField(TEXT("isolated_exact_count"), IsolatedParity);

    TArray<TSharedPtr<FJsonValue>> StatusRows;
    TArray<FName> StatusList = ReferencedStatuses.Array();
    StatusList.Sort(FNameLexicalLess());
    for (FName Id : StatusList)
    {
        const FString Path = FRPGAlchemistAuthoring::GetStatusObjectPath(Id);
        const auto* Status = LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Path);
        TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
        Record->SetStringField(TEXT("status_id"), Id.ToString());
        Record->SetStringField(TEXT("source_path"), Path);
        const bool bValid = IsValid(Status) && Status->EffectId == Id && Status->IsValidDefinition();
        Record->SetBoolField(TEXT("valid_source"), bValid);
        if (IsValid(Status))
        {
            if (Status->GetOutermost()->IsDirty()) return 4;
            Record->SetStringField(TEXT("source_authored_properties"),
                FGridEditorNativeStoneLootPlan::Export(*Status));
        }
        StatusRows.Add(MakeShared<FJsonValueObject>(Record));
    }
    Result->SetArrayField(TEXT("explicit_status_sources"), StatusRows);
    for (const Audit::FRow& Row : Rows)
        if (Row.Source->GetOutermost()->IsDirty()) return 4;
    const UObject* Protected[] = {Root.Get(), Root->WeaponCatalogue.Get(),
        Root->WeaponVisualCatalogue.Get(), Root->ActionCatalogue.Get(), Root->ActionPresentationCatalogue.Get()};
    for (const UObject* Object : Protected)
        if (Object->GetOutermost()->IsDirty()) return 4;

    FString Json;
    if (!FJsonSerializer::Serialize(Result, TJsonWriterFactory<>::Create(&Json)) ||
        !SafeReport(Output) ||
        !FFileHelper::SaveStringToFile(Json, *Output, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        return 5;
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4X62] READ ONLY COMPLETE rows=%d unregistered=%d isolated_exact=%d shared_ids=%d report=%s"),
        Rows.Num(), Audit::LegacyCount, IsolatedParity, CollidingIds.Num(), *Output);
    return 0;
}
