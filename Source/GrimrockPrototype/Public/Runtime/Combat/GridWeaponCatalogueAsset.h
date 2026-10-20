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

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridWeaponCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Exact import of 82 archetypes x 20 fixed item levels. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Catalogue")
	TArray<FGridWeaponLevelProfile> Profiles;

	/** Validates 1640 unique profiles, exactly one entry per ARM001..ARM082 and level 1..20. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Catalogue")
	bool IsValidCatalogue() const;

	/** Look up an immutable template; never modifies instance state. */
	const FGridWeaponLevelProfile* FindProfile(FName ArchetypeId, int32 ItemLevel) const;

	/** Builds only a damage definition. No rarity, penalty, forge or item-instance mutation. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Catalogue")
	bool TryBuildAttack(FName ArchetypeId, int32 ItemLevel, FGridAttackDefinition& OutAttack) const;
};
