#include "ARMEMENT/GridArmClean064ONonArmActionSemanticsCommandlet.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

namespace
{
    const TCHAR* RootPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

    TSharedPtr<FJsonObject> ReadJson(const FString& Path)
    {
        FString Text;
        TSharedPtr<FJsonObject> Result;
        if (!FFileHelper::LoadFileToString(Text, *Path) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Result))
        {
            return nullptr;
        }
        return Result;
    }

    FString StringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
    {
        FString Result;
        if (Object.IsValid()) { Object->TryGetStringField(Field, Result); }
        return Result;
    }

    FString Export(const FProperty& Property, const void* Owner, UObject* ContainerOwner)
    {
        FString Result;
        Property.ExportTextItem_Direct(Result,
            Property.ContainerPtrToValuePtr<void>(Owner), nullptr, ContainerOwner, PPF_None);
        return Result;
    }

    bool IsKnownActionField(const FString& Field)
    {
        return Field == TEXT("CombatActions") ||
            Field == TEXT("bProvidesQuickItemCombatAction") ||
            Field == TEXT("QuickItemCombatAction") ||
            Field == TEXT("QuickItemActionIdOverride") ||
            Field == TEXT("bProvidesAttackPresentation") ||
            Field == TEXT("PlayerAttackPresentationProfile");
    }

    /** Keep actual authored values, not just their count. Everything is
     * emitted only to a private Windows file. No default action is invented. */
    TSharedRef<FJsonObject> SerializeAction(const FGridCombatActionDefinition& Action)
    {
        TSharedRef<FJsonObject> Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("action_id"), Action.ActionId.ToString());
        Row->SetBoolField(TEXT("source_valid"), Action.IsValid());
        TSharedRef<FJsonObject> Values = MakeShared<FJsonObject>();
        TArray<TSharedPtr<FJsonValue>> Changed;
        FGridCombatActionDefinition Default;
        for (TFieldIterator<FProperty> It(FGridCombatActionDefinition::StaticStruct()); It; ++It)
        {
            Values->SetStringField(It->GetName(), Export(**It, &Action, nullptr));
            if (!It->Identical_InContainer(&Action, &Default))
            {
                Changed.Add(MakeShared<FJsonValueString>(It->GetName()));
            }
        }
        Row->SetObjectField(TEXT("full_property_export"), Values);
        Row->SetArrayField(TEXT("nondefault_properties"), Changed);
        return Row;
    }
}

UGridArmClean064ONonArmActionSemanticsCommandlet::
    UGridArmClean064ONonArmActionSemanticsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064ONonArmActionSemanticsCommandlet::Main(const FString& Params)
{
    FString OwnerPath, ThrowPath, OutputPath;
    if (!FParse::Value(*Params, TEXT("Ownership="), OwnerPath) ||
        !FParse::Value(*Params, TEXT("GenericThrow="), ThrowPath) ||
        !FParse::Value(*Params, TEXT("Output="), OutputPath) ||
        OwnerPath.IsEmpty() || ThrowPath.IsEmpty() || OutputPath.IsEmpty() ||
        FPaths::IsSamePath(OwnerPath, ThrowPath) ||
        FPaths::IsSamePath(OwnerPath, OutputPath) ||
        FPaths::IsSamePath(ThrowPath, OutputPath))
    {
        return 1;
    }
    const TSharedPtr<FJsonObject> OwnerReport = ReadJson(OwnerPath);
    const TSharedPtr<FJsonObject> ThrowReport = ReadJson(ThrowPath);
    const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
    if (!OwnerReport.IsValid() || !ThrowReport.IsValid() ||
        StringField(OwnerReport, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
        StringField(ThrowReport, TEXT("schema")) != TEXT("ARM-CLEAN06.4N/nonarm-generic-throw-data-parity/v1") ||
        !OwnerReport->TryGetArrayField(TEXT("private_item_ownership"), Items) ||
        !Items || Items->Num() != 11 ||
        OwnerReport->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
        OwnerReport->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
        ThrowReport->GetIntegerField(TEXT("verified_item_ids")) != 11 ||
        ThrowReport->GetIntegerField(TEXT("exact_reflection_copies")) != 121 ||
        OwnerReport->GetBoolField(TEXT("migration_authorized")) ||
        ThrowReport->GetBoolField(TEXT("migration_authorized")))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4O] Private source scope drift."));
        return 2;
    }
    const TSharedPtr<FJsonObject>* Domains = nullptr;
    if (!OwnerReport->TryGetObjectField(TEXT("domain_counts"), Domains) || !Domains ||
        (*Domains)->GetIntegerField(TEXT("ITEM_ACTION")) != 11)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4O] Action ownership count drift."));
        return 2;
    }

    const UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RootPath);
    if (!IsValid(Root) || Root->ItemDefinitions.Num() != 23 ||
        !IsValid(Root->ActionCatalogue)) { return 3; }
    const bool WasRootDirty = Root->GetOutermost()->IsDirty();
    const bool WasCatalogueDirty = Root->ActionCatalogue->GetOutermost()->IsDirty();
    TSet<FName> Registered;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Registered.Contains(Definition->ItemId)) { return 3; }
        Registered.Add(Definition->ItemId);
    }
    TSet<FName> CatalogueActions;
    for (const FGridCanonicalAction& Action : Root->ActionCatalogue->Actions)
    {
        if (Action.ActionId.IsNone() || CatalogueActions.Contains(Action.ActionId)) { return 3; }
        CatalogueActions.Add(Action.ActionId);
    }

    int32 WorldOccurrences = 0, AllLegacyActive = 0, ActiveItemAction = 0;
    int32 AuthoredActions = 0, InvalidActions = 0, QuickEnabled = 0, QuickValid = 0;
    int32 PresentationsEnabled = 0, PresentationsValid = 0, CanonicalIdCollisions = 0;
    int32 NondefaultNestedFields = 0, DirtyChanges = 0;
    TSet<FName> Seen;
    TArray<TSharedPtr<FJsonValue>> PrivateItems;

    for (const TSharedPtr<FJsonValue>& Raw : *Items)
    {
        const TSharedPtr<FJsonObject> SourceRow = Raw.IsValid() ? Raw->AsObject() : nullptr;
        const FName ItemId(*StringField(SourceRow, TEXT("item_id")));
        const FString Path = StringField(SourceRow, TEXT("source_asset"));
        const TArray<TSharedPtr<FJsonValue>>* FieldReviews = nullptr;
        if (!SourceRow.IsValid() || ItemId.IsNone() || Seen.Contains(ItemId) ||
            Registered.Contains(ItemId) || SourceRow->GetBoolField(TEXT("conversion_authorized")) ||
            !Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
            Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
            Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
            !Path.Contains(TEXT(".")) ||
            !SourceRow->TryGetArrayField(TEXT("field_reviews"), FieldReviews) ||
            !FieldReviews || FieldReviews->Num() != SourceRow->GetIntegerField(TEXT("active_unmapped_fields")))
        {
            return 4;
        }
        const UGridItemDefinitionAsset* Source =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        if (!IsValid(Source) || Source->ItemDefinitionId != ItemId ||
            !Source->WeaponArchetypeId.IsNone()) { return 4; }
        const bool WasDirty = Source->GetOutermost()->IsDirty();
        Seen.Add(ItemId);
        WorldOccurrences += SourceRow->GetIntegerField(TEXT("affected_world_occurrences"));

        TArray<TSharedPtr<FJsonValue>> PrivateActiveActionFields;
        TSet<FString> Names;
        for (const TSharedPtr<FJsonValue>& RawField : *FieldReviews)
        {
            const TSharedPtr<FJsonObject> Field = RawField.IsValid() ? RawField->AsObject() : nullptr;
            const FString Name = StringField(Field, TEXT("property"));
            const FString Domain = StringField(Field, TEXT("domain"));
            const FProperty* P = Source->GetClass()->FindPropertyByName(FName(*Name));
            if (!Field.IsValid() || !P || Name.IsEmpty() || Names.Contains(Name) ||
                Export(*P, Source, const_cast<UGridItemDefinitionAsset*>(Source)) !=
                    StringField(Field, TEXT("source_value_exact")) ||
                StringField(Field, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4O] Legacy source changed."));
                return 5;
            }
            Names.Add(Name);
            ++AllLegacyActive;
            if (Domain == TEXT("ITEM_ACTION"))
            {
                if (!IsKnownActionField(Name)) { return 5; }
                ++ActiveItemAction;
                TSharedRef<FJsonObject> Record = MakeShared<FJsonObject>();
                Record->SetStringField(TEXT("property"), Name);
                Record->SetStringField(TEXT("source_value_exact"), Export(
                    *P, Source, const_cast<UGridItemDefinitionAsset*>(Source)));
                Record->SetStringField(TEXT("destination"), TEXT("ACTION_OR_PRESENTATION_CATALOGUE_REVIEW"));
                Record->SetBoolField(TEXT("semantic_parity_approved"), false);
                PrivateActiveActionFields.Add(MakeShared<FJsonValueObject>(Record));
            }
        }
        TSharedRef<FJsonObject> Private = MakeShared<FJsonObject>();
        Private->SetStringField(TEXT("item_id"), ItemId.ToString());
        Private->SetStringField(TEXT("source_asset"), Path);
        Private->SetArrayField(TEXT("active_action_properties"), PrivateActiveActionFields);
        Private->SetBoolField(TEXT("action_migration_authorized"), false);

        TArray<TSharedPtr<FJsonValue>> Actions;
        for (const FGridCombatActionDefinition& Action : Source->CombatActions)
        {
            TSharedRef<FJsonObject> Record = SerializeAction(Action);
            NondefaultNestedFields += Record->GetArrayField(TEXT("nondefault_properties")).Num();
            if (!Action.IsValid()) { ++InvalidActions; }
            if (!Action.ActionId.IsNone() && CatalogueActions.Contains(Action.ActionId))
            {
                ++CanonicalIdCollisions;
            }
            ++AuthoredActions;
            Actions.Add(MakeShared<FJsonValueObject>(Record));
        }
        Private->SetArrayField(TEXT("equipped_authored_actions_full"), Actions);
        if (Source->bProvidesQuickItemCombatAction)
        {
            ++QuickEnabled;
            FGridCombatActionDefinition Normalized;
            const bool bQuick = Source->BuildQuickItemCombatActionDefinition(Normalized);
            Private->SetBoolField(TEXT("normalized_quick_valid"), bQuick);
            if (bQuick)
            {
                ++QuickValid;
                if (CatalogueActions.Contains(Normalized.ActionId)) { ++CanonicalIdCollisions; }
                TSharedRef<FJsonObject> Record = SerializeAction(Normalized);
                NondefaultNestedFields += Record->GetArrayField(TEXT("nondefault_properties")).Num();
                Private->SetObjectField(TEXT("normalized_quick_full"), Record);
            }
            else
            {
                Private->SetObjectField(TEXT("raw_invalid_quick_full"),
                    SerializeAction(Source->QuickItemCombatAction));
            }
        }
        else
        {
            Private->SetBoolField(TEXT("normalized_quick_valid"), false);
        }
        if (Source->bProvidesAttackPresentation)
        {
            ++PresentationsEnabled;
            if (Source->HasValidPlayerAttackPresentation()) { ++PresentationsValid; }
        }
        Private->SetBoolField(TEXT("presentation_enabled"), Source->bProvidesAttackPresentation);
        Private->SetBoolField(TEXT("presentation_valid"), Source->HasValidPlayerAttackPresentation());
        // Capture full nested presentation source in private, even if disabled.
        const FProperty* PresentationField = Source->GetClass()->
            FindPropertyByName(FName(TEXT("PlayerAttackPresentationProfile")));
        if (!PresentationField) { return 5; }
        Private->SetStringField(TEXT("presentation_source_exact"), Export(
            *PresentationField, Source, const_cast<UGridItemDefinitionAsset*>(Source)));
        if (WasDirty != Source->GetOutermost()->IsDirty()) { ++DirtyChanges; }
        PrivateItems.Add(MakeShared<FJsonValueObject>(Private));
    }
    if (Seen.Num() != 11 || WorldOccurrences != 40 ||
        AllLegacyActive != 50 || ActiveItemAction != 11 || DirtyChanges != 0 ||
        Root->GetOutermost()->IsDirty() != WasRootDirty ||
        Root->ActionCatalogue->GetOutermost()->IsDirty() != WasCatalogueDirty)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4O] Scope or package dirty invariant failed."));
        return 6;
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4O/nonarm-action-semantic-inventory/v1"));
    Report->SetNumberField(TEXT("verified_item_ids"), Seen.Num());
    Report->SetNumberField(TEXT("world_occurrences_preserved"), WorldOccurrences);
    Report->SetNumberField(TEXT("legacy_active_fields_reverified"), AllLegacyActive);
    Report->SetNumberField(TEXT("active_action_properties"), ActiveItemAction);
    Report->SetNumberField(TEXT("legacy_equipped_action_records"), AuthoredActions);
    Report->SetNumberField(TEXT("legacy_equipped_invalid_actions"), InvalidActions);
    Report->SetNumberField(TEXT("quick_item_flags_enabled"), QuickEnabled);
    Report->SetNumberField(TEXT("quick_item_normalized_valid"), QuickValid);
    Report->SetNumberField(TEXT("presentations_enabled"), PresentationsEnabled);
    Report->SetNumberField(TEXT("presentations_valid"), PresentationsValid);
    Report->SetNumberField(TEXT("preexisting_canonical_action_id_collisions"), CanonicalIdCollisions);
    Report->SetNumberField(TEXT("nested_nondefault_action_fields"), NondefaultNestedFields);
    Report->SetNumberField(TEXT("semantic_parity_approved"), 0);
    Report->SetNumberField(TEXT("source_dirty_state_changes"), DirtyChanges);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetBoolField(TEXT("migration_authorized"), false);
    Report->SetArrayField(TEXT("private_item_action_inventory"), PrivateItems);
    FString Serialized;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Serialized)) ||
        !FFileHelper::SaveStringToFile(Serialized, *OutputPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4O] Private result write failed."));
        return 7;
    }
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4O] All 11 active non-ARM action properties captured. No migration authority."));
    return 0;
}
