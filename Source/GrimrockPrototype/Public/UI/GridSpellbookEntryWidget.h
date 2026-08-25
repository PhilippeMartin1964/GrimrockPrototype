#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Magic/GridSpellbookUI.h"
#include "GridSpellbookEntryWidget.generated.h"

class UDragDropOperation;
class UTextBlock;

/**
 * Native presentation row for one resolved Spellbook entry.
 *
 * The row owns no gameplay state. It receives the immutable UI projection built
 * by UGridSpellbookWidget and mirrors it into optional bound UMG text fields.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridSpellbookEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	FGridSpellbookEntryView Entry;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UTextBlock> Text_SpellName;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UTextBlock> Text_School;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UTextBlock> Text_Cost;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UTextBlock> Text_Range;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UTextBlock> Text_Hotbar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Magic|Spellbook|UI")
	TObjectPtr<UTextBlock> Text_Description;

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook|UI")
	void InitializeSpellEntry(const FGridSpellbookEntryView& InEntry);

	UFUNCTION(BlueprintCallable, Category = "Magic|Spellbook|UI")
	void RefreshEntryVisual();

protected:
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

private:
	FText GetSpellNameText() const;
	FText GetSchoolText() const;
	FText GetCostText() const;
	FText GetRangeText() const;
	FText GetHotbarText() const;
};
