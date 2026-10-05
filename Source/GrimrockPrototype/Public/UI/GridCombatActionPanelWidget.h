#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridCombatHudWidget.h"
#include "GridCombatActionPanelWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWidget;

/**
 * Pure presentation widget for one party member inside the combat HUD.
 * UGridCombatHudWidget owns all runtime reads and provides the canonical
 * FGridCombatHudPartyMemberView snapshot.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridCombatActionPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Combat|UI")
	FGridCombatHudPartyMemberView View;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|UI|Appearance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DisabledOpacity = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|UI|Appearance")
	FLinearColor ReadyColor = FLinearColor(0.20f, 0.80f, 0.25f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|UI|Appearance")
	FLinearColor WaitingColor = FLinearColor(0.75f, 0.60f, 0.15f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|UI|Appearance")
	FLinearColor AlreadyActedColor = FLinearColor(0.32f, 0.32f, 0.32f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|UI|Appearance")
	FLinearColor IncapacitatedColor = FLinearColor(0.80f, 0.35f, 0.10f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|UI|Appearance")
	FLinearColor DefeatedColor = FLinearColor(0.65f, 0.08f, 0.08f, 1.0f);

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UImage> Image_Portrait;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UTextBlock> Text_Health;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UTextBlock> Text_Mana;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UTextBlock> Text_ActionPoints;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UTextBlock> Text_ActionState;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UBorder> Border_ActionState;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI")
	TObjectPtr<UWidget> Panel_DisabledOverlay;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI|Status Effects")
	TObjectPtr<UTextBlock> Text_StatusEffects;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Combat|UI|Status Effects")
	TObjectPtr<UTextBlock> Text_StatusFeedback;

	/** Applies one already-built combat HUD party-member snapshot. */
	void SetView(const FGridCombatHudPartyMemberView& InView);

private:
	FText GetActionStateText() const;
	void RefreshBoundWidgets();
};
