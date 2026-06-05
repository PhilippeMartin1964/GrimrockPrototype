#pragma once

#include "CoreMinimal.h"
#include "GridInventoryUiTypes.generated.h"

UENUM (BlueprintType)
enum class EInventoryTopTab : uint8
{
    Inventory = 0,
    Skills = 1,
    Journal = 2,
    Map = 3,
    Recipes = 4,
    Codex = 5
};

UENUM (BlueprintType)
enum class EGridInventoryUiSlotType : uint8
{
    Inventory,
    MainHand,
    OffHand,
    Cursor
};

inline const TCHAR* GetGridInventoryUiSlotTypeName (EGridInventoryUiSlotType SlotType)
{
    switch (SlotType)
    {
    case EGridInventoryUiSlotType::Inventory:
        return TEXT ("Inventory");
    case EGridInventoryUiSlotType::MainHand:
        return TEXT ("MainHand");
    case EGridInventoryUiSlotType::OffHand:
        return TEXT ("OffHand");
    case EGridInventoryUiSlotType::Cursor:
        return TEXT ("Cursor");
    default:
        return TEXT ("Unknown");
    }
}
