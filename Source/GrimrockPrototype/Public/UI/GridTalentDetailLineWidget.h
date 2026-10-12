#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/GridSkillsUiTypes.h"
#include "GridTalentDetailLineWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/** Presentation-only row for one semantic Talent detail line: glyph + label + value. */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridTalentDetailLineWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	FGridTalentDetailLineView LineView;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	FText ResolvedLabelText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	FText ResolvedValueText;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	int32 ResolvedNumericValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	EGridTalentDetailValueStyle ResolvedValueStyle = EGridTalentDetailValueStyle::Text;

	/**
	 * Kept false so existing Blueprint bindings cannot duplicate the sign now embedded in ResolvedValueText.
	 * Remove the corresponding optional UMG fragments when the Blueprint is next cleaned up.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	bool bShowPositiveSign = false;

	/** Same migration guard for the '%' fragment; ResolvedValueText already contains the full numeric token. */
	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	bool bShowPercentUnit = false;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	TObjectPtr<UTexture2D> ResolvedIcon;

	UPROPERTY(BlueprintReadOnly, Category = "RPG|Talents|Detail|Line")
	bool bInitialized = false;

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Detail|Line")
	bool InitializeLine(
		const FGridTalentDetailLineView& InLine,
		UTexture2D* InIcon,
		const FText& InResolvedLabelText,
		const FText& InResolvedValueText);

	UFUNCTION(BlueprintCallable, Category = "RPG|Talents|Detail|Line")
	void ClearLine();

protected:
	virtual void NativeConstruct() override;

private:
	void ApplyPresentation();

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Image_LineIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_LineLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_LineValue;
};
