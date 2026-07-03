#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "RPG/RPGCharacterTypes.h"
#include "UI/GrimrockDesignSurfaceWidget.h"
#include "RPGCharacterCreationWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UComboBoxString;
class UEditableText;
class UGridPartyInventoryComponent;
class UImage;
class URPGCharacterPortraitSetAsset;
class URPGClassAsset;
class URPGClassVisualAsset;
class URPGRaceAsset;
class UTextBlock;
class UTexture2D;

UCLASS ()
class GRIMROCKPROTOTYPE_API URPGCharacterCreationWidget : public UGrimrockDesignSurfaceWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<URPGRaceAsset> RaceDefinition;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<URPGClassAsset> ClassDefinition;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Choices")
    TArray<TObjectPtr<URPGRaceAsset>> AvailableRaceDefinitions;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Choices")
    TArray<TObjectPtr<URPGClassAsset>> AvailableClassDefinitions;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Choices")
    TArray<TObjectPtr<URPGCharacterPortraitSetAsset>> AvailablePortraitSets;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Choices")
    TArray<TObjectPtr<URPGClassVisualAsset>> AvailableClassVisuals;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Choices")
    TArray<FRPGRaceIllustrationOption> AvailableRaceIllustrations;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Gender Buttons")
    TSoftObjectPtr<UTexture2D> GenderMaleButtonIcon;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation|Gender Buttons")
    TSoftObjectPtr<UTexture2D> GenderFemaleButtonIcon;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    ERPGCharacterPortraitGender SelectedPortraitGender = ERPGCharacterPortraitGender::Male;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    FName SelectedPortraitVariantId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Character Creation")
    TSoftObjectPtr<UTexture2D> DefaultPortrait;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "RPG|Character Creation")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

    UPROPERTY (meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UEditableText> EditableText_Name;

    UPROPERTY (meta = (BindWidget), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UButton> Button_CreateCharacter;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UImage> Image_Portrait;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UImage> Image_RaceIllustration;
    
    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UImage> Image_RaceEnvironment;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UImage> Image_ClassIcon;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UButton> Button_GenderMale;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UButton> Button_GenderFemale;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UImage> Image_GenderMale;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UImage> Image_GenderFemale;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UComboBoxString> ComboBox_Race;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UComboBoxString> ComboBox_Class;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UComboBoxString> ComboBox_Gender;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UComboBoxString> ComboBox_PortraitVariant;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_RaceValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_ClassValue;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_RaceDescription;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_ClassDescription;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_PortraitDescription;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_IdentityTitle;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_IdentityHelp;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_NameLabel;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_IdentitySummaryTitle;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_IdentityRace;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_IdentityClass;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_IdentityGender;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_PortraitVariantLabel;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_PortraitDescriptionTitle;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "RPG|Character Creation|Widgets")
    TObjectPtr<UTextBlock> Text_PortraitCaption;

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
    virtual void RefreshPreview ();

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    void FocusNameInput ();

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    virtual bool CanSubmitCharacterCreation () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    virtual bool GetPreviewAttributes (FRPGAttributes& OutAttributes) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    virtual bool GetPreviewDerivedStats (FRPGDerivedStats& OutDerivedStats) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Character Creation")
    virtual float GetPreviewCarryWeight () const;

    UFUNCTION (BlueprintCallable, Category = "RPG|Character Creation")
    virtual bool SubmitCharacterCreation ();

protected:
    virtual void NativeConstruct () override;

    void SetValidationMessage (const FText& Message, bool bIsError);

private:
    UFUNCTION ()
    void HandleCreateCharacterClicked ();

    UFUNCTION ()
    void HandleGenderMaleClicked ();

    UFUNCTION ()
    void HandleGenderFemaleClicked ();

    UFUNCTION ()
    void HandleNameChanged (const FText& NewText);

    UFUNCTION ()
    void HandleNameCommitted (const FText& NewText, ETextCommit::Type CommitMethod);

    UFUNCTION ()
    void HandleRaceSelectionChanged (FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION ()
    void HandleClassSelectionChanged (FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION ()
    void HandleGenderSelectionChanged (FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION ()
    void HandlePortraitVariantSelectionChanged (FString SelectedItem, ESelectInfo::Type SelectionType);

    void BindWidgetEvents ();
    void PopulateDefinitionOptions ();
    void PopulatePortraitOptions ();
    FRPGCharacterCreationRequest BuildCreationRequest () const;
    FText GetNormalizedNameText () const;
    FText ResolveSelectedPortraitDescription () const;
    const URPGCharacterPortraitSetAsset* FindPortraitSetForSelectedRace () const;
    const URPGClassVisualAsset* FindClassVisualForSelectedClass () const;
    const FRPGRaceIllustrationOption* FindRaceIllustrationForSelectedRaceAndGender () const;
    bool TryResolveSelectedPortraitVariant (FRPGCharacterPortraitVariant& OutVariant) const;
    void SelectPortraitVariant (const FRPGCharacterPortraitVariant& PortraitVariant);
    void SelectFirstValidPortraitForCurrentRaceAndGender ();
    void SelectPortraitGender (ERPGCharacterPortraitGender NewGender);
    TSoftObjectPtr<UTexture2D> ResolveSelectedClassIcon () const;
    TSoftObjectPtr<UTexture2D> ResolveSelectedRaceIllustration () const;
    TSoftObjectPtr<UTexture2D> ResolveSelectedRaceEnvironment () const;
    void RefreshClassIconPreview ();
    void RefreshRaceIllustrationPreview ();
    void RefreshRaceEnvironmentPreview ();
    void RefreshGenderButtonVisualState ();
};
