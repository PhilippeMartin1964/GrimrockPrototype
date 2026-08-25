#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Magic/GridSpellbookUI.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridCombatHotbarDragDropOperation.generated.h"

/** Payload used to move a shortcut or assign an action/spell palette entry. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridCombatHotbarDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
	int32 SourceSlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
	FGridCombatHotbarBinding Binding;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
	bool bFromActionPalette = false;

	UPROPERTY(BlueprintReadOnly, Category = "Combat|Hotbar|Drag")
	bool bFromSpellbook = false;

	UFUNCTION(BlueprintCallable, Category = "Combat|Hotbar|Drag")
	void InitializeFromHotbarSlot(int32 InCharacterIndex, int32 InSourceSlotIndex, const FGridCombatHotbarBinding& InBinding);

	UFUNCTION(BlueprintCallable, Category = "Combat|Hotbar|Drag")
	void InitializeFromActionPalette(int32 InCharacterIndex, const FGridAvailableCombatAction& InAction);

	UFUNCTION(BlueprintCallable, Category = "Combat|Hotbar|Drag")
	void InitializeFromSpellbookEntry(int32 InCharacterIndex, const FGridSpellbookEntryView& InEntry);

	/** Blueprint/WBP drop hook for MON18.7a; no item is moved or consumed. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Hotbar|Drag")
	EGridSpellHotbarAssignmentResult CommitSpellbookDrop(
		UGridPartyInventoryComponent* InventoryComponent, const FGridCharacterSpellbookState& Spellbook, int32 TargetSlotIndex) const;
};
