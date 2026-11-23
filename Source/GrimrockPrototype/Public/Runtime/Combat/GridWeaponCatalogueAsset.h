#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridWeaponProgressionTypes.h"
#include "GridWeaponCatalogueAsset.generated.h"

/** One N1 baseline plus five integer progressions, instead of 20 full profiles. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponArchetypeProgression
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	FGridWeaponLevelProfile BaseProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	FGridWeaponIntegerGrowthRule DiceCountGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	FGridWeaponIntegerGrowthRule DiceSidesGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	FGridWeaponIntegerGrowthRule FlatBonusGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	FGridWeaponIntegerGrowthRule PhysicalArmorGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Growth")
	FGridWeaponIntegerGrowthRule MagicalArmorGrowth;

	bool TryBuildProfile(int32 ItemLevel, FGridWeaponLevelProfile& OutProfile) const;
};

/** Advisory only: imported from the canonical ARMEMENT01.2 XLSX, never an equip restriction. */
UENUM(BlueprintType)
enum class EGridWeaponClassAffinity : uint8
{
    Unspecified,
    Secondary,
    Primary
};

/** Strictly a kit-planning read model. Keeps the legacy affinity enum unchanged.
 * NeedsReview means the original XLSX cell contains a qualifier or cannot
 * be matched safely to a class; it never forbids equipping anything.
 */
UENUM(BlueprintType)
enum class EGridWeaponKitAffinity : uint8
{
    Unspecified,
    Secondary,
    Primary,
    NeedsReview
};

/** Exactly one entry per catalogue archetype; no duplicated level profiles. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponArchetypeClassAffinity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FName ArchetypeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FName PrimaryClassId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    TArray<FName> SecondaryClassIds;

    /** Original text including special qualifiers (for design audit only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FString SecondarySourceLabel;
};

/** Imported directly from the six rows of the workbook Classes tab. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponClassPreference
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FName ClassId = NAME_None;

    /** Informational design data. Do not display these raw French strings in UMG. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FString CanonicalBranches;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FString PriorityWeapons;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FString EmblematicExamples;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FString Alternatives;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FString DominantAttributes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    FString Synergies;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridWeaponCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Exact import of 82 archetypes x 20 fixed item levels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Catalogue")
	TArray<FGridWeaponLevelProfile> Profiles;

	/** Opt-in lossless compression, mutually exclusive with Profiles. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Catalogue")
	TArray<FGridWeaponArchetypeProgression> ArchetypeProgressions;

    /** Derived projection of the XLSX Catalogue P/Q columns, not authored in Unreal. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    TArray<FGridWeaponArchetypeClassAffinity> ClassAffinities;

    /** Derived projection of the XLSX Classes tab. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Class Affinity")
    TArray<FGridWeaponClassPreference> ClassPreferences;


	/** Validates 1640 unique profiles, exactly one entry per ARM001..ARM082 and level 1..20. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Catalogue")
	bool IsValidCatalogue() const;

	/** Look up an immutable template; never modifies instance state. */
	const FGridWeaponLevelProfile* FindProfile(FName ArchetypeId, int32 ItemLevel) const;

	/** Read-only compression from the existing 1,640 profiles; requires exact parity. */
	bool BuildCompactCandidate(TArray<FGridWeaponArchetypeProgression>& OutProgressions,
		int32& OutExceptionCount) const;

	void InvalidateProfileCache() const { ExpandedProfileCache.Reset(); }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif


    /** Fails if the full workbook-derived metadata has not yet been materialized. */
    UFUNCTION(BlueprintCallable, Category="Weapon|Class Affinity")
    bool IsValidClassAffinityData() const;

    /** Advisory classification only. Unspecified never means forbidden. */
    UFUNCTION(BlueprintPure, Category="Weapon|Class Affinity")
    EGridWeaponClassAffinity GetClassAffinity(FName ArchetypeId, FName ClassId) const;

    /** Kit authoring ONLY: examine the original source qualifier for this class.
     * Do not use the lossy GetClassAffinity result to approve a starter kit.
     * NeedsReview is not a restriction, and never influences CanEquip.
     */
    UFUNCTION(BlueprintPure, Category="Weapon|Class Affinity")
    EGridWeaponKitAffinity GetKitRecommendationAffinity(FName ArchetypeId, FName ClassId) const;


    const FGridWeaponClassPreference* FindClassPreference(FName ClassId) const;


	/** Builds only a damage definition. No rarity, penalty, forge or item-instance mutation. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Catalogue")
	bool TryBuildAttack(FName ArchetypeId, int32 ItemLevel, FGridAttackDefinition& OutAttack) const;
private:
	/** Expanded runtime view is transient: does not reintroduce 1,640 disk records. */
	const TArray<FGridWeaponLevelProfile>* GetRuntimeProfiles() const;
	mutable TArray<FGridWeaponLevelProfile> ExpandedProfileCache;

};
