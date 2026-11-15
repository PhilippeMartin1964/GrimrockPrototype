#include "ARMEMENT/GridArmClean064NNonArmGenericThrowCommandlet.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalNonArmamentThrowReadModel.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
    const TCHAR* RootPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

    TSharedPtr<FJsonObject> ReadJson(const FString& Path)
    {
        FString Raw;
        TSharedPtr<FJsonObject> Obj;
        if (!FFileHelper::LoadFileToString(Raw, *Path) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Obj))
        {
            return nullptr;
        }
        return Obj;
    }

    FString FieldString(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Field)
    {
        FString Value;
        if (Obj.IsValid()) { Obj->TryGetStringField(Field, Value); }
        return Value;
    }

    FString Export(const FProperty& Property, const void* Container, UObject* Owner)
    {
        FString Result;
        Property.ExportTextItem_Direct(Result,
            Property.ContainerPtrToValuePtr<void>(Container), nullptr, Owner, PPF_None);
        return Result;
    }

    struct FFieldMap
    {
        const TCHAR* Source;
        const TCHAR* Target;
    };

    const TArray<FFieldMap>& FieldMappings()
    {
        static const TArray<FFieldMap> Fields = {
            { TEXT("bCombatThrowWeapon"), TEXT("bCombatRecoverable") },
            { TEXT("ThrowSpeed"), TEXT("InitialSpeed") },
            { TEXT("ThrowArc"), TEXT("Arc") },
            { TEXT("ThrowLifeSeconds"), TEXT("LifeSeconds") },
            { TEXT("ThrowImpactDropOffset"), TEXT("ImpactDropOffset") },
            { TEXT("ThrowVisualRelativeRotation"), TEXT("VisualRelativeRotation") },
            { TEXT("ThrowVisualRelativeScale"), TEXT("VisualRelativeScale") },
            { TEXT("ProjectileMeshForwardAxis"), TEXT("MeshForwardAxis") },
            { TEXT("ThrowVisualMode"), TEXT("VisualMode") },
            { TEXT("ThrowVisualTumbleAxis"), TEXT("VisualTumbleAxis") },
            { TEXT("ThrowVisualTumbleDegreesPerSecond"), TEXT("VisualTumbleDegreesPerSecond") },
            { TEXT("ThrowVisualSpinDegreesPerSecond"), TEXT("VisualSpinDegreesPerSecond") }
        };
        return Fields;
    }

    /** Visual enum uses a new non-weapon type; assert explicit ordinal mapping
     * instead of treating two unrelated EEnumProperty instances as identical. */
    bool MapVisualMode(const UGridItemDefinitionAsset& Legacy,
        FGridNonArmamentThrowPolicy& Target)
    {
        switch (Legacy.ThrowVisualMode)
        {
        case EGridThrowVisualMode::Stable:
            Target.VisualMode = EGridNonArmamentThrowVisualMode::Stable;
            break;
        case EGridThrowVisualMode::Tumble:
            Target.VisualMode = EGridNonArmamentThrowVisualMode::Tumble;
            break;
        case EGridThrowVisualMode::Spin:
            Target.VisualMode = EGridNonArmamentThrowVisualMode::Spin;
            break;
        default:
            return false;
        }
        return (Legacy.ThrowVisualMode == EGridThrowVisualMode::Stable &&
            Target.VisualMode == EGridNonArmamentThrowVisualMode::Stable) ||
            (Legacy.ThrowVisualMode == EGridThrowVisualMode::Tumble &&
                Target.VisualMode == EGridNonArmamentThrowVisualMode::Tumble) ||
            (Legacy.ThrowVisualMode == EGridThrowVisualMode::Spin &&
                Target.VisualMode == EGridNonArmamentThrowVisualMode::Spin);
    }

    bool CopyExact(const UGridItemDefinitionAsset& Source,
        UGridNonArmamentDefinitionAsset& Candidate, const FFieldMap& Mapping)
    {
        const FProperty* A = Source.GetClass()->FindPropertyByName(FName(Mapping.Source));
        const FProperty* B = FGridNonArmamentThrowPolicy::StaticStruct()->
            FindPropertyByName(FName(Mapping.Target));
        if (!A || !B) { return false; }
        if (FString(Mapping.Source) == TEXT("ThrowVisualMode"))
        {
            return MapVisualMode(Source, Candidate.GenericThrow);
        }
        if (!B->SameType(A)) { return false; }
        const void* OldPtr = A->ContainerPtrToValuePtr<void>(&Source);
        void* NewPtr = B->ContainerPtrToValuePtr<void>(&Candidate.GenericThrow);
        B->CopyCompleteValue(NewPtr, OldPtr);
        return B->Identical(NewPtr, OldPtr) &&
            Export(*A, &Source, const_cast<UGridItemDefinitionAsset*>(&Source)) ==
            Export(*B, &Candidate.GenericThrow, &Candidate);
    }

    FVector OldVisualScale(const UGridItemDefinitionAsset& Old)
    {
        FVector Scale = Old.ThrowVisualRelativeScale;
        if (Scale.ContainsNaN()) { Scale = FVector(1.5f); }
        Scale.X = FMath::Clamp(FMath::Abs(Scale.X), 0.01f, 100.0f);
        Scale.Y = FMath::Clamp(FMath::Abs(Scale.Y), 0.01f, 100.0f);
        Scale.Z = FMath::Clamp(FMath::Abs(Scale.Z), 0.01f, 100.0f);
        return Scale;
    }

    FVector OldVisualAngularVelocity(const UGridItemDefinitionAsset& Old)
    {
        switch (Old.ThrowVisualMode)
        {
        case EGridThrowVisualMode::Stable:
            return FVector::ZeroVector;
        case EGridThrowVisualMode::Spin:
            return FVector(0.0f, 0.0f, FMath::Max(0.0f, Old.ThrowVisualSpinDegreesPerSecond));
        case EGridThrowVisualMode::Tumble:
        default:
            FVector Axis = Old.ThrowVisualTumbleAxis;
            if (Axis.ContainsNaN() || Axis.IsNearlyZero())
            {
                Axis = FVector(0.65f, 0.35f, 1.0f);
            }
            Axis.Normalize();
            return Axis * FMath::Max(0.0f, Old.ThrowVisualTumbleDegreesPerSecond);
        }
    }
}

UGridArmClean064NNonArmGenericThrowCommandlet::
    UGridArmClean064NNonArmGenericThrowCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064NNonArmGenericThrowCommandlet::Main(const FString& Params)
{
    FString OwnerPath;
    FString PreviousPath;
    FString OutputPath;
    if (!FParse::Value(*Params, TEXT("Ownership="), OwnerPath) ||
        !FParse::Value(*Params, TEXT("ReadingWorld="), PreviousPath) ||
        !FParse::Value(*Params, TEXT("Output="), OutputPath) ||
        OwnerPath.IsEmpty() || PreviousPath.IsEmpty() || OutputPath.IsEmpty() ||
        FPaths::IsSamePath(OwnerPath, PreviousPath) ||
        FPaths::IsSamePath(OwnerPath, OutputPath) ||
        FPaths::IsSamePath(PreviousPath, OutputPath))
    {
        return 1;
    }
    const TSharedPtr<FJsonObject> Owner = ReadJson(OwnerPath);
    const TSharedPtr<FJsonObject> Prior = ReadJson(PreviousPath);
    const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
    if (!Owner.IsValid() || !Prior.IsValid() ||
        FieldString(Owner, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
        FieldString(Prior, TEXT("schema")) != TEXT("ARM-CLEAN06.4M/nonarm-read-world-policy-parity/v1") ||
        !Owner->TryGetArrayField(TEXT("private_item_ownership"), Rows) ||
        !Rows || Rows->Num() != 11 ||
        Owner->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
        Owner->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
        Prior->GetIntegerField(TEXT("verified_item_ids")) != 11 ||
        Prior->GetIntegerField(TEXT("exact_reflected_copies_with_defaults")) != 33 ||
        Prior->GetIntegerField(TEXT("world_occurrences_preserved")) != 40 ||
        Owner->GetBoolField(TEXT("migration_authorized")) ||
        Prior->GetBoolField(TEXT("migration_authorized")))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Source scope drift."));
        return 2;
    }
    const TSharedPtr<FJsonObject>* Domains = nullptr;
    if (!Owner->TryGetObjectField(TEXT("domain_counts"), Domains) || !Domains ||
        (*Domains)->GetIntegerField(TEXT("GENERIC_THROW")) != 3)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Generic throw domain drift."));
        return 2;
    }
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RootPath);
    if (!IsValid(Root) || Root->ItemDefinitions.Num() != 23) { return 3; }
    TSet<FName> Registered;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Registered.Contains(Definition->ItemId)) { return 3; }
        Registered.Add(Definition->ItemId);
    }
    const bool RootDirty = Root->GetOutermost()->IsDirty();

    TSet<FName> SeenIds;
    int32 WorldOccurrences = 0;
    int32 ActiveFields = 0;
    int32 ActiveThrowFields = 0;
    int32 ExactComparableFields = 0;
    int32 ExplicitEnumMappings = 0;
    int32 BallisticProbes = 0;
    int32 VisualProbes = 0;
    int32 DirtiedSources = 0;
    TArray<TSharedPtr<FJsonValue>> PrivateRows;

    for (const TSharedPtr<FJsonValue>& Raw : *Rows)
    {
        const TSharedPtr<FJsonObject> Item = Raw.IsValid() ? Raw->AsObject() : nullptr;
        const FName Id(*FieldString(Item, TEXT("item_id")));
        const FString Path = FieldString(Item, TEXT("source_asset"));
        const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
        if (!Item.IsValid() || Id.IsNone() || SeenIds.Contains(Id) ||
            Registered.Contains(Id) || Item->GetBoolField(TEXT("conversion_authorized")) ||
            !Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
            Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
            Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
            !Path.Contains(TEXT(".")) ||
            !Item->TryGetArrayField(TEXT("field_reviews"), Reviews) ||
            !Reviews || Reviews->Num() != Item->GetIntegerField(TEXT("active_unmapped_fields")))
        {
            return 4;
        }
        const UGridItemDefinitionAsset* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        if (!IsValid(Old) || Old->ItemDefinitionId != Id ||
            !Old->WeaponArchetypeId.IsNone()) { return 4; }
        const bool OldDirty = Old->GetOutermost()->IsDirty();
        SeenIds.Add(Id);
        WorldOccurrences += Item->GetIntegerField(TEXT("affected_world_occurrences"));

        UGridNonArmamentDefinitionAsset* Candidate =
            NewObject<UGridNonArmamentDefinitionAsset>(GetTransientPackage(), NAME_None, RF_Transient);
        if (!IsValid(Candidate) || Candidate->GetOutermost() != GetTransientPackage()) { return 5; }
        Candidate->ItemId = Id;
        Candidate->WeightKg = Old->Weight;

        TSet<FString> Names;
        int32 ItemActiveThrow = 0;
        for (const TSharedPtr<FJsonValue>& RawField : *Reviews)
        {
            const TSharedPtr<FJsonObject> Review = RawField.IsValid() ? RawField->AsObject() : nullptr;
            const FString Name = FieldString(Review, TEXT("property"));
            const FString Domain = FieldString(Review, TEXT("domain"));
            const FProperty* SourceProperty = Old->GetClass()->FindPropertyByName(FName(*Name));
            if (!Review.IsValid() || Name.IsEmpty() || Names.Contains(Name) ||
                !SourceProperty ||
                Export(*SourceProperty, Old, const_cast<UGridItemDefinitionAsset*>(Old)) !=
                    FieldString(Review, TEXT("source_value_exact")) ||
                FieldString(Review, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Historical item field drift."));
                return 5;
            }
            Names.Add(Name);
            ++ActiveFields;
            if (Domain == TEXT("GENERIC_THROW"))
            {
                const bool bKnown = FieldMappings().ContainsByPredicate(
                    [&Name](const FFieldMap& Mapping) {return Name == Mapping.Source;});
                if (!bKnown)
                {
                    UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Unknown throw field."));
                    return 5;
                }
                ++ItemActiveThrow;
                ++ActiveThrowFields;
            }
        }
        for (const FFieldMap& Mapping : FieldMappings())
        {
            if (!CopyExact(*Old, *Candidate, Mapping))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Generic throw mapping mismatch."));
                return 6;
            }
            if (FString(Mapping.Source) == TEXT("ThrowVisualMode"))
            {
                ++ExplicitEnumMappings;
            }
            else
            {
                ++ExactComparableFields;
            }
        }

        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid(13, 21, 34, static_cast<uint32>(SeenIds.Num()));
        Instance.ItemDefinitionId = Id;
        Instance.Quantity = 1;
        // These are DATA/POLICY previews only: actual source hand permission,
        // strength, collision, action costs and SaveGame remain separate gates.
        for (const int32 Level : {0, 1, 21})
        {
            Instance.ItemLevel = Level;
            float Speed = 0.0f;
            float Arc = 0.0f;
            const bool bOld = Instance.IsValid() &&
                FMath::IsFinite(Old->ThrowSpeed) &&
                Old->ThrowSpeed > KINDA_SMALL_NUMBER &&
                FMath::IsFinite(Old->ThrowArc);
            const bool bNew = FGridCanonicalNonArmamentThrowReadModel::
                TryBallistics(Candidate, Instance, Speed, Arc);
            if (bOld != bNew || (bOld &&
                (!FMath::IsNearlyEqual(Speed, Old->ThrowSpeed) ||
                 !FMath::IsNearlyEqual(Arc, Old->ThrowArc))))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Signed-arc ballistic mismatch."));
                return 6;
            }
            ++BallisticProbes;
        }
        if (!OldVisualScale(*Old).Equals(
            FGridCanonicalNonArmamentThrowReadModel::ClampedVisualScale(Candidate->GenericThrow)) ||
            !OldVisualAngularVelocity(*Old).Equals(
                FGridCanonicalNonArmamentThrowReadModel::VisualAngularVelocity(Candidate->GenericThrow)) ||
            !FMath::IsNearlyEqual(FMath::Max(0.01f, Old->ThrowLifeSeconds),
                FGridCanonicalNonArmamentThrowReadModel::FlightLifeSeconds(Candidate->GenericThrow)) ||
            !FMath::IsNearlyEqual(FMath::Max(0.0f, Old->ThrowImpactDropOffset),
                FGridCanonicalNonArmamentThrowReadModel::ImpactDropOffset(Candidate->GenericThrow)) ||
            Old->bCombatThrowWeapon != Candidate->GenericThrow.bCombatRecoverable)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Generic visual/lifetime parity failed."));
            return 6;
        }
        VisualProbes += 4;
        if (Old->GetOutermost()->IsDirty() != OldDirty) { ++DirtiedSources; }

        TSharedRef<FJsonObject> Private = MakeShared<FJsonObject>();
        Private->SetStringField(TEXT("item_id"), Id.ToString());
        Private->SetStringField(TEXT("source_asset"), Path);
        Private->SetNumberField(TEXT("active_throw_fields"), ItemActiveThrow);
        Private->SetNumberField(TEXT("copied_struct_fields"), FieldMappings().Num());
        Private->SetBoolField(TEXT("in_memory_data_parity"), true);
        Private->SetBoolField(TEXT("runtime_collision_parity_approved"), false);
        PrivateRows.Add(MakeShared<FJsonValueObject>(Private));
    }

    if (SeenIds.Num() != 11 || WorldOccurrences != 40 || ActiveFields != 50 ||
        ActiveThrowFields != 3 || ExactComparableFields != 121 ||
        ExplicitEnumMappings != 11 || BallisticProbes != 33 ||
        VisualProbes != 44 || DirtiedSources != 0 ||
        Root->GetOutermost()->IsDirty() != RootDirty)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Scope or no-write contract failed."));
        return 7;
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4N/nonarm-generic-throw-data-parity/v1"));
    Report->SetNumberField(TEXT("verified_item_ids"), SeenIds.Num());
    Report->SetNumberField(TEXT("world_occurrences_preserved"), WorldOccurrences);
    Report->SetNumberField(TEXT("source_active_fields_reverified"), ActiveFields);
    Report->SetNumberField(TEXT("active_generic_throw_fields"), ActiveThrowFields);
    Report->SetNumberField(TEXT("exact_reflection_copies"), ExactComparableFields);
    Report->SetNumberField(TEXT("explicit_mode_mappings"), ExplicitEnumMappings);
    Report->SetNumberField(TEXT("ballistics_policy_probes"), BallisticProbes);
    Report->SetNumberField(TEXT("visual_policy_probes"), VisualProbes);
    Report->SetNumberField(TEXT("source_dirty_state_changes"), DirtiedSources);
    Report->SetNumberField(TEXT("runtime_collision_parity_approved"), 0);
    Report->SetBoolField(TEXT("binary_writes_performed"), false);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetBoolField(TEXT("migration_authorized"), false);
    Report->SetArrayField(TEXT("private_item_projections"), PrivateRows);
    FString Serialized;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Serialized)) ||
        !FFileHelper::SaveStringToFile(Serialized, *OutputPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4N] Private audit output failed."));
        return 8;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4N] 3 generic throw active fields, 132 mapped properties, 77 pure probes. No SavePackage."));
    return 0;
}
