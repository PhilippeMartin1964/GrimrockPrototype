#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GridDungeonBuildProgressWidget.generated.h"

class SProgressBar;
class STextBlock;
class UProgressBar;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class GRIMROCKPROTOTYPE_API UGridDungeonBuildProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGridDungeonBuildProgressWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Build Progress")
	void SetBuildTitle(const FText& InTitle);

	UFUNCTION(BlueprintCallable, Category = "Dungeon|Build Progress")
	void SetBuildProgress(float InProgress, const FText& InStatusText);

	UFUNCTION(BlueprintPure, Category = "Dungeon|Build Progress")
	float GetBuildProgress() const;

	UFUNCTION(BlueprintPure, Category = "Dungeon|Build Progress")
	FText GetBuildStatusText() const;

	virtual void SynchronizeProperties() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Build Progress")
	FText DefaultTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|Build Progress")
	FText DefaultStatusText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Dungeon|Build Progress")
	TObjectPtr<UProgressBar> ProgressBar_BuildProgress;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Dungeon|Build Progress")
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Dungeon|Build Progress")
	TObjectPtr<UTextBlock> Text_Status;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Dungeon|Build Progress")
	TObjectPtr<UTextBlock> Text_Percent;

private:
	FText BuildPercentText() const;
	void SynchronizeNativeAndBlueprintWidgets();

private:
	UPROPERTY(Transient)
	float CurrentProgress = 0.0f;

	UPROPERTY(Transient)
	FText CurrentTitle;

	UPROPERTY(Transient)
	FText CurrentStatusText;

	TSharedPtr<SProgressBar> NativeProgressBar;
	TSharedPtr<STextBlock> NativeTitleText;
	TSharedPtr<STextBlock> NativeStatusText;
	TSharedPtr<STextBlock> NativePercentText;
};
