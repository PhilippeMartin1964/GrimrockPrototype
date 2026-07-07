#include "UI/RPGCharacterCreationWizardWidget.h"

#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/World.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockGameInstance.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/RPGCharacterCreationAttributesStepWidget.h"

namespace
{
    constexpr int32 CharacterCreationWizardStepCount = 5;
    const FName AttributeStrength (TEXT ("Strength"));
    const FName AttributeDexterity (TEXT ("Dexterity"));
    const FName AttributeConstitution (TEXT ("Constitution"));
    const FName AttributeIntelligence (TEXT ("Intelligence"));
    const FName AttributeWisdom (TEXT ("Wisdom"));
    const FName AttributeCharisma (TEXT ("Charisma"));

    int32 GetWizardStepIndex (ERPGCharacterCreationWizardStep Step)
    {
        switch (Step)
        {
        case ERPGCharacterCreationWizardStep::Race: return 0;
        case ERPGCharacterCreationWizardStep::Class: return 1;
        case ERPGCharacterCreationWizardStep::Attributes: return 2;
        case ERPGCharacterCreationWizardStep::Identity: return 3;
        case ERPGCharacterCreationWizardStep::Summary: return 4;
        default: return 0;
        }
    }

    ERPGCharacterCreationWizardStep GetWizardStepFromIndex (int32 StepIndex)
    {
        switch (FMath::Clamp (StepIndex, 0, CharacterCreationWizardStepCount - 1))
        {
        case 0: return ERPGCharacterCreationWizardStep::Race;
        case 1: return ERPGCharacterCreationWizardStep::Class;
        case 2: return ERPGCharacterCreationWizardStep::Attributes;
        case 3: return ERPGCharacterCreationWizardStep::Identity;
        case 4: return ERPGCharacterCreationWizardStep::Summary;
        default: return ERPGCharacterCreationWizardStep::Race;
        }
    }

    FText GetWizardStepTitleText (ERPGCharacterCreationWizardStep Step)
    {
        switch (Step)
        {
        case ERPGCharacterCreationWizardStep::Race: return FText::FromString (TEXT ("Race"));
        case ERPGCharacterCreationWizardStep::Class: return FText::FromString (TEXT ("Classe"));
        case ERPGCharacterCreationWizardStep::Attributes: return FText::FromString (TEXT ("Caractéristiques"));
        case ERPGCharacterCreationWizardStep::Identity: return FText::FromString (TEXT ("Identité"));
        case ERPGCharacterCreationWizardStep::Summary: return FText::FromString (TEXT ("Résumé"));
        default: return FText::GetEmpty ();
        }
    }

    ESlateVisibility GetVisibleWhen (bool bShouldBeVisible)
    {
        return bShouldBeVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    }

    int32 GetSafeAllocationMinimum (int32 InMinimum, int32 InMaximum)
    {
        return FMath::Max (1, FMath::Min (InMinimum, InMaximum));
    }

    int32 GetSafeAllocationMaximum (int32 InMinimum, int32 InMaximum)
    {
        return FMath::Max (GetSafeAllocationMinimum (InMinimum, InMaximum), InMaximum);
    }

    int32 GetAttributeValue (const FRPGAttributes& Attributes, FName AttributeId)
    {
        if (AttributeId == AttributeStrength) return Attributes.Strength;
        if (AttributeId == AttributeDexterity) return Attributes.Dexterity;
        if (AttributeId == AttributeConstitution) return Attributes.Constitution;
        if (AttributeId == AttributeIntelligence) return Attributes.Intelligence;
        if (AttributeId == AttributeWisdom) return Attributes.Wisdom;
        if (AttributeId == AttributeCharisma) return Attributes.Charisma;
        return 0;
    }

    void SetAttributeValue (FRPGAttributes& Attributes, FName AttributeId, int32 Value)
    {
        if (AttributeId == AttributeStrength) Attributes.Strength = Value;
        else if (AttributeId == AttributeDexterity) Attributes.Dexterity = Value;
        else if (AttributeId == AttributeConstitution) Attributes.Constitution = Value;
        else if (AttributeId == AttributeIntelligence) Attributes.Intelligence = Value;
        else if (AttributeId == AttributeWisdom) Attributes.Wisdom = Value;
        else if (AttributeId == AttributeCharisma) Attributes.Charisma = Value;
    }

    FRPGAttributes ClampAllocatedAttributes (const FRPGAttributes& Attributes, int32 InMinimum, int32 InMaximum)
    {
        const int32 Minimum = GetSafeAllocationMinimum (InMinimum, InMaximum);
        const int32 Maximum = GetSafeAllocationMaximum (InMinimum, InMaximum);
        FRPGAttributes Result = Attributes;
        Result.Strength = FMath::Clamp (Result.Strength, Minimum, Maximum);
        Result.Dexterity = FMath::Clamp (Result.Dexterity, Minimum, Maximum);
        Result.Constitution = FMath::Clamp (Result.Constitution, Minimum, Maximum);
        Result.Intelligence = FMath::Clamp (Result.Intelligence, Minimum, Maximum);
        Result.Wisdom = FMath::Clamp (Result.Wisdom, Minimum, Maximum);
        Result.Charisma = FMath::Clamp (Result.Charisma, Minimum, Maximum);
        return Result;
    }

    int32 SumAttributePointsAboveMinimum (const FRPGAttributes& Attributes, int32 Minimum)
    {
        return FMath::Max (0, Attributes.Strength - Minimum) + FMath::Max (0, Attributes.Dexterity - Minimum) + FMath::Max (0, Attributes.Constitution - Minimum) +
            FMath::Max (0, Attributes.Intelligence - Minimum) + FMath::Max (0, Attributes.Wisdom - Minimum) + FMath::Max (0, Attributes.Charisma - Minimum);
    }

    FString NormalizeCharacterName (UEditableText* EditableText)
    {
        FString Name = EditableText ? EditableText->GetText ().ToString () : FString ();
        Name.TrimStartAndEndInline ();
        return Name;
    }

    FString GetDisplayNameString (const FText& DisplayName, FName FallbackId)
    {
        return DisplayName.IsEmpty () ? (FallbackId.IsNone () ? FString (TEXT ("-")) : FallbackId.ToString ()) : DisplayName.ToString ();
    }

    FString GetGenderDisplayNameString (ERPGCharacterPortraitGender Gender)
    {
        return Gender == ERPGCharacterPortraitGender::Female ? FString (TEXT ("Féminin")) : FString (TEXT ("Masculin"));
    }

    FString FormatAttributeSummaryLine (const TCHAR* Label, FName AttributeId, const FRPGAttributes& ClassAttributes, const FRPGAttributes& RaceBonuses, const FRPGAttributes& FinalAttributes)
    {
        const int32 ClassValue = GetAttributeValue (ClassAttributes, AttributeId);
        const int32 RaceValue = GetAttributeValue (RaceBonuses, AttributeId);
        const int32 FinalValue = GetAttributeValue (FinalAttributes, AttributeId);
        const int32 Modifier = URPGCharacterRulesLibrary::GetAttributeModifier (FinalValue);
        return FString::Printf (TEXT ("%s : %d + %+d = %d  mod %+d"), Label, ClassValue, RaceValue, FinalValue, Modifier);
    }

    void SetWizardOptionalText (UTextBlock* TextBlock, const FString& Value)
    {
        if (TextBlock)
        {
            TextBlock->SetText (FText::FromString (Value));
        }
    }

    void SetWizardOptionalImageBrush (UImage* Image, const TSoftObjectPtr<UTexture2D>& TextureRef, bool bCollapseWhenMissing)
    {
        if (!Image)
        {
            return;
        }

        if (TextureRef.IsNull ())
        {
            if (bCollapseWhenMissing)
            {
                Image->SetVisibility (ESlateVisibility::Collapsed);
            }
            return;
        }

        UTexture2D* Texture = TextureRef.LoadSynchronous ();
        if (Texture)
        {
            Image->SetBrushFromTexture (Texture, true);
            Image->SetVisibility (ESlateVisibility::Visible);
        }
        else if (bCollapseWhenMissing)
        {
            Image->SetVisibility (ESlateVisibility::Collapsed);
        }
    }

    FString BuildWizardValidationMessage (const URPGCharacterCreationWizardWidget* Widget, bool& bOutIsError)
    {
        bOutIsError = false;

        if (!Widget)
        {
            return FString ();
        }

        if (!Widget->InventoryComponent)
        {
            bOutIsError = true;
            return FString (TEXT ("Composant d'inventaire indisponible."));
        }

        if (!Widget->RaceDefinition || !Widget->RaceDefinition->IsValidDefinition ())
        {
            bOutIsError = true;
            return FString (TEXT ("Race non configurée : renseignez une race valide dans le widget."));
        }

        if (!Widget->ClassDefinition || !Widget->ClassDefinition->IsValidDefinition ())
        {
            bOutIsError = true;
            return FString (TEXT ("Classe non configurée : renseignez une classe valide dans le widget."));
        }

        const int32 RemainingAttributePoints = Widget->GetRemainingAttributePoints ();
        const FString NormalizedName = NormalizeCharacterName (Widget->EditableText_Name);

        if (RemainingAttributePoints > 0 &&
            (Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Attributes ||
                Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Identity ||
                Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Summary))
        {
            return FString::Printf (TEXT ("Il reste %d point(s) de caractéristiques à répartir."), RemainingAttributePoints);
        }

        if (NormalizedName.Len () < 1 &&
            (Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Identity ||
                Widget->CurrentWizardStep == ERPGCharacterCreationWizardStep::Summary))
        {
            return FString (TEXT ("Saisissez un nom pour pouvoir créer le personnage."));
        }

        if (NormalizedName.Len () > 24)
        {
            bOutIsError = true;
            return FString (TEXT ("Le nom du personnage dépasse 24 caractères."));
        }

        switch (Widget->CurrentWizardStep)
        {
        case ERPGCharacterCreationWizardStep::Race:
            return FString (TEXT ("Choisissez la race du personnage, puis passez à la classe."));
        case ERPGCharacterCreationWizardStep::Class:
            return FString (TEXT ("Choisissez la classe du personnage, puis passez aux caractéristiques."));
        case ERPGCharacterCreationWizardStep::Attributes:
            return FString (TEXT ("Caractéristiques validées. Vous pouvez passer à l'identité."));
        case ERPGCharacterCreationWizardStep::Identity:
            return FString (TEXT ("Identité validée. Vous pouvez consulter le résumé."));
        case ERPGCharacterCreationWizardStep::Summary:
            if (Widget->CanSubmitCharacterCreation ())
            {
                return FString (TEXT ("Tous les choix sont valides. Vous pouvez créer le personnage."));
            }

            bOutIsError = true;
            return FString (TEXT ("Création impossible : vérifiez les choix précédents."));
        default:
            return FString ();
        }
    }

    void RefreshWizardValidationMessage (URPGCharacterCreationWizardWidget* Widget)
    {
        if (!Widget || !Widget->Text_ValidationMessage)
        {
            return;
        }

        bool bIsError = false;
        const FString Message = BuildWizardValidationMessage (Widget, bIsError);
        Widget->Text_ValidationMessage->SetText (FText::FromString (Message));
        Widget->Text_ValidationMessage->SetColorAndOpacity (
            bIsError ? FSlateColor (FLinearColor (0.85f, 0.12f, 0.08f)) : FSlateColor (FLinearColor (0.80f, 0.72f, 0.55f)));
        Widget->Text_ValidationMessage->SetVisibility (
            Message.IsEmpty () ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
}

void URPGCharacterCreationWizardWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    CurrentWizardStep = InitialWizardStep;
    BindWizardButtons ();
    BindWizardSubmitButton ();
    if (Widget_StepAttributes)
    {
        Widget_StepAttributes->InitializeAttributesStep (this);
    }
    else
    {
        UE_LOG (LogTemp, Error, TEXT ("CharacterCreationWizard MissingRequiredWidget Widget=%s Expected=Widget_StepAttributes"), *GetName ());
    }
    ApplyWizardStepToSwitcher ();
    RefreshWizardShell ();
    RefreshPreview ();
}

void URPGCharacterCreationWizardWidget::BindWizardButtons ()
{
    if (Button_Previous)
    {
        Button_Previous->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandlePreviousClicked);
        Button_Previous->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandlePreviousClicked);
    }
    if (Button_Next)
    {
        Button_Next->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleNextClicked);
        Button_Next->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleNextClicked);
    }
    if (Button_Cancel)
    {
        Button_Cancel->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleCancelClicked);
        Button_Cancel->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleCancelClicked);
    }
}

void URPGCharacterCreationWizardWidget::BindWizardSubmitButton ()
{
    if (!Button_CreateCharacter)
    {
        return;
    }
    Button_CreateCharacter->OnClicked.RemoveAll (this);
    Button_CreateCharacter->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleWizardCreateCharacterClicked);
}

void URPGCharacterCreationWizardWidget::SetCurrentWizardStep (ERPGCharacterCreationWizardStep NewStep)
{
    const ERPGCharacterCreationWizardStep PreviousStep = CurrentWizardStep;
    CurrentWizardStep = NewStep;
    ApplyWizardStepToSwitcher ();
    RefreshWizardShell ();
    if (PreviousStep != CurrentWizardStep)
    {
        OnWizardStepChanged (PreviousStep, CurrentWizardStep);
    }
    if (bFocusNameInputOnIdentityStep && CurrentWizardStep == ERPGCharacterCreationWizardStep::Identity)
    {
        FocusNameInput ();
    }
}

bool URPGCharacterCreationWizardWidget::GoToNextWizardStep ()
{
    if (!CanGoToNextWizardStep ()) return false;
    SetCurrentWizardStep (GetWizardStepFromIndex (GetCurrentWizardStepIndex () + 1));
    return true;
}

bool URPGCharacterCreationWizardWidget::GoToPreviousWizardStep ()
{
    if (!CanGoToPreviousWizardStep ()) return false;
    SetCurrentWizardStep (GetWizardStepFromIndex (GetCurrentWizardStepIndex () - 1));
    return true;
}

void URPGCharacterCreationWizardWidget::CancelWizard ()
{
    UE_LOG (LogTemp, Log, TEXT ("CharacterCreationWizard Cancelled Widget=%s"), *GetName ());
    RemoveFromParent ();

    UGrimrockGameInstance* GrimrockGameInstance = GetWorld()
        ? GetWorld()->GetGameInstance<UGrimrockGameInstance>()
        : nullptr;

    if (GrimrockGameInstance)
    {
        GrimrockGameInstance->RequestReturnToMainMenu(this);
        return;
    }

    UE_LOG (LogTemp, Error, TEXT ("CharacterCreationWizard Cancel Failed Widget=%s Reason=NoGrimrockGameInstance"), *GetName ());
}

bool URPGCharacterCreationWizardWidget::CanGoToNextWizardStep () const { return GetCurrentWizardStepIndex () < CharacterCreationWizardStepCount - 1; }
bool URPGCharacterCreationWizardWidget::CanGoToPreviousWizardStep () const { return GetCurrentWizardStepIndex () > 0; }
bool URPGCharacterCreationWizardWidget::IsWizardOnLastStep () const { return GetCurrentWizardStepIndex () == CharacterCreationWizardStepCount - 1; }
int32 URPGCharacterCreationWizardWidget::GetCurrentWizardStepIndex () const { return GetWizardStepIndex (CurrentWizardStep); }
int32 URPGCharacterCreationWizardWidget::GetCurrentWizardStepNumber () const { return GetCurrentWizardStepIndex () + 1; }
int32 URPGCharacterCreationWizardWidget::GetWizardStepCount () const { return CharacterCreationWizardStepCount; }
FText URPGCharacterCreationWizardWidget::GetCurrentWizardStepTitle () const { return GetWizardStepTitleText (CurrentWizardStep); }

void URPGCharacterCreationWizardWidget::RefreshPreview ()
{
    EnsureAttributeAllocationInitialized ();
    Super::RefreshPreview ();
    if (Widget_StepAttributes)
    {
        Widget_StepAttributes->RefreshFromWizardState ();
    }
    RefreshWizardShell ();
}

bool URPGCharacterCreationWizardWidget::CanSubmitCharacterCreation () const
{
    return Super::CanSubmitCharacterCreation () && GetRemainingAttributePoints () == 0;
}

bool URPGCharacterCreationWizardWidget::GetPreviewAttributes (FRPGAttributes& OutAttributes) const
{
    OutAttributes = FRPGAttributes ();
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition () || !ClassDefinition || !ClassDefinition->IsValidDefinition ()) return false;
    OutAttributes = URPGCharacterRulesLibrary::AddAttributes (GetAllocatedClassAttributesForPreview (), RaceDefinition->AttributeBonuses);
    return true;
}

bool URPGCharacterCreationWizardWidget::GetPreviewDerivedStats (FRPGDerivedStats& OutDerivedStats) const
{
    OutDerivedStats = FRPGDerivedStats ();
    FRPGAttributes Attributes;
    if (!GetPreviewAttributes (Attributes)) return false;
    OutDerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats (Attributes, ClassDefinition, 1);
    return true;
}

float URPGCharacterCreationWizardWidget::GetPreviewCarryWeight () const
{
    FRPGAttributes Attributes;
    return GetPreviewAttributes (Attributes) ? URPGCharacterRulesLibrary::CalculateMaxCarryWeight (Attributes) : 0.0f;
}

void URPGCharacterCreationWizardWidget::RefreshWizardShell ()
{
    if (Text_StepTitle) Text_StepTitle->SetText (GetCurrentWizardStepTitle ());
    if (Text_StepCounter)
    {
        Text_StepCounter->SetText (FText::Format (FText::FromString (TEXT ("{0} / {1}")), FText::AsNumber (GetCurrentWizardStepNumber ()), FText::AsNumber (GetWizardStepCount ())));
    }
    if (Button_Previous) Button_Previous->SetIsEnabled (CanGoToPreviousWizardStep ());
    if (Button_Next)
    {
        Button_Next->SetVisibility (GetVisibleWhen (CanGoToNextWizardStep ()));
        Button_Next->SetIsEnabled (CanGoToNextWizardStep ());
    }
    if (Button_CreateCharacter)
    {
        Button_CreateCharacter->SetVisibility (GetVisibleWhen (IsWizardOnLastStep ()));
        Button_CreateCharacter->SetIsEnabled (IsWizardOnLastStep () && CanSubmitCharacterCreation ());
    }
    if (Button_Cancel)
    {
        Button_Cancel->SetVisibility (ESlateVisibility::Visible);
        Button_Cancel->SetIsEnabled (true);
    }
    RefreshWizardValidationMessage (this);
    RefreshSummaryStep ();
    UE_LOG (LogTemp, Verbose, TEXT ("CharacterCreationWizard Refreshed Widget=%s Step=%d StepName=%s"), *GetName (), GetCurrentWizardStepIndex (), *GetCurrentWizardStepTitle ().ToString ());
}

void URPGCharacterCreationWizardWidget::RefreshSummaryStep ()
{
    const FString Name = NormalizeCharacterName (EditableText_Name);
    const FString DisplayName = Name.IsEmpty () ? FString (TEXT ("-")) : Name;
    const FString RaceName = RaceDefinition && RaceDefinition->IsValidDefinition () ? GetDisplayNameString (RaceDefinition->DisplayName, RaceDefinition->RaceId) : FString (TEXT ("-"));
    const FString ClassName = ClassDefinition && ClassDefinition->IsValidDefinition () ? GetDisplayNameString (ClassDefinition->DisplayName, ClassDefinition->ClassId) : FString (TEXT ("-"));
    const FString GenderName = GetGenderDisplayNameString (SelectedPortraitGender);

    FRPGCharacterPortraitVariant PortraitVariant;
    FString PortraitName = TEXT ("-");
    if (TryResolveWizardSelectedPortraitVariant (PortraitVariant)) PortraitName = GetDisplayNameString (PortraitVariant.DisplayName, PortraitVariant.VariantId);
    else if (!SelectedPortraitVariantId.IsNone ()) PortraitName = SelectedPortraitVariantId.ToString ();

    FRPGAttributes FinalAttributes;
    const bool bHasAttributes = GetPreviewAttributes (FinalAttributes);
    const FRPGAttributes ClassAttributes = GetAllocatedClassAttributesForPreview ();
    const FRPGAttributes RaceBonuses = RaceDefinition ? RaceDefinition->AttributeBonuses : FRPGAttributes (0, 0, 0, 0, 0, 0);
    FRPGDerivedStats DerivedStats;
    const bool bHasDerivedStats = GetPreviewDerivedStats (DerivedStats);

    const FString StrengthLine = bHasAttributes ? FormatAttributeSummaryLine (TEXT ("Force"), AttributeStrength, ClassAttributes, RaceBonuses, FinalAttributes) : FString (TEXT ("Force : -"));
    const FString DexterityLine = bHasAttributes ? FormatAttributeSummaryLine (TEXT ("Dextérité"), AttributeDexterity, ClassAttributes, RaceBonuses, FinalAttributes) : FString (TEXT ("Dextérité : -"));
    const FString ConstitutionLine = bHasAttributes ? FormatAttributeSummaryLine (TEXT ("Constitution"), AttributeConstitution, ClassAttributes, RaceBonuses, FinalAttributes) : FString (TEXT ("Constitution : -"));
    const FString IntelligenceLine = bHasAttributes ? FormatAttributeSummaryLine (TEXT ("Intelligence"), AttributeIntelligence, ClassAttributes, RaceBonuses, FinalAttributes) : FString (TEXT ("Intelligence : -"));
    const FString WisdomLine = bHasAttributes ? FormatAttributeSummaryLine (TEXT ("Sagesse"), AttributeWisdom, ClassAttributes, RaceBonuses, FinalAttributes) : FString (TEXT ("Sagesse : -"));
    const FString CharismaLine = bHasAttributes ? FormatAttributeSummaryLine (TEXT ("Charisme"), AttributeCharisma, ClassAttributes, RaceBonuses, FinalAttributes) : FString (TEXT ("Charisme : -"));
    const FString HealthLine = bHasDerivedStats ? FString::Printf (TEXT ("PV : %d"), DerivedStats.MaxHealth) : FString (TEXT ("PV : -"));
    const FString ManaLine = bHasDerivedStats ? FString::Printf (TEXT ("Mana : %d"), DerivedStats.MaxMana) : FString (TEXT ("Mana : -"));
    const FString CarryLine = bHasAttributes ? FString::Printf (TEXT ("Charge max. : %d"), FMath::RoundToInt (GetPreviewCarryWeight ())) : FString (TEXT ("Charge max. : -"));

    FString State;
    if (CanSubmitCharacterCreation ()) State = TEXT ("Prêt à créer le personnage.");
    else if (!InventoryComponent) State = TEXT ("Création impossible : inventaire indisponible.");
    else if (Name.Len () < 1) State = TEXT ("Création impossible : saisissez un nom.");
    else if (Name.Len () > 24) State = TEXT ("Création impossible : le nom dépasse 24 caractères.");
    else if (!RaceDefinition || !RaceDefinition->IsValidDefinition ()) State = TEXT ("Création impossible : race invalide.");
    else if (!ClassDefinition || !ClassDefinition->IsValidDefinition ()) State = TEXT ("Création impossible : classe invalide.");
    else if (GetRemainingAttributePoints () > 0) State = FString::Printf (TEXT ("Création impossible : %d point(s) à répartir."), GetRemainingAttributePoints ());
    else State = TEXT ("Création impossible : vérifiez les choix précédents.");

    SetWizardOptionalText (Text_SummaryName, FString::Printf (TEXT ("Nom : %s"), *DisplayName));
    SetWizardOptionalText (Text_SummaryRace, FString::Printf (TEXT ("Race : %s"), *RaceName));
    SetWizardOptionalText (Text_SummaryClass, FString::Printf (TEXT ("Classe : %s"), *ClassName));
    SetWizardOptionalText (Text_SummaryGender, FString::Printf (TEXT ("Genre : %s"), *GenderName));
    SetWizardOptionalText (Text_SummaryPortrait, FString::Printf (TEXT ("Portrait : %s"), *PortraitName));
    SetWizardOptionalText (Text_SummaryStrength, StrengthLine);
    SetWizardOptionalText (Text_SummaryDexterity, DexterityLine);
    SetWizardOptionalText (Text_SummaryConstitution, ConstitutionLine);
    SetWizardOptionalText (Text_SummaryIntelligence, IntelligenceLine);
    SetWizardOptionalText (Text_SummaryWisdom, WisdomLine);
    SetWizardOptionalText (Text_SummaryCharisma, CharismaLine);
    SetWizardOptionalText (Text_SummaryHealth, HealthLine);
    SetWizardOptionalText (Text_SummaryMana, ManaLine);
    SetWizardOptionalText (Text_SummaryCarryWeight, CarryLine);
    SetWizardOptionalText (Text_SummaryValidationState, State);

    const bool bCanCreateCharacter = CanSubmitCharacterCreation ();
    RefreshSummaryVisuals (bCanCreateCharacter);

    const FString FullSummary = FString::Printf (TEXT ("%s\n%s · %s · %s\nPortrait : %s\n\nCaractéristiques finales\n%s\n%s\n%s\n%s\n%s\n%s\n\nStatistiques dérivées\n%s | %s | %s\n\n%s"),
        *DisplayName, *RaceName, *ClassName, *GenderName, *PortraitName, *StrengthLine, *DexterityLine, *ConstitutionLine, *IntelligenceLine, *WisdomLine, *CharismaLine, *HealthLine, *ManaLine, *CarryLine, *State);
    SetWizardOptionalText (Text_SummaryHelp, FullSummary);
}

void URPGCharacterCreationWizardWidget::RefreshSummaryVisuals (bool bCanCreateCharacter)
{
    SetWizardOptionalImageBrush (Image_SummaryPortrait, ResolveWizardSelectedPortrait (), true);
    SetWizardOptionalImageBrush (Image_SummaryClassIcon, ResolveWizardSelectedClassIcon (), true);
    SetWizardOptionalImageBrush (Image_SummaryValidationIcon, bCanCreateCharacter ? SummaryReadyIcon : SummaryBlockedIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryStrengthIcon, SummaryStrengthIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryDexterityIcon, SummaryDexterityIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryConstitutionIcon, SummaryConstitutionIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryIntelligenceIcon, SummaryIntelligenceIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryWisdomIcon, SummaryWisdomIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryCharismaIcon, SummaryCharismaIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryHealthIcon, SummaryHealthIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryManaIcon, SummaryManaIcon, false);
    SetWizardOptionalImageBrush (Image_SummaryCarryWeightIcon, SummaryCarryWeightIcon, false);
}

void URPGCharacterCreationWizardWidget::ApplyWizardStepToSwitcher ()
{
    if (!WidgetSwitcher_Steps) return;
    UWidget* StepPanel = GetPanelForWizardStep (CurrentWizardStep);
    if (StepPanel && StepPanel->GetParent () == WidgetSwitcher_Steps)
    {
        WidgetSwitcher_Steps->SetActiveWidget (StepPanel);
        return;
    }
    WidgetSwitcher_Steps->SetActiveWidgetIndex (GetCurrentWizardStepIndex ());
}

UWidget* URPGCharacterCreationWizardWidget::GetPanelForWizardStep (ERPGCharacterCreationWizardStep Step) const
{
    switch (Step)
    {
    case ERPGCharacterCreationWizardStep::Race: return Panel_StepRace;
    case ERPGCharacterCreationWizardStep::Class: return Panel_StepClass;
    case ERPGCharacterCreationWizardStep::Attributes: return Panel_StepAttributes;
    case ERPGCharacterCreationWizardStep::Identity: return Panel_StepIdentity;
    case ERPGCharacterCreationWizardStep::Summary: return Panel_StepSummary;
    default: return nullptr;
    }
}

void URPGCharacterCreationWizardWidget::ResetAttributeAllocationToClassDefinition ()
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        AllocatedClassAttributes = FRPGAttributes ();
        AllocatedClassId = NAME_None;
        bHasInitializedAllocatedClassAttributes = false;
        return;
    }
    AllocatedClassAttributes = ClampAllocatedAttributes (ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
    AllocatedClassId = ClassDefinition->ClassId;
    bHasInitializedAllocatedClassAttributes = true;
}

void URPGCharacterCreationWizardWidget::EnsureAttributeAllocationInitialized ()
{
    if (!bHasInitializedAllocatedClassAttributes || !ClassDefinition || !ClassDefinition->IsValidDefinition () || AllocatedClassId != ClassDefinition->ClassId)
    {
        ResetAttributeAllocationToClassDefinition ();
    }
}

void URPGCharacterCreationWizardWidget::AdjustAllocatedAttribute (FName AttributeId, int32 Delta)
{
    EnsureAttributeAllocationInitialized ();
    if (Delta > 0 && !CanIncreaseAllocatedAttribute (AttributeId)) return;
    if (Delta < 0 && !CanDecreaseAllocatedAttribute (AttributeId)) return;
    const int32 CurrentValue = GetAttributeValue (AllocatedClassAttributes, AttributeId);
    SetAttributeValue (AllocatedClassAttributes, AttributeId, FMath::Clamp (CurrentValue + Delta, GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum), GetSafeAllocationMaximum (AttributeAllocationMinimum, AttributeAllocationMaximum)));
    SetValidationMessage (FText::GetEmpty (), false);
    RefreshPreview ();
}

bool URPGCharacterCreationWizardWidget::CanIncreaseAllocatedAttribute (FName AttributeId) const
{
    return ClassDefinition && ClassDefinition->IsValidDefinition () && GetRemainingAttributePoints () > 0 &&
        GetAttributeValue (GetAllocatedClassAttributesForPreview (), AttributeId) < GetSafeAllocationMaximum (AttributeAllocationMinimum, AttributeAllocationMaximum);
}

bool URPGCharacterCreationWizardWidget::CanDecreaseAllocatedAttribute (FName AttributeId) const
{
    return ClassDefinition && ClassDefinition->IsValidDefinition () &&
        GetAttributeValue (GetAllocatedClassAttributesForPreview (), AttributeId) > GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum);
}

int32 URPGCharacterCreationWizardWidget::GetAttributePointBudget () const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ()) return 0;
    return SumAttributePointsAboveMinimum (ClampAllocatedAttributes (ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum), GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum));
}

int32 URPGCharacterCreationWizardWidget::GetAllocatedAttributePointsSpent () const
{
    return SumAttributePointsAboveMinimum (GetAllocatedClassAttributesForPreview (), GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum));
}

int32 URPGCharacterCreationWizardWidget::GetRemainingAttributePoints () const
{
    return FMath::Max (0, GetAttributePointBudget () - GetAllocatedAttributePointsSpent ());
}

FRPGAttributes URPGCharacterCreationWizardWidget::GetAllocatedClassAttributesForPreview () const
{
    if (bHasInitializedAllocatedClassAttributes && ClassDefinition && ClassDefinition->IsValidDefinition () && AllocatedClassId == ClassDefinition->ClassId)
    {
        return ClampAllocatedAttributes (AllocatedClassAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
    }
    return ClassDefinition && ClassDefinition->IsValidDefinition () ? ClampAllocatedAttributes (ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum) : FRPGAttributes ();
}

bool URPGCharacterCreationWizardWidget::IsUsingRecommendedAttributes () const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ()) return true;
    const FRPGAttributes Current = GetAllocatedClassAttributesForPreview ();
    const FRPGAttributes Recommended = ClampAllocatedAttributes (ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
    return Current.Strength == Recommended.Strength && Current.Dexterity == Recommended.Dexterity && Current.Constitution == Recommended.Constitution &&
        Current.Intelligence == Recommended.Intelligence && Current.Wisdom == Recommended.Wisdom && Current.Charisma == Recommended.Charisma;
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWizardWidget::ResolveWizardSelectedClassIcon () const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ()) return TSoftObjectPtr<UTexture2D> ();
    for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
    {
        if (ClassVisual && ClassVisual->IsValidForClass (ClassDefinition->ClassId)) return ClassVisual->ClassIcon;
    }
    return TSoftObjectPtr<UTexture2D> ();
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWizardWidget::ResolveWizardSelectedPortrait () const
{
    FRPGCharacterPortraitVariant PortraitVariant;
    return TryResolveWizardSelectedPortraitVariant (PortraitVariant) && !PortraitVariant.Portrait.IsNull ()
        ? PortraitVariant.Portrait
        : DefaultPortrait;
}

bool URPGCharacterCreationWizardWidget::TryResolveWizardSelectedPortraitVariant (FRPGCharacterPortraitVariant& OutVariant) const
{
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition ()) return false;
    for (const URPGCharacterPortraitSetAsset* PortraitSet : AvailablePortraitSets)
    {
        if (PortraitSet && PortraitSet->IsValidForRace (RaceDefinition->RaceId) && PortraitSet->FindPortraitVariant (SelectedPortraitGender, SelectedPortraitVariantId, OutVariant)) return true;
    }
    return false;
}

bool URPGCharacterCreationWizardWidget::SubmitCharacterCreation ()
{
    if (!InventoryComponent)
    {
        SetValidationMessage (FText::FromString (TEXT ("Composant d'inventaire indisponible.")), true);
        return false;
    }
    if (GetRemainingAttributePoints () > 0)
    {
        SetValidationMessage (FText::FromString (TEXT ("Répartissez tous les points de caractéristiques avant de créer le personnage.")), true);
        return false;
    }
    const FString NormalizedName = NormalizeCharacterName (EditableText_Name);
    if (NormalizedName.Len () < 1 || NormalizedName.Len () > 24)
    {
        SetValidationMessage (FText::FromString (TEXT ("Le nom du personnage doit contenir entre 1 et 24 caractères.")), true);
        return false;
    }
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition () || !ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        SetValidationMessage (FText::FromString (TEXT ("Une race et une classe valides sont requises.")), true);
        return false;
    }

    URPGClassAsset* EffectiveClassDefinition = DuplicateObject<URPGClassAsset> (ClassDefinition, this);
    if (!EffectiveClassDefinition)
    {
        SetValidationMessage (FText::FromString (TEXT ("Impossible de préparer la classe du personnage.")), true);
        return false;
    }
    EffectiveClassDefinition->BaseAttributes = GetAllocatedClassAttributesForPreview ();

    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString (NormalizedName);
    Request.RaceDefinition = RaceDefinition;
    Request.ClassDefinition = EffectiveClassDefinition;
    Request.PortraitGender = SelectedPortraitGender;
    Request.PortraitVariantId = SelectedPortraitVariantId;
    Request.Portrait = DefaultPortrait;
    Request.ClassIcon = ResolveWizardSelectedClassIcon ();

    FText Error;
    if (!InventoryComponent->CreateInitialCharacter (Request, Error))
    {
        SetValidationMessage (Error.IsEmpty () ? FText::FromString (TEXT ("Création du personnage impossible.")) : Error, true);
        RefreshSummaryStep ();
        return false;
    }

    InventoryComponent->SetCharacterVisualSelection (0, Request.PortraitGender, Request.PortraitVariantId, Request.Portrait, Request.ClassIcon);
    const FRPGAttributes CreatedAttributes = GetAllocatedClassAttributesForPreview ();
    UE_LOG (LogTemp, Log, TEXT ("CharacterCreationWizard CreatedCharacter Name=%s Race=%s Class=%s ClassAttributes=%d/%d/%d/%d/%d/%d"), *NormalizedName,
        RaceDefinition ? *RaceDefinition->RaceId.ToString () : TEXT ("None"), ClassDefinition ? *ClassDefinition->ClassId.ToString () : TEXT ("None"), CreatedAttributes.Strength,
        CreatedAttributes.Dexterity, CreatedAttributes.Constitution, CreatedAttributes.Intelligence, CreatedAttributes.Wisdom, CreatedAttributes.Charisma);

    SetValidationMessage (FText::GetEmpty (), false);
    if (OwningPartyPawn)
    {
        OwningPartyPawn->HandleInitialCharacterCreated ();
    }
    return true;
}

void URPGCharacterCreationWizardWidget::HandlePreviousClicked () { GoToPreviousWizardStep (); }
void URPGCharacterCreationWizardWidget::HandleNextClicked () { GoToNextWizardStep (); }
void URPGCharacterCreationWizardWidget::HandleCancelClicked () { CancelWizard (); }
void URPGCharacterCreationWizardWidget::HandleWizardCreateCharacterClicked () { SubmitCharacterCreation (); }
