#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridPartyMemberWidget.generated.h"

class UBorder;
class UDragDropOperation;
class UGridInventoryWidget;
class UImage;
class URPGClassVisualAsset;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridPartyMemberClicked, int32, CharacterIndex);

UCLASS()
class GRIMROCKPROTOTYPE_API UGridPartyMemberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Party")
	int32 CharacterIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Party")
	FGridInventoryCharacterSummary CachedSummary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Party|Visuals")
	TArray<TObjectPtr<URPGClassVisualAsset>> AvailableClassVisuals;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Party")
	FOnGridPartyMemberClicked OnPartyMemberClicked;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UGridInventoryWidget> OwningInventoryWidget;

	/** Full portrait used by the party selector. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UImage> Image_Portrait;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UImage> Image_ClassIcon;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UBorder> Border_ClassAccent;

	/** Decorative overlay only: visible for the authoritative selected character. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UBorder> Border_Selected;

	/** UI-FEEDBACK01.1: presentation-only warning shown when this character is overloaded. */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UImage> Image_WeightAlert;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UTextBlock> Text_ClassLevel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UTextBlock> Text_Weight;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void InitializePartyMember(int32 InCharacterIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void SetCharacterSummary(const FGridInventoryCharacterSummary& InSummary);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void SetAvailableClassVisuals(const TArray<URPGClassVisualAsset*>& InAvailableClassVisuals);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	FString GetDisplayNameText() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	FString GetClassLevelText() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	FString GetWeightText() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	bool IsSelected() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void HandleClicked();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void SetOwnerInventoryWidget(UGridInventoryWidget* InOwnerInventoryWidget);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Inventory|Party")
	void RefreshMemberVisual();

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
	const URPGClassVisualAsset* FindClassVisualForCachedClass() const;
	void RefreshBoundMemberFields();
	void RefreshBoundMemberVisuals();
};
