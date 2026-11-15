#include "ARMEMENT/GridArmClean064MNonArmReadingPhysicsCommandlet.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalNonArmamentReadPolicies.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
    const TCHAR* CanonicalPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    const FString LegacyRoot(TEXT("/Game/GrimrockPrototype/Core/DataAssets/"));

    TSharedPtr<FJsonObject> ReadJson(const FString& Path)
    {
        FString Raw;
        TSharedPtr<FJsonObject> Object;
        if (!FFileHelper::LoadFileToString(Raw, *Path) ||
            !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Object))
        {
            return nullptr;
        }
        return Object;
    }

    FString StringField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
    {
        FString Value;
        if (Object.IsValid()) { Object->TryGetStringField(Field, Value); }
        return Value;
    }

    FString Export(const FProperty& Property, const UObject& Object)
    {
        FString Result;
        Property.ExportTextItem_Direct(Result,
            Property.ContainerPtrToValuePtr<void>(&Object), nullptr,
            const_cast<UObject*>(&Object), PPF_None);
        return Result;
    }

    const TArray<FString>& PolicyFields()
    {
        static const TArray<FString> Names = {
            TEXT("ReadText"), TEXT("bUseItemWeightAsWorldPhysicsMass"),
            TEXT("WorldPhysicsInitialTiltDegrees")
        };
        return Names;
    }

    bool CopyExact(const UGridItemDefinitionAsset& Old,
        UGridNonArmamentDefinitionAsset& Candidate, const FString& Name)
    {
        const FProperty* A = Old.GetClass()->FindPropertyByName(FName(*Name));
        const FProperty* B = Candidate.GetClass()->FindPropertyByName(FName(*Name));
        if (!A || !B || !B->SameType(A)) { return false; }
        const void* SourceValue = A->ContainerPtrToValuePtr<void>(&Old);
        void* DestinationValue = B->ContainerPtrToValuePtr<void>(&Candidate);
        B->CopyCompleteValue(DestinationValue, SourceValue);
        return B->Identical(DestinationValue, SourceValue) &&
            Export(*A, Old) == Export(*B, Candidate);
    }

    bool LegacyReadableDefinition(const UGridItemDefinitionAsset& Old)
    {
        return Old.ItemType == EGridItemType::Book ||
            Old.ItemType == EGridItemType::Scroll ||
            Old.ItemTags.Contains(FName(TEXT("Readable"))) ||
            Old.ItemTags.Contains(FName(TEXT("Lisible")));
    }

    // Exact existing policy from GridWorldPhysicsMassReadModel for unknown
    // canonical identities: the OLD opt-in, OLD unit weight, valid instance.
    bool LegacyWorldMass(const UGridItemDefinitionAsset& Old,
        const FGridItemInstance& Instance, float& MassKg)
    {
        MassKg = 0.0f;
        if (!Old.bUseItemWeightAsWorldPhysicsMass || !Instance.IsValid() ||
            Instance.ItemDefinitionId != Old.ItemDefinitionId ||
            !FMath::IsFinite(Old.Weight) || Old.Weight <= KINDA_SMALL_NUMBER)
        {
            return false;
        }
        MassKg = Old.Weight;
        return true;
    }

    float LegacyTiltDegrees(const UGridItemDefinitionAsset& Old)
    {
        const float Degrees = FMath::Clamp(
            Old.WorldPhysicsInitialTiltDegrees, 0.0f, 45.0f);
        return FMath::IsFinite(Degrees) && Degrees > KINDA_SMALL_NUMBER
            ? Degrees : 0.0f;
    }
}

UGridArmClean064MNonArmReadingPhysicsCommandlet::
    UGridArmClean064MNonArmReadingPhysicsCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064MNonArmReadingPhysicsCommandlet::Main(const FString& Params)
{
    FString OwnerFile;
    FString PresentationFile;
    FString OutputFile;
    if (!FParse::Value(*Params, TEXT("Ownership="), OwnerFile) ||
        !FParse::Value(*Params, TEXT("Presentation="), PresentationFile) ||
        !FParse::Value(*Params, TEXT("Output="), OutputFile) ||
        OwnerFile.IsEmpty() || PresentationFile.IsEmpty() || OutputFile.IsEmpty() ||
        FPaths::IsSamePath(OwnerFile, OutputFile) ||
        FPaths::IsSamePath(PresentationFile, OutputFile) ||
        FPaths::IsSamePath(OwnerFile, PresentationFile))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Distinct private file paths required."));
        return 1;
    }
    const TSharedPtr<FJsonObject> Owners = ReadJson(OwnerFile);
    const TSharedPtr<FJsonObject> Presentation = ReadJson(PresentationFile);
    const TArray<TSharedPtr<FJsonValue>>* Items = nullptr;
    if (!Owners.IsValid() || !Presentation.IsValid() ||
        StringField(Owners, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
        StringField(Presentation, TEXT("schema")) != TEXT("ARM-CLEAN06.4L/nonarm-presentation-data-parity/v1") ||
        !Owners->TryGetArrayField(TEXT("private_item_ownership"), Items) ||
        !Items || Items->Num() != 11 ||
        Owners->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
        Owners->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
        Owners->GetIntegerField(TEXT("distinct_active_unmapped_property_names")) != 26 ||
        Owners->GetBoolField(TEXT("migration_authorized")) ||
        Presentation->GetIntegerField(TEXT("verified_item_ids")) != 11 ||
        Presentation->GetIntegerField(TEXT("exact_reflected_copies_including_defaults")) != 88 ||
        Presentation->GetBoolField(TEXT("migration_authorized")))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Pinned private source scope invalid."));
        return 2;
    }
    const TSharedPtr<FJsonObject>* Domains = nullptr;
    if (!Owners->TryGetObjectField(TEXT("domain_counts"), Domains) || !Domains ||
        (*Domains)->GetIntegerField(TEXT("READING")) != 1 ||
        (*Domains)->GetIntegerField(TEXT("WORLD_CHAOS")) != 2)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Reading/Chaos domains drift."));
        return 2;
    }
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, CanonicalPath);
    if (!IsValid(Root) || Root->ItemDefinitions.Num() != 23) { return 3; }
    TSet<FName> Registered;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Registered.Contains(Definition->ItemId)) { return 3; }
        Registered.Add(Definition->ItemId);
    }
    const bool RootWasDirty = Root->GetOutermost()->IsDirty();

    TSet<FName> SeenIds;
    int32 OriginalOccurrences = 0;
    int32 AllLegacyActive = 0;
    int32 ActiveReading = 0;
    int32 ActiveWorld = 0;
    int32 ExactCopies = 0;
    int32 ReadProbes = 0;
    int32 WorldProbes = 0;
    int32 DirtyChanges = 0;
    int32 ReadableDefinitions = 0;
    TArray<TSharedPtr<FJsonValue>> PrivateRows;
    for (const TSharedPtr<FJsonValue>& Raw : *Items)
    {
        const TSharedPtr<FJsonObject> Row = Raw.IsValid() ? Raw->AsObject() : nullptr;
        const FName Id(*StringField(Row, TEXT("item_id")));
        const FString Path = StringField(Row, TEXT("source_asset"));
        const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
        if (!Row.IsValid() || Id.IsNone() || SeenIds.Contains(Id) ||
            Registered.Contains(Id) || Row->GetBoolField(TEXT("conversion_authorized")) ||
            !Path.StartsWith(LegacyRoot, ESearchCase::CaseSensitive) ||
            Path.StartsWith(TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/")) ||
            Path.Contains(TEXT("..")) || Path.Contains(TEXT("\\")) ||
            !Path.Contains(TEXT(".")) ||
            !Row->TryGetArrayField(TEXT("field_reviews"), Reviews) ||
            !Reviews || Reviews->Num() != Row->GetIntegerField(TEXT("active_unmapped_fields")))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Item scope invalid."));
            return 4;
        }
        const UGridItemDefinitionAsset* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        if (!IsValid(Old) || Old->ItemDefinitionId != Id ||
            !Old->WeaponArchetypeId.IsNone() || !FMath::IsFinite(Old->Weight) ||
            Old->Weight < 0.0f ||
            !FMath::IsFinite(Old->WorldPhysicsInitialTiltDegrees))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Real UE item is invalid or drifted."));
            return 4;
        }
        const bool WasDirty = Old->GetOutermost()->IsDirty();
        SeenIds.Add(Id);
        OriginalOccurrences += Row->GetIntegerField(TEXT("affected_world_occurrences"));

        UGridNonArmamentDefinitionAsset* Candidate =
            NewObject<UGridNonArmamentDefinitionAsset>(GetTransientPackage(), NAME_None, RF_Transient);
        if (!IsValid(Candidate) || Candidate->GetOutermost() != GetTransientPackage()) { return 5; }
        Candidate->ItemId = Id;
        Candidate->DisplayName = Old->DisplayName;
        Candidate->Description = Old->Description;
        Candidate->WeightKg = Old->Weight;
        Candidate->Tags = Old->ItemTags;
        Candidate->Icon = Old->Icon;
        Candidate->WorldMesh = Old->WorldMesh;
        Candidate->bReadable = LegacyReadableDefinition(*Old);
        if (Candidate->bReadable) { ++ReadableDefinitions; }

        int32 ActiveForItem = 0;
        TSet<FString> Encountered;
        for (const TSharedPtr<FJsonValue>& F : *Reviews)
        {
            const TSharedPtr<FJsonObject> Review = F.IsValid() ? F->AsObject() : nullptr;
            const FString Name = StringField(Review, TEXT("property"));
            const FString Domain = StringField(Review, TEXT("domain"));
            const FProperty* OldProperty = Old->GetClass()->FindPropertyByName(FName(*Name));
            if (!Review.IsValid() || Name.IsEmpty() || Encountered.Contains(Name) ||
                !OldProperty ||
                Export(*OldProperty, *Old) != StringField(Review, TEXT("source_value_exact")) ||
                StringField(Review, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Historical property changed."));
                return 5;
            }
            Encountered.Add(Name);
            ++AllLegacyActive;
            if (Domain == TEXT("READING") || Domain == TEXT("WORLD_CHAOS"))
            {
                if (!PolicyFields().Contains(Name) ||
                    (Domain == TEXT("READING") && Name != TEXT("ReadText")) ||
                    (Domain == TEXT("WORLD_CHAOS") && Name == TEXT("ReadText")))
                {
                    UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Unrecognized active policy field."));
                    return 5;
                }
                ++ActiveForItem;
                if (Domain == TEXT("READING")) { ++ActiveReading; }
                else { ++ActiveWorld; }
            }
        }
        for (const FString& Name : PolicyFields())
        {
            if (!CopyExact(*Old, *Candidate, Name))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Native reflection mismatch."));
                return 6;
            }
            ++ExactCopies;
        }

        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid(3, 4, 5, static_cast<uint32>(SeenIds.Num()));
        Instance.ItemDefinitionId = Id;
        Instance.ItemLevel = 0;
        Instance.Quantity = 1;
        UGridReadableContentAsset* Reusable = NewObject<UGridReadableContentAsset>(
            GetTransientPackage(), NAME_None, RF_Transient);
        if (!IsValid(Reusable)) { return 6; }
        Reusable->BodyText = FText::FromString(TEXT("ARM-CLEAN06.4M synthetic instance text"));
        // Verify actual legacy reader (not a duplicate of its implementation)
        // on both definition-only and instance-owned text combinations.
        for (int32 Mode = 0; Mode < 4; ++Mode)
        {
            Instance.ReadTextOverride = (Mode & 1)
                ? FText::FromString(TEXT("ARM-CLEAN06.4M synthetic override"))
                : FText::GetEmpty();
            Instance.ReadableContentAsset = (Mode & 2) ? Reusable : nullptr;
            if (UGridItemContextActionLibrary::IsItemReadable(Instance, Old) !=
                FGridCanonicalNonArmamentReadPolicies::CanRead(Candidate, Instance))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Reading semantics diverged."));
                return 6;
            }
            ++ReadProbes;
        }
        Instance.ReadTextOverride = FText::GetEmpty();
        Instance.ReadableContentAsset = nullptr;
        for (int32 Quantity : {1, 5})
        {
            Instance.Quantity = Quantity;
            float LegacyMass = 0.0f;
            float CanonicalMass = 0.0f;
            const bool bOld = LegacyWorldMass(*Old, Instance, LegacyMass);
            const bool bNew = FGridCanonicalNonArmamentReadPolicies::
                TryResolveWorldPickupUnitMass(Candidate, Instance, CanonicalMass);
            if (bOld != bNew || !FMath::IsNearlyEqual(LegacyMass, CanonicalMass))
            {
                UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Pickup unit mass differs."));
                return 6;
            }
            ++WorldProbes;
        }
        if (!FMath::IsNearlyEqual(LegacyTiltDegrees(*Old),
            FGridCanonicalNonArmamentReadPolicies::InitialPlacementTiltDegrees(Candidate)))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] World placement tilt mismatch."));
            return 6;
        }
        ++WorldProbes;
        if (Old->GetOutermost()->IsDirty() != WasDirty) { ++DirtyChanges; }
        TSharedRef<FJsonObject> Detail = MakeShared<FJsonObject>();
        Detail->SetStringField(TEXT("item_id"), Id.ToString());
        Detail->SetStringField(TEXT("legacy_source"), Path);
        Detail->SetNumberField(TEXT("active_source_fields"), ActiveForItem);
        Detail->SetNumberField(TEXT("exact_reflected_copies"), PolicyFields().Num());
        Detail->SetBoolField(TEXT("reading_capability_projected"), true);
        Detail->SetBoolField(TEXT("data_parity"), true);
        Detail->SetBoolField(TEXT("pie_chaos_runtime_parity_approved"), false);
        PrivateRows.Add(MakeShared<FJsonValueObject>(Detail));
    }

    if (SeenIds.Num() != 11 || OriginalOccurrences != 40 ||
        AllLegacyActive != 50 || ActiveReading != 1 || ActiveWorld != 2 ||
        ExactCopies != 33 || ReadProbes != 44 || WorldProbes != 33 ||
        DirtyChanges != 0 || RootWasDirty != Root->GetOutermost()->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Scope or no-write contract failed."));
        return 7;
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4M/nonarm-read-world-policy-parity/v1"));
    Report->SetNumberField(TEXT("verified_item_ids"), SeenIds.Num());
    Report->SetNumberField(TEXT("world_occurrences_preserved"), OriginalOccurrences);
    Report->SetNumberField(TEXT("legacy_active_fields_revalidated"), AllLegacyActive);
    Report->SetNumberField(TEXT("active_reading_fields"), ActiveReading);
    Report->SetNumberField(TEXT("active_world_physics_fields"), ActiveWorld);
    Report->SetNumberField(TEXT("exact_reflected_copies_with_defaults"), ExactCopies);
    Report->SetNumberField(TEXT("read_behavior_probes"), ReadProbes);
    Report->SetNumberField(TEXT("world_physics_probes"), WorldProbes);
    Report->SetNumberField(TEXT("explicit_readable_capabilities"), ReadableDefinitions);
    Report->SetNumberField(TEXT("source_dirty_state_changes"), DirtyChanges);
    Report->SetNumberField(TEXT("runtime_pie_chaos_parity_approved"), 0);
    Report->SetBoolField(TEXT("binary_writes_performed"), false);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetBoolField(TEXT("migration_authorized"), false);
    Report->SetArrayField(TEXT("private_item_projections"), PrivateRows);
    FString Json;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *OutputFile,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARM-CLEAN06.4M] Private report write failed."));
        return 8;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4M] 3 active reading/Chaos fields; 33 reflected copies; 77 pure behavior probes. No binary writes."));
    return 0;
}
