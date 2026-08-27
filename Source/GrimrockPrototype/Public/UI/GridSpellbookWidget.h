#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Magic/GridSpellbookUI.h"
#include "GridSpellbookWidget.generated.h"

class AGrimrockPartyPawn;
class UGridPartyInventoryComponent;
class UGridPartySpellbookComponent;
class UGridSpellbookEntryWidget;
class UPanelWidget;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGridSpellbookWidgetRefreshedSignature);

/**
 * Native presentation bridge for the in-game Spellbook page.
 *
 * The widget owns no gameplay copy of spell knowledge or hotbar state. It reads
 * the selected character from the existing party inventory, reads spell
 * knowledge from UGridPartySpellbookComponent, and projects both through the
 * MON18.7a UGridSpellbookUILibrary.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridSpellbookWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UGridPartySpellbookComponent> SpellbookComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	int32 SelectedCharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	FGuid SelectedCharacterId;

	UPROPERTY(BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TArray<FGridSpellbookEntryView> SpellEntries;

	/** Blueprint row class used to render one spell entry in Panel_SpellEntries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Magic|Spellbook|UI|Presentation")
	TSubclassOf<UGridSpellbookEntryWidget> SpellEntryWidgetClass;

	/** Optional UMG container populated automatically from SpellEntries. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI|Presentation")
	TObjectPtr<UPanelWidget> Panel_SpellEntries;

	/** Optional empty-state label shown only when the selected character knows no spells. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI|Presentation")
	TObjectPtr<UTextBlock> Text_EmptySpellbook;

	/** Presentation notification. Blueprint may add presentation-only reactions here. */
	UPROPERTY(BlueprintAssignable, Category = "Magic|Spellbook|UI|Events")
	FGridSpellbookWidgetRefreshedSignature OnSpellbookRefreshed;

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook|UI")
	void InitializeSpellbookWidget(AGrimrockPartyPawn* InPartyPawn);

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook|UI")
	void RefreshSpellbook();

	/** Rebuilds the optional UMG list from the current SpellEntries projection. */
	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook|UI|Presentation")
	void RebuildSpellEntryWidgets();

	UFUNCTION(BlueprintPure, Category = "Magic|Spellbook|UI")
	int32 GetSpellEntryCount() const;

	UFUNCTION(BlueprintPure, Category = "Magic|Spellbook|UI")
	bool GetSpellEntry(int32 EntryIndex, FGridSpellbookEntryView& OutEntry) const;

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook|UI|Hotbar")
	EGridSpellHotbarAssignmentResult AssignSpellToHotbar(FName SpellId, int32 TargetSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook|UI|Hotbar")
	EGridSpellHotbarAssignmentResult UnassignSpellFromHotbar(FName SpellId);

protected:
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandlePartyInventoryChanged(int32 CharacterIndex);

	UFUNCTION()
	void HandleSpellbookChanged();

	UGridPartySpellbookComponent* ResolveOrCreateSpellbookComponent(AGrimrockPartyPawn* PartyPawn) const;

	bool GetSelectedCharacterSpellbook(FGridCharacterSpellbookState& OutState) const;

	void ClearViewState();
	void NotifySpellbookRefreshed();

	bool bRefreshInProgress = false;
};
