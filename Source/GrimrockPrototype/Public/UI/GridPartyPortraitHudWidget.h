#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridPartyPortraitTypes.h"
#include "GridPartyPortraitHudWidget.generated.h"

class UBorder;
class UImage;
class UProgressBar;

/**
 * UI-PARTYHUD01.2 compact persistent party portrait presenter.
 * This widget renders FGridPartyPortraitView only; it owns no gameplay state.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridPartyPortraitHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Persistent|Party")
	FGridPartyPortraitView View;

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

	/** Decorative selection frame only. UI-PARTYHUD01.3 will add click selection. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD|Persistent|Party")
	TObjectPtr<UBorder> Border_Selected;

	UFUNCTION(BlueprintCallable, Category = "HUD|Persistent|Party")
	void SetPortraitView(const FGridPartyPortraitView& InView);

	UFUNCTION(BlueprintPure, Category = "HUD|Persistent|Party")
	FGuid GetCharacterId() const;

	UFUNCTION(BlueprintPure, Category = "HUD|Persistent|Party")
	bool IsSelected() const;

	/** Optional Blueprint presentation hook; gameplay rules must stay outside UMG. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "HUD|Persistent|Party")
	void RefreshPortraitVisual();

private:
	void RefreshBoundVisuals();
};
