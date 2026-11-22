#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridItemSubsystem.generated.h"

UENUM(BlueprintType)
enum class EGridCanonicalItemResolveError : uint8
{
	None,
	RegistryNotConfigured,
	UnknownItemId,
	InvalidItemLevel,
	InvalidArchetype,
	MissingVisual,
	IncompleteProjectileVisual
};

/**
 * Read-only projection, rebuilt from the single registry per lookup.
 * No UObject catalogue references or static stats are persisted in SaveGame.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridResolvedItem
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	EGridCanonicalItemResolveError Error = EGridCanonicalItemResolveError::RegistryNotConfigured;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FName ItemId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	int32 ItemLevel = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FText DisplayName;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FText Description;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	float WeightKg = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TArray<FName> Tags;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	/** Stable visual identity read from the unique canonical visual catalogue. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FName VisualId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/** Authored pickup-only visual offset, resolved from the single visual catalogue. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FTransform WorldVisualTransform = FTransform::Identity;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TSoftObjectPtr<UStaticMesh> EquippedMesh;

	/** Canonical non-armament equipment permissions (torch, armor, jewelry).
	 * Empty for ammo/consumables and ARM; no legacy slot data. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Equipment")
	TArray<EGridEquipmentSlot> NonArmEquipmentSlots;

	/** Transient projection of the existing specialized non-ARM definition;
	 * never serialized into an instance or duplicated on an ARM. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Equipment")
	FGridEquipmentStatBonus NonArmEquipmentBonuses;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Equipment")
	FGridDamageResistanceSet NonArmEquipmentResistances;

	/** Two-hand canonical non-ARM occupancy; OffHand is a virtual reservation. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Equipment")
	bool bNonArmTwoHanded = false;

	/** Explicit physical hand policy, not inferred from a presentation tag. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Equipment")
	EGridNonArmamentHandPolicy NonArmHandPolicy = EGridNonArmamentHandPolicy::NotHandHeld;

	/** Authored non-ARM flight settings; transient projection only. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Physics")
	FGridNonArmamentThrowPolicy GenericThrow;

	/** Flight shape and presentation are resolved from the one visual catalogue. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TSoftObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FGridCanonicalProjectileVisual ProjectileAppearance;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FName ArchetypeId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	bool bIsArmament = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FGridCanonicalResolvedWeapon Weapon;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	FName AmmoKindId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	int32 MaxStack = 1;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TArray<FGridItemActionBinding> AvailableActions;

	/** ARM-CLEAN06.4X.6: transient non-ARM presentation directly from the
	 * single canonical definition. These values never enter SaveGame. */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	FGridLightEmitterConfig LightEmitter;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	FGridItemVisualEffectsConfig VisualEffects;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	bool bEnableWorldSparkle = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	TSoftObjectPtr<UMaterialInterface> WorldSparkleMaterial;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	FLinearColor WorldSparkleColor = FLinearColor(1.f, 0.78f, 0.42f, 1.f);

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	float WorldSparkleIntensity = 2.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	float WorldSparkleSpeed = 1.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Presentation")
	float WorldSparkleVariation = 0.35f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Reading")
	FText ReadText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Reading")
	bool bReadable = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Physics")
	bool bUseItemWeightAsWorldPhysicsMass = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved|Physics")
	float WorldPhysicsInitialTiltDegrees = 0.f;

	bool IsValid() const { return Error == EGridCanonicalItemResolveError::None; }
};

UENUM(BlueprintType)
enum class EGridCanonicalBoundActionError : uint8
{
	None,
	RegistryNotConfigured,
	ItemResolutionFailed,
	ActionNotBound,
	ActionDefinitionMissing,
	PresentationDefinitionMissing,
	AttackSourceNotAuthorized,
	ActionIsNotAttack,
	InvalidSourceInstance,
	EquipmentSourceMismatch
};

/**
 * One transient view of an explicitly bound action and its presentation.
 * No action/presentation data is stored in ItemDefinition or SaveGame.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridResolvedBoundAction
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	EGridCanonicalBoundActionError Error = EGridCanonicalBoundActionError::RegistryNotConfigured;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	EGridCanonicalItemResolveError ItemError = EGridCanonicalItemResolveError::None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	FName ItemId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	int32 ItemLevel = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	FName ActionId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	FName PresentationId = NAME_None;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	FGridCanonicalAction Action;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Action")
	FGridCanonicalActionPresentation Presentation;

	bool IsValid() const { return Error == EGridCanonicalBoundActionError::None; }
};

/**
 * The new subsystem is unconfigured until DA_ItemSystem is authored in 06.2.
 * Never probes the old per-weapon catalogues, looks up legacy definitions or
 * silently resolves an unknown identity. Configuration occurs ONCE per GI.
 */
UCLASS(Config = Game, DefaultConfig)
class GRIMROCKPROTOTYPE_API UGridItemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** One-time boot configuration; also accepts transient assets in automation tests. */
	bool RegisterItemSystem(UGridItemSystemAsset* System, TArray<FString>& Errors);

	UFUNCTION(BlueprintCallable, Category = "Item System")
	bool ResolveItem(FName ItemId, int32 ItemLevel, FGridResolvedItem& OutItem) const;

	/** The validated registry owns a UNIQUE AmmoKindId -> concrete ItemId join.
	 * Never guess a weapon's ammunition from a legacy arrow asset or name. */
	bool ResolveAmmoKind(FName AmmoKindId, FGridResolvedItem& OutAmmo) const;

	/**
	 * Read remaining armor for this *specific persisted item instance*.
	 * ARM N1-N20 nominal armor comes from the catalogue; non-ARM N0 armor
	 * from its specialized equipment definition. Spent armor is per-instance.
	 * Fails closed for a wrong physical level or a non-equippable item.
	 * Neither the instance nor any registry data is mutated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item System|Armor")
	bool ResolveArmorReserveForInstance(const FGridItemInstance& Instance,
		FGridCanonicalArmorReserveView& OutReserve) const;

	/** Resolve ONLY an authored ItemActionBinding via the four global catalogues. */
	UFUNCTION(BlueprintCallable, Category = "Item System")
	bool ResolveBoundAction(FName ItemId, int32 ItemLevel, FName ActionId,
		FGridResolvedBoundAction& OutAction) const;

	/** Full authored non-arm action, looked up ONLY in the single global
	 * action catalogue and an explicit binding on this item's concrete asset.
	 * No legacy definition, compact re-synthesis or fallback. */
	bool ResolveDetailedNonArmAction(FName ItemId, int32 ItemLevel,
		FName ActionId, FGridCombatActionDefinition& OutAction) const;

	/** A native non-ARM equipped action requires an ACTUAL physical source
	 * slot, never a virtual second attack for a two-handed item. No legacy DA. */
	bool ResolveDetailedNonArmEquippedAction(const FGridItemInstance& Instance,
		EGridEquipmentSlot SourceHand, FName ActionId,
		FGridCombatActionDefinition& OutAction) const;

	/** The ONE detailed non-ARM presentation attached to a physically held
	 * equipment source. Never reads the old UGridItemDefinitionAsset. */
	bool ResolveDetailedNonArmEquippedPresentation(FName ItemId,
		EGridEquipmentSlot SourceHand,
		FGridPlayerAttackPresentationProfile& OutPresentation) const;

	/** Strict equipment-hand entry point for ATTACK only.
	 * An item that can equip OffHand does NOT automatically gain an OffHand attack.
	 * Use ResolveBoundAction for non-attack consumables/abilities.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item System")
	bool ResolveBoundAttackForHand(FName ItemId, int32 ItemLevel, FName ActionId,
		EGridEquipmentSlot SourceHand, FGridResolvedBoundAction& OutAttack) const;

	/**
	 * Verify a persisted equipped item against the identity read from the
	 * CURRENT equipment slot, then resolve its real hand's bound attack.
	 * The caller must read the observed GUID, owner and index from the live
	 * inventory; these arguments do NOT themselves query inventory state.
	 * Rejects stale hotbar GUIDs, cursor/bag items and legacy item level 0.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item System|Combat")
	bool ResolveAttackFromEquippedInstance(const FGridItemInstance& Instance,
		FName ActionId, EGridEquipmentSlot SourceHand,
		FGuid ObservedSlotRuntimeObjectId, FGuid ObservedCharacterId,
		int32 ObservedCharacterIndex, FGridResolvedBoundAction& OutAttack) const;

	UFUNCTION(BlueprintPure, Category = "Item System")
	bool IsConfigured() const { return RootRegistry != nullptr; }

	/** The same config-backed root, exposed only for a transient preview registry
	 * in editor worlds without a GameInstance. Never reads historical catalogues. */
	UGridItemSystemAsset* LoadConfiguredItemSystemForPreview() const
	{
		return ConfiguredItemSystem.LoadSynchronous();
	}

	/** Sorted read-only identity inventory for diagnostics. No asset pointers escape. */
	void GetItemIds(TArray<FName>& OutIds) const
	{
		OutIds.Reset();
		if (IsConfigured())
		{
			ItemIndex.GetKeys(OutIds);
			OutIds.Sort(FNameLexicalLess());
		}
	}

	/** Exact ItemId membership; unknown IDs are handled only by their not-yet-migrated readers. */
	bool ContainsItem(FName ItemId) const
	{
		return RootRegistry && !ItemId.IsNone() && ItemIndex.Contains(ItemId);
	}

	/** Read a visual CATEGORY from the sole canonical WeaponVisualCatalogue.
	 * Does not grant any combat action and never returns the object's ItemIcon.
	 */
	TSoftObjectPtr<UTexture2D> FindEquippedActionIcon(
		EGridEquippedWeaponActionIconKind Kind) const;

private:
	/** Authored once in DefaultGame.ini once DA_ItemSystem exists (06.2). */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Item System")
	TSoftObjectPtr<UGridItemSystemAsset> ConfiguredItemSystem;

	UPROPERTY(Transient)
	TObjectPtr<UGridItemSystemAsset> RootRegistry;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UGridCanonicalItemDefinitionAsset>> ItemIndex;

	bool bConfigurationAttempted = false;
};
