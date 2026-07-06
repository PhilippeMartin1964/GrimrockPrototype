#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGCharacterTypes.h"
#include "GridInventoryTypes.generated.h"

class UGridReadableContentAsset;
class UTexture2D;

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
    QuickSlot2,
    Face,
    Shirt,
    Bracers,
    Earring1,
    Earring2
};

USTRUCT (BlueprintType)
struct FGridEquipmentStatBonus
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 StrengthBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 DexterityBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 ConstitutionBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 IntelligenceBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 WisdomBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 CharismaBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 MaxHealthBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 MaxManaBonus = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    float CarryWeightBonus = 0.0f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment|Stats")
    int32 ArmorBonus = 0;

    bool HasAnyBonus () const
    {
        return StrengthBonus != 0 ||
            DexterityBonus != 0 ||
            ConstitutionBonus != 0 ||
            IntelligenceBonus != 0 ||
            WisdomBonus != 0 ||
            CharismaBonus != 0 ||
            MaxHealthBonus != 0 ||
            MaxManaBonus != 0 ||
            !FMath::IsNearlyZero (CarryWeightBonus) ||
            ArmorBonus != 0;
    }
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
    FText DisplayName;

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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Reading")
    TObjectPtr<UGridReadableContentAsset> ReadableContentAsset = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Reading")
    FName ReadableContentId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Reading")
    FText ReadTitleOverride;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Reading", meta = (MultiLine = "true"))
    FText ReadTextOverride;

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
    FText ClassDisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    FName RaceId = TEXT ("Human");

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    FText RaceDisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    int32 Level = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character", meta = (ClampMin = "0"))
    int32 Experience = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG")
    FRPGAttributes Attributes;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG")
    FRPGDerivedStats DerivedStats;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    ERPGCharacterPortraitGender PortraitGender = ERPGCharacterPortraitGender::Male;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    FName PortraitVariantId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Character")
    TSoftObjectPtr<UTexture2D> ClassIcon;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG")
    bool bRPGAttributesInitialized = false;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "Stats",
        meta = (DeprecatedProperty, DeprecationMessage = "Use Attributes.Strength. Kept temporarily for serialized data migration."))
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
struct FGridInventoryCharacterSummary
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    FName CharacterId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    FText DisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    FName ClassId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    FText ClassDisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    FName RaceId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    FText RaceDisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    int32 Level = 1;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    int32 Experience = 0;

    UPROPERTY (BlueprintReadOnly, Category = "RPG")
    FRPGAttributes Attributes;

    UPROPERTY (BlueprintReadOnly, Category = "RPG")
    FRPGDerivedStats DerivedStats;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    ERPGCharacterPortraitGender PortraitGender = ERPGCharacterPortraitGender::Male;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    FName PortraitVariantId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    TSoftObjectPtr<UTexture2D> ClassIcon;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    int32 UsedInventorySlots = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    int32 MaxInventorySlots = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    float CurrentWeight = 0.0f;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    float MaxWeight = 0.0f;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    bool bOverloaded = false;

    UPROPERTY (BlueprintReadOnly, Category = "Character")
    bool bIsSelected = false;
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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Shoulders;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Gloves;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Belt;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Cloak;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Talisman;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance QuickSlot1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance QuickSlot2;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Face;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Shirt;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Bracers;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Earring1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    FGridItemInstance Earring2;

    FGridItemInstance* GetMutableSlot (EGridEquipmentSlot Slot)
    {
        switch (Slot)
        {
        case EGridEquipmentSlot::MainHand:
            return &MainHand;
        case EGridEquipmentSlot::OffHand:
            return &OffHand;
        case EGridEquipmentSlot::Head:
            return &Head;
        case EGridEquipmentSlot::Chest:
            return &Chest;
        case EGridEquipmentSlot::Legs:
            return &Legs;
        case EGridEquipmentSlot::Feet:
            return &Feet;
        case EGridEquipmentSlot::Amulet:
            return &Amulet;
        case EGridEquipmentSlot::Ring1:
            return &Ring1;
        case EGridEquipmentSlot::Ring2:
            return &Ring2;
        case EGridEquipmentSlot::Shoulders:
            return &Shoulders;
        case EGridEquipmentSlot::Gloves:
            return &Gloves;
        case EGridEquipmentSlot::Belt:
            return &Belt;
        case EGridEquipmentSlot::Cloak:
            return &Cloak;
        case EGridEquipmentSlot::Talisman:
            return &Talisman;
        case EGridEquipmentSlot::QuickSlot1:
            return &QuickSlot1;
        case EGridEquipmentSlot::QuickSlot2:
            return &QuickSlot2;
        case EGridEquipmentSlot::Face:
            return &Face;
        case EGridEquipmentSlot::Shirt:
            return &Shirt;
        case EGridEquipmentSlot::Bracers:
            return &Bracers;
        case EGridEquipmentSlot::Earring1:
            return &Earring1;
        case EGridEquipmentSlot::Earring2:
            return &Earring2;
        default:
            return nullptr;
        }
    }

    const FGridItemInstance* GetSlot (EGridEquipmentSlot Slot) const
    {
        return const_cast<FGridCharacterEquipmentState*> (this)->GetMutableSlot (Slot);
    }

    bool IsSlotOccupied (EGridEquipmentSlot Slot) const
    {
        const FGridItemInstance* Item = GetSlot (Slot);
        return Item && Item->IsValid ();
    }

    void ClearSlot (EGridEquipmentSlot Slot)
    {
        if (FGridItemInstance* Item = GetMutableSlot (Slot))
        {
            *Item = FGridItemInstance ();
        }
    }
};

USTRUCT (BlueprintType)
struct FGridPartyInventoryState
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Party")
    int32 SelectedCharacterIndex = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Party")
    int32 MaxActiveCharacters = 6;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    bool bInitialCharacterCreationCompleted = false;

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
