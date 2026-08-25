#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridItemActionTypes.generated.h"

class AActor;
class AGrimrockPartyPawn;
class UGridItemDefinitionAsset;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class EGridItemActionType : uint8
{
	None,
	Equip,
	Unequip,
	Consume,
	Read,
	Examine,
	Use,
	UseOnTarget,
	InsertIntoTarget,
	PlaceOnTarget,
	DropToGround,
	Throw,
	Combine,
	SplitStack,
	ToggleLight
};

UENUM(BlueprintType)
enum class EGridFacingTargetType : uint8
{
	None,
	WallLock,
	Receptacle,
	TorchHolder,
	Readable,
	Door,
	Mechanism
};

USTRUCT(BlueprintType)
struct FGridFacingTargetContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	bool bIsValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	TObjectPtr<UPrimitiveComponent> TargetComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	FGuid TargetObjectId;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	EGridFacingTargetType TargetType = EGridFacingTargetType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	EGridLevelObjectType LevelObjectType = EGridLevelObjectType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	bool bAcceptsCurrentItem = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions|Target")
	FText IncompatibilityReason;
};

USTRUCT(BlueprintType)
struct FGridItemActionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Item Actions")
	TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Item Actions")
	FGridItemInstance Item;

	UPROPERTY(BlueprintReadWrite, Category = "Item Actions")
	TObjectPtr<UGridItemDefinitionAsset> ItemDefinition = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Item Actions")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Item Actions")
	int32 InventorySlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Item Actions")
	EGridEquipmentSlot EquipmentSlot = EGridEquipmentSlot::None;
};

USTRUCT(BlueprintType)
struct FGridItemContextAction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions")
	EGridItemActionType ActionType = EGridItemActionType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions")
	FText DisabledReason;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions")
	FGuid TargetObjectId;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Item Actions")
	EGridEquipmentSlot EquipmentSlot = EGridEquipmentSlot::None;
};
