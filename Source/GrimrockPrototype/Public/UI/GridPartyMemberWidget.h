#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridPartyMemberWidget.generated.h"

class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FOnGridPartyMemberClicked,
    int32, CharacterIndex);

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridPartyMemberWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Party")
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Party")
    FGridInventoryCharacterSummary CachedSummary;

    UPROPERTY (BlueprintAssignable, Category = "Inventory|Party")
    FOnGridPartyMemberClicked OnPartyMemberClicked;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
    TObjectPtr<UTextBlock> Text_Name;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
    TObjectPtr<UTextBlock> Text_ClassLevel;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
    TObjectPtr<UTextBlock> Text_Weight;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void InitializePartyMember (int32 InCharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void SetCharacterSummary (const FGridInventoryCharacterSummary& InSummary);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    FString GetDisplayNameText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    FString GetClassLevelText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    FString GetWeightText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool IsSelected () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void HandleClicked ();

    UFUNCTION (BlueprintCallable, BlueprintNativeEvent, Category = "Inventory|Party")
    void RefreshMemberVisual ();

private:
    void RefreshBoundMemberFields ();
};
