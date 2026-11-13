#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
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

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Item|Resolved")
	TSoftObjectPtr<UStaticMesh> EquippedMesh;

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

	/**
	 * Read remaining armor for this *specific persisted item instance*.
	 * Nominal armor comes from the one N1-N20 catalogue, spent armor from
	 * this instance only. Fails closed for legacy level 0 and non-armaments.
	 * Neither the instance nor any registry data is mutated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Item System|Armor")
	bool ResolveArmorReserveForInstance(const FGridItemInstance& Instance,
		FGridCanonicalArmorReserveView& OutReserve) const;

	/** Resolve ONLY an authored ItemActionBinding via the four global catalogues. */
	UFUNCTION(BlueprintCallable, Category = "Item System")
	bool ResolveBoundAction(FName ItemId, int32 ItemLevel, FName ActionId,
		FGridResolvedBoundAction& OutAction) const;

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

	/** Exact ItemId membership; unknown IDs are handled only by their not-yet-migrated readers. */
	bool HasCanonicalItem(FName ItemId) const
	{
		return RootRegistry && !ItemId.IsNone() && ItemIndex.Contains(ItemId);
	}

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
