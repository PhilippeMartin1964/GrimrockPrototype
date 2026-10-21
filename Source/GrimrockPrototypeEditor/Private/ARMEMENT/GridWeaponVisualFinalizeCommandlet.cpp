#include "ARMEMENT/GridWeaponVisualFinalizeCommandlet.h"

#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstance.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"

namespace ARMEMENT0163Final
{
	const TCHAR* const CataloguePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");

	struct FApprovedPilot
	{
		const TCHAR* Archetype;
		const TCHAR* CurrentVisualId;
		const TCHAR* Family;
		const TCHAR* MeshName;
		const TCHAR* IconName;
		const TCHAR* MaterialName;
		const TCHAR* TextureName;
	};

	const FApprovedPilot Pilots[] =
	{
		{TEXT("ARM001"), TEXT("VIS_Dagger_01"), TEXT("Light"), TEXT("SM_Weapon_Dagger_01"),
			TEXT("T_Weapon_Dagger_01"), TEXT("MI_Dagger"), TEXT("Dagger")},
		{TEXT("ARM045"), TEXT("VIS_ShortBow_01"), TEXT("Ranged"), TEXT("SM_Weapon_ShortBow_01"),
			TEXT("T_Weapon_ShortBow_01"), TEXT("MI_Bow"), TEXT("Bow")},
		{TEXT("ARM075"), TEXT("VIS_RoundShield_01"), TEXT("Shield"), TEXT("SM_Weapon_RoundShield_01"),
			TEXT("T_Weapon_RoundShield_01"), TEXT("MI_Shield"), TEXT("Shield")}
	};

	FString MeshPath(const FApprovedPilot& P)
	{
		return FString::Printf(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/%s.%s"),
			P.MeshName, P.MeshName);
	}
	FString IconPath(const FApprovedPilot& P)
	{
		return FString::Printf(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/%s.%s"),
			P.IconName, P.IconName);
	}
	FString MaterialPath(const FApprovedPilot& P)
	{
		return FString::Printf(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Weapons/%s.%s"),
			P.MaterialName, P.MaterialName);
	}
	FString TexturePath(const FApprovedPilot& P, const TCHAR* Suffix)
	{
		return FString::Printf(TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/%s/T_%s_01_%s.T_%s_01_%s"),
			P.TextureName, P.TextureName, Suffix, P.TextureName, Suffix);
	}

	bool CheckDependencies(const FApprovedPilot& P)
	{
		const FString IconObjectPath = IconPath(P);
		UTexture2D* Icon = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath(P));
		UMaterialInstance* Material = LoadObject<UMaterialInstance>(nullptr, *MaterialPath(P));
		if (!IsValid(Icon) || !IsValid(Mesh) || !IsValid(Material))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Missing icon, mesh or material for %s"), P.Archetype);
			return false;
		}
		if (!IsValid(Material->Parent))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] MI lacks parent: %s"),
				*MaterialPath(P));
			return false;
		}
		const FIntPoint IconSize = Icon->GetImportedSize();
		if (IconSize.X <= 0 || IconSize.X != IconSize.Y || Mesh->GetNumLODs() <= 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Bad icon size or mesh geometry: %s"),
				P.Archetype);
			return false;
		}
		const TArray<FStaticMaterial>& Slots = Mesh->GetStaticMaterials();
		bool bUsesApprovedMaterial = false;
		for (const FStaticMaterial& Slot : Slots)
		{
			if (!IsValid(Slot.MaterialInterface))
			{
				UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Empty Material Slot: %s"),
					P.MeshName);
				return false;
			}
			if (Slot.MaterialInterface == Material)
			{
				bUsesApprovedMaterial = true;
			}
		}
		if (!bUsesApprovedMaterial)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[ARMEMENT01.6.3-FINAL] %s has no Material Slot assigned to %s; correct in UE before finalizing."),
				P.MeshName, P.MaterialName);
			return false;
		}
		const TCHAR* Suffixes[] = {TEXT("BC"), TEXT("N"), TEXT("ORM")};
		for (const TCHAR* Suffix : Suffixes)
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath(P, Suffix));
			if (!IsValid(Texture))
			{
				UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Missing %s texture for %s"),
					Suffix, P.Archetype);
				return false;
			}
			const FIntPoint Size = Texture->GetImportedSize();
			if (Size.X <= 0 || Size.Y <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Empty PBR map %s"), *TexturePath(P, Suffix));
				return false;
			}
			if ((FCString::Strcmp(Suffix, TEXT("N")) == 0 ||
					FCString::Strcmp(Suffix, TEXT("ORM")) == 0) && Texture->SRGB)
			{
				UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Non-color map has sRGB ON: %s"),
					*TexturePath(P, Suffix));
				return false;
			}
		}
		return true;
	}
}

UGridWeaponVisualFinalizeCommandlet::UGridWeaponVisualFinalizeCommandlet()
{
	IsEditor = true;
	IsClient = false;
	IsServer = false;
	LogToConsole = true;
}

int32 UGridWeaponVisualFinalizeCommandlet::Main(const FString& Params)
{
	using namespace ARMEMENT0163Final;
	// By default, no asset is saved. Explicit -Apply required for modifications.
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	UGridWeaponVisualCatalogueAsset* Catalogue =
		LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, CataloguePath);
	if (!IsValid(Catalogue))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Missing existing catalogue %s"), CataloguePath);
		return 1;
	}
	if (Catalogue->ArchetypeBindings.Num() != 3 || Catalogue->VisualProfiles.Num() != 3)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ARMEMENT01.6.3-FINAL] Expected exactly 3 pilot bindings/profiles; refusing to discard other content."));
		return 1;
	}

	// Preflight all 19 approved assets before any mutation, keeping the two
	// unrelated Shuriken / ThrowingKnife asset families out of scope.
	for (const FApprovedPilot& P : Pilots)
	{
		if (!CheckDependencies(P))
		{
			return 1;
		}
	}

	TArray<FGridWeaponVisualArchetypeBinding> NewBindings;
	TArray<FGridWeaponVisualProfile> NewProfiles;
	for (const FApprovedPilot& P : Pilots)
	{
		const FGridWeaponVisualProfile* Existing =
			Catalogue->FindProfileForArchetype(FName(P.Archetype));
		if (!Existing)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[ARMEMENT01.6.3-FINAL] Missing existing mapping for %s; refusing an implicit migration."),
				P.Archetype);
			return 1;
		}
		FGridWeaponVisualProfile Profile = *Existing;
		Profile.VisualId = FName(P.CurrentVisualId);
		Profile.FamilyTag = FName(P.Family);
		Profile.BaseIcon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath(P)));
		Profile.WorldMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MeshPath(P)));
		// The pilot's world mesh is sufficient for the future equipped preview.
		// Any explicit EquippedMesh / transforms / ornaments remain untouched.
		FGridWeaponVisualArchetypeBinding Binding;
		Binding.ArchetypeId = FName(P.Archetype);
		Binding.VisualId = FName(P.CurrentVisualId);
		NewProfiles.Add(MoveTemp(Profile));
		NewBindings.Add(Binding);
	}

	// Validate the prepared state before replacing any field of the
	// asset resident in the Editor. Never save on dry-run.
	UGridWeaponVisualCatalogueAsset* Candidate = NewObject<UGridWeaponVisualCatalogueAsset>();
	Candidate->ArchetypeBindings = NewBindings;
	Candidate->VisualProfiles = NewProfiles;
	if (!Candidate->IsValidCatalogue())
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Resulting catalogue is invalid."));
		return 1;
	}

	if (!bApply)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[ARMEMENT01.6.3-FINAL] DRY RUN OK: ARM001/ARM045/ARM075 mapped to numeric _01 assets. No saves."));
		return 0;
	}

	Catalogue->Modify();
	Catalogue->ArchetypeBindings = MoveTemp(NewBindings);
	Catalogue->VisualProfiles = MoveTemp(NewProfiles);
	Catalogue->MarkPackageDirty();

	UPackage* Package = Catalogue->GetOutermost();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, Catalogue, *Filename, Args))
	{
		UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.3-FINAL] Catalogue save failed: %s"), *Filename);
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[ARMEMENT01.6.3-FINAL] SAVED three realistic profiles with numeric _01 suffix. No other package modified."));
	return 0;
}
