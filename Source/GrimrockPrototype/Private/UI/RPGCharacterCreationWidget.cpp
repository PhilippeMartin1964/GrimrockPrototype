#include "UI/RPGCharacterCreationWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
    FText GetDefinitionDisplayName (const FText& DisplayName, FName DefinitionId)
    {
        return DisplayName.IsEmpty () ? FText::FromName (DefinitionId) : DisplayName;
    }

    void SetOptionalText (UTextBlock* TextBlock, const FText& Value)
    {
        if (TextBlock)
        {
            TextBlock->SetText (Value);
        }
    }
}

void URPGCharacterCreationWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    BindWidgetEvents ();
    RefreshPreview ();
}

void URPGCharacterCreationWidget::BindWidgetEvents ()
{
    if (Button_CreateCharacter)
    {
        Button_CreateCharacter->OnClicked.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleCreateCharacterClicked);
        Button_CreateCharacter->OnClicked.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleCreateCharacterClicked);
    }

    if (EditableTextBox_Name)
    {
        EditableTextBox_Name->OnTextChanged.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameChanged);
        EditableTextBox_Name->OnTextChanged.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameChanged);
        EditableTextBox_Name->OnTextCommitted.RemoveDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameCommitted);
        EditableTextBox_Name->OnTextCommitted.AddDynamic (
            this,
            &URPGCharacterCreationWidget::HandleNameCommitted);
    }
}

void URPGCharacterCreationWidget::InitializeCharacterCreationWidget (AGrimrockPartyPawn* InPartyPawn)
{
    OwningPartyPawn = InPartyPawn;
    InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent.Get () : nullptr;
    SetValidationMessage (FText::GetEmpty (), false);
    RefreshPreview ();

}

void URPGCharacterCreationWidget::RefreshPreview ()
{
    const bool bHasRaceDefinition = RaceDefinition && RaceDefinition->IsValidDefinition ();
    const bool bHasClassDefinition = ClassDefinition && ClassDefinition->IsValidDefinition ();
    const FText UnavailableValue = FText::FromString (TEXT ("-"));

    SetOptionalText (
        Text_RaceValue,
        bHasRaceDefinition
            ? GetDefinitionDisplayName (RaceDefinition->DisplayName, RaceDefinition->RaceId)
            : FText::FromString (TEXT ("Race non configurée")));
    SetOptionalText (
        Text_ClassValue,
        bHasClassDefinition
            ? GetDefinitionDisplayName (ClassDefinition->DisplayName, ClassDefinition->ClassId)
            : FText::FromString (TEXT ("Classe non configurée")));

    FRPGAttributes Attributes;
    const bool bHasAttributes = GetPreviewAttributes (Attributes);
    SetOptionalText (Text_StrengthValue, bHasAttributes ? FText::AsNumber (Attributes.Strength) : UnavailableValue);
    SetOptionalText (Text_DexterityValue, bHasAttributes ? FText::AsNumber (Attributes.Dexterity) : UnavailableValue);
    SetOptionalText (Text_ConstitutionValue, bHasAttributes ? FText::AsNumber (Attributes.Constitution) : UnavailableValue);
    SetOptionalText (Text_IntelligenceValue, bHasAttributes ? FText::AsNumber (Attributes.Intelligence) : UnavailableValue);
    SetOptionalText (Text_WisdomValue, bHasAttributes ? FText::AsNumber (Attributes.Wisdom) : UnavailableValue);
    SetOptionalText (Text_CharismaValue, bHasAttributes ? FText::AsNumber (Attributes.Charisma) : UnavailableValue);

    FRPGDerivedStats DerivedStats;
    const bool bHasDerivedStats = GetPreviewDerivedStats (DerivedStats);
    SetOptionalText (Text_HealthValue, bHasDerivedStats ? FText::AsNumber (DerivedStats.MaxHealth) : UnavailableValue);
    SetOptionalText (Text_ManaValue, bHasDerivedStats ? FText::AsNumber (DerivedStats.MaxMana) : UnavailableValue);
    SetOptionalText (
        Text_CarryWeightValue,
        bHasAttributes
            ? FText::AsNumber (FMath::RoundToInt (GetPreviewCarryWeight ()))
            : UnavailableValue);

    if (Image_Portrait)
    {
        if (DefaultPortrait.IsNull ())
        {
            Image_Portrait->SetVisibility (ESlateVisibility::Collapsed);
        }
        else
        {
            Image_Portrait->SetBrushFromSoftTexture (DefaultPortrait, false);
            Image_Portrait->SetVisibility (ESlateVisibility::HitTestInvisible);
        }
    }

    if (!bHasRaceDefinition || !bHasClassDefinition)
    {
        SetValidationMessage (
            FText::FromString (
                TEXT ("Assignez DA_Race_Human et DA_Class_Warrior dans Class Defaults du widget.")),
            true);
    }
    else if (!InventoryComponent)
    {
        SetValidationMessage (FText::FromString (TEXT ("Composant d'inventaire indisponible.")), true);
    }

    const bool bCanSubmit = CanSubmitCharacterCreation ();
    if (Button_CreateCharacter)
    {
        Button_CreateCharacter->SetIsEnabled (bCanSubmit);
    }

    const FString NormalizedName = GetNormalizedNameText ().ToString ();
    UE_LOG (
        LogTemp,
        Log,
        TEXT ("CharacterCreation SubmitState CanSubmit=%s Button=%s ButtonEnabled=%s NameLength=%d Inventory=%s Completed=%s Race=%s Class=%s Attributes=%s"),
        bCanSubmit ? TEXT ("true") : TEXT ("false"),
        *GetNameSafe (Button_CreateCharacter),
        Button_CreateCharacter && Button_CreateCharacter->GetIsEnabled () ? TEXT ("true") : TEXT ("false"),
        NormalizedName.Len (),
        InventoryComponent ? TEXT ("true") : TEXT ("false"),
        InventoryComponent && InventoryComponent->HasCompletedInitialCharacterCreation () ? TEXT ("true") : TEXT ("false"),
        bHasRaceDefinition ? TEXT ("true") : TEXT ("false"),
        bHasClassDefinition ? TEXT ("true") : TEXT ("false"),
        bHasAttributes ? TEXT ("true") : TEXT ("false"));
}

void URPGCharacterCreationWidget::FocusNameInput ()
{
    if (EditableTextBox_Name)
    {
        EditableTextBox_Name->SetKeyboardFocus ();
    }
}

bool URPGCharacterCreationWidget::CanSubmitCharacterCreation () const
{
    if (!InventoryComponent ||
        InventoryComponent->HasCompletedInitialCharacterCreation () ||
        !RaceDefinition ||
        !RaceDefinition->IsValidDefinition () ||
        !ClassDefinition ||
        !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }

    const FString NormalizedName = GetNormalizedNameText ().ToString ();
    FRPGAttributes Attributes;
    return NormalizedName.Len () >= 1 &&
        NormalizedName.Len () <= 24 &&
        GetPreviewAttributes (Attributes) &&
        URPGCharacterRulesLibrary::AreAttributesInRange (Attributes);
}

bool URPGCharacterCreationWidget::GetPreviewAttributes (FRPGAttributes& OutAttributes) const
{
    OutAttributes = FRPGAttributes ();
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition () ||
        !ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }

    OutAttributes = URPGCharacterRulesLibrary::AddAttributes (
        ClassDefinition->BaseAttributes,
        RaceDefinition->AttributeBonuses);
    return true;
}

bool URPGCharacterCreationWidget::GetPreviewDerivedStats (FRPGDerivedStats& OutDerivedStats) const
{
    OutDerivedStats = FRPGDerivedStats ();
    FRPGAttributes Attributes;
    if (!GetPreviewAttributes (Attributes))
    {
        return false;
    }

    OutDerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats (
        Attributes,
        ClassDefinition,
        1);
    return true;
}

float URPGCharacterCreationWidget::GetPreviewCarryWeight () const
{
    FRPGAttributes Attributes;
    return GetPreviewAttributes (Attributes)
        ? URPGCharacterRulesLibrary::CalculateMaxCarryWeight (Attributes)
        : 0.0f;
}

bool URPGCharacterCreationWidget::SubmitCharacterCreation ()
{
    if (!InventoryComponent)
    {
        SetValidationMessage (FText::FromString (TEXT ("Composant d'inventaire indisponible.")), true);
        return false;
    }

    FText Error;
    if (!InventoryComponent->CreateInitialCharacter (BuildCreationRequest (), Error))
    {
        SetValidationMessage (
            Error.IsEmpty () ? FText::FromString (TEXT ("Création du personnage impossible.")) : Error,
            true);
        RefreshPreview ();
        return false;
    }

    SetValidationMessage (FText::GetEmpty (), false);
    if (OwningPartyPawn)
    {
        OwningPartyPawn->HandleInitialCharacterCreated ();
    }
    return true;
}

void URPGCharacterCreationWidget::HandleCreateCharacterClicked ()
{
    SubmitCharacterCreation ();
}

void URPGCharacterCreationWidget::HandleNameChanged (const FText& NewText)
{
    (void)NewText;
    SetValidationMessage (FText::GetEmpty (), false);
    RefreshPreview ();
}

void URPGCharacterCreationWidget::HandleNameCommitted (
    const FText& NewText,
    ETextCommit::Type CommitMethod)
{
    (void)NewText;
    RefreshPreview ();

    if (CommitMethod == ETextCommit::OnEnter && CanSubmitCharacterCreation ())
    {
        SubmitCharacterCreation ();
    }
}

FRPGCharacterCreationRequest URPGCharacterCreationWidget::BuildCreationRequest () const
{
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = GetNormalizedNameText ();
    Request.RaceDefinition = RaceDefinition;
    Request.ClassDefinition = ClassDefinition;
    Request.Portrait = DefaultPortrait;
    return Request;
}

FText URPGCharacterCreationWidget::GetNormalizedNameText () const
{
    FString Name = EditableTextBox_Name ? EditableTextBox_Name->GetText ().ToString () : FString ();
    Name.TrimStartAndEndInline ();
    return FText::FromString (Name);
}

void URPGCharacterCreationWidget::SetValidationMessage (const FText& Message, bool bIsError)
{
    if (!Text_ValidationMessage)
    {
        return;
    }

    Text_ValidationMessage->SetText (Message);
    Text_ValidationMessage->SetColorAndOpacity (
        bIsError ? FSlateColor (FLinearColor (0.85f, 0.12f, 0.08f)) : FSlateColor (FLinearColor::White));
    Text_ValidationMessage->SetVisibility (
        Message.IsEmpty () ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
