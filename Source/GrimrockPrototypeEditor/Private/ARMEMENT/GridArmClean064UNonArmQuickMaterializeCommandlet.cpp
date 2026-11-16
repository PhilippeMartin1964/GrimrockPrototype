#include "ARMEMENT/GridArmClean064UNonArmQuickMaterializeCommandlet.h"

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
            Item.bUseWeaponVisualCatalogue || !Item.CombatActions.IsEmpty() ||
            !Item.bProvidesQuickItemCombatAction || Item.bProvidesAttackPresentation ||
            Item.ItemType == EGridItemType::Weapon ||
            Item.ItemType == EGridItemType::Shield ||
            Item.ItemType == EGridItemType::None)
        {
            return EFamily::Skip;
        }
        const bool bHand = Item.GetEffectiveHandUsage() != EGridItemHandUsage::NotHandHeld;
        const bool bEquip = bHand || !Item.CompatibleEquipmentSlots.IsEmpty() ||
            Item.ItemType == EGridItemType::Armor || Item.ItemType == EGridItemType::Jewelry;
        if (bEquip)
        {
            // Equipped actions require a hand-specific executable route.
            // Keep them for the next native cutover; no fictitious QuickItem hand.
            return EFamily::Skip;
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

UGridArmClean064UNonArmQuickMaterializeCommandlet::
    UGridArmClean064UNonArmQuickMaterializeCommandlet()
{
    IsClient = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UGridArmClean064UNonArmQuickMaterializeCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    const bool bVerify = FParse::Param(*Params, TEXT("Verify"));
    FString OwnerFile, OutputFile;
    if (bApply == bVerify ||
        !FParse::Value(*Params, TEXT("Ownership="), OwnerFile) ||
        !FParse::Value(*Params, TEXT("Output="), OutputFile) ||
        OwnerFile.IsEmpty() || OutputFile.IsEmpty() ||
        FPaths::IsSamePath(OwnerFile, OutputFile))
    {
        return 1;
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
        !IsValid(Root->WeaponVisualCatalogue))
    {
        return 3;
    }
    TSet<FName> Existing;
    for (const UGridCanonicalItemDefinitionAsset* Item : Root->ItemDefinitions)
    {
        if (!IsValid(Item) || Item->ItemId.IsNone() ||
            Existing.Contains(Item->ItemId)) { return 3; }
        Existing.Add(Item->ItemId);
    }
    if ((bApply && Root->ItemDefinitions.Num() != 29) ||
        (bVerify && Root->ItemDefinitions.Num() != 32) ||
        (bApply && !Root->ActionCatalogue->DetailedActions.IsEmpty()) ||
        (bVerify && Root->ActionCatalogue->DetailedActions.Num() != 3))
    {
        return 3;
    }
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bActionCatalogueDirty = Root->ActionCatalogue->GetOutermost()->IsDirty();
    const bool bPresentationCatalogueDirty =
        Root->ActionPresentationCatalogue->GetOutermost()->IsDirty();
    TSet<FName> Seen;
    int32 ExistingCanonicals = 0;
    int32 FullActionsCopied = 0;
    TArray<UGridNonArmamentDefinitionAsset*> Created;
    TArray<TSharedPtr<FJsonValue>> PrivateRows;
    int32 Deferred = 0;
    int32 WorldReferences = 0;
    int32 Materialized = 0;
    for (const TSharedPtr<FJsonValue>& Raw : *Rows)
    {
        const TSharedPtr<FJsonObject> Row = Raw.IsValid() ? Raw->AsObject() : nullptr;
        const FName Id(*String(Row, TEXT("item_id")));
        const FString SourcePath = String(Row, TEXT("source_asset"));
        if (!Row.IsValid() || Id.IsNone() || Seen.Contains(Id) ||
            !SourcePath.StartsWith(LegacyDirectory, ESearchCase::CaseSensitive) ||
            SourcePath.StartsWith(NewDirectory, ESearchCase::CaseSensitive) ||
            SourcePath.Contains(TEXT("..")) || SourcePath.Contains(TEXT("\\")) ||
            !SourcePath.Contains(TEXT(".")))
        {
            return 4;
        }
        Seen.Add(Id);
        WorldReferences += Row->GetIntegerField(TEXT("affected_world_occurrences"));
        const UGridItemDefinitionAsset* Old =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *SourcePath);
        if (!IsValid(Old) || Old->ItemDefinitionId != Id ||
            !FMath::IsFinite(Old->Weight) || Old->Weight < 0.0f ||
            !Old->IsValidDefinition()) { return 4; }
        // Exact private property exports from the signed 06.4K evidence
        // prevent silently migrating a different version of the old asset.
        const TArray<TSharedPtr<FJsonValue>>* Reviews = nullptr;
        if (!Row->TryGetArrayField(TEXT("field_reviews"), Reviews) || !Reviews ||
            Reviews->Num() != Row->GetIntegerField(TEXT("active_unmapped_fields")))
        {
            return 5;
        }
        for (const TSharedPtr<FJsonValue>& RawField : *Reviews)
        {
            const TSharedPtr<FJsonObject> Field = RawField.IsValid() ? RawField->AsObject() : nullptr;
            const FString Name = String(Field, TEXT("property"));
            const FProperty* Property = Old->GetClass()->FindPropertyByName(FName(*Name));
            if (!Property ||
                Export(*Property, *Old) != String(Field, TEXT("source_value_exact")) ||
                String(Field, TEXT("status")) != TEXT("BLOCKED_REQUIRES_SEMANTIC_PARITY"))
            {
                return 5;
            }
        }

        const bool bAlreadyInRoot = Existing.Contains(Id);
        // The six action-free 06.4T sources must be skipped on BOTH passes.
        // On -Verify, the three newly saved 06.4U QuickItem sources are also
        // present in the root. They MUST be reopened and checked, not skipped
        // or rejected as a pre-existing 06.4T owner.
        if (bAlreadyInRoot && !Old->bProvidesQuickItemCombatAction &&
            Old->CombatActions.IsEmpty() && !Old->bProvidesAttackPresentation)
        {
            ++ExistingCanonicals;
            continue;
        }
        // The two remaining action/equipment owners are intentionally
        // deferred and MUST be absent from the persisted registry on BOTH
        // passes. Determine their family BEFORE requiring the -Verify
        // presence of a successfully converted QuickItem.
        const EFamily Kind = Family(*Old);
        if (Kind == EFamily::Skip)
        {
            if (bAlreadyInRoot) { return 5; }
            ++Deferred;
            continue;
        }
        if ((bApply && bAlreadyInRoot) || (bVerify && !bAlreadyInRoot))
        {
            return 5;
        }
        FGridCombatActionDefinition AuthoredQuick;
        if (!Old->BuildQuickItemCombatActionDefinition(AuthoredQuick) ||
            !AuthoredQuick.IsValid() ||
            AuthoredQuick.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ||
            AuthoredQuick.ActionId.IsNone() ||
            Root->ActionCatalogue->Find(AuthoredQuick.ActionId))
        {
            return 5;
        }
        const FGridCombatActionDefinition* ExistingDetail =
            Root->ActionCatalogue->FindDetailed(AuthoredQuick.ActionId);
        if ((bApply && ExistingDetail) ||
            (bVerify && !ExistingDetail))
        {
            return 5;
        }
        // The existing neutral presentation really is an authored value
        // in the canonical root; no new sound/Niagara/presentation invented.
        const FName PresentationId = AuthoredQuick.PresentationProfileId.IsNone()
            ? FName(TEXT("PRES_AttackNoFX"))
            : AuthoredQuick.PresentationProfileId;
        if (!Root->ActionPresentationCatalogue->Find(PresentationId) &&
            !Root->ActionPresentationCatalogue->FindDetailed(PresentationId))
        {
            return 5; // Never invent a presentation when source references it.
        }
        FGridItemActionBinding Binding;
        Binding.ActionId = AuthoredQuick.ActionId;
        Binding.PresentationId = PresentationId;
        // Names are derived from the actual stable ItemId; refuse path
        // separators and unsafe characters rather than guessing a slug.
        const FString Stem = Id.ToString();
        for (const TCHAR Character : Stem)
        {
            if (!FChar::IsAlnum(Character) && Character != TCHAR('_'))
            {
                return 6;
            }
        }
        const FString PackageName = FString(NewDirectory) + TEXT("DA_Canonical_") + Stem;
        const FString ObjectName = TEXT("DA_Canonical_") + Stem;
        UGridNonArmamentDefinitionAsset* New = nullptr;
        if (bApply)
        {
            if (Existing.Contains(Id) ||
                FPackageName::DoesPackageExist(PackageName))
            {
                return 6;
            }
            UPackage* Package = CreatePackage(*PackageName);
            if (!Package) { return 6; }
            New = Cast<UGridNonArmamentDefinitionAsset>(NewObject<UObject>(
                Package, ClassFor(Kind), FName(*ObjectName),
                RF_Public | RF_Standalone | RF_Transactional));
        }
        else
        {
            const FString ObjectPath = PackageName + TEXT(".") + ObjectName;
            New = LoadObject<UGridNonArmamentDefinitionAsset>(nullptr, *ObjectPath);
        }
        if (!IsValid(New) || New->GetClass() != ClassFor(Kind)) { return 6; }
        auto BindExactQuick = [&](UGridNonArmamentDefinitionAsset& Candidate) -> bool
        {
            if (UGridConsumableDefinitionAsset* Consumable =
                Cast<UGridConsumableDefinitionAsset>(&Candidate))
            {
                if (!Consumable->UseActions.IsEmpty() ||
                    !Consumable->ActionBindings.IsEmpty()) { return false; }
                Consumable->UseActions.Add(Binding);
            }
            else
            {
                if (!Candidate.ActionBindings.IsEmpty()) { return false; }
                Candidate.ActionBindings.Add(Binding);
            }
            return true;
        };
        if (bApply)
        {
            if (!Copy(*Old, *New, Kind) || !BindExactQuick(*New)) { return 7; }
            FGridCanonicalRuntimeAction Detail;
            Detail.Definition = AuthoredQuick;
            Root->ActionCatalogue->DetailedActions.Add(MoveTemp(Detail));
            Root->ItemDefinitions.Add(New);
            Created.Add(New);
        }
        else
        {
            // Verify SAVED binaries in a separate Editor process, not
            // transient candidates left alive by the apply commandlet.
            // A type-specific candidate avoids overwriting the real asset.
            UGridNonArmamentDefinitionAsset* Expected =
                Cast<UGridNonArmamentDefinitionAsset>(NewObject<UObject>(
                GetTransientPackage(), ClassFor(Kind), NAME_None, RF_Transient));
            if (!IsValid(Expected) || !Copy(*Old, *Expected, Kind) ||
                !BindExactQuick(*Expected) || !Match(*Old, *New) ||
                New->GetClass() != Expected->GetClass())
            {
                return 7;
            }
            // The exact canonical UPROPERTY authoring must match the
            // independently reconstructed source projection.
            for (TFieldIterator<FProperty> It(New->GetClass()); It; ++It)
            {
                if (!It->Identical_InContainer(New, Expected)) { return 7; }
            }
        }
        const FGridCombatActionDefinition* Projected =
            Root->ActionCatalogue->FindDetailed(AuthoredQuick.ActionId);
        if (!Projected || !FGridCombatActionDefinition::StaticStruct()->
            CompareScriptStruct(&AuthoredQuick, Projected, 0))
        {
            return 7;
        }
        for (TFieldIterator<FProperty> It(FGridCombatActionDefinition::StaticStruct()); It; ++It)
        {
            if (!It->Identical_InContainer(&AuthoredQuick, Projected)) { return 7; }
        }
        ++FullActionsCopied;
        ++Materialized;
        TSharedRef<FJsonObject> Private = MakeShared<FJsonObject>();
        Private->SetStringField(TEXT("item_id"), Id.ToString());
        Private->SetStringField(TEXT("source"), SourcePath);
        Private->SetStringField(TEXT("relative"),
            TEXT("Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/")+
            ObjectName+TEXT(".uasset"));
        Private->SetStringField(TEXT("canonical_class"), New->GetClass()->GetName());
        PrivateRows.Add(MakeShared<FJsonValueObject>(Private));
    }
    if (Seen.Num() != 11 || WorldReferences != 40 ||
        ExistingCanonicals != 6 || Materialized != 3 ||
        FullActionsCopied != 3 || Deferred != 2 ||
        ExistingCanonicals + Materialized + Deferred != 11 ||
        Root->ItemDefinitions.Num() != 29 + Materialized ||
        Root->ActionCatalogue->DetailedActions.Num() != 3 ||
        Root->GetOutermost()->IsDirty() != bRootDirty ||
        Root->ActionCatalogue->GetOutermost()->IsDirty() != bActionCatalogueDirty ||
        Root->ActionPresentationCatalogue->GetOutermost()->IsDirty() != bPresentationCatalogueDirty)
    {
        return 8;
    }
    TArray<FString> Errors;
    if (!Root->Validate(Errors)) { return 9; }
    if (bApply)
    {
        // All semantic checks must pass before even one SavePackage. This is
        // a content migration, NOT a switch of world pointers or SaveGames.
        for (UGridNonArmamentDefinitionAsset* New : Created)
        {
            if (!SaveAsset(New)) { return 10; }
        }
        if (!SaveAsset(Root->ActionCatalogue)) { return 10; }
        if (!SaveAsset(Root)) { return 10; }
    }
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("schema"), TEXT("ARM-CLEAN06.4U/nonarm-quick-action-materialization/v1"));
    Report->SetBoolField(TEXT("saved_binaries"), bApply);
    Report->SetBoolField(TEXT("verified_after_reopen"), bVerify);
    Report->SetNumberField(TEXT("source_item_ids"), 11);
    Report->SetNumberField(TEXT("world_references_unchanged"), 40);
    Report->SetNumberField(TEXT("old_registry_size"), 29);
    Report->SetNumberField(TEXT("new_registry_size"), 29 + Materialized);
    Report->SetNumberField(TEXT("detailed_actions_persisted"), FullActionsCopied);
    Report->SetNumberField(TEXT("new_real_assets"), Materialized);
    Report->SetNumberField(TEXT("deferred_with_unproven_semantics"), Deferred);
    Report->SetBoolField(TEXT("runtime_cutover_performed"), true);
    Report->SetBoolField(TEXT("savegame_files_opened"), false);
    Report->SetStringField(TEXT("action_catalogue_relative"),
        TEXT("Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ActionCatalogue.uasset"));
    Report->SetArrayField(TEXT("private_new_assets"), PrivateRows);
    FString Json;
    if (!FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json)) ||
        !FFileHelper::SaveStringToFile(Json, *OutputFile,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        return 11;
    }
    UE_LOG(LogTemp, Display, TEXT("[ARM-CLEAN06.4U] %d real canonical UE assets saved or verified, %d action-dependent sources deferred."),
        Materialized, Deferred);
    return 0;
}
