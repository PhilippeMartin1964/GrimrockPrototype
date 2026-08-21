#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Magic/GridSpellbookTypes.h"
#include "Magic/GridSpellTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridSpellbookUI.generated.h"

class UGridPartyInventoryComponent;

USTRUCT (BlueprintType)
struct FGridSpellbookEntryView
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    FName SpellId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    FText DisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    FText Description;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    EGridSpellSchool School = EGridSpellSchool::None;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    int32 ManaCost = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    int32 ActionPointCost = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    int32 MinRangeCells = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    int32 MaxRangeCells = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    EGridCombatTargetingPolicy TargetingPolicy = EGridCombatTargetingPolicy::None;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    bool bRequiresLineOfSight = false;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    bool bDefinitionResolved = false;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    bool bCanAssignToHotbar = false;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    bool bAssignedToHotbar = false;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    int32 AssignedHotbarSlotIndex = INDEX_NONE;

    /** Existing MON12 action view used by the hotbar for name/cost/targeting. */
    UPROPERTY (BlueprintReadOnly, Category = "Magic|Spellbook|UI")
    FGridCombatActionDefinition CombatActionDefinition;
};

UENUM (BlueprintType)
enum class EGridSpellHotbarAssignmentResult : uint8
{
    Success           UMETA (DisplayName = "Success"),
    InvalidCharacter  UMETA (DisplayName = "Invalid Character"),
    InvalidSlot       UMETA (DisplayName = "Invalid Slot"),
    UnknownSpell      UMETA (DisplayName = "Unknown Spell"),
    InvalidDefinition UMETA (DisplayName = "Invalid Definition"),
    HotbarRejected    UMETA (DisplayName = "Hotbar Rejected"),
    NotAssigned       UMETA (DisplayName = "Not Assigned")
};

/**
 * MON18.7a native UI bridge. It projects spell knowledge into UI-ready rows and
 * reuses the persistent MON12 ten-slot hotbar instead of creating a second bar.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridSpellbookUILibrary
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY ()

public:
    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook|UI")
    static void BuildProductionSpellbookEntries (
        const FGridCharacterSpellbookState& Spellbook,
        const TArray<FGridCombatHotbarBinding>& HotbarBindings,
        TArray<FGridSpellbookEntryView>& OutEntries);

    UFUNCTION (BlueprintPure, Category = "Magic|Spellbook|UI")
    static FGridCombatActionDefinition MakeSpellCombatActionDefinition (
        const FGridSpellDefinition& SpellDefinition);

    UFUNCTION (BlueprintPure, Category = "Magic|Spellbook|UI")
    static FGridCombatHotbarBinding MakeSpellHotbarBinding (
        FName SpellId,
        int32 SlotIndex);

    UFUNCTION (BlueprintPure, Category = "Magic|Spellbook|UI")
    static bool IsSpellHotbarBinding (
        const FGridCombatHotbarBinding& Binding,
        FName SpellId);

    UFUNCTION (BlueprintPure, Category = "Magic|Spellbook|UI")
    static int32 FindAssignedSpellSlot (
        const TArray<FGridCombatHotbarBinding>& HotbarBindings,
        FName SpellId);

    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook|UI")
    static EGridSpellHotbarAssignmentResult AssignKnownSpellToHotbar (
        UGridPartyInventoryComponent* InventoryComponent,
        int32 CharacterIndex,
        const FGridCharacterSpellbookState& Spellbook,
        FName SpellId,
        int32 TargetSlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Magic|Spellbook|UI")
    static EGridSpellHotbarAssignmentResult UnassignSpellFromHotbar (
        UGridPartyInventoryComponent* InventoryComponent,
        int32 CharacterIndex,
        const FGridCharacterSpellbookState& Spellbook,
        FName SpellId);
};
