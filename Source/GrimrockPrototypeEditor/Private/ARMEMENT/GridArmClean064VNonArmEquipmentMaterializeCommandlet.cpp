#include "ARMEMENT/GridArmClean064VNonArmEquipmentMaterializeCommandlet.h"

#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace
{
    constexpr TCHAR RootAsset[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem");
    constexpr TCHAR NewDirectory[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/");
    constexpr TCHAR LegacyDirectory[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/");

    TSharedPtr<FJsonObject> Read(const FString& Filename)
    {
        FString Text;
        TSharedPtr<FJsonObject> Object;
        return FFileHelper::LoadFileToString(Text, *Filename) &&
            FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object)
            ? Object : nullptr;
    }

    FString String(const TSharedPtr<FJsonObject>& Row, const TCHAR* Name)
    {
        FString Value;
        if (Row.IsValid()) { Row->TryGetStringField(Name, Value); }
        return Value;
    }

    FString Export(const FProperty& Field, const UObject& Object)
    {
        FString Value;
        Field.ExportTextItem_Direct(Value,
            Field.ContainerPtrToValuePtr<void>(&Object), nullptr,
            const_cast<UObject*>(&Object), PPF_None);
        return Value;
    }

    bool ExactCopy(const UGridItemDefinitionAsset& Source,
        UGridNonArmamentDefinitionAsset& Target, const TCHAR* SourceName,
        const TCHAR* TargetName)
    {
        const FProperty* Old = Source.GetClass()->FindPropertyByName(FName(SourceName));
        const FProperty* New = Target.GetClass()->FindPropertyByName(FName(TargetName));
        if (!Old || !New || !New->SameType(Old)) { return false; }
        const void* OldValue = Old->ContainerPtrToValuePtr<void>(&Source);
        void* NewValue = New->ContainerPtrToValuePtr<void>(&Target);
        New->CopyCompleteValue(NewValue, OldValue);
        return New->Identical(OldValue, NewValue) &&
            Export(*Old, Source) == Export(*New, Target);
    }

    bool ExactStructField(const UGridItemDefinitionAsset& Old,
        FGridNonArmamentThrowPolicy& New, const TCHAR* OldName, const TCHAR* NewName)
    {
        const FProperty* A = Old.GetClass()->FindPropertyByName(FName(OldName));
        const FProperty* B = FGridNonArmamentThrowPolicy::StaticStruct()->
            FindPropertyByName(FName(NewName));
        if (!A || !B || !B->SameType(A)) { return false; }
        const void* V = A->ContainerPtrToValuePtr<void>(&Old);
        void* W = B->ContainerPtrToValuePtr<void>(&New);
        B->CopyCompleteValue(W, V);
        return B->Identical(W, V);
    }

    bool CopyThrow(const UGridItemDefinitionAsset& Old,
        UGridNonArmamentDefinitionAsset& New)
    {
        auto& P = New.GenericThrow;
        if (!ExactStructField(Old, P, TEXT("bCombatThrowWeapon"), TEXT("bCombatRecoverable")) ||
            !ExactStructField(Old, P, TEXT("ThrowSpeed"), TEXT("InitialSpeed")) ||
            !ExactStructField(Old, P, TEXT("ThrowArc"), TEXT("Arc")) ||
            !ExactStructField(Old, P, TEXT("ThrowLifeSeconds"), TEXT("LifeSeconds")) ||
            !ExactStructField(Old, P, TEXT("ThrowImpactDropOffset"), TEXT("ImpactDropOffset")) ||
            !ExactStructField(Old, P, TEXT("ThrowVisualRelativeRotation"), TEXT("VisualRelativeRotation")) ||
            !ExactStructField(Old, P, TEXT("ThrowVisualRelativeScale"), TEXT("VisualRelativeScale")) ||
            !ExactStructField(Old, P, TEXT("ProjectileMeshForwardAxis"), TEXT("MeshForwardAxis")) ||
            !ExactStructField(Old, P, TEXT("ThrowVisualTumbleAxis"), TEXT("VisualTumbleAxis")) ||
            !ExactStructField(Old, P, TEXT("ThrowVisualTumbleDegreesPerSecond"), TEXT("VisualTumbleDegreesPerSecond")) ||
            !ExactStructField(Old, P, TEXT("ThrowVisualSpinDegreesPerSecond"), TEXT("VisualSpinDegreesPerSecond")))
        {
            return false;
        }
        switch (Old.ThrowVisualMode)
        {
        case EGridThrowVisualMode::Stable:
            P.VisualMode = EGridNonArmamentThrowVisualMode::Stable; break;
        case EGridThrowVisualMode::Tumble:
            P.VisualMode = EGridNonArmamentThrowVisualMode::Tumble; break;
        case EGridThrowVisualMode::Spin:
            P.VisualMode = EGridNonArmamentThrowVisualMode::Spin; break;
        default: return false;
        }
        return true;
    }

    bool Match(const UGridItemDefinitionAsset& Old,
        const UGridNonArmamentDefinitionAsset& New)
    {
        return New.ItemId == Old.ItemDefinitionId &&
            New.DisplayName.EqualTo(Old.DisplayName) &&
            New.Description.EqualTo(Old.Description) &&
            FMath::IsNearlyEqual(New.WeightKg, Old.Weight, 0.0001f) &&
            New.Tags == Old.ItemTags &&
            New.Icon.ToSoftObjectPath() == Old.Icon.ToSoftObjectPath() &&
            New.WorldMesh.ToSoftObjectPath() == Old.WorldMesh.ToSoftObjectPath() &&
            New.ReadText.EqualTo(Old.ReadText) &&
            New.bReadable == (Old.ItemType == EGridItemType::Book ||
                Old.ItemType == EGridItemType::Scroll ||
                Old.ItemTags.Contains(FName(TEXT("Readable"))) ||
                Old.ItemTags.Contains(FName(TEXT("Lisible")))) &&
            New.bUseItemWeightAsWorldPhysicsMass ==
                Old.bUseItemWeightAsWorldPhysicsMass &&
            FMath::IsNearlyEqual(New.WorldPhysicsInitialTiltDegrees,
                Old.WorldPhysicsInitialTiltDegrees) &&
            New.GenericThrow.bCombatRecoverable == Old.bCombatThrowWeapon &&
            FMath::IsNearlyEqual(New.GenericThrow.InitialSpeed, Old.ThrowSpeed);
    }

    enum class EFamily : uint8 { Skip, Plain, Consumable, Stack, Equipment };

    EFamily Family(const UGridItemDefinitionAsset& Item)
    {
        // Never silently migrate a source whose combat/visual bindings need
        // an unapproved execution route; those are a later Editor cutover.
        if (!Item.WeaponArchetypeId.IsNone() || Item.bUseWeaponCatalogue ||
            Item.bUseWeaponVisualCatalogue || Item.bProvidesQuickItemCombatAction)
        {
            return EFamily::Skip;
        }
        const bool bHand = Item.GetEffectiveHandUsage() != EGridItemHandUsage::NotHandHeld;
        const bool bEquip = bHand || !Item.CompatibleEquipmentSlots.IsEmpty() ||
            Item.ItemType == EGridItemType::Armor || Item.ItemType == EGridItemType::Jewelry ||
            Item.ItemType == EGridItemType::Weapon || Item.ItemType == EGridItemType::Shield;
        if (bEquip)
        {
            return EFamily::Equipment;
        }
        if (Item.ItemType == EGridItemType::Potion ||
            Item.ItemType == EGridItemType::Food ||
            Item.ItemType == EGridItemType::Scroll) { return EFamily::Consumable; }
        if (Item.bStackable) { return EFamily::Stack; }
        return EFamily::Plain;
    }

    bool Copy(const UGridItemDefinitionAsset& Old,
        UGridNonArmamentDefinitionAsset& New, EFamily FamilyKind)
    {
        New.ItemId = Old.ItemDefinitionId;
        New.DisplayName = Old.DisplayName;
        New.Description = Old.Description;
        New.WeightKg = Old.Weight;
        New.Tags = Old.ItemTags;
        New.Icon = Old.Icon;
        New.WorldMesh = Old.WorldMesh;
        New.bReadable = Old.ItemType == EGridItemType::Book ||
            Old.ItemType == EGridItemType::Scroll ||
            Old.ItemTags.Contains(FName(TEXT("Readable"))) ||
            Old.ItemTags.Contains(FName(TEXT("Lisible")));
        const TCHAR* Fields[] = {
            TEXT("ReadText"), TEXT("LightEmitter"), TEXT("VisualEffects"),
            TEXT("bEnableWorldSparkle"), TEXT("WorldSparkleMaterial"),
            TEXT("WorldSparkleColor"), TEXT("WorldSparkleIntensity"),
            TEXT("WorldSparkleSpeed"), TEXT("WorldSparkleVariation"),
            TEXT("bUseItemWeightAsWorldPhysicsMass"),
            TEXT("WorldPhysicsInitialTiltDegrees")
        };
        for (const TCHAR* Field : Fields)
        {
            if (!ExactCopy(Old, New, Field, Field)) { return false; }
        }
        if (!CopyThrow(Old, New)) { return false; }
        if (FamilyKind == EFamily::Stack)
        {
            UGridGenericStackableDefinitionAsset* Stack =
                Cast<UGridGenericStackableDefinitionAsset>(&New);
            if (!Stack) { return false; }
            Stack->MaxStack = FMath::Max(1, Old.MaxStackSize);
        }
        if (FamilyKind == EFamily::Consumable)
        {
            UGridConsumableDefinitionAsset* Consumable =
                Cast<UGridConsumableDefinitionAsset>(&New);
            if (!Consumable) { return false; }
            Consumable->MaxStack =
                Old.bStackable ? FMath::Max(1, Old.MaxStackSize) : 1;
        }
        if (FamilyKind == EFamily::Equipment)
        {
            UGridOtherEquipmentDefinitionAsset* Equipment =
                Cast<UGridOtherEquipmentDefinitionAsset>(&New);
            if (!Equipment ||
                !ExactCopy(Old, *Equipment, TEXT("CompatibleEquipmentSlots"), TEXT("EquipmentSlots")) ||
                !ExactCopy(Old, *Equipment, TEXT("EquipmentStatBonus"), TEXT("Bonuses")) ||
                !ExactCopy(Old, *Equipment, TEXT("EquipmentResistanceBonus"), TEXT("Resistances")) ||
                !ExactCopy(Old, *Equipment, TEXT("EquippedMesh"), TEXT("EquippedMesh")))
            {
                return false;
            }
            switch (Old.GetEffectiveHandUsage())
            {
            case EGridItemHandUsage::NotHandHeld:
                Equipment->HandPolicy = EGridNonArmamentHandPolicy::NotHandHeld; break;
            case EGridItemHandUsage::OneHanded:
                Equipment->HandPolicy = EGridNonArmamentHandPolicy::OneHanded; break;
            case EGridItemHandUsage::TwoHanded:
                Equipment->HandPolicy = EGridNonArmamentHandPolicy::TwoHanded; break;
            default: return false;
            }
            Equipment->MaxStack =
                Old.bStackable ? FMath::Max(1, Old.MaxStackSize) : 1;
        }
        return Match(Old, New);
    }

    UClass* ClassFor(EFamily Family)
    {
        switch (Family)
        {
        case EFamily::Plain: return UGridNonArmamentDefinitionAsset::StaticClass();
        case EFamily::Consumable: return UGridConsumableDefinitionAsset::StaticClass();
        case EFamily::Stack: return UGridGenericStackableDefinitionAsset::StaticClass();
        case EFamily::Equipment: return UGridOtherEquipmentDefinitionAsset::StaticClass();
        default: return nullptr;
        }
    }

    bool SaveAsset(UObject* Asset)
    {
        UPackage* Package = Asset ? Asset->GetOutermost() : nullptr;
        if (!IsValid(Package)) { return false; }
        const FString File = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(File), true);
        Package->MarkPackageDirty();
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        return UPackage::SavePackage(Package, Asset, *File, Args);
    }
}

UGridArmClean064VNonArmEquipmentMaterializeCommandlet::
    UGridArmClean064VNonArmEquipmentMaterializeCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064VNonArmEquipmentMaterializeCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString OwnerFile, OutputFile, ExpectedFile;
    FParse::Value(*Params, TEXT("Expected="), ExpectedFile);
    if (bApply == bVerify ||
        !FParse::Value(*Params, TEXT("Ownership="), OwnerFile) ||
        !FParse::Value(*Params, TEXT("Output="), OutputFile) ||
        OwnerFile.IsEmpty() || OutputFile.IsEmpty() ||
        FPaths::IsSamePath(OwnerFile, OutputFile) ||
        (bVerify && !ExpectedFile.IsEmpty() &&
          FPaths::IsSamePath(ExpectedFile, OutputFile)) ||
        (bApply && !ExpectedFile.IsEmpty()))
    {
        return 1;
    }
    TSet<FName> TargetIds;
    // During the immediate post-Apply check we demand the signed private
    // manifest. During final-commit verification there is no -Expected
    // parameter: the two persisted tail entries are checked below against
    // all eleven historical source identities in a fresh Editor process.
    // Never attempt Read("") on the final committed run.
    if (bVerify && !ExpectedFile.IsEmpty())
    {
        const TSharedPtr<FJsonObject> Expected = Read(ExpectedFile);
        const TArray<TSharedPtr<FJsonValue>>* Targets = nullptr;
        if (!Expected.IsValid() ||
            String(Expected, TEXT("schema")) !=
              TEXT("ARM-CLEAN06.4V/nonarm-equipped-materialization/v1") ||
            !Expected->TryGetArrayField(TEXT("private_new_assets"), Targets) ||
            !Targets || Targets->Num() != 2) { return 1; }
        for (const TSharedPtr<FJsonValue>& RawTarget : *Targets)
        {
            const TSharedPtr<FJsonObject> Row =
                RawTarget.IsValid() ? RawTarget->AsObject() : nullptr;
            const FName ItemId(*String(Row, TEXT("item_id")));
            if (!Row.IsValid() || ItemId.IsNone() || TargetIds.Contains(ItemId))
            {
                return 1;
            }
            TargetIds.Add(ItemId);
        }
    }
    const TSharedPtr<FJsonObject> Owners = Read(OwnerFile);
    const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
    if (!Owners.IsValid() ||
        String(Owners, TEXT("schema")) != TEXT("ARM-CLEAN06.4K/field-ownership/v1") ||
        !Owners->TryGetArrayField(TEXT("private_item_ownership"), Rows) ||
        !Rows || Rows->Num() != 11 ||
        Owners->GetIntegerField(TEXT("verified_world_occurrences")) != 40 ||
        Owners->GetIntegerField(TEXT("active_unmapped_property_occurrences")) != 50 ||
        Owners->GetBoolField(TEXT("migration_authorized")))
    {
        return 2;
    }
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr, RootAsset);
    if (!IsValid(Root) || !IsValid(Root->ActionCatalogue) ||
        !IsValid(Root->ActionPresentationCatalogue) ||
        !IsValid(Root->WeaponCatalogue) ||
        !IsValid(Root->WeaponVisualCatalogue) ||
        Root->ItemDefinitions.Num() != (bApply ? 32 : 34) ||
        Root->ActionCatalogue->DetailedActions.Num() != (bApply ? 3 : 4) ||
        Root->ActionPresentationCatalogue->DetailedPresentations.Num() != (bApply ? 0 : 1))
    {
        return 3;
    }
    if (bVerify && ExpectedFile.IsEmpty())
    {
        // The final Git SHA is verified in a fresh run with no private
        // -Apply report attached. The two new entries occupy exactly the
        // tail of the 32-definition parent, and never derive from asset names.
        for (int32 Index = 32; Index != 34; ++Index)
        {
            const UGridCanonicalItemDefinitionAsset* Entry =
                Root->ItemDefinitions[Index];
            if (!IsValid(Entry) || Entry->ItemId.IsNone() ||
                TargetIds.Contains(Entry->ItemId)) { return 3; }
            TargetIds.Add(Entry->ItemId);
        }
    }
    TSet<FName> Existing;
    for (const UGridCanonicalItemDefinitionAsset* Definition : Root->ItemDefinitions)
    {
        if (!IsValid(Definition) || Definition->ItemId.IsNone() ||
            Existing.Contains(Definition->ItemId)) { return 3; }
        Existing.Add(Definition->ItemId);
    }
    TSet<FName> Seen;
    int32 ExistingCount = 0;
    int32 CreatedCount = 0;
    int32 EquipmentActions = 0;
    int32 Presentations = 0;
    int32 WorldReferences = 0;
    TArray<UGridNonArmamentDefinitionAsset*> NewObjects;
    TArray<TSharedPtr<FJsonValue>> PrivateRows;
    for (const TSharedPtr<FJsonValue>& Raw : *Rows)
    {
        const TSharedPtr<FJsonObject> Row = Raw.IsValid() ? Raw->AsObject() : nullptr;
        const FName Id(*String(Row, TEXT("item_id")));
        const FString SourcePath = String(Row, TEXT("source_asset"));
        if (!Row.IsValid() || Id.IsNone() || Seen.Contains(Id) ||
            !SourcePath.StartsWith(LegacyDirectory, ESearchCase::CaseSensitive) ||
            SourcePath.StartsWith(NewDirectory, ESearchCase::CaseSensitive) ||
            SourcePath.Contains(TEXT("..")) || SourcePath.Contains(TEXT("\\")) ||
            !SourcePath.Contains(TEXT("."))) { return 4; }
        Seen.Add(Id);
        WorldReferences += Row->GetIntegerField(TEXT("affected_world_occurrences"));
        const UGridItemDefinitionAsset* Old =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *SourcePath);
        if (!IsValid(Old) || Old->ItemDefinitionId != Id ||
            !FMath::IsFinite(Old->Weight) || Old->Weight < 0.0f ||
            !Old->IsValidDefinition()) { return 4; }
        const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
        if (!Row->TryGetArrayField(TEXT("field_reviews"), Reviews) || !Reviews ||
            Reviews->Num() != Row->GetIntegerField(TEXT("active_unmapped_fields")))
        {
            return 4;
        }
        for (const TSharedPtr<FJsonValue>& RawField : *Reviews)
        {
            const TSharedPtr<FJsonObject> Field = RawField.IsValid() ? RawField->AsObject() : nullptr;
            const FString Name = String(Field, TEXT("property"));
            const FProperty* Property = Old->GetClass()->FindPropertyByName(FName(*Name));
            if (!Field.IsValid() || !Property ||
                Export(*Property, *Old) != String(Field, TEXT("source_value_exact")) ||
                String(Field, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                return 4;
            }
        }
        const bool bSelected = bApply
            ? !Existing.Contains(Id)
            : TargetIds.Contains(Id);
        if (!bSelected)
        {
            if (!Existing.Contains(Id)) { return 5; }
            ++ExistingCount;
            continue;
        }
        if ((bApply && Existing.Contains(Id)) ||
            (bVerify && !Existing.Contains(Id))) { return 5; }
        const EFamily Kind = Family(*Old);
        if (Kind == EFamily::Skip || !Old->HasValidCombatActions() ||
            Old->bProvidesQuickItemCombatAction ||
            Old->CombatActions.Num() > 1) { return 5; }
        const FString Stem = Id.ToString();
        for (const TCHAR Char : Stem)
        {
            if (!FChar::IsAlnum(Char) && Char != TCHAR('_')) { return 5; }
        }
        const FString ObjectName = TEXT("DA_Canonical_") + Stem;
        const FString PackageName = FString(NewDirectory) + ObjectName;
        if (bApply && FPackageName::DoesPackageExist(PackageName)) { return 5; }
        UGridNonArmamentDefinitionAsset* New = nullptr;
        if (bApply)
        {
            UPackage* Package = CreatePackage(*PackageName);
            New = Package ? Cast<UGridNonArmamentDefinitionAsset>(
                NewObject<UObject>(Package, ClassFor(Kind), FName(*ObjectName),
                RF_Public | RF_Standalone | RF_Transactional)) : nullptr;
        }
        else
        {
            const FString ObjectPath = PackageName + TEXT(".") + ObjectName;
            New = LoadObject<UGridNonArmamentDefinitionAsset>(nullptr, *ObjectPath);
        }
        UGridNonArmamentDefinitionAsset* Candidate =
            Cast<UGridNonArmamentDefinitionAsset>(NewObject<UObject>(
                GetTransientPackage(), ClassFor(Kind), NAME_None, RF_Transient));
        if (!IsValid(New) || !IsValid(Candidate) ||
            New->GetClass() != ClassFor(Kind) || !Copy(*Old, *Candidate, Kind))
        {
            return 6;
        }
        if (bApply && !Copy(*Old, *New, Kind)) { return 6; }
        if (Old->CombatActions.Num() == 1)
        {
            // An equipment action must retain the real physically authorized
            // hand. Generic items never acquire an invented equipped action.
            const FGridCombatActionDefinition& Original = Old->CombatActions[0];
            const bool bAttack = Original.ResolutionProfile ==
                EGridCombatActionResolutionProfile::Attack;
            if (Kind != EFamily::Equipment ||
                Original.SourcePolicy != EGridCombatActionSourcePolicy::Equipment ||
                !Original.IsValid() ||
                (bAttack &&
                 !Old->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand) &&
                 !Old->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand)) ||
                Root->ActionCatalogue->Find(Original.ActionId) ||
                (bApply && Root->ActionCatalogue->FindDetailed(Original.ActionId)) ||
                (bVerify && !Root->ActionCatalogue->FindDetailed(Original.ActionId)))
            {
                return 7;
            }
            FGridCombatActionDefinition Saved = Original;
            if (Saved.DisplayName.IsEmpty()) { Saved.DisplayName = Old->DisplayName; }
            if (Saved.Description.IsEmpty()) { Saved.Description = Old->Description; }
            if (Saved.Icon.IsNull()) { Saved.Icon = Old->Icon; }
            for (const FName Tag : Old->ItemTags)
            {
                if (!Tag.IsNone()) { Saved.SourceTags.AddUnique(Tag); }
            }
            if (Old->IsCombatThrowable() && bAttack)
            {
                Saved.ResourceCosts.SourceItemQuantityCost = 1;
            }
            // The real authored per-item presentation must not be
            // discarded in favour of a generic weapon preset. Create one
            // unique canonical ID from the stable source identity.
            const FName PresentationId = Old->bProvidesAttackPresentation
                ? FName(*(TEXT("PRES_NonArm_") + Stem))
                : (Saved.PresentationProfileId.IsNone()
                    ? FName(TEXT("PRES_AttackNoFX"))
                    : Saved.PresentationProfileId);
            Saved.PresentationProfileId = PresentationId;
            if (!Saved.IsValid() || PresentationId.IsNone()) { return 7; }
            if (Old->bProvidesAttackPresentation)
            {
                if (!Old->HasValidPlayerAttackPresentation() ||
                    Root->ActionPresentationCatalogue->Find(PresentationId) ||
                    (bApply &&
                      Root->ActionPresentationCatalogue->FindDetailed(PresentationId)) ||
                    (bVerify &&
                      !Root->ActionPresentationCatalogue->FindDetailed(PresentationId)))
                {
                    return 7;
                }
                if (bApply)
                {
                    FGridCanonicalRuntimePresentation RealPresentation;
                    RealPresentation.PresentationId = PresentationId;
                    RealPresentation.Profile = Old->PlayerAttackPresentationProfile;
                    Root->ActionPresentationCatalogue->DetailedPresentations.Add(
                        MoveTemp(RealPresentation));
                }
                const FGridPlayerAttackPresentationProfile* Persisted =
                    Root->ActionPresentationCatalogue->FindDetailed(PresentationId);
                if (!Persisted ||
                    !FGridPlayerAttackPresentationProfile::StaticStruct()->
                        CompareScriptStruct(&Old->PlayerAttackPresentationProfile, Persisted, 0))
                {
                    return 7;
                }
                ++Presentations;
            }
            else if (!Root->ActionPresentationCatalogue->Find(PresentationId) &&
                !Root->ActionPresentationCatalogue->FindDetailed(PresentationId))
            {
                return 7;
            }
            FGridItemActionBinding Binding;
            Binding.ActionId = Saved.ActionId;
            Binding.PresentationId = PresentationId;
            Candidate->ActionBindings.Add(Binding);
            if (bApply)
            {
                New->ActionBindings.Add(Binding);
                FGridCanonicalRuntimeAction Full;
                Full.Definition = Saved;
                Root->ActionCatalogue->DetailedActions.Add(MoveTemp(Full));
            }
            const FGridCombatActionDefinition* PersistedAction =
                Root->ActionCatalogue->FindDetailed(Saved.ActionId);
            if (!PersistedAction ||
                !FGridCombatActionDefinition::StaticStruct()->
                    CompareScriptStruct(&Saved, PersistedAction, 0))
            {
                return 7;
            }
            ++EquipmentActions;
        }
        else if (Old->bProvidesAttackPresentation)
        {
            // A presentation without a consuming action is not silently
            // migrated as if it were a playable equipped ability.
            return 7;
        }
        if (bApply)
        {
            for (TFieldIterator<FProperty> It(Candidate->GetClass()); It; ++It)
            {
                if (!It->Identical_InContainer(New, Candidate)) { return 7; }
            }
            Root->ItemDefinitions.Add(New);
            NewObjects.Add(New);
        }
        else
        {
            for (TFieldIterator<FProperty> It(Candidate->GetClass()); It; ++It)
            {
                if (!It->Identical_InContainer(New, Candidate)) { return 7; }
            }
        }
        ++CreatedCount;
        TSharedRef<FJsonObject> Detail = MakeShared<FJsonObject>();
        Detail->SetStringField(TEXT("item_id"), Id.ToString());
        Detail->SetStringField(TEXT("source"), SourcePath);
        Detail->SetStringField(TEXT("relative"),
            TEXT("Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/") +
            ObjectName + TEXT(".uasset"));
        Detail->SetStringField(TEXT("canonical_class"), New->GetClass()->GetName());
        PrivateRows.Add(MakeShared<FJsonValueObject>(Detail));
    }
    if (Seen.Num() != 11 || WorldReferences != 40 ||
        ExistingCount != 9 || CreatedCount != 2 ||
        EquipmentActions != 1 || Presentations != 1 ||
        Root->ItemDefinitions.Num() != 34 ||
        Root->ActionCatalogue->DetailedActions.Num() != 4 ||
        Root->ActionPresentationCatalogue->DetailedPresentations.Num() != 1)
    {
        return 8;
    }
    TArray<FString> Errors;
    if (!Root->Validate(Errors)) { return 9; }
    if (bApply)
    {
        for (UGridNonArmamentDefinitionAsset* New : NewObjects)
        {
            if (!SaveAsset(New)) { return 10; }
        }
        if (!SaveAsset(Root->ActionCatalogue) ||
            !SaveAsset(Root->ActionPresentationCatalogue) ||
            !SaveAsset(Root)) { return 10; }
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"),
        TEXT("ARM-CLEAN06.4V/nonarm-equipped-materialization/v1"));
    Report->SetBoolField(TEXT("saved_binaries"), bApply);
    Report->SetBoolField(TEXT("verified_after_reopen"), bVerify);
    Report->SetNumberField(TEXT("source_item_ids"), 11);
    Report->SetNumberField(TEXT("world_references_unchanged"), WorldReferences);
    Report->SetNumberField(TEXT("old_registry_size"), 32);
    Report->SetNumberField(TEXT("new_registry_size"), Root->ItemDefinitions.Num());
    Report->SetNumberField(TEXT("new_real_assets"), 2);
    Report->SetNumberField(TEXT("existing_canonical_assets"), 9);
    Report->SetNumberField(TEXT("detailed_equipped_actions_persisted"), 1);
    Report->SetNumberField(TEXT("detailed_presentations_persisted"), 1);
    Report->SetBoolField(TEXT("runtime_equipped_action_route_available"), true);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetArrayField(TEXT("private_new_assets"), PrivateRows);
    FString Json;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *OutputFile,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return 11;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARM-CLEAN06.4V] %d UE assets saved, one exact equipped action and one presentation. No level/save files written."),
        CreatedCount);
    return 0;
}
