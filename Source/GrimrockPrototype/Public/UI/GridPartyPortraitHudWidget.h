#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridPartyPortraitTypes.h"
#include "GridPartyPortraitHudWidget.generated.h"

class UBorder;
class UImage;
class UProgressBar;
class UDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridPartyPortraitClickedSignature, FGuid, CharacterId);

/**
 * UI-PARTYHUD01 compact persistent party portrait presenter.
 * This widget renders FGridPartyPortraitView only; it owns no gameplay state.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridPartyPortraitHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Persistent|Party")
	FGridPartyPortraitView View;

	/** UI-PARTYHUD01.3 presentation event. CharacterId is the stable identity; no runtime index is broadcast. */
	UPROPERTY(BlueprintAssignable, Category = "HUD|Persistent|Party|Interaction")
	FGridPartyPortraitClickedSignature OnPortraitClicked;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UImage> Image_Portrait;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UProgressBar> ProgressBar_Health;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UProgressBar> ProgressBar_Mana;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UProgressBar> ProgressBar_PhysicalArmor;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UProgressBar> ProgressBar_MagicalArmor;

	/** Decorative selection frame driven by the authoritative selected-character projection. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UBorder> Border_Selected;

	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent|Party")
	void SetPortraitView(const FGridPartyPortraitView& InView);

	UFUNCTION(BlueprintPure, Category = "HUD|Persistent|Party")
	FGuid GetCharacterId() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Persistent|Party")
	bool IsSelected() const;

	/** Emits a selection request for the current stable CharacterId. Gameplay authority stays in the parent HUD/component. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent|Party|Interaction")
	bool RequestSelection();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;

public:
	/** Optional Blueprint presentation hook; gameplay rules must stay outside UMG. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUD|Persistent|Party")
	void RefreshPortraitVisual();

private:
	bool bLeftMousePressed = false;
	bool bDragDetected = false;

	void RefreshBoundVisuals();
};
