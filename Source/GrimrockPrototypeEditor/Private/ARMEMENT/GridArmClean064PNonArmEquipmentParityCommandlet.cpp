#include "ARMEMENT/GridArmClean064PNonArmEquipmentParityCommandlet.h"

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

    TSharedPtr<FJsonObject> LoadJson(const FString& Path)
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

    FString StringField(const TSharedPtr<FJsonObject>& Source, const TCHAR* Key)
    {
        FString Result;
        if (Source.IsValid()) { Source->TryGetStringField(Key, Result); }
        return Result;
    }

    FString Export(const FProperty& Field, const UObject& Source)
    {
        FString Result;
        Field.ExportTextItem_Direct(Result,
            Field.ContainerPtrToValuePtr<void>(&Source), nullptr,
            const_cast<UObject*>(&Source), PPF_None);
        return Result;
    }

    bool CopyExactField(const UGridItemDefinitionAsset& Source,
        UGridOtherEquipmentDefinitionAsset& Target, const TCHAR* OldName,
        const TCHAR* NewName)
    {
        const FProperty* OldField = Source.GetClass()->FindPropertyByName(FName(OldName));
        const FProperty* NewField = Target.GetClass()->FindPropertyByName(FName(NewName));
        if (!OldField || !NewField || !NewField->SameType(OldField))
        {
            return false;
        }
        const void* SourcePtr = OldField->ContainerPtrToValuePtr<void>(&Source);
        void* DestinationPtr = NewField->ContainerPtrToValuePtr<void>(&Target);
        NewField->CopyCompleteValue(DestinationPtr, SourcePtr);
        return NewField->Identical(DestinationPtr, SourcePtr) &&
            Export(*OldField, Source) == Export(*NewField, Target);
    }

    bool ResolveAuthoredHandPolicy(EGridItemHandUsage Source,
        EGridNonArmamentHandPolicy& Target)
    {
        switch (Source)
        {
        case EGridItemHandUsage::NotHandHeld:
            Target = EGridNonArmamentHandPolicy::NotHandHeld;
            return true;
        case EGridItemHandUsage::OneHanded:
            Target = EGridNonArmamentHandPolicy::OneHanded;
            return true;
        case EGridItemHandUsage::TwoHanded:
            Target = EGridNonArmamentHandPolicy::TwoHanded;
            return true;
        case EGridItemHandUsage::Auto:
        default:
            return false; // No implicit Auto is allowed in canonical runtime.
        }
    }

    TArray<EGridEquipmentSlot> LegacyVisibleSlots(const UGridItemDefinitionAsset& Item)
    {
        TArray<EGridEquipmentSlot> Slots;
        if (Item.CanEquipToSlot(EGridEquipmentSlot::MainHand))
        {
            Slots.Add(EGridEquipmentSlot::MainHand);
        }
        if (Item.GetEffectiveHandUsage() != EGridItemHandUsage::TwoHanded &&
            Item.CanEquipToSlot(EGridEquipmentSlot::OffHand))
        {
            Slots.Add(EGridEquipmentSlot::OffHand);
        }
        for (const EGridEquipmentSlot Slot : Item.CompatibleEquipmentSlots)
        {
            if (Slot != EGridEquipmentSlot::None && Slot != EGridEquipmentSlot::MainHand &&
                Slot != EGridEquipmentSlot::OffHand && !Slots.Contains(Slot) &&
                Item.CanEquipToSlot(Slot))
            {
                Slots.Add(Slot);
            }
        }
        return Slots;
    }

    bool KnownEquipmentField(const FString& Name)
    {
        return Name == TEXT("HandUsage") ||
            Name == TEXT("CompatibleEquipmentSlots") ||
            Name == TEXT("EquipmentStatBonus") ||
            Name == TEXT("EquipmentResistanceBonus") ||
            Name == TEXT("EquippedMesh");
    }
}

UGridArmClean064PNonArmEquipmentParityCommandlet::
    UGridArmClean064PNonArmEquipmentParityCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064PNonArmEquipmentParityCommandlet::Main(const FString& Params)
{
    FString OwnershipPath;
    FString ActionsPath;
    FString OutputPath;
    if (!FParse::Value(*Params, TEXT("Ownership="), OwnershipPath) ||
        !FParse::Value(*Params, TEXT("ActionInventory="), ActionsPath) ||
        !FParse::Value(*Params, TEXT("Output="), OutputPath) ||
        OwnershipPath.IsEmpty() || ActionsPath.IsEmpty() || OutputPath.IsEmpty() ||
        FPaths::IsSamePath(OwnershipPath, ActionsPath) ||
        FPaths::IsSamePath(OwnershipPath, OutputPath) ||
        FPaths::IsSamePath(ActionsPath, OutputPath))
    {
        return 1;
    }

    const TSharedPtr<FJsonObject> Owners = LoadJson(OwnershipPath);
    const TSharedPtr<FJsonObject> Actions = LoadJson(ActionsPath);
    const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
    if (!Owners.IsValid() || !Actions.IsValid() ||
        StringField(Owners, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
        StringField(Actions, TEXT("schema")) != TEXT("ARM-CLEAN06.4O/nonarm-action-semantic-inventory/v1") ||
        !Owners->TryGetArrayField(TEXT("private_item_ownership"), Items) ||
        !Items || Items->Num() != 11 ||
        Owners->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
        Owners->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
        Owners->GetIntegerField(TEXT("distinct_active_unmapped_property_names")) != 26 ||
        Actions->GetIntegerField(TEXT("verified_item_ids")) != 11 ||
        Actions->GetIntegerField(TEXT("active_action_properties")) != 11 ||
        Actions->GetBoolField(TEXT("migration_authorized")) ||
        Owners->GetBoolField(TEXT("migration_authorized")))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Pinned input source changed."));
        return 2;
    }
    const TSharedPtr<FJsonObject>* Domains = nullptr;
    if (!Owners->TryGetObjectField(TEXT("domain_counts"), Domains) || !Domains ||
        (*Domains)->GetIntegerField(TEXT("NONARM_EQUIPMENT")) != 15)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Equipment ownership changed."));
        return 2;
    }

    const UGridItemSystemAsset* Root =
        LoadObject<UGridItemSystemAsset>(nullptr, CanonicalRoot);
    if (!IsValid(Root) || Root->ItemDefinitions.Num() != 23)
    {
        return 3;
    }
    const bool RootWasDirty = Root->GetOutermost()->IsDirty();
    TSet<FName> Registered;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Registered.Contains(Definition->ItemId)) { return 3; }
        Registered.Add(Definition->ItemId);
    }

    TSet<FName> Seen;
    int32 WorldOccurrences = 0;
    int32 ActiveFields = 0;
    int32 ActiveEquipment = 0;
    int32 ExactFieldCopies = 0;
    int32 HandMappings = 0;
    int32 DirectSlotProbes = 0;
    int32 VisibleSlotsProbes = 0;
    int32 StatBonusProbes = 0;
    int32 MeshFallbackProbes = 0;
    int32 DirtySourceChanges = 0;
    int32 AutoResolvedHandPolicies = 0;
    TArray<TSharedPtr<FJsonValue>> PrivateRows;

    for (const TSharedPtr<FJsonValue>& RawItem : *Items)
    {
        const TSharedPtr<FJsonObject> Item = RawItem.IsValid() ? RawItem->AsObject() : nullptr;
        const FName ItemId(*StringField(Item, TEXT("item_id")));
        const FString Path = StringField(Item, TEXT("source_asset"));
        const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
        if (!Item.IsValid() || ItemId.IsNone() || Seen.Contains(ItemId) ||
            Registered.Contains(ItemId) || Item->GetBoolField(TEXT("conversion_authorized")) ||
            !Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
            Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
            Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
            !Path.Contains(TEXT(".")) ||
            !Item->TryGetArrayField(TEXT("field_reviews"), Reviews) || !Reviews ||
            Reviews->Num() != Item->GetIntegerField(TEXT("active_unmapped_fields")))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Invalid real source scope."));
            return 4;
        }
        const UGridItemDefinitionAsset* Legacy =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        if (!IsValid(Legacy) || Legacy->ItemDefinitionId != ItemId ||
            !Legacy->WeaponArchetypeId.IsNone() || Legacy->bUseWeaponCatalogue)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Non-ARM assumption invalid."));
            return 4;
        }
        const bool SourceWasDirty = Legacy->GetOutermost()->IsDirty();
        Seen.Add(ItemId);
        WorldOccurrences += Item->GetIntegerField(TEXT("affected_world_occurrences"));

        TSet<FString> Encountered;
        int32 LocalActive = 0;
        for (const TSharedPtr<FJsonValue>& RawField : *Reviews)
        {
            const TSharedPtr<FJsonObject> Field = RawField.IsValid() ? RawField->AsObject() : nullptr;
            const FString Name = StringField(Field, TEXT("property"));
            const FString Domain = StringField(Field, TEXT("domain"));
            const FProperty* OldProperty = Legacy->GetClass()->FindPropertyByName(FName(*Name));
            if (!Field.IsValid() || Name.IsEmpty() || Encountered.Contains(Name) ||
                !OldProperty || Export(*OldProperty, *Legacy) !=
                StringField(Field, TEXT("source_value_exact")) ||
                StringField(Field, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Saved real field diverged."));
                return 5;
            }
            Encountered.Add(Name);
            ++ActiveFields;
            if (Domain == TEXT("NONARM_EQUIPMENT"))
            {
                if (!KnownEquipmentField(Name)) { return 5; }
                ++ActiveEquipment;
                ++LocalActive;
            }
        }

        UGridOtherEquipmentDefinitionAsset* Candidate =
            NewObject<UGridOtherEquipmentDefinitionAsset>(
                GetTransientPackage(), NAME_None, RF_Transient);
        if (!IsValid(Candidate) || Candidate->GetOutermost() != GetTransientPackage())
        {
            return 5;
        }
        Candidate->ItemId = ItemId;
        Candidate->WeightKg = Legacy->Weight;
        Candidate->WorldMesh = Legacy->WorldMesh;
        if (!CopyExactField(*Legacy, *Candidate, TEXT("CompatibleEquipmentSlots"), TEXT("EquipmentSlots")) ||
            !CopyExactField(*Legacy, *Candidate, TEXT("EquipmentStatBonus"), TEXT("Bonuses")) ||
            !CopyExactField(*Legacy, *Candidate, TEXT("EquipmentResistanceBonus"), TEXT("Resistances")) ||
            !CopyExactField(*Legacy, *Candidate, TEXT("EquippedMesh"), TEXT("EquippedMesh")))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Unreal FProperty copy diverged."));
            return 6;
        }
        ExactFieldCopies += 4;

        const EGridItemHandUsage SourceHand = Legacy->GetEffectiveHandUsage();
        if (!ResolveAuthoredHandPolicy(SourceHand, Candidate->HandPolicy))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Unknown source hand policy."));
            return 6;
        }
        if (Legacy->HandUsage == EGridItemHandUsage::Auto) { ++AutoResolvedHandPolicies; }
        ++HandMappings;

        // Explicit compatibility is a separate rule from the UI-visible
        // reservation of OffHand for two-handed items. Compare BOTH.
        for (uint8 Value = static_cast<uint8>(EGridEquipmentSlot::None);
             Value <= static_cast<uint8>(EGridEquipmentSlot::Bracers); ++Value)
        {
            const EGridEquipmentSlot Slot = static_cast<EGridEquipmentSlot>(Value);
            if (Legacy->CanEquipToSlot(Slot) != Candidate->CanEquipToSlot(Slot))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Slot permission mismatch."));
                return 6;
            }
            ++DirectSlotProbes;
        }
        if (LegacyVisibleSlots(*Legacy) != Candidate->GetAvailableSlots())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] UI slot order mismatch."));
            return 6;
        }
        ++VisibleSlotsProbes;

        // Non-ARM bonus and resistance sources remain independent of the
        // weapon catalogue; preserved fixed level N0 is NOT coerced to N1.
        for (const int32 Level : {0, 1, 20})
        {
            FGridItemInstance Instance;
            Instance.RuntimeObjectId = FGuid(6, 4, 16, static_cast<uint32>(Seen.Num()));
            Instance.ItemDefinitionId = ItemId;
            Instance.Quantity = 1;
            Instance.ItemLevel = Level;
            const FGridEquipmentStatBonus RuntimeBonus =
                Legacy->GetEffectiveEquipmentStatBonusForInstance(Instance);
            const FProperty* BonusProp = Candidate->GetClass()->FindPropertyByName(TEXT("Bonuses"));
            if (!BonusProp || Export(*BonusProp, *Candidate) != Export(
                    *Legacy->GetClass()->FindPropertyByName(TEXT("EquipmentStatBonus")), *Legacy) ||
                !FGridEquipmentStatBonus::StaticStruct()->CompareScriptStruct(
                    &RuntimeBonus, &Candidate->Bonuses, 0))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Level-indexed bonus mismatch."));
                return 6;
            }
            ++StatBonusProbes;
        }

        const TSoftObjectPtr<UStaticMesh> OldEffectiveMesh =
            Legacy->EquippedMesh.IsNull() ? Legacy->WorldMesh : Legacy->EquippedMesh;
        if (OldEffectiveMesh != Candidate->GetEffectiveEquippedMesh())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Equipped/world mesh fallback changed."));
            return 6;
        }
        ++MeshFallbackProbes;
        if (Legacy->GetOutermost()->IsDirty() != SourceWasDirty) { ++DirtySourceChanges; }

        TSharedRef<FJsonObject> Private = MakeShared<FJsonObject>();
        Private->SetStringField(TEXT("item_id"), ItemId.ToString());
        Private->SetStringField(TEXT("source_asset"), Path);
        Private->SetNumberField(TEXT("active_equipment_fields"), LocalActive);
        Private->SetNumberField(TEXT("exact_reflected_copies"), 4);
        Private->SetBoolField(TEXT("hand_auto_resolved"), Legacy->HandUsage == EGridItemHandUsage::Auto);
        Private->SetBoolField(TEXT("data_and_pure_policy_parity"), true);
        Private->SetBoolField(TEXT("runtime_pie_parity_approved"), false);
        PrivateRows.Add(MakeShared<FJsonValueObject>(Private));
    }

    if (Seen.Num() != 11 || WorldOccurrences != 40 || ActiveFields != 50 ||
        ActiveEquipment != 15 || ExactFieldCopies != 44 || HandMappings != 11 ||
        DirectSlotProbes != 209 || VisibleSlotsProbes != 11 ||
        StatBonusProbes != 33 || MeshFallbackProbes != 11 ||
        DirtySourceChanges != 0 || Root->GetOutermost()->IsDirty() != RootWasDirty)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Exact scope or immutable source failed."));
        return 7;
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4P/nonarm-equipment-parity/v1"));
    Report->SetNumberField(TEXT("verified_item_ids"), Seen.Num());
    Report->SetNumberField(TEXT("world_occurrences_preserved"), WorldOccurrences);
    Report->SetNumberField(TEXT("source_active_fields_reverified"), ActiveFields);
    Report->SetNumberField(TEXT("active_nonarm_equipment_fields"), ActiveEquipment);
    Report->SetNumberField(TEXT("exact_reflected_copies"), ExactFieldCopies);
    Report->SetNumberField(TEXT("explicit_hand_mappings"), HandMappings);
    Report->SetNumberField(TEXT("source_auto_hand_mappings"), AutoResolvedHandPolicies);
    Report->SetNumberField(TEXT("direct_slot_probes"), DirectSlotProbes);
    Report->SetNumberField(TEXT("visible_slot_probes"), VisibleSlotsProbes);
    Report->SetNumberField(TEXT("fixed_level_stat_bonus_probes"), StatBonusProbes);
    Report->SetNumberField(TEXT("equipped_mesh_fallback_probes"), MeshFallbackProbes);
    Report->SetNumberField(TEXT("source_dirty_state_changes"), DirtySourceChanges);
    Report->SetNumberField(TEXT("runtime_semantic_parity_approved"), 0);
    Report->SetBoolField(TEXT("binary_writes_performed"), false);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetBoolField(TEXT("migration_authorized"), false);
    Report->SetArrayField(TEXT("private_item_projections"), PrivateRows);
    FString Json;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *OutputPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4P] Private report writing failed."));
        return 8;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4P] 15 active equipment properties projected; no binary migration."));
    return 0;
}
