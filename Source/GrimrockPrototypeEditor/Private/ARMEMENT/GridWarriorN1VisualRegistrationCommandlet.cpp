#include "ARMEMENT/GridWarriorN1VisualRegistrationCommandlet.h"
#include "ARMEMENT/GridWarriorN1VisualSpecs.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT011021
{
    const TCHAR* const CataloguePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    const TCHAR* const RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");

    // The preflight is read-only, including when invoked on four incomplete
    // weapons. Never silently substitute a pilot art asset.
    bool CheckResources(const FWarriorVisualSpec& Spec)
    {
        const FString IconObject = IconPath(Spec);
        const FString MeshObject = MeshPath(Spec);
        const FString MaterialObject = MaterialPath(Spec);
        UTexture2D* Icon = LoadObject<UTexture2D>(nullptr, *IconObject);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshObject);
        UMaterialInstance* Material = LoadObject<UMaterialInstance>(nullptr, *MaterialObject);
        if (!IsValid(Icon) || !IsValid(Mesh) || !IsValid(Material))
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: missing UE icon/mesh/MI (%s | %s | %s)"),
                Spec.ArchetypeId, *IconObject, *MeshObject, *MaterialObject);
            return false;
        }
        const FIntPoint IconSize = Icon->GetImportedSize();
        if (IconSize.X < 256 || IconSize.X != IconSize.Y)
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: icon must be square, at least 256 pixels"),
                Spec.ArchetypeId);
            return false;
        }
        if (Mesh->GetNumLODs() <= 0 ||
            Mesh->GetBounds().BoxExtent.ContainsNaN() ||
            Mesh->GetBounds().BoxExtent.GetMax() < 5.0)
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: empty or implausibly small UE mesh"),
                Spec.ArchetypeId);
            return false;
        }
        if (!IsValid(Material->Parent))
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: MI has no valid parent"),
                Spec.ArchetypeId);
            return false;
        }

        bool bMaterialUsed = false;
        const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
        if (Slots.IsEmpty())
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: mesh has no Material Slots"),
                Spec.ArchetypeId);
            return false;
        }
        for (const FStaticMaterial& Slot : Slots)
        {
            if (!IsValid(Slot.MaterialInterface))
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: empty Material Slot"), Spec.ArchetypeId);
                return false;
            }
            bMaterialUsed |= Slot.MaterialInterface == Material;
        }
        if (!bMaterialUsed)
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: approved MI is not on the StaticMesh Material Slots"),
                Spec.ArchetypeId);
            return false;
        }

        const TCHAR* const Suffixes[] = { TEXT("BC"), TEXT("N"), TEXT("ORM") };
        for (const TCHAR* Suffix : Suffixes)
        {
            const FString ObjectPath = PbrPath(Spec, Suffix);
            UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
            if (!IsValid(Texture))
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: PBR texture absent %s"),
                    Spec.ArchetypeId, *ObjectPath);
                return false;
            }
            const FIntPoint Size = Texture->GetImportedSize();
            if (Size.X < 1 || Size.Y < 1)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: invalid PBR texture size %s"),
                    Spec.ArchetypeId, *ObjectPath);
                return false;
            }

            const bool bExpectedSRGB = FCString::Strcmp(Suffix, TEXT("BC")) == 0;
            if (Texture->SRGB != bExpectedSRGB)
            {
                UE_LOG(LogTemp, Display,
                    TEXT("[ARMEMENT01.10.2.1] BLOCKED %s: incorrect sRGB for %s"),
                    Spec.ArchetypeId, *ObjectPath);
                return false;
            }
        }
        return true;
    }

    bool IsSameRegistration(const UGridWeaponVisualCatalogueAsset& Catalogue,
        const FWarriorVisualSpec& Spec)
    {
        if (Catalogue.FindVisualId(FName(Spec.ArchetypeId)) != FName(Spec.VisualId))
        {
            return false;
        }
        const FGridWeaponVisualProfile* Existing =
            Catalogue.FindProfileForArchetype(FName(Spec.ArchetypeId));
        return Existing &&
            Existing->VisualId == FName(Spec.VisualId) &&
            Existing->FamilyTag == FName(Spec.FamilyTag) &&
            Existing->BaseIcon.ToSoftObjectPath() == FSoftObjectPath(IconPath(Spec)) &&
            Existing->WorldMesh.ToSoftObjectPath() == FSoftObjectPath(MeshPath(Spec));
    }

    FGridWeaponVisualProfile BuildProfile(const FWarriorVisualSpec& Spec)
    {
        FGridWeaponVisualProfile Profile;
        Profile.VisualId = FName(Spec.VisualId);
        Profile.FamilyTag = FName(Spec.FamilyTag);
        Profile.BaseIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath(Spec)));
        Profile.WorldMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath(Spec)));
        // The existing resolver uses WorldMesh for equipment when
        // EquippedMesh is not explicitly set. No guessed transforms/ornaments.
        return Profile;
    }
}

UGridWarriorN1VisualRegistrationCommandlet::UGridWarriorN1VisualRegistrationCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridWarriorN1VisualRegistrationCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString Requested;
    FParse::Value(*Params, TEXT("Archetype="), Requested);
    if (bApply && Requested.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.1] -Apply requires exactly one -Archetype=ARMxxx"));
        return 1;
    }

    UGridWeaponVisualCatalogueAsset* Catalogue =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT011021::CataloguePath);
    UGridWeaponCatalogueAsset* Rpg =
        LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT011021::RpgPath);
    if (!IsValid(Catalogue) || !IsValid(Rpg) ||
        !Catalogue->IsValidCatalogue() || !Rpg->IsValidCatalogue())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.1] Invalid existing visual or RPG catalogue"));
        return 1;
    }

    const bool bCatalogueInitiallyDirty = Catalogue->GetOutermost()->IsDirty();
    int32 Selected = 0;
    int32 Ready = 0;
    int32 Blocked = 0;
    int32 AlreadyRegistered = 0;

    for (const ARMEMENT011021::FWarriorVisualSpec& Spec : ARMEMENT011021::Specs)
    {
        if (!Requested.IsEmpty() && Requested != Spec.ArchetypeId)
        {
            continue;
        }
        ++Selected;
        const FGridWeaponLevelProfile* N1 = Rpg->FindProfile(FName(Spec.ArchetypeId), 1);
        if (!N1 || !N1->IsValidProfile())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.1] Missing canonical N1 profile: %s"),
                Spec.ArchetypeId);
            return 1;
        }

        const FName ExistingVisual = Catalogue->FindVisualId(FName(Spec.ArchetypeId));
        if (!ExistingVisual.IsNone())
        {
            if (!ARMEMENT011021::IsSameRegistration(*Catalogue, Spec))
            {
                UE_LOG(LogTemp, Error,
                    TEXT("[ARMEMENT01.10.2.1] CONFLICT %s: existing mapping differs; manual review mandatory"),
                    Spec.ArchetypeId);
                return 1;
            }
            if (!ARMEMENT011021::CheckResources(Spec))
            {
                ++Blocked;
                continue;
            }
            ++AlreadyRegistered;
            UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2.1] REGISTERED %s (art checked; no changes)"),
                Spec.ArchetypeId);
            continue;
        }
        if (Catalogue->FindVisualProfile(FName(Spec.VisualId)))
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ARMEMENT01.10.2.1] CONFLICT: VisualId already used: %s"), Spec.VisualId);
            return 1;
        }
        if (!ARMEMENT011021::CheckResources(Spec))
        {
            ++Blocked;
            continue;
        }
        ++Ready;
        UE_LOG(LogTemp, Display,
            TEXT("[ARMEMENT01.10.2.1] READY %s: approved art can be registered"),
            Spec.ArchetypeId);
        if (!bApply)
        {
            continue;
        }

        // One new binding + profile, preserving all pilot and future records.
        TArray<FGridWeaponVisualArchetypeBinding> Bindings = Catalogue->ArchetypeBindings;
        TArray<FGridWeaponVisualProfile> Profiles = Catalogue->VisualProfiles;
        FGridWeaponVisualArchetypeBinding Binding;
        Binding.ArchetypeId = FName(Spec.ArchetypeId);
        Binding.VisualId = FName(Spec.VisualId);
        Bindings.Add(Binding);
        Profiles.Add(ARMEMENT011021::BuildProfile(Spec));

        UGridWeaponVisualCatalogueAsset* Candidate =
            NewObject<UGridWeaponVisualCatalogueAsset>(GetTransientPackage());
        Candidate->ArchetypeBindings = Bindings;
        Candidate->VisualProfiles = Profiles;
        if (!Candidate->IsValidCatalogue())
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ARMEMENT01.10.2.1] Candidate failed strict visual catalogue validation"));
            return 1;
        }

        if (bCatalogueInitiallyDirty)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.1] Catalogue dirty before apply; refusing write"));
            return 1;
        }

        UPackage* Package = Catalogue->GetOutermost();
        if (!IsValid(Package))
        {
            return 1;
        }
        Catalogue->Modify();
        Catalogue->ArchetypeBindings = MoveTemp(Bindings);
        Catalogue->VisualProfiles = MoveTemp(Profiles);
        Catalogue->MarkPackageDirty();

        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Catalogue, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.1] Catalogue save failed: %s"),
                *Filename);
            return 1;
        }
        UE_LOG(LogTemp, Display,
            TEXT("[ARMEMENT01.10.2.1] SAVED %s; appended exactly one visual binding/profile"),
            Spec.ArchetypeId);
    }

    if (Selected != (Requested.IsEmpty() ? 4 : 1))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.1] Unknown archetype selector: %s"), *Requested);
        return 1;
    }

    if (!bApply)
    {
        if (Catalogue->GetOutermost()->IsDirty() != bCatalogueInitiallyDirty)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2.1] Dry run dirtied catalogue"));
            return 1;
        }
        UE_LOG(LogTemp, Display,
            TEXT("[ARMEMENT01.10.2.1] READ-ONLY PREFLIGHT: %d ready, %d blocked, %d registered; no writes"),
            Ready, Blocked, AlreadyRegistered);
        return 0;
    }

    if (Blocked > 0)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARMEMENT01.10.2.1] BLOCKED: missing verified art; nothing saved"));
        return 2;
    }

    UE_LOG(LogTemp, Display,
        TEXT("[ARMEMENT01.10.2.1] APPLY complete: %d newly registered, %d already present"),
        Ready, AlreadyRegistered);
    return 0;
}
