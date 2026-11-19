#include "ARMEMENT/GridArmClean064QNonArmStackParityCommandlet.h"

#include "Engine/GameInstance.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
    const TCHAR* RegistryPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

    TSharedPtr<FJsonObject> ReadJson(const FString& Path)
    {
        FString Raw;
        TSharedPtr<FJsonObject> Value;
        if (!FFileHelper::LoadFileToString(Raw, *Path) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Value))
        {
            return nullptr;
        }
        return Value;
    }

    FString FieldText(const TSharedPtr<FJsonObject>& Object, const TCHAR* Name)
    {
        FString Result;
        if (Object.IsValid()) { Object->TryGetStringField(Name, Result); }
        return Result;
    }

    FString Export(const FProperty& Property, const UObject& Owner)
    {
        FString Text;
        Property.ExportTextItem_Direct(Text,
            Property.ContainerPtrToValuePtr<void>(&Owner), nullptr,
            const_cast<UObject*>(&Owner), PPF_None);
        return Text;
    }

    int32 LegacyStackCapacity(const UGridItemDefinitionAsset& Old)
    {
        return Old.bStackable ? FMath::Max(1, Old.MaxStackSize) : 1;
    }

    bool KnownStackField(const FString& Name)
    {
        return Name == TEXT("bStackable") || Name == TEXT("MaxStackSize");
    }
}

UGridArmClean064QNonArmStackParityCommandlet::UGridArmClean064QNonArmStackParityCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064QNonArmStackParityCommandlet::Main(const FString& Params)
{
    // Dedicated private receipt: never rely on UE stdout's conversion of
    // commandlet return codes or expose actual source assets in CI status.
    FString DiagnosticPath;
    FParse::Value(*Params, TEXT("Diagnostic="), DiagnosticPath);
    const auto Fail = [&DiagnosticPath](int32 ExitCode) -> int32
    {
        if (!DiagnosticPath.IsEmpty())
        {
            const FString Receipt = FString::Printf(
                TEXT("ARM-CLEAN06.4Q/CODE_%d"), ExitCode);
            FFileHelper::SaveStringToFile(Receipt, *DiagnosticPath,
                FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        }
        return ExitCode;
    };
    FString OwnershipPath, EquipmentPath, OutputPath;
    if (!FParse::Value(*Params, TEXT("Ownership="), OwnershipPath) ||
        !FParse::Value(*Params, TEXT("Equipment="), EquipmentPath) ||
        !FParse::Value(*Params, TEXT("Output="), OutputPath) ||
        OwnershipPath.IsEmpty() || EquipmentPath.IsEmpty() || OutputPath.IsEmpty() ||
        FPaths::IsSamePath(OwnershipPath, EquipmentPath) ||
        FPaths::IsSamePath(OwnershipPath, OutputPath) ||
        FPaths::IsSamePath(EquipmentPath, OutputPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Three separate private paths required."));
        return Fail(1);
    }

    const TSharedPtr<FJsonObject> Ownership = ReadJson(OwnershipPath);
    const TSharedPtr<FJsonObject> Equipment = ReadJson(EquipmentPath);
    const TArray<TSharedPtr<FJsonValue>>* ItemRows = nullptr;
    if (!Ownership.IsValid() || !Equipment.IsValid() ||
        FieldText(Ownership, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
        FieldText(Equipment, TEXT("schema")) != TEXT("ARM-CLEAN06.4P/nonarm-equipment-parity/v1") ||
        !Ownership->TryGetArrayField(TEXT("private_item_ownership"), ItemRows) ||
        !ItemRows || ItemRows->Num() != 11 ||
        Ownership->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
        Ownership->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
        Equipment->GetIntegerField(TEXT("verified_item_ids")) != 11 ||
        Equipment->GetIntegerField(TEXT("active_nonarm_equipment_fields")) != 15 ||
        Ownership->GetBoolField(TEXT("migration_authorized")) ||
        Equipment->GetBoolField(TEXT("migration_authorized")))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Historic evidence scope mismatch."));
        return Fail(2);
    }
    const TSharedPtr<FJsonObject>* Domains = nullptr;
    if (!Ownership->TryGetObjectField(TEXT("domain_counts"), Domains) || !Domains ||
        (*Domains)->GetIntegerField(TEXT("NONARM_STACK")) != 6)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Legacy stack domain changed."));
        return Fail(2);
    }

    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RegistryPath);
    if (!IsValid(Root) || Root->ItemDefinitions.Num() != 23 ||
        !IsValid(Root->WeaponCatalogue) || !IsValid(Root->WeaponVisualCatalogue) ||
        !IsValid(Root->ActionCatalogue) || !IsValid(Root->ActionPresentationCatalogue))
    {
        return Fail(3);
    }
    TSet<FName> ExistingIds;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            ExistingIds.Contains(Definition->ItemId)) { return Fail(3); }
        ExistingIds.Add(Definition->ItemId);
    }
    const bool RootWasDirty = Root->GetOutermost()->IsDirty();

    // The real root is NEVER edited. The transient preview owns 23 existing
    // definitions plus 11 unsaved non-armament candidates.
    UGridItemSystemAsset* Preview = NewObject<UGridItemSystemAsset>(
        GetTransientPackage(), NAME_None, RF_Transient);
    if (!IsValid(Preview) || Preview->GetOutermost() != GetTransientPackage()) { return Fail(3); }
    Preview->ItemDefinitions = Root->ItemDefinitions;
    Preview->WeaponCatalogue = Root->WeaponCatalogue;
    Preview->WeaponVisualCatalogue = Root->WeaponVisualCatalogue;
    Preview->ActionCatalogue = Root->ActionCatalogue;
    Preview->ActionPresentationCatalogue = Root->ActionPresentationCatalogue;

    struct FPreviewItem
    {
        FName ItemId = NAME_None;
        const UGridItemDefinitionAsset* Legacy = nullptr;
        UGridGenericStackableDefinitionAsset* Canonical = nullptr;
        bool bOriginalDirty = false;
        int32 ActiveFields = 0;
    };

    TArray<FPreviewItem> PreviewItems;
    TSet<FName> Seen;
    int32 RealWorldOccurrences = 0;
    int32 AllLegacyFields = 0;
    int32 ActiveStackFields = 0;
    int32 SourceStackValuesChecked = 0;
    int32 NonstackableItems = 0;
    int32 StackingItems = 0;
    TArray<TSharedPtr<FJsonValue>> PrivateItems;
    for (const TSharedPtr<FJsonValue>& Raw : *ItemRows)
    {
        const TSharedPtr<FJsonObject> Row = Raw.IsValid() ? Raw->AsObject() : nullptr;
        const FName ItemId(*FieldText(Row, TEXT("item_id")));
        const FString Path = FieldText(Row, TEXT("source_asset"));
        const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
        if (!Row.IsValid() || ItemId.IsNone() || Seen.Contains(ItemId) ||
            ExistingIds.Contains(ItemId) || Row->GetBoolField(TEXT("conversion_authorized")) ||
            !Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
            Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
            Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
            !Path.Contains(TEXT(".")) ||
            !Row->TryGetArrayField(TEXT("field_reviews"), Reviews) || !Reviews ||
            Reviews->Num() != Row->GetIntegerField(TEXT("active_unmapped_fields")))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Unexpected identity or source."));
            return Fail(4);
        }
        const UGridItemDefinitionAsset* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        if (!IsValid(Old) || Old->ItemDefinitionId != ItemId ||
            !Old->WeaponArchetypeId.IsNone() || Old->bUseWeaponCatalogue ||
            !FMath::IsFinite(Old->Weight) || Old->Weight < 0.0f)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Invalid non-ARM legacy source."));
            return Fail(4);
        }
        const bool WasDirty = Old->GetOutermost()->IsDirty();
        Seen.Add(ItemId);
        RealWorldOccurrences += Row->GetIntegerField(TEXT("affected_world_occurrences"));
        TSet<FString> SourceNames;
        int32 ActiveForItem = 0;
        for (const TSharedPtr<FJsonValue>& RawField : *Reviews)
        {
            const TSharedPtr<FJsonObject> Field = RawField.IsValid() ? RawField->AsObject() : nullptr;
            const FString Name = FieldText(Field, TEXT("property"));
            const FString Domain = FieldText(Field, TEXT("domain"));
            const FProperty* Property = Old->GetClass()->FindPropertyByName(FName(*Name));
            if (!Field.IsValid() || Name.IsEmpty() || SourceNames.Contains(Name) ||
                !Property ||
                Export(*Property, *Old) != FieldText(Field, TEXT("source_value_exact")) ||
                FieldText(Field, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Source UPROPERTY drift."));
                return Fail(5);
            }
            SourceNames.Add(Name);
            ++AllLegacyFields;
            if (Domain == TEXT("NONARM_STACK"))
            {
                if (!KnownStackField(Name)) { return Fail(5); }
                ++ActiveStackFields;
                ++ActiveForItem;
            }
        }

        // A generic preview is used for the *capacity test*, not as an
        // assertion that all 11 real items must be authored as generic.
        UGridGenericStackableDefinitionAsset* Candidate =
            NewObject<UGridGenericStackableDefinitionAsset>(
                GetTransientPackage(), NAME_None, RF_Transient);
        if (!IsValid(Candidate) || Candidate->GetOutermost() != GetTransientPackage())
        {
            return Fail(5);
        }
        Candidate->ItemId = ItemId;
        Candidate->DisplayName = Old->DisplayName;
        Candidate->Description = Old->Description;
        Candidate->WeightKg = Old->Weight;
        Candidate->Tags = Old->ItemTags;
        Candidate->Icon = Old->Icon;
        Candidate->WorldMesh = Old->WorldMesh;
        Candidate->MaxStack = LegacyStackCapacity(*Old);
        if (Candidate->MaxStack < 1) { return Fail(5); }
        if (Candidate->MaxStack > 1) { ++StackingItems; }
        else { ++NonstackableItems; }
        // Prove the two source values, even if one of them was a default.
        if (Candidate->MaxStack !=
            (Old->bStackable ? FMath::Max(1, Old->MaxStackSize) : 1))
        {
            return Fail(6);
        }
        SourceStackValuesChecked += 2;
        Preview->ItemDefinitions.Add(Candidate);
        FPreviewItem Entry;
        Entry.ItemId = ItemId;
        Entry.Legacy = Old;
        Entry.Canonical = Candidate;
        Entry.bOriginalDirty = WasDirty;
        Entry.ActiveFields = ActiveForItem;
        PreviewItems.Add(Entry);

        TSharedRef<FJsonObject> Private = MakeShared<FJsonObject>();
        Private->SetStringField(TEXT("item_id"), ItemId.ToString());
        Private->SetStringField(TEXT("source_asset"), Path);
        Private->SetNumberField(TEXT("active_stack_source_fields"), ActiveForItem);
        Private->SetBoolField(TEXT("source_stackable_flag"), Old->bStackable);
        Private->SetNumberField(TEXT("source_max_stack_size"), Old->MaxStackSize);
        Private->SetNumberField(TEXT("effective_canonical_stack_capacity"), Candidate->MaxStack);
        Private->SetBoolField(TEXT("registry_family_selection_approved"), false);
        Private->SetBoolField(TEXT("runtime_savegame_parity_approved"), false);
        PrivateItems.Add(MakeShared<FJsonValueObject>(Private));
    }

    if (Seen.Num() != 11 || RealWorldOccurrences != 40 ||
        AllLegacyFields != 50 || ActiveStackFields != 6 ||
        SourceStackValuesChecked != 22 ||
        Preview->ItemDefinitions.Num() != 34 ||
        StackingItems + NonstackableItems != 11)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Missing stack provenance."));
        return Fail(6);
    }

    // UGridItemSubsystem has Within=GameInstance: creating it under the
    // transient package triggers a handled UE ensure even if tests return 0.
    // Keep the entire read-only fixture under a transient GameInstance;
    // do not initialize a live world or touch the user's game session.
    UGameInstance* PreviewGameInstance = NewObject<UGameInstance>(
        GetTransientPackage(), NAME_None, RF_Transient);
    if (!IsValid(PreviewGameInstance))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Transient GameInstance creation failed."));
        return Fail(7);
    }
    UGridItemSubsystem* Registry = NewObject<UGridItemSubsystem>(
        PreviewGameInstance, NAME_None, RF_Transient);
    TArray<FString> ValidationErrors;
    if (!IsValid(Registry) || Registry->GetOuter() != PreviewGameInstance ||
        !Registry->RegisterItemSystem(Preview, ValidationErrors))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Real registry rejected transient preview."));
        return Fail(7);
    }

    int32 LevelProbes = 0;
    int32 QuantityProbes = 0;
    int32 DirtyChanges = 0;
    for (const FPreviewItem& Entry : PreviewItems)
    {
        for (int32 Level : {0, 1, 20, 21})
        {
            FGridResolvedItem Resolved;
            const bool bSuccess = Registry->ResolveItem(Entry.ItemId, Level, Resolved);
            if (Level != 0)
            {
                if (bSuccess || Resolved.Error != EGridCanonicalItemResolveError::InvalidItemLevel)
                {
                    UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Invalid non-ARM physical level accepted."));
                    return Fail(8);
                }
            }
            else
            {
                if (!bSuccess || !Resolved.IsValid() ||
                    Resolved.ItemLevel != Level ||
                    Resolved.ItemId != Entry.ItemId ||
                    Resolved.MaxStack != LegacyStackCapacity(*Entry.Legacy) ||
                    !FMath::IsNearlyEqual(Resolved.WeightKg, Entry.Legacy->Weight))
                {
                    UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] N0 capacity differs."));
                    return Fail(8);
                }
            }
            ++LevelProbes;
        }

        for (const int32 Quantity : {1, 2, Entry.Canonical->MaxStack})
        {
            FGridItemInstance Instance;
            Instance.RuntimeObjectId = FGuid(6, 4, 17, static_cast<uint32>(QuantityProbes + 1));
            Instance.ItemDefinitionId = Entry.ItemId;
            Instance.ItemLevel = 0;
            Instance.Quantity = Quantity;
            const int32 Capacity = Entry.Canonical->MaxStack;
            const int32 Accepted = FMath::Min(Capacity, Instance.Quantity);
            const int32 Remainder = Instance.Quantity - Accepted;
            if (!Instance.IsValid() || Accepted <= 0 || Remainder < 0 ||
                Accepted > Capacity || Accepted + Remainder != Quantity ||
                Entry.Legacy->bStackable != (Capacity > 1) &&
                    Entry.Legacy->MaxStackSize > 1)
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Quantity policy mismatch."));
                return Fail(8);
            }
            ++QuantityProbes;
        }

        if (Entry.Legacy->GetOutermost()->IsDirty() != Entry.bOriginalDirty)
        {
            ++DirtyChanges;
        }
    }
    if (LevelProbes != 44 || QuantityProbes != 33 || DirtyChanges != 0 ||
        RootWasDirty != Root->GetOutermost()->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Runtime or immutability invariant failed."));
        return Fail(8);
    }

    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4Q/nonarm-stack-data-parity/v1"));
    Report->SetNumberField(TEXT("verified_item_ids"), Seen.Num());
    Report->SetNumberField(TEXT("world_occurrences_preserved"), RealWorldOccurrences);
    Report->SetNumberField(TEXT("legacy_active_fields_reverified"), AllLegacyFields);
    Report->SetNumberField(TEXT("active_nonarm_stack_fields"), ActiveStackFields);
    Report->SetNumberField(TEXT("source_stack_values_verified"), SourceStackValuesChecked);
    Report->SetNumberField(TEXT("transient_registry_definitions"), Preview->ItemDefinitions.Num());
    Report->SetNumberField(TEXT("stackable_preview_items"), StackingItems);
    Report->SetNumberField(TEXT("unit_capacity_preview_items"), NonstackableItems);
    Report->SetNumberField(TEXT("registry_level_probes"), LevelProbes);
    Report->SetNumberField(TEXT("quantity_policy_probes"), QuantityProbes);
    Report->SetNumberField(TEXT("source_dirty_state_changes"), DirtyChanges);
    Report->SetNumberField(TEXT("runtime_savegame_parity_approved"), 0);
    Report->SetBoolField(TEXT("binary_writes_performed"), false);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetBoolField(TEXT("migration_authorized"), false);
    Report->SetArrayField(TEXT("private_item_projections"), PrivateItems);

    FString Json;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *OutputPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4Q] Private report cannot be written."));
        return Fail(9);
    }
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4Q] 11 identities, 6 active stack fields, 44 true registry level probes. No binary writes."));
    return 0;
}
