#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Runtime/Combat/GridWeaponProgressionTypes.h"
#include "Runtime/Combat/GridProjectileGeometry.h"
#include "Runtime/Combat/GridPlayerAttackPresentationTypes.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridEquippedWeaponActionIconTypes.h"
#include "GridItemCatalogues.generated.h"

class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;
class UStaticMesh;
class UTexture2D;

/** Canonical ARM category: shields and passive focus items have no invented attack. */
UENUM(BlueprintType)
enum class EGridArmamentKind : uint8
{
	Weapon,
	Shield,
	Focus
};

UENUM(BlueprintType)
enum class EGridHandPolicy : uint8
{
	MainHand,
	OffHand,
	EitherHand,
	TwoHanded
};

UENUM(BlueprintType)
enum class EGridActionKind : uint8
{
	Attack,
	Defend,
	Use,
	Ability
};

UENUM(BlueprintType)
enum class EGridEffectKind : uint8
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
enum class EGridReactionTrigger : uint8
{
	IncomingAttack,
	ParrySuccess,
	BlockSuccess
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridActionEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EGridEffectKind Kind = EGridEffectKind::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	int32 Magnitude = 0;

	/** Required for ApplyStatus/RemoveStatus; the status asset is an external authority. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	FName StatusId = NAME_None;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EGridActionKind Kind = EGridActionKind::Attack;

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
	TArray<FGridActionEffect> Effects;
};

/** Lossless authored execution policy for non-ARM actions using the very
 * same combat definition consumed by the existing combat runtime.
 * Disjoint ActionIds from compact weapon actions; one global catalogue. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridRuntimeAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action|Runtime")
	FGridCombatActionDefinition Definition;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridActionCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions")
	TArray<FGridAction> Actions;

	/** Detailed non-ARM policies, never stored a second time in Actions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Actions|Detailed")
	TArray<FGridRuntimeAction> DetailedActions;

	const FGridAction* Find(FName ActionId) const;
	const FGridCombatActionDefinition* FindDetailed(FName ActionId) const;
	bool Validate(TArray<FString>& Errors) const;
};

/**
 * One audio-event source with all authored variants. The old Sword swing
 * profiles contain multiple sounds, independent volume and pitch bounds.
 * A single SoundBase pointer would irreversibly discard that information.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridAudioVariants
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
struct GRIMROCKPROTOTYPE_API FGridVFXVariants
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
struct GRIMROCKPROTOTYPE_API FGridActionPresentation
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
	FGridAudioVariants AttackAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants ImpactHitAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants ImpactMissAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants ImpactFleshAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants ImpactMetalAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants ImpactWoodAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants ParryAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants BlockAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants CriticalAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|Audio")
	FGridAudioVariants HeavyAudio;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridVFXVariants AttackVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridVFXVariants ImpactHitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridVFXVariants ImpactMissVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridVFXVariants ParryVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation|VFX")
	FGridVFXVariants BlockVFX;

	bool IsValid() const;
};

/** Full player presentation data where compact channels are not yet a
 * proven lossless projection; disjoint IDs in the SAME global catalogue. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridRuntimePresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FName PresentationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FGridPlayerAttackPresentationProfile Profile;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridActionPresentationCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentations")
	TArray<FGridActionPresentation> Presentations;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentations|Detailed")
	TArray<FGridRuntimePresentation> DetailedPresentations;

	const FGridActionPresentation* Find(FName PresentationId) const;
	const FGridPlayerAttackPresentationProfile* FindDetailed(FName PresentationId) const;
	bool Validate(TArray<FString>& Errors) const;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponVisualBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName ArchetypeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName VisualId = NAME_None;
};

/** Visual-only mode; map from EGridThrowVisualMode explicitly in the Editor converter. */
UENUM(BlueprintType)
enum class EGridProjectileVisualMode : uint8
{
	Stable,
	Tumble,
	Spin
};

/**
 * Projectile presentation belongs to VisualProfiles, never to numeric
 * weapon damage or concrete item definitions.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridProjectileVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	FRotator RelativeRotation = FRotator(-90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	FVector RelativeScale = FVector(1.5f);

	/** Axis of the imported projectile mesh's tip, not dungeon facing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	EGridProjectileForwardAxis MeshForwardAxis = EGridProjectileForwardAxis::PositiveX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	EGridProjectileVisualMode Mode = EGridProjectileVisualMode::Tumble;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	FVector TumbleAxis = FVector(0.65f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	float TumbleDegreesPerSecond = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile|Visual")
	float SpinDegreesPerSecond = 1080.0f;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponVisual
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName VisualId = NAME_None;

	/** Preserves the authored art grouping; never a substitute for ArmamentKind. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName FamilyTag = NAME_None;

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

	/** Persistent pickup presentation, independent of attack SFX/VFX.
	 * Existing UE assets retain default values until verified native migration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|World")
	FGridLightEmitterConfig LightEmitter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|World")
	FGridItemVisualEffectsConfig VisualEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Sparkle")
	bool bEnableWorldSparkle = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Sparkle")
	TSoftObjectPtr<UMaterialInterface> WorldSparkleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Sparkle")
	FLinearColor WorldSparkleColor = FLinearColor(1.f, 0.78f, 0.42f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Sparkle", meta = (ClampMin = "0.0"))
	float WorldSparkleIntensity = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Sparkle", meta = (ClampMin = "0.0"))
	float WorldSparkleSpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual|Sparkle", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WorldSparkleVariation = 0.35f;

	/** Appearance only: cannot influence combat statistics or ItemLevel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TArray<FName> LevelFinishIds;

	/** Projectile-specific mesh. Can equal WorldMesh, but remains a visual-only role. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	/** Exact authored Stable/Tumble/Spin presentation; no gameplay physics. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FGridProjectileVisual ProjectileAppearance;
};

UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridCanonicalWeaponVisualCatalogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TArray<FGridWeaponVisualBinding> ArchetypeBindings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TArray<FGridWeaponVisual> VisualProfiles;

	/** Object icon is distinct from action icon; icons never create actions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TMap<EGridEquippedWeaponActionIconKind, TSoftObjectPtr<UTexture2D>> ActionIcons;

	const FGridWeaponVisual* FindForArchetype(FName ArchetypeId) const;
	bool Validate(TArray<FString>& Errors) const;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridProjectilePolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	bool bEnabled = false;

	/** Ordinary physical hand toss: authored numeric policy, NOT a
	 * recoverable combat projectile or an attack permission. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	bool bGenericThrowAuthored = false;

	/** Physical permission differs from a hand slot: passive shields/foci
	 * can be equipped off-hand without being one-handed throwable objects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	bool bGenericThrowOneHanded = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (ClampMin = "0"))
	float InitialSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile")
	float Arc = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (ClampMin = "0"))
	float FlightLifetime = 0.0f;

	/** Physical landing offset of a recoverable projectile, not a mesh transform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (ClampMin = "0"))
	float ImpactDropOffset = 0.0f;
};

USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridReactionPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction")
	FName ReactionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction")
	EGridReactionTrigger Trigger = EGridReactionTrigger::IncomingAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction", meta = (ClampMin = "0", ClampMax = "1"))
	float Chance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Reaction", meta = (ClampMin = "0"))
	int32 Absorption = 0;
};

/** Fully resolved N1..N20 numeric view. NEVER stored on an item or in SaveGame. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridResolvedWeapon
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	EGridArmamentKind Kind = EGridArmamentKind::Weapon;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	EGridHandPolicy HandPolicy = EGridHandPolicy::MainHand;

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

	/** Transient runtime view of the global projectile policy. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Resolved")
	FGridProjectilePolicy Projectile;
};

/**
 * Read-only armor reserve for a single persisted item instance.
 * The nominal maxima come ONLY from WeaponCatalogue N1..N20.
 * The spent counters come ONLY from the instance's mutable state.
 * This view never repairs, refills or persists a depleted reserve.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridArmorReserveView
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Armor")
	int32 NominalPhysicalArmor = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Armor")
	int32 SpentPhysicalArmor = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Armor")
	int32 RemainingPhysicalArmor = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Armor")
	int32 NominalMagicalArmor = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Armor")
	int32 SpentMagicalArmor = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Weapon|Armor")
	int32 RemainingMagicalArmor = 0;
};

/**
 * Pure read model; does not accept an ItemId or alter an instance/SaveGame.
 * The caller first resolves the actual item's N1..N20 profile via registry.
 */
class GRIMROCKPROTOTYPE_API FGridArmorReserveResolver
{
public:
	static bool TryCompute(const FGridResolvedWeapon& Weapon,
		int32 SpentPhysicalArmor, int32 SpentMagicalArmor,
		FGridArmorReserveView& OutView);
};

/** One archetype owns combat ONLY, with the same five lossless growth rules as CLEAN03. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWeaponArchetype
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName ArchetypeId = NAME_None;

	/**
	 * A numeric-only ARM may be present in the library before its hand/type
	 * gameplay semantics are authored. Never permit it for a concrete item.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Semantics")
	bool bHandAndKindProven = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGridArmamentKind Kind = EGridArmamentKind::Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EGridHandPolicy HandPolicy = EGridHandPolicy::MainHand;

	/** World pickup physical policy, per archetype rather than shared VisualId.
	 * Numeric mass remains the concrete item's WeightKg. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|World Physics")
	bool bUseItemWeightAsWorldPhysicsMass = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|World Physics", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float WorldPhysicsInitialTiltDegrees = 0.0f;

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
	FGridProjectilePolicy Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FGridReactionPolicy> Reactions;

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

	bool TryResolve(int32 Level, FGridResolvedWeapon& Out) const;
};

/** Canonical copy of each of the six workbook Classes-tab rows. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridClassPreference
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
	TArray<FGridWeaponArchetype> Archetypes;

	/** Global class advice from six original rows; no per-weapon duplicates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Class Affinity")
	TArray<FGridClassPreference> ClassPreferences;

	const FGridWeaponArchetype* Find(FName ArchetypeId) const;
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
