#pragma once

#include "CoreMinimal.h"
#include "GridInventoryTypes.generated.h"

UENUM (BlueprintType)
enum class EGridItemOwnerType : uint8
{
    None,
    World,
    Receptacle,
    CharacterInventory,
    EquipmentSlot,
    Cursor,
    HeldBySelectedCharacter,
    Removed
};

UENUM (BlueprintType)
enum class EGridEquipmentSlot : uint8
{
    None,
    MainHand,
    OffHand,
    Head,
    Chest,
    Legs,
    Feet,
    Amulet,
    Ring1,
    Ring2,
    Shoulders,
    Gloves,
    Belt,
    Cloak,
    Talisman,
    QuickSlot1,
    QuickSlot2
};

USTRUCT (BlueprintType)
struct FGridItemInstance
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    FGuid RuntimeObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    FName ItemDefinitionId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    int32 Quantity = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    float Weight = 0.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Ownership")
    EGridItemOwnerType OwnerType = EGridItemOwnerType::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Ownership")
    FGuid OwnerGuid;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Ownership")
    int32 OwnerCharacterIndex = INDEX_NONE;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    EGridEquipmentSlot EquipmentSlot = EGridEquipmentSlot::None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Item")
    bool bLightsEnabled = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "World")
    FTransform LastWorldTransform = FTransform::Identity;

    bool IsValid () const
    {
        return RuntimeObjectId.IsValid () && ItemDefinitionId != NAME_None && Quantity > 0;
    }
};

USTRUCT (BlueprintType)
struct FGridInventorySlot
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    bool bOccupied = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FGridItemInstance Item;

    bool IsEmpty () const
    {
        return !bOccupied || !Item.IsValid ();
    }
};

USTRUCT (BlueprintType)
struct FGridCharacterInventoryState
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    FGuid CharacterId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    FName ClassId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Level = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float Strength = 10.0f;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    float CurrentWeight = 0.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    float MaxCarryWeight = 50.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TArray<FGridInventorySlot> InventorySlots;

    bool IsOverloaded () const
    {
        return CurrentWeight > MaxCarryWeight;
    }
};

USTRUCT (BlueprintType)
struct FGridCharacterEquipmentState
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance MainHand;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance OffHand;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Head;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Chest;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Legs;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Feet;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Amulet;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Ring1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Ring2;
};

USTRUCT (BlueprintType)
struct FGridPartyInventoryState
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Party")
    int32 SelectedCharacterIndex = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Party")
    int32 MaxActiveCharacters = 6;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Party")
    TArray<FGridCharacterInventoryState> ActiveCharacters;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Party")
    TArray<FGridCharacterEquipmentState> ActiveEquipment;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Party")
    TArray<FGridCharacterInventoryState> CharacterPool;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Cursor")
    bool bHasCursorItem = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Cursor")
    FGridItemInstance CursorItem;

    bool IsValidActiveCharacterIndex (int32 Index) const
    {
        return ActiveCharacters.IsValidIndex (Index);
    }
};
