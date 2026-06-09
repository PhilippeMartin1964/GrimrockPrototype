#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridItemTransferService.generated.h"

class AGridReceptacleActor;
class UGridPartyInventoryComponent;

UENUM (BlueprintType)
enum class EGridItemTransferResult : uint8
{
    Success,
    InvalidSource,
    InvalidDestination,
    InvalidItem,
    SourceRemoveFailed,
    DestinationRejectsItem,
    DestinationInsertFailed,
    InventoryFull,
    RollbackFailed
};

USTRUCT (BlueprintType)
struct FGridItemTransferResult
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Item Transfer")
    bool bSuccess = false;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Item Transfer")
    EGridItemTransferResult Result = EGridItemTransferResult::InvalidSource;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Item Transfer")
    FText Message;
};

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridItemTransferService : public UBlueprintFunctionLibrary
{
    GENERATED_BODY ()

public:
    UFUNCTION (BlueprintCallable, Category = "Grid|Item Transfer")
    static FGridItemTransferResult TransferInventorySlotToReceptacle (
        UGridPartyInventoryComponent* Inventory,
        int32 CharacterIndex,
        int32 InventorySlotIndex,
        AGridReceptacleActor* Receptacle);

    UFUNCTION (BlueprintCallable, Category = "Grid|Item Transfer")
    static FGridItemTransferResult TransferEquipmentSlotToReceptacle (
        UGridPartyInventoryComponent* Inventory,
        int32 CharacterIndex,
        EGridEquipmentSlot EquipmentSlot,
        AGridReceptacleActor* Receptacle);

    UFUNCTION (BlueprintCallable, Category = "Grid|Item Transfer")
    static FGridItemTransferResult TransferReceptacleItemToInventory (
        AGridReceptacleActor* Receptacle,
        int32 ContainedItemIndex,
        UGridPartyInventoryComponent* Inventory,
        int32 CharacterIndex);
};
