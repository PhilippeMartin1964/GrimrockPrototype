#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridWeaponCatalogueAsset.generated.h"

/**
 * ARMEMENT01.4 - one immutable authored profile per archetype and item level.
 * The canonical spreadsheet is the design authority; this asset is the cooked
 * runtime lookup. No character level, rarity or crafting state is stored here.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponLevelProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity", meta=(ClampMin="1", ClampMax="20"))
	int32 ItemLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FString Family;

	/** Informational; equipment slot restrictions continue to live in item definitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Identity")
	FString HandUsage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	EGridAttackScalingAttribute ScalingAttribute = EGridAttackScalingAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	int32 AccuracyBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	FGridDamageDiceProfile DamageDice;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Defense")
	int32 PhysicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Defense")
	int32 MagicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	int32 ActionPointCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Combat")
	int32 RangeCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Inventory")
	float WeightKg = 0.0f;

	/** Rejects invalid authoring, including fake dice on passive shields/focuses. */
	bool IsValidProfile() const;

	/** Returns false for passive profiles. Does not create or resolve a combat action. */
	bool TryBuildAttackDefinition(FGridAttackDefinition& OutAttack) const;
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
};
