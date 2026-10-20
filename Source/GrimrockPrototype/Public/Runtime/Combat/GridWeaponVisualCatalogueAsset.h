#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GridWeaponVisualCatalogueAsset.generated.h"

class UTexture2D;
class UStaticMesh;

/**
 * ARMEMENT01.6.1: one stable, non-gameplay binding from a weapon archetype to
 * a reusable visual silhouette. Multiple archetypes may share a VisualId.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponVisualArchetypeBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Identity")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Identity")
	FName VisualId = NAME_None;

	bool IsValidBinding() const;
};

/**
 * Art-only visual data, independent from ItemLevel, dice, rarity and crafting.
 * The StaticMesh owns its normal material slots (MATERIAL-OWNERSHIP01).
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponVisualProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Identity")
	FName VisualId = NAME_None;

	/** Presentation grouping, not an alternate combat family or item type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Identity")
	FName FamilyTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Assets")
	TSoftObjectPtr<UTexture2D> BaseIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Assets")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/** May be null: the visual resolver will then use WorldMesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Assets")
	TSoftObjectPtr<UStaticMesh> EquippedMesh;

	/** Presentation-only offset, never dungeon placement or physics authority. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Transform")
	FTransform WorldVisualTransform = FTransform::Identity;

	/** Optional, non-colliding child decoration. Never an alternate material override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Ornament")
	TSoftObjectPtr<UStaticMesh> OrnamentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Ornament")
	FTransform OrnamentRelativeTransform = FTransform::Identity;

	/** Structural validation only; does not synchronously load image/mesh assets. */
	bool IsValidProfile() const;
};

/** Reserved for the display-only resolution stage ARMEMENT01.6.2. */
UENUM(BlueprintType)
enum class EGridWeaponVisualResolutionError : uint8
{
	None,
	CatalogueUnavailable,
	MissingBinding,
	UnknownVisualId,
	MissingVisualResource,
	InstanceContextUnavailable
};

/**
 * Transient read model: may be sent to inventory/hotbar/world preview.
 * It is never copied into FGridItemInstance nor persisted in the SaveGame.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridResolvedWeaponVisual
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	bool bResolved = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	bool bLegacyFallback = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	FName VisualId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	TSoftObjectPtr<UStaticMesh> EquippedMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	TSoftObjectPtr<UStaticMesh> OrnamentMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	FTransform WorldVisualTransform = FTransform::Identity;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	FTransform OrnamentRelativeTransform = FTransform::Identity;

	/** False for definition-only previews or Legacy items. No invented level. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	bool bHasItemLevel = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	int32 ItemLevel = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Visual|Resolved")
	EGridWeaponVisualResolutionError Error = EGridWeaponVisualResolutionError::None;
};

/**
 * A single visual-asset library, authored independently from
 * DA_WeaponCatalogue. A partially mapped pilot catalogue is valid.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridWeaponVisualCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Catalogue")
	TArray<FGridWeaponVisualArchetypeBinding> ArchetypeBindings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Catalogue")
	TArray<FGridWeaponVisualProfile> VisualProfiles;

	/** Accepts canonical ARM001..ARM082 only; never guesses IDs. */
	static bool IsCanonicalArchetypeId(FName ArchetypeId);

	/** Structural validation only. Resource existence is verified in later stages. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Visual|Catalogue")
	bool IsValidCatalogue() const;

	/** Returns NAME_None for unmapped/invalid archetypes. No level is involved. */
	UFUNCTION(BlueprintPure, Category = "Weapon|Visual|Catalogue")
	FName FindVisualId(FName ArchetypeId) const;

	/** An immutable authored profile; null if VisualId is unknown. */
	const FGridWeaponVisualProfile* FindVisualProfile(FName VisualId) const;

	/** Convenience lookup; returns null when an archetype has no visual binding. */
	const FGridWeaponVisualProfile* FindProfileForArchetype(FName ArchetypeId) const;
};
