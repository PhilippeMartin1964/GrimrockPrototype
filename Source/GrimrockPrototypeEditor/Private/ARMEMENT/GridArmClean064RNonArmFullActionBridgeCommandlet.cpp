#include "ARMEMENT/GridArmClean064RNonArmFullActionBridgeCommandlet.h"

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
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
    const TCHAR* CanonicalRoot =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

    TSharedPtr<FJsonObject> ReadJson(const FString& Path)
    {
        FString Content;
        TSharedPtr<FJsonObject> Parsed;
        if (!FFileHelper::LoadFileToString(Content, *Path) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Content), Parsed))
        {
            return nullptr;
        }
        return Parsed;
    }

    FString FieldString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name)
    {
        FString Value;
        if (Object.IsValid()) { Object->TryGetStringField(Name, Value); }
        return Value;
    }

    FString ExportField(const FProperty& Field, const void* StructOrObject, UObject* Owner)
    {
        FString Result;
        Field.ExportTextItem_Direct(Result,
            Field.ContainerPtrToValuePtr<void>(StructOrObject), nullptr, Owner, PPF_None);
        return Result;
    }

    bool CompareCompleteAction(const FGridCombatActionDefinition& Source,
        const FGridCombatActionDefinition& Candidate)
    {
        const UScriptStruct* Type = FGridCombatActionDefinition::StaticStruct();
        if (!Type->CompareScriptStruct(&Source, &Candidate, 0)) { return false; }
        for (TFieldIterator<FProperty> It(Type); It; ++It)
        {
            if (!It->Identical_InContainer(&Source, &Candidate) ||
                ExportField(**It, &Source, nullptr) !=
                    ExportField(**It, &Candidate, nullptr))
            {
                return false;
            }
        }
        return true;
    }

    bool MatchesPrivateSnapshot(const FGridCombatActionDefinition& Action,
        const TSharedPtr<FJsonObject>& SourceRow)
    {
        if (!SourceRow.IsValid() ||
            FieldString(SourceRow, TEXT("action_id")) != Action.ActionId.ToString() ||
            SourceRow->GetBoolField(TEXT("source_valid")) != Action.IsValid())
        {
            return false;
        }
        const TSharedPtr<FJsonObject>* Fields = nullptr;
        if (!SourceRow->TryGetObjectField(TEXT("full_property_export"), Fields) ||
            !Fields || !Fields->IsValid())
        {
            return false;
        }
        int32 Count = 0;
        for (TFieldIterator<FProperty> It(FGridCombatActionDefinition::StaticStruct()); It; ++It)
        {
            FString Expected;
            if (!(*Fields)->TryGetStringField(It->GetName(), Expected) ||
                Expected != ExportField(**It, &Action, nullptr))
            {
                return false;
            }
            ++Count;
        }
        return (*Fields)->Values.Num() == Count;
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
}

UGridArmClean064RNonArmFullActionBridgeCommandlet::
    UGridArmClean064RNonArmFullActionBridgeCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064RNonArmFullActionBridgeCommandlet::Main(const FString& Params)
{
    FString OwnershipPath, HistoricalActionsPath, StackPath, OutputPath;
    if (!FParse::Value(*Params, TEXT("Ownership="), OwnershipPath) ||
        !FParse::Value(*Params, TEXT("ActionInventory="), HistoricalActionsPath) ||
        !FParse::Value(*Params, TEXT("Stack="), StackPath) ||
        !FParse::Value(*Params, TEXT("Output="), OutputPath) ||
        OwnershipPath.IsEmpty() || HistoricalActionsPath.IsEmpty() ||
        StackPath.IsEmpty() || OutputPath.IsEmpty() ||
        FPaths::IsSamePath(OwnershipPath, OutputPath) ||
        FPaths::IsSamePath(HistoricalActionsPath, OutputPath) ||
        FPaths::IsSamePath(StackPath, OutputPath))
    {
        return 1;
    }
    const TSharedPtr<FJsonObject> Ownership = ReadJson(OwnershipPath);
    const TSharedPtr<FJsonObject> Historical = ReadJson(HistoricalActionsPath);
    const TSharedPtr<FJsonObject> Stack = ReadJson(StackPath);
    const TArray<TSharedPtr<FJsonValue>>* Owners = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* HistoricItems = nullptr;
    if (!Ownership.IsValid() || !Historical.IsValid() || !Stack.IsValid() ||
        FieldString(Ownership, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
        FieldString(Historical, TEXT("schema")) != TEXT("ARM-CLEAN06.4O/nonarm-action-semantic-inventory/v1") ||
        FieldString(Stack, TEXT("schema")) != TEXT("ARM-CLEAN06.4Q/nonarm-stack-data-parity/v1") ||
        !Ownership->TryGetArrayField(TEXT("private_item_ownership"), Owners) ||
        !Historical->TryGetArrayField(TEXT("private_item_action_inventory"), HistoricItems) ||
        !Owners || !HistoricItems || Owners->Num() != 11 || HistoricItems->Num() != 11 ||
        Ownership->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
        Ownership->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
        Historical->GetIntegerField(TEXT("active_action_properties")) != 11 ||
        Historical->GetIntegerField(TEXT("legacy_equipped_action_records")) != 1 ||
        Historical->GetIntegerField(TEXT("quick_item_normalized_valid")) != 3 ||
        Historical->GetIntegerField(TEXT("presentations_valid")) != 1 ||
        Historical->GetIntegerField(TEXT("nested_nondefault_action_fields")) != 54 ||
        Stack->GetIntegerField(TEXT("verified_item_ids")) != 11 ||
        Stack->GetIntegerField(TEXT("active_nonarm_stack_fields")) != 6 ||
        Ownership->GetBoolField(TEXT("migration_authorized")) ||
        Historical->GetBoolField(TEXT("migration_authorized")) ||
        Stack->GetBoolField(TEXT("migration_authorized")))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4R] Historical chain or immutable scope invalid."));
        return 2;
    }
    const TSharedPtr<FJsonObject>* Domains = nullptr;
    if (!Ownership->TryGetObjectField(TEXT("domain_counts"), Domains) || !Domains ||
        (*Domains)->GetIntegerField(TEXT("ITEM_ACTION")) != 11)
    {
        return 2;
    }

    const UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, CanonicalRoot);
    if (!IsValid(Root) || !IsValid(Root->ActionCatalogue) ||
        !IsValid(Root->ActionPresentationCatalogue) ||
        Root->ItemDefinitions.Num() != 23) { return 3; }
    const bool RootDirty = Root->GetOutermost()->IsDirty();
    const bool ActionsDirty = Root->ActionCatalogue->GetOutermost()->IsDirty();
    const bool PresentationsDirty = Root->ActionPresentationCatalogue->GetOutermost()->IsDirty();

    UGridActionCatalogueAsset* ActionPreview =
        NewObject<UGridActionCatalogueAsset>(GetTransientPackage(), NAME_None, RF_Transient);
    UGridActionPresentationCatalogueAsset* PresentationPreview =
        NewObject<UGridActionPresentationCatalogueAsset>(GetTransientPackage(), NAME_None, RF_Transient);
    UGridItemSystemAsset* RegistryPreview =
        NewObject<UGridItemSystemAsset>(GetTransientPackage(), NAME_None, RF_Transient);
    if (!IsValid(ActionPreview) || !IsValid(PresentationPreview) || !IsValid(RegistryPreview))
    {
        return 3;
    }
    ActionPreview->Actions = Root->ActionCatalogue->Actions;
    ActionPreview->DetailedActions = Root->ActionCatalogue->DetailedActions;
    PresentationPreview->Presentations = Root->ActionPresentationCatalogue->Presentations;
    PresentationPreview->DetailedPresentations =
        Root->ActionPresentationCatalogue->DetailedPresentations;
    RegistryPreview->ItemDefinitions = Root->ItemDefinitions;
    RegistryPreview->WeaponCatalogue = Root->WeaponCatalogue;
    RegistryPreview->WeaponVisualCatalogue = Root->WeaponVisualCatalogue;
    RegistryPreview->ActionCatalogue = ActionPreview;
    RegistryPreview->ActionPresentationCatalogue = PresentationPreview;

    TSet<FName> Registered;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Registered.Contains(Definition->ItemId)) { return 3; }
        Registered.Add(Definition->ItemId);
    }
    TMap<FName, TSharedPtr<FJsonObject>> HistoryById;
    for (const TSharedPtr<FJsonValue>& Entry : *HistoricItems)
    {
        const TSharedPtr<FJsonObject> Row = Entry.IsValid() ? Entry->AsObject() : nullptr;
        const FName Id(*FieldString(Row, TEXT("item_id")));
        if (!Row.IsValid() || Id.IsNone() || HistoryById.Contains(Id) ||
            Row->GetBoolField(TEXT("action_migration_authorized")))
        {
            return 3;
        }
        HistoryById.Add(Id, Row);
    }
    TSet<FName> Seen;
    TSet<FName> StagedActionIds;
    int32 Occurrences = 0, ActiveFields = 0, ActiveActionFields = 0;
    int32 DetailedActions = 0, QuickActions = 0, Presentations = 0;
    int32 VerifiedProperties = 0, NondefaultProperties = 0;
    int32 PackageDirtyChanges = 0;
    int32 BindingPlansNotApproved = 0;
    TArray<TSharedPtr<FJsonValue>> PrivateRows;

    for (const TSharedPtr<FJsonValue>& Raw : *Owners)
    {
        const TSharedPtr<FJsonObject> OwnerRow = Raw.IsValid() ? Raw->AsObject() : nullptr;
        const FName Id(*FieldString(OwnerRow, TEXT("item_id")));
        const FString Path = FieldString(OwnerRow, TEXT("source_asset"));
        const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
        const TSharedPtr<FJsonObject>* SavedPointer = HistoryById.Find(Id);
        if (!OwnerRow.IsValid() || Id.IsNone() || Seen.Contains(Id) ||
            Registered.Contains(Id) || !SavedPointer || !SavedPointer->IsValid() ||
            !Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
            Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
            Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
            !Path.Contains(TEXT(".")) ||
            !OwnerRow->TryGetArrayField(TEXT("field_reviews"), Reviews) ||
            !Reviews || Reviews->Num() != OwnerRow->GetIntegerField(TEXT("active_unmapped_fields")) ||
            FieldString(*SavedPointer, TEXT("source_asset")) != Path)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4R] Private action source identity drift."));
            return 4;
        }
        const UGridItemDefinitionAsset* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        if (!IsValid(Old) || Old->ItemDefinitionId != Id ||
            !Old->WeaponArchetypeId.IsNone()) { return 4; }
        const bool OldDirty = Old->GetOutermost()->IsDirty();
        Seen.Add(Id);
        Occurrences += OwnerRow->GetIntegerField(TEXT("affected_world_occurrences"));

        TSet<FString> SeenFields;
        for (const TSharedPtr<FJsonValue>& R : *Reviews)
        {
            const TSharedPtr<FJsonObject> Review = R.IsValid() ? R->AsObject() : nullptr;
            const FString Name = FieldString(Review, TEXT("property"));
            const FString Domain = FieldString(Review, TEXT("domain"));
            const FProperty* Property = Old->GetClass()->FindPropertyByName(FName(*Name));
            if (!Review.IsValid() || Name.IsEmpty() || SeenFields.Contains(Name) ||
                !Property ||
                ExportField(*Property, Old, const_cast<UGridItemDefinitionAsset*>(Old)) !=
                    FieldString(Review, TEXT("source_value_exact")) ||
                FieldString(Review, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                return 5;
            }
            SeenFields.Add(Name);
            ++ActiveFields;
            if (Domain == TEXT("ITEM_ACTION"))
            {
                if (!IsKnownActionField(Name)) { return 5; }
                ++ActiveActionFields;
            }
        }

        UGridNonArmamentDefinitionAsset* Definition =
            NewObject<UGridNonArmamentDefinitionAsset>(
                GetTransientPackage(), NAME_None, RF_Transient);
        if (!IsValid(Definition)) { return 5; }
        Definition->ItemId = Id;
        Definition->WeightKg = Old->Weight;
        Definition->DisplayName = Old->DisplayName;
        Definition->Description = Old->Description;
        Definition->Tags = Old->ItemTags;
        RegistryPreview->ItemDefinitions.Add(Definition);

        const TSharedPtr<FJsonObject> Saved = *SavedPointer;
        const TArray<TSharedPtr<FJsonValue>>* StoredEquipped = nullptr;
        if (!Saved->TryGetArrayField(TEXT("equipped_authored_actions_full"), StoredEquipped) ||
            !StoredEquipped || StoredEquipped->Num() != Old->CombatActions.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4R] Legacy action count differs from 06.4O."));
            return 5;
        }
        TArray<TSharedPtr<FJsonValue>> Bindings;
        const auto AppendAction = [&](const FGridCombatActionDefinition& Source,
            const TSharedPtr<FJsonObject>& Snapshot, bool bQuick) -> bool
        {
            if (!Source.IsValid() || Source.ActionId.IsNone() ||
                StagedActionIds.Contains(Source.ActionId) ||
                ActionPreview->Find(Source.ActionId) ||
                ActionPreview->FindDetailed(Source.ActionId) ||
                !MatchesPrivateSnapshot(Source, Snapshot))
            {
                return false;
            }
            StagedActionIds.Add(Source.ActionId);
            FGridCanonicalRuntimeAction New;
            New.Definition = Source;
            ActionPreview->DetailedActions.Add(MoveTemp(New));
            const FGridCombatActionDefinition* Candidate =
                ActionPreview->FindDetailed(Source.ActionId);
            if (!Candidate || !CompareCompleteAction(Source, *Candidate)) { return false; }
            int32 FieldsChecked = 0;
            for (TFieldIterator<FProperty> It(FGridCombatActionDefinition::StaticStruct()); It; ++It)
            {
                ++FieldsChecked;
            }
            VerifiedProperties += FieldsChecked;
            NondefaultProperties += Snapshot->GetArrayField(TEXT("nondefault_properties")).Num();
            ++DetailedActions;
            if (bQuick) { ++QuickActions; }
            TSharedRef<FJsonObject> Link = MakeShared<FJsonObject>();
            Link->SetStringField(TEXT("action_id"), Source.ActionId.ToString());
            Link->SetStringField(TEXT("source_policy"),
                bQuick ? TEXT("QUICK_ITEM") : TEXT("EQUIPMENT"));
            Link->SetBoolField(TEXT("semantic_execution_approved"), false);
            Link->SetBoolField(TEXT("presentation_binding_approved"), false);
            Bindings.Add(MakeShared<FJsonValueObject>(Link));
            ++BindingPlansNotApproved;
            return true;
        };
        for (int32 i = 0; i < Old->CombatActions.Num(); ++i)
        {
            const TSharedPtr<FJsonObject> Snapshot =
                (*StoredEquipped)[i].IsValid() ? (*StoredEquipped)[i]->AsObject() : nullptr;
            if (!AppendAction(Old->CombatActions[i], Snapshot, false))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4R] Equipped action data mismatch."));
                return 6;
            }
        }
        if (Old->bProvidesQuickItemCombatAction)
        {
            FGridCombatActionDefinition Quick;
            if (!Old->BuildQuickItemCombatActionDefinition(Quick) ||
                !Saved->GetBoolField(TEXT("normalized_quick_valid")))
            {
                return 6;
            }
            const TSharedPtr<FJsonObject>* QuickSnapshot = nullptr;
            if (!Saved->TryGetObjectField(TEXT("normalized_quick_full"), QuickSnapshot) ||
                !QuickSnapshot || !AppendAction(Quick, *QuickSnapshot, true))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4R] Normalized QuickItem drift."));
                return 6;
            }
        }
        if (Old->bProvidesAttackPresentation)
        {
            if (!Old->HasValidPlayerAttackPresentation() ||
                !Saved->GetBoolField(TEXT("presentation_valid")))
            {
                return 6;
            }
            const FProperty* P = Old->GetClass()->FindPropertyByName(
                TEXT("PlayerAttackPresentationProfile"));
            if (!P || ExportField(*P, Old, const_cast<UGridItemDefinitionAsset*>(Old)) !=
                FieldString(Saved, TEXT("presentation_source_exact")))
            {
                return 6;
            }
            // This is a STAGING key only, never a published binding or
            // an invented player-facing action/presentation name.
            const FName DraftId(*FString::Printf(TEXT("NonArmPreview_%s"), *Id.ToString()));
            if (PresentationPreview->Find(DraftId) ||
                PresentationPreview->FindDetailed(DraftId))
            {
                return 6;
            }
            FGridCanonicalRuntimePresentation Draft;
            Draft.PresentationId = DraftId;
            Draft.Profile = Old->PlayerAttackPresentationProfile;
            PresentationPreview->DetailedPresentations.Add(MoveTemp(Draft));
            const FGridPlayerAttackPresentationProfile* Projected =
                PresentationPreview->FindDetailed(DraftId);
            if (!Projected ||
                !FGridPlayerAttackPresentationProfile::StaticStruct()->CompareScriptStruct(
                    &Old->PlayerAttackPresentationProfile, Projected, 0))
            {
                return 6;
            }
            ++Presentations;
        }
        TSharedRef<FJsonObject> Private = MakeShared<FJsonObject>();
        Private->SetStringField(TEXT("item_id"), Id.ToString());
        Private->SetStringField(TEXT("source_asset"), Path);
        Private->SetArrayField(TEXT("candidate_unapproved_bindings"), Bindings);
        Private->SetBoolField(TEXT("semantic_parity_approved"), false);
        Private->SetBoolField(TEXT("binary_migration_authorized"), false);
        PrivateRows.Add(MakeShared<FJsonValueObject>(Private));
        if (OldDirty != Old->GetOutermost()->IsDirty()) { ++PackageDirtyChanges; }
    }

    TArray<FString> ValidationErrors;
    if (Seen.Num() != 11 || Occurrences != 40 || ActiveFields != 50 ||
        ActiveActionFields != 11 || DetailedActions != 4 || QuickActions != 3 ||
        Presentations != 1 || NondefaultProperties != 54 ||
        RegistryPreview->ItemDefinitions.Num() != 34 || PackageDirtyChanges != 0 ||
        !RegistryPreview->Validate(ValidationErrors) ||
        RootDirty != Root->GetOutermost()->IsDirty() ||
        ActionsDirty != Root->ActionCatalogue->GetOutermost()->IsDirty() ||
        PresentationsDirty != Root->ActionPresentationCatalogue->GetOutermost()->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4R] Full action staging/catalogue validation failed."));
        return 7;
    }

    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4R/nonarm-lossless-action-bridge/v1"));
    Report->SetNumberField(TEXT("verified_item_ids"), Seen.Num());
    Report->SetNumberField(TEXT("world_occurrences_preserved"), Occurrences);
    Report->SetNumberField(TEXT("legacy_active_fields_reverified"), ActiveFields);
    Report->SetNumberField(TEXT("active_action_fields"), ActiveActionFields);
    Report->SetNumberField(TEXT("detailed_actions_projected"), DetailedActions);
    Report->SetNumberField(TEXT("quick_actions_projected"), QuickActions);
    Report->SetNumberField(TEXT("detailed_presentations_projected"), Presentations);
    Report->SetNumberField(TEXT("full_property_exports_compared"), VerifiedProperties);
    Report->SetNumberField(TEXT("nondefault_nested_properties_preserved"), NondefaultProperties);
    Report->SetNumberField(TEXT("candidate_bindings_not_approved"), BindingPlansNotApproved);
    Report->SetNumberField(TEXT("transient_registry_definitions"), RegistryPreview->ItemDefinitions.Num());
    Report->SetNumberField(TEXT("package_dirty_state_changes"), PackageDirtyChanges);
    Report->SetNumberField(TEXT("approved_runtime_action_routes"), 0);
    Report->SetBoolField(TEXT("binary_writes_performed"), false);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetBoolField(TEXT("migration_authorized"), false);
    Report->SetArrayField(TEXT("private_item_action_projections"), PrivateRows);
    FString Json;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *OutputPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return 8;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4R] Four full actions and one full presentation staged in canonical catalogues. No binary conversion."));
    return 0;
}
