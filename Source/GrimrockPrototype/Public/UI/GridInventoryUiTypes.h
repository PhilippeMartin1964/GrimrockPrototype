#pragma once

#include "CoreMinimal.h"
#include "GridInventoryUiTypes.generated.h"

UENUM (BlueprintType)
enum class EGridInventoryUiSlotType : uint8
{
    Inventory,
    MainHand,
    OffHand,
    Cursor
};
