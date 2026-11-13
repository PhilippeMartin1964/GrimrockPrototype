#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
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

	/** Concrete attack execution mode; does not invent a new attack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	bool bRangedAttack = false;

	/** Independent line-of-sight requirement, preserved from the authored action. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	bool bRequiresLineOfSight = false;

	/** Action-specific item consumption (throwing weapons); not an ammo kind. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action", meta = (ClampMin = "0"))
	int32 SourceItemQuantityCost = 0;

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

/**
 * One audio-event source with all authored variants. The old Sword swing
 * profiles contain multiple sounds, independent volume and pitch bounds.
 * A single SoundBase pointer would irreversibly discard that information.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalAudioVariants
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	TArray<TSoftObjectPtr<USoundBase>> Sounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio", meta = (ClampMin = "0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio", meta = (ClampMin = "0.01"))
	float PitchMin = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio", meta = (ClampMin = "0.01"))
	float PitchMax = 1.0f;

	bool IsValid() const;
};

/** A Niagara event may own several systems and its attachment transform. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalVFXVariants
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	TArray<TSoftObjectPtr<UNiagaraSystem>> Systems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	bool bAttachToHeldItem = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FVector Scale = FVector::OneVector;

	bool IsValid() const;
};

/**
 * Canonical authored animation and FX. Every channel has exactly one owner:
 * the ActionPresentationCatalogue, never the concrete DA_Weapon_*.
 * AttackAudio, ImpactHitAudio and ImpactMissAudio map the existing UE player
 * attack presentation fields without discarding arrays, pitch or timing.
 * Material/reaction/outcome channels are genuine opt-in extensions.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalActionPresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FName PresentationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion")
	EGridPlayerAttackMotionStyle MotionStyle = EGridPlayerAttackMotionStyle::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion")
	bool bAnimateHeldItem = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion", meta = (ClampMin = "0"))
	float MotionDurationSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion", meta = (ClampMin = "0"))
	float ImpactDelaySeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion", meta = (ClampMin = "0"))
	float RecoveryDelaySeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion")
	FVector PeakLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Motion")
	FRotator PeakRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Feedback", meta = (ClampMin = "0.1"))
	float FeedbackDurationSeconds = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants AttackAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants ImpactHitAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants ImpactMissAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants ImpactFleshAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants ImpactMetalAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants ImpactWoodAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants ParryAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants BlockAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants CriticalAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridCanonicalAudioVariants HeavyAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridCanonicalVFXVariants AttackVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridCanonicalVFXVariants ImpactHitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridCanonicalVFXVariants ImpactMissVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridCanonicalVFXVariants ParryVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridCanonicalVFXVariants BlockVFX;

	bool IsValid() const;
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

	/** Explicit attack-source permission, independent of hand equip permission. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	TArray<EGridEquipmentSlot> AttackSourceSlots;

	bool CanEquipInHand(EGridEquipmentSlot Slot) const;
	bool HasAttackSourceInHand(EGridEquipmentSlot Slot) const;

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

	/** Original Primary/Secondary advisory metadata, never an equip restriction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FName PrimaryClassId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	TArray<FName> SecondaryClassIds;

	/** Preserve original XLSX qualifiers verbatim; do not infer class IDs from prose. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FString SecondarySourceLabel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FGridItemActionBinding> Actions;

	/**
	 * MainHand/OffHand sources of an ALREADY AUTHORed attack. A one-handed
	 * weapon may be equippable in both hands without exposing a second attack.
	 * Kept only in the weapon catalogue, never on the concrete DA_Weapon.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Combat")
	TArray<EGridEquipmentSlot> AttackSourceSlots;

	bool TryResolve(int32 Level, FGridCanonicalResolvedWeapon& Out) const;
};

/** Canonical copy of each of the six workbook Classes-tab rows. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCanonicalClassPreference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FName ClassId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FString CanonicalBranches;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FString PriorityWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FString EmblematicExamples;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FString Alternatives;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FString DominantAttributes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	FString Synergies;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridCanonicalWeaponCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Catalogue")
	TArray<FGridCanonicalWeaponArchetype> Archetypes;

	/** Global class advice from six original rows; no per-weapon duplicates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	TArray<FGridCanonicalClassPreference> ClassPreferences;

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
