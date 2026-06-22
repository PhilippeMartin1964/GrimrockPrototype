#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPGCharacterCreationWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UEditableTextBox;
class UGridPartyInventoryComponent;
class UImage;
class URPGClassAsset;
class URPGRaceAsset;
class UTextBlock;
class UTexture2D;

UCLASS ()
class GRIMROCKPROTOTYPE_API URPGCharacterCreationWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<URPGRaceAsset> RaceDefinition;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<URPGClassAsset> ClassDefinition;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    TSoftObjectPtr<UTexture2D> DefaultPortrait;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

    UPROPERTY (meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UEditableTextBox> EditableTextBox_Name;

    UPROPERTY (meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UButton> Button_CreateCharacter;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UImage> Image_Portrait;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_RaceValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_ClassValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_StrengthValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_DexterityValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_ConstitutionValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_IntelligenceValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_WisdomValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_CharismaValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_HealthValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_ManaValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_CarryWeightValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_ValidationMessage;

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    void InitializeCharacterCreationWidget (AGrimrockPartyPawn* InPartyPawn);

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    void RefreshPreview ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    void FocusNameInput ();

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    bool CanSubmitCharacterCreation () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    bool GetPreviewAttributes (FRPGAttributes& OutAttributes) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    bool GetPreviewDerivedStats (FRPGDerivedStats& OutDerivedStats) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    float GetPreviewCarryWeight () const;

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    bool SubmitCharacterCreation ();

protected:
    virtual void NativeConstruct () override;

private:
    UFUNCTION ()
    void HandleCreateCharacterClicked ();

    UFUNCTION ()
    void HandleNameChanged (const FText& NewText);

    UFUNCTION ()
    void HandleNameCommitted (const FText& NewText, ETextCommit::Type CommitMethod);

    void BindWidgetEvents ();
    FRPGCharacterCreationRequest BuildCreationRequest () const;
    FText GetNormalizedNameText () const;
    void SetValidationMessage (const FText& Message, bool bIsError);
};
