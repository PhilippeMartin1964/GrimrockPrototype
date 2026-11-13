#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "UI/GridEquippedWeaponActionIconPalette.h"
#include "GridCanonicalItemCatalogues.generated.h"

class UNiagaraSystem;
class USoundBase;
class UStaticMesh;
class UTexture2D;

/** Canonical ARM category: shields and passive focus items have no invented attack. */
UENUM(BlueprintType)
enum class EGridCanonicalArmamentKind : uint8
{
	Weapon,
	Shield,
	Focus
};

UENUM(BlueprintType)
enum class EGridCanonicalHandPolicy : uint8
{
	MainHand,
	OffHand,
	EitherHand,
	TwoHanded
};

UENUM(BlueprintType)
enum class EGridCanonicalActionKind : uint8
{
	Attack,
	Defend,
	Use,
	Ability
};

UENUM(BlueprintType)
enum class EGridCanonicalEffectKind : uint8
{
	Damage,
	Heal,
	ApplyStatus,
	RemoveStatus,
	Armor,
	Move,
	Surface,
	Trap,
	Teleport
};

UENUM(BlueprintType)
enum class EGridCanonicalReactionTrigger : uint8
{
	IncomingAttack,
	ParrySuccess,
	BlockSuccess
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalActionEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EGridCanonicalEffectKind Kind = EGridCanonicalEffectKind::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	int32 Magnitude = 0;

	/** Required for ApplyStatus/RemoveStatus; the status asset is an external authority. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	FName StatusId = NAME_None;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EGridCanonicalActionKind Kind = EGridCanonicalActionKind::Attack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EGridCombatTargetingPolicy Targeting = EGridCombatTargetingPolicy::Hostile;

	/** Costs specific to this action; standard weapon PA and range are NOT copied here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0"))
	int32 ManaCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0"))
	int32 CooldownRounds = 0;

	/** Special ability coefficient only; 100 means normal resolved weapon damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0", ClampMax = "500"))
	int32 WeaponDamagePercent = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	TArray<FGridCanonicalActionEffect> Effects;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridActionCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TArray<FGridCanonicalAction> Actions;

	const FGridCanonicalAction* Find(FName ActionId) const;
	bool Validate(TArray<FString>& Errors) const;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalActionPresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FName PresentationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FName MotionPresetId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> Swing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> ImpactFlesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> ImpactMetal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> ImpactWood;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> Parry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> Block;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> Critical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> Heavy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	TSoftObjectPtr<UNiagaraSystem> AttackNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	TSoftObjectPtr<UNiagaraSystem> ImpactNiagara;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridActionPresentationCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentations")
	TArray<FGridCanonicalActionPresentation> Presentations;

	const FGridCanonicalActionPresentation* Find(FName PresentationId) const;
	bool Validate(TArray<FString>& Errors) const;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalWeaponVisualBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName VisualId = NAME_None;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalWeaponVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName VisualId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TSoftObjectPtr<UStaticMesh> EquippedMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TSoftObjectPtr<UStaticMesh> OrnamentMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FTransform OrnamentTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FTransform WorldTransform = FTransform::Identity;

	/** Appearance only: cannot influence combat statistics or ItemLevel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TArray<FName> LevelFinishIds;

	/** Presentation-only projectile mesh and attachment offset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridCanonicalWeaponVisualCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TArray<FGridCanonicalWeaponVisualBinding> ArchetypeBindings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TArray<FGridCanonicalWeaponVisual> VisualProfiles;

	/** Object icon is distinct from action icon; icons never create actions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TMap<EGridEquippedWeaponActionIconKind, TSoftObjectPtr<UTexture2D>> ActionIcons;

	const FGridCanonicalWeaponVisual* FindForArchetype(FName ArchetypeId) const;
	bool Validate(TArray<FString>& Errors) const;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalProjectilePolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (ClampMin = "0"))
	float InitialSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	float Arc = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (ClampMin = "0"))
	float FlightLifetime = 0.0f;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalReactionPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction")
	FName ReactionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction")
	EGridCanonicalReactionTrigger Trigger = EGridCanonicalReactionTrigger::IncomingAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction", meta = (ClampMin = "0"))
	int32 Absorption = 0;
};

/** Fully resolved N1..N20 numeric view. NEVER stored on an item or in SaveGame. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalResolvedWeapon
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	EGridCanonicalArmamentKind Kind = EGridCanonicalArmamentKind::Weapon;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	EGridCanonicalHandPolicy HandPolicy = EGridCanonicalHandPolicy::MainHand;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	EGridAttackScalingAttribute ScalingAttribute = EGridAttackScalingAttribute::None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	FGridDamageDiceProfile DamageDice;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	int32 AccuracyBonus = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	int32 ActionPointCost = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	int32 RangeCells = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	int32 PhysicalArmor = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	int32 MagicalArmor = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	FName RequiredAmmoKindId = NAME_None;
};

/** One archetype owns combat ONLY, with the same five lossless growth rules as CLEAN03. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalWeaponArchetype
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGridCanonicalArmamentKind Kind = EGridCanonicalArmamentKind::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGridCanonicalHandPolicy HandPolicy = EGridCanonicalHandPolicy::MainHand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGridDamageType DamageType = EGridDamageType::Physical;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGridPhysicalDamageSubtype PhysicalSubtype = EGridPhysicalDamageSubtype::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGridAttackScalingAttribute ScalingAttribute = EGridAttackScalingAttribute::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 AccuracyBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 ActionPointCost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	int32 RangeCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	FGridDamageDiceProfile DamageDiceN1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	int32 PhysicalArmorN1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	int32 MagicalArmorN1 = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	FGridWeaponIntegerGrowthRule DiceCountGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	FGridWeaponIntegerGrowthRule DiceSidesGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	FGridWeaponIntegerGrowthRule FlatBonusGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	FGridWeaponIntegerGrowthRule PhysicalArmorGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|N1")
	FGridWeaponIntegerGrowthRule MagicalArmorGrowth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName RequiredAmmoKindId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FGridCanonicalProjectilePolicy Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FGridCanonicalReactionPolicy> Reactions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FName> RecommendedClassIds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FGridItemActionBinding> Actions;

	bool TryResolve(int32 Level, FGridCanonicalResolvedWeapon& Out) const;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridCanonicalWeaponCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Catalogue")
	TArray<FGridCanonicalWeaponArchetype> Archetypes;

	const FGridCanonicalWeaponArchetype* Find(FName ArchetypeId) const;
	bool Validate(TArray<FString>& Errors) const;
};

/** One root registry, four global catalogue references, no per-item pointers. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridItemSystemAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item System")
	TArray<TObjectPtr<UGridCanonicalItemDefinitionAsset>> ItemDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item System")
	TObjectPtr<UGridCanonicalWeaponCatalogueAsset> WeaponCatalogue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item System")
	TObjectPtr<UGridCanonicalWeaponVisualCatalogueAsset> WeaponVisualCatalogue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item System")
	TObjectPtr<UGridActionCatalogueAsset> ActionCatalogue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item System")
	TObjectPtr<UGridActionPresentationCatalogueAsset> ActionPresentationCatalogue;

	bool Validate(TArray<FString>& Errors) const;
};
