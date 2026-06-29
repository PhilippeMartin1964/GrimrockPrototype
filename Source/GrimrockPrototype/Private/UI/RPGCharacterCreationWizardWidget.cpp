#include "UI/RPGCharacterCreationWizardWidget.h"

#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

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
        case ERPGCharacterCreationWizardStep::Race:
            return 0;
        case ERPGCharacterCreationWizardStep::Class:
            return 1;
        case ERPGCharacterCreationWizardStep::Attributes:
            return 2;
        case ERPGCharacterCreationWizardStep::Identity:
            return 3;
        case ERPGCharacterCreationWizardStep::Summary:
            return 4;
        default:
            return 0;
        }
    }

    ERPGCharacterCreationWizardStep GetWizardStepFromIndex (int32 StepIndex)
    {
        switch (FMath::Clamp (StepIndex, 0, CharacterCreationWizardStepCount - 1))
        {
        case 0:
            return ERPGCharacterCreationWizardStep::Race;
        case 1:
            return ERPGCharacterCreationWizardStep::Class;
        case 2:
            return ERPGCharacterCreationWizardStep::Attributes;
        case 3:
            return ERPGCharacterCreationWizardStep::Identity;
        case 4:
            return ERPGCharacterCreationWizardStep::Summary;
        default:
            return ERPGCharacterCreationWizardStep::Race;
        }
    }

    FText GetWizardStepTitleText (ERPGCharacterCreationWizardStep Step)
    {
        switch (Step)
        {
        case ERPGCharacterCreationWizardStep::Race:
            return FText::FromString (TEXT ("Race"));
        case ERPGCharacterCreationWizardStep::Class:
            return FText::FromString (TEXT ("Classe"));
        case ERPGCharacterCreationWizardStep::Attributes:
            return FText::FromString (TEXT ("Caractéristiques"));
        case ERPGCharacterCreationWizardStep::Identity:
            return FText::FromString (TEXT ("Identité"));
        case ERPGCharacterCreationWizardStep::Summary:
            return FText::FromString (TEXT ("Résumé"));
        default:
            return FText::GetEmpty ();
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
        if (AttributeId == AttributeStrength)
        {
            return Attributes.Strength;
        }
        if (AttributeId == AttributeDexterity)
        {
            return Attributes.Dexterity;
        }
        if (AttributeId == AttributeConstitution)
        {
            return Attributes.Constitution;
        }
        if (AttributeId == AttributeIntelligence)
        {
            return Attributes.Intelligence;
        }
        if (AttributeId == AttributeWisdom)
        {
            return Attributes.Wisdom;
        }
        if (AttributeId == AttributeCharisma)
        {
            return Attributes.Charisma;
        }
        return 0;
    }

    void SetAttributeValue (FRPGAttributes& Attributes, FName AttributeId, int32 Value)
    {
        if (AttributeId == AttributeStrength)
        {
            Attributes.Strength = Value;
        }
        else if (AttributeId == AttributeDexterity)
        {
            Attributes.Dexterity = Value;
        }
        else if (AttributeId == AttributeConstitution)
        {
            Attributes.Constitution = Value;
        }
        else if (AttributeId == AttributeIntelligence)
        {
            Attributes.Intelligence = Value;
        }
        else if (AttributeId == AttributeWisdom)
        {
            Attributes.Wisdom = Value;
        }
        else if (AttributeId == AttributeCharisma)
        {
            Attributes.Charisma = Value;
        }
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
        return FMath::Max (0, Attributes.Strength - Minimum) +
            FMath::Max (0, Attributes.Dexterity - Minimum) +
            FMath::Max (0, Attributes.Constitution - Minimum) +
            FMath::Max (0, Attributes.Intelligence - Minimum) +
            FMath::Max (0, Attributes.Wisdom - Minimum) +
            FMath::Max (0, Attributes.Charisma - Minimum);
    }

    FText FormatSignedInteger (int32 Value)
    {
        return FText::FromString (FString::Printf (TEXT ("%+d"), Value));
    }

    FText FormatModifier (int32 Value)
    {
        return FormatSignedInteger (URPGCharacterRulesLibrary::GetAttributeModifier (Value));
    }

    void SetWizardOptionalText (UTextBlock* TextBlock, const FText& Value)
    {
        if (TextBlock)
        {
            TextBlock->SetText (Value);
        }
    }

    FString NormalizeCharacterName (UEditableText* EditableText)
    {
        FString Name = EditableText ? EditableText->GetText ().ToString () : FString ();
        Name.TrimStartAndEndInline ();
        return Name;
    }
}

void URPGCharacterCreationWizardWidget::NativeConstruct ()
{
    Super::NativeConstruct ();
    CurrentWizardStep = InitialWizardStep;
    BindWizardButtons ();
    BindAttributeAllocationButtons ();
    BindWizardSubmitButton ();
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

void URPGCharacterCreationWizardWidget::BindAttributeAllocationButtons ()
{
    if (Button_ResetRecommendedAttributes)
    {
        Button_ResetRecommendedAttributes->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleResetRecommendedAttributesClicked);
        Button_ResetRecommendedAttributes->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleResetRecommendedAttributesClicked);
    }
    if (Button_StrengthMinus)
    {
        Button_StrengthMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleStrengthMinusClicked);
        Button_StrengthMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleStrengthMinusClicked);
    }
    if (Button_StrengthPlus)
    {
        Button_StrengthPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleStrengthPlusClicked);
        Button_StrengthPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleStrengthPlusClicked);
    }
    if (Button_DexterityMinus)
    {
        Button_DexterityMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleDexterityMinusClicked);
        Button_DexterityMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleDexterityMinusClicked);
    }
    if (Button_DexterityPlus)
    {
        Button_DexterityPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleDexterityPlusClicked);
        Button_DexterityPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleDexterityPlusClicked);
    }
    if (Button_ConstitutionMinus)
    {
        Button_ConstitutionMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleConstitutionMinusClicked);
        Button_ConstitutionMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleConstitutionMinusClicked);
    }
    if (Button_ConstitutionPlus)
    {
        Button_ConstitutionPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleConstitutionPlusClicked);
        Button_ConstitutionPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleConstitutionPlusClicked);
    }
    if (Button_IntelligenceMinus)
    {
        Button_IntelligenceMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleIntelligenceMinusClicked);
        Button_IntelligenceMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleIntelligenceMinusClicked);
    }
    if (Button_IntelligencePlus)
    {
        Button_IntelligencePlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleIntelligencePlusClicked);
        Button_IntelligencePlus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleIntelligencePlusClicked);
    }
    if (Button_WisdomMinus)
    {
        Button_WisdomMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleWisdomMinusClicked);
        Button_WisdomMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleWisdomMinusClicked);
    }
    if (Button_WisdomPlus)
    {
        Button_WisdomPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleWisdomPlusClicked);
        Button_WisdomPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleWisdomPlusClicked);
    }
    if (Button_CharismaMinus)
    {
        Button_CharismaMinus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleCharismaMinusClicked);
        Button_CharismaMinus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleCharismaMinusClicked);
    }
    if (Button_CharismaPlus)
    {
        Button_CharismaPlus->OnClicked.RemoveDynamic (this, &URPGCharacterCreationWizardWidget::HandleCharismaPlusClicked);
        Button_CharismaPlus->OnClicked.AddDynamic (this, &URPGCharacterCreationWizardWidget::HandleCharismaPlusClicked);
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
    if (!CanGoToNextWizardStep ())
    {
        return false;
    }
    SetCurrentWizardStep (GetWizardStepFromIndex (GetCurrentWizardStepIndex () + 1));
    return true;
}

bool URPGCharacterCreationWizardWidget::GoToPreviousWizardStep ()
{
    if (!CanGoToPreviousWizardStep ())
    {
        return false;
    }
    SetCurrentWizardStep (GetWizardStepFromIndex (GetCurrentWizardStepIndex () - 1));
    return true;
}

void URPGCharacterCreationWizardWidget::CancelWizard ()
{
    if (!bAllowCancel)
    {
        UE_LOG (LogTemp, Log, TEXT ("CharacterCreationWizard Cancel Ignored Widget=%s Reason=CancelDisabled"), *GetName ());
        return;
    }
    UE_LOG (LogTemp, Log, TEXT ("CharacterCreationWizard Cancelled Widget=%s"), *GetName ());
    RemoveFromParent ();
}

bool URPGCharacterCreationWizardWidget::CanGoToNextWizardStep () const
{
    return GetCurrentWizardStepIndex () < CharacterCreationWizardStepCount - 1;
}

bool URPGCharacterCreationWizardWidget::CanGoToPreviousWizardStep () const
{
    return GetCurrentWizardStepIndex () > 0;
}

bool URPGCharacterCreationWizardWidget::IsWizardOnLastStep () const
{
    return GetCurrentWizardStepIndex () == CharacterCreationWizardStepCount - 1;
}

int32 URPGCharacterCreationWizardWidget::GetCurrentWizardStepIndex () const
{
    return GetWizardStepIndex (CurrentWizardStep);
}

int32 URPGCharacterCreationWizardWidget::GetCurrentWizardStepNumber () const
{
    return GetCurrentWizardStepIndex () + 1;
}

int32 URPGCharacterCreationWizardWidget::GetWizardStepCount () const
{
    return CharacterCreationWizardStepCount;
}

FText URPGCharacterCreationWizardWidget::GetCurrentWizardStepTitle () const
{
    return GetWizardStepTitleText (CurrentWizardStep);
}

void URPGCharacterCreationWizardWidget::RefreshPreview ()
{
    EnsureAttributeAllocationInitialized ();
    Super::RefreshPreview ();
    RefreshAttributeAllocationPreview ();
    RefreshAttributeAllocationControls ();
    RefreshWizardShell ();
}

bool URPGCharacterCreationWizardWidget::CanSubmitCharacterCreation () const
{
    return Super::CanSubmitCharacterCreation () && GetRemainingAttributePoints () == 0;
}

bool URPGCharacterCreationWizardWidget::GetPreviewAttributes (FRPGAttributes& OutAttributes) const
{
    OutAttributes = FRPGAttributes ();
    if (!RaceDefinition || !RaceDefinition->IsValidDefinition () || !ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }

    OutAttributes = URPGCharacterRulesLibrary::AddAttributes (
        GetAllocatedClassAttributesForPreview (),
        RaceDefinition->AttributeBonuses);
    return true;
}

bool URPGCharacterCreationWizardWidget::GetPreviewDerivedStats (FRPGDerivedStats& OutDerivedStats) const
{
    OutDerivedStats = FRPGDerivedStats ();
    FRPGAttributes Attributes;
    if (!GetPreviewAttributes (Attributes))
    {
        return false;
    }

    OutDerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats (Attributes, ClassDefinition, 1);
    return true;
}

float URPGCharacterCreationWizardWidget::GetPreviewCarryWeight () const
{
    FRPGAttributes Attributes;
    return GetPreviewAttributes (Attributes)
        ? URPGCharacterRulesLibrary::CalculateMaxCarryWeight (Attributes)
        : 0.0f;
}

void URPGCharacterCreationWizardWidget::RefreshWizardShell ()
{
    if (Text_StepTitle)
    {
        Text_StepTitle->SetText (GetCurrentWizardStepTitle ());
    }

    if (Text_StepCounter)
    {
        Text_StepCounter->SetText (FText::Format (FText::FromString (TEXT ("{0} / {1}")), FText::AsNumber (GetCurrentWizardStepNumber ()),
            FText::AsNumber (GetWizardStepCount ())));
    }

    if (Button_Previous)
    {
        Button_Previous->SetIsEnabled (CanGoToPreviousWizardStep ());
    }

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
        Button_Cancel->SetVisibility (GetVisibleWhen (bAllowCancel));
        Button_Cancel->SetIsEnabled (bAllowCancel);
    }

    UE_LOG (LogTemp, Verbose, TEXT ("CharacterCreationWizard Refreshed Widget=%s Step=%d StepName=%s"), *GetName (), GetCurrentWizardStepIndex (),
        *GetCurrentWizardStepTitle ().ToString ());
}

void URPGCharacterCreationWizardWidget::ApplyWizardStepToSwitcher ()
{
    if (!WidgetSwitcher_Steps)
    {
        return;
    }
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
    case ERPGCharacterCreationWizardStep::Race:
        return Panel_StepRace;
    case ERPGCharacterCreationWizardStep::Class:
        return Panel_StepClass;
    case ERPGCharacterCreationWizardStep::Attributes:
        return Panel_StepAttributes;
    case ERPGCharacterCreationWizardStep::Identity:
        return Panel_StepIdentity;
    case ERPGCharacterCreationWizardStep::Summary:
        return Panel_StepSummary;
    default:
        return nullptr;
    }
}

void URPGCharacterCreationWizardWidget::RefreshAttributeAllocationPreview ()
{
    const int32 Budget = GetAttributePointBudget ();
    const int32 Remaining = GetRemainingAttributePoints ();

    SetWizardOptionalText (
        Text_AttributePointsRemaining,
        FText::Format (FText::FromString (TEXT ("Points restants : {0} / {1}")), FText::AsNumber (Remaining), FText::AsNumber (Budget)));
    SetWizardOptionalText (
        Text_AttributeHelp,
        FText::FromString (TEXT ("Les valeurs de classe peuvent être ajustées. Les bonus de race sont appliqués ensuite.")));

    FRPGAttributes FinalAttributes;
    const bool bHasAttributes = GetPreviewAttributes (FinalAttributes);
    FRPGDerivedStats DerivedStats;
    const bool bHasDerivedStats = GetPreviewDerivedStats (DerivedStats);
    const FRPGAttributes ClassAttributes = GetAllocatedClassAttributesForPreview ();
    const FRPGAttributes RaceBonuses = RaceDefinition ? RaceDefinition->AttributeBonuses : FRPGAttributes (0, 0, 0, 0, 0, 0);
    const FText Unavailable = FText::FromString (TEXT ("-"));

    auto SetAttributeRow = [&] (
        FName AttributeId,
        UTextBlock* ClassText,
        UTextBlock* RaceText,
        UTextBlock* TotalText,
        UTextBlock* ModifierText,
        UTextBlock* EffectsText)
    {
        const int32 ClassValue = GetAttributeValue (ClassAttributes, AttributeId);
        const int32 RaceValue = GetAttributeValue (RaceBonuses, AttributeId);
        const int32 TotalValue = GetAttributeValue (FinalAttributes, AttributeId);
        SetWizardOptionalText (ClassText, bHasAttributes ? FText::AsNumber (ClassValue) : Unavailable);
        SetWizardOptionalText (RaceText, bHasAttributes ? FormatSignedInteger (RaceValue) : Unavailable);
        SetWizardOptionalText (TotalText, bHasAttributes ? FText::AsNumber (TotalValue) : Unavailable);
        SetWizardOptionalText (ModifierText, bHasAttributes ? FormatModifier (TotalValue) : Unavailable);

        if (!EffectsText)
        {
            return;
        }

        if (!bHasAttributes)
        {
            EffectsText->SetText (Unavailable);
            return;
        }

        const int32 Modifier = URPGCharacterRulesLibrary::GetAttributeModifier (TotalValue);
        if (AttributeId == AttributeStrength)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (
                TEXT ("Charge %d · CàC %+d"),
                FMath::RoundToInt (GetPreviewCarryWeight ()),
                Modifier)));
        }
        else if (AttributeId == AttributeDexterity)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (
                TEXT ("Précision %+d · Esquive %+d"),
                bHasDerivedStats ? DerivedStats.Accuracy : Modifier,
                bHasDerivedStats ? DerivedStats.Evasion : Modifier)));
        }
        else if (AttributeId == AttributeConstitution)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (
                TEXT ("Santé %d · Résistance %+d"),
                bHasDerivedStats ? DerivedStats.MaxHealth : 0,
                Modifier)));
        }
        else if (AttributeId == AttributeIntelligence)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (TEXT ("Savoirs %+d · Alchimie %+d"), Modifier, Modifier)));
        }
        else if (AttributeId == AttributeWisdom)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (TEXT ("Perception %+d · Volonté %+d"), Modifier, Modifier)));
        }
        else if (AttributeId == AttributeCharisma)
        {
            EffectsText->SetText (FText::FromString (FString::Printf (TEXT ("Influence %+d · Recrutement %+d"), Modifier, Modifier)));
        }
    };

    SetAttributeRow (AttributeStrength, Text_StrengthClassValue, Text_StrengthRaceBonus, Text_StrengthTotalValue, Text_StrengthModifier, Text_StrengthEffects);
    SetAttributeRow (AttributeDexterity, Text_DexterityClassValue, Text_DexterityRaceBonus, Text_DexterityTotalValue, Text_DexterityModifier, Text_DexterityEffects);
    SetAttributeRow (AttributeConstitution, Text_ConstitutionClassValue, Text_ConstitutionRaceBonus, Text_ConstitutionTotalValue, Text_ConstitutionModifier, Text_ConstitutionEffects);
    SetAttributeRow (AttributeIntelligence, Text_IntelligenceClassValue, Text_IntelligenceRaceBonus, Text_IntelligenceTotalValue, Text_IntelligenceModifier, Text_IntelligenceEffects);
    SetAttributeRow (AttributeWisdom, Text_WisdomClassValue, Text_WisdomRaceBonus, Text_WisdomTotalValue, Text_WisdomModifier, Text_WisdomEffects);
    SetAttributeRow (AttributeCharisma, Text_CharismaClassValue, Text_CharismaRaceBonus, Text_CharismaTotalValue, Text_CharismaModifier, Text_CharismaEffects);
}

void URPGCharacterCreationWizardWidget::RefreshAttributeAllocationControls ()
{
    auto RefreshButtons = [this] (FName AttributeId, UButton* MinusButton, UButton* PlusButton)
    {
        if (MinusButton)
        {
            MinusButton->SetIsEnabled (CanDecreaseAllocatedAttribute (AttributeId));
        }
        if (PlusButton)
        {
            PlusButton->SetIsEnabled (CanIncreaseAllocatedAttribute (AttributeId));
        }
    };

    RefreshButtons (AttributeStrength, Button_StrengthMinus, Button_StrengthPlus);
    RefreshButtons (AttributeDexterity, Button_DexterityMinus, Button_DexterityPlus);
    RefreshButtons (AttributeConstitution, Button_ConstitutionMinus, Button_ConstitutionPlus);
    RefreshButtons (AttributeIntelligence, Button_IntelligenceMinus, Button_IntelligencePlus);
    RefreshButtons (AttributeWisdom, Button_WisdomMinus, Button_WisdomPlus);
    RefreshButtons (AttributeCharisma, Button_CharismaMinus, Button_CharismaPlus);

    if (Button_ResetRecommendedAttributes)
    {
        Button_ResetRecommendedAttributes->SetIsEnabled (!IsUsingRecommendedAttributes ());
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

    AllocatedClassAttributes = ClampAllocatedAttributes (
        ClassDefinition->BaseAttributes,
        AttributeAllocationMinimum,
        AttributeAllocationMaximum);
    AllocatedClassId = ClassDefinition->ClassId;
    bHasInitializedAllocatedClassAttributes = true;
}

void URPGCharacterCreationWizardWidget::EnsureAttributeAllocationInitialized ()
{
    if (!bHasInitializedAllocatedClassAttributes ||
        !ClassDefinition ||
        !ClassDefinition->IsValidDefinition () ||
        AllocatedClassId != ClassDefinition->ClassId)
    {
        ResetAttributeAllocationToClassDefinition ();
    }
}

void URPGCharacterCreationWizardWidget::AdjustAllocatedAttribute (FName AttributeId, int32 Delta)
{
    EnsureAttributeAllocationInitialized ();
    if (Delta > 0 && !CanIncreaseAllocatedAttribute (AttributeId))
    {
        return;
    }
    if (Delta < 0 && !CanDecreaseAllocatedAttribute (AttributeId))
    {
        return;
    }

    const int32 CurrentValue = GetAttributeValue (AllocatedClassAttributes, AttributeId);
    const int32 Minimum = GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum);
    const int32 Maximum = GetSafeAllocationMaximum (AttributeAllocationMinimum, AttributeAllocationMaximum);
    SetAttributeValue (AllocatedClassAttributes, AttributeId, FMath::Clamp (CurrentValue + Delta, Minimum, Maximum));
    SetValidationMessage (FText::GetEmpty (), false);
    RefreshPreview ();
}

bool URPGCharacterCreationWizardWidget::CanIncreaseAllocatedAttribute (FName AttributeId) const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }
    return GetRemainingAttributePoints () > 0 &&
        GetAttributeValue (GetAllocatedClassAttributesForPreview (), AttributeId) < GetSafeAllocationMaximum (AttributeAllocationMinimum, AttributeAllocationMaximum);
}

bool URPGCharacterCreationWizardWidget::CanDecreaseAllocatedAttribute (FName AttributeId) const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return false;
    }
    return GetAttributeValue (GetAllocatedClassAttributesForPreview (), AttributeId) > GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum);
}

int32 URPGCharacterCreationWizardWidget::GetAttributePointBudget () const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return 0;
    }
    const int32 Minimum = GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum);
    return SumAttributePointsAboveMinimum (
        ClampAllocatedAttributes (ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum),
        Minimum);
}

int32 URPGCharacterCreationWizardWidget::GetAllocatedAttributePointsSpent () const
{
    const int32 Minimum = GetSafeAllocationMinimum (AttributeAllocationMinimum, AttributeAllocationMaximum);
    return SumAttributePointsAboveMinimum (GetAllocatedClassAttributesForPreview (), Minimum);
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

    if (ClassDefinition && ClassDefinition->IsValidDefinition ())
    {
        return ClampAllocatedAttributes (ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
    }

    return FRPGAttributes ();
}

bool URPGCharacterCreationWizardWidget::IsUsingRecommendedAttributes () const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return true;
    }

    const FRPGAttributes Current = GetAllocatedClassAttributesForPreview ();
    const FRPGAttributes Recommended = ClampAllocatedAttributes (ClassDefinition->BaseAttributes, AttributeAllocationMinimum, AttributeAllocationMaximum);
    return Current.Strength == Recommended.Strength &&
        Current.Dexterity == Recommended.Dexterity &&
        Current.Constitution == Recommended.Constitution &&
        Current.Intelligence == Recommended.Intelligence &&
        Current.Wisdom == Recommended.Wisdom &&
        Current.Charisma == Recommended.Charisma;
}

TSoftObjectPtr<UTexture2D> URPGCharacterCreationWizardWidget::ResolveWizardSelectedClassIcon () const
{
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return TSoftObjectPtr<UTexture2D> ();
    }

    for (const URPGClassVisualAsset* ClassVisual : AvailableClassVisuals)
    {
        if (ClassVisual && ClassVisual->IsValidForClass (ClassDefinition->ClassId))
        {
            return ClassVisual->ClassIcon;
        }
    }

    return TSoftObjectPtr<UTexture2D> ();
}

bool URPGCharacterCreationWizardWidget::SubmitWizardCharacterCreation ()
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
        SetValidationMessage (
            Error.IsEmpty () ? FText::FromString (TEXT ("Création du personnage impossible.")) : Error,
            true);
        RefreshPreview ();
        return false;
    }

    InventoryComponent->SetCharacterVisualSelection (
        0,
        Request.PortraitGender,
        Request.PortraitVariantId,
        Request.Portrait,
        Request.ClassIcon);

    SetValidationMessage (FText::GetEmpty (), false);
    if (OwningPartyPawn)
    {
        OwningPartyPawn->HandleInitialCharacterCreated ();
    }
    return true;
}

void URPGCharacterCreationWizardWidget::HandlePreviousClicked ()
{
    GoToPreviousWizardStep ();
}

void URPGCharacterCreationWizardWidget::HandleNextClicked ()
{
    GoToNextWizardStep ();
}

void URPGCharacterCreationWizardWidget::HandleCancelClicked ()
{
    CancelWizard ();
}

void URPGCharacterCreationWizardWidget::HandleWizardCreateCharacterClicked ()
{
    SubmitWizardCharacterCreation ();
}

void URPGCharacterCreationWizardWidget::HandleResetRecommendedAttributesClicked ()
{
    ResetAttributeAllocationToClassDefinition ();
    SetValidationMessage (FText::GetEmpty (), false);
    RefreshPreview ();
}

void URPGCharacterCreationWizardWidget::HandleStrengthMinusClicked ()
{
    AdjustAllocatedAttribute (AttributeStrength, -1);
}

void URPGCharacterCreationWizardWidget::HandleStrengthPlusClicked ()
{
    AdjustAllocatedAttribute (AttributeStrength, 1);
}

void URPGCharacterCreationWizardWidget::HandleDexterityMinusClicked ()
{
    AdjustAllocatedAttribute (AttributeDexterity, -1);
}

void URPGCharacterCreationWizardWidget::HandleDexterityPlusClicked ()
{
    AdjustAllocatedAttribute (AttributeDexterity, 1);
}

void URPGCharacterCreationWizardWidget::HandleConstitutionMinusClicked ()
{
    AdjustAllocatedAttribute (AttributeConstitution, -1);
}

void URPGCharacterCreationWizardWidget::HandleConstitutionPlusClicked ()
{
    AdjustAllocatedAttribute (AttributeConstitution, 1);
}

void URPGCharacterCreationWizardWidget::HandleIntelligenceMinusClicked ()
{
    AdjustAllocatedAttribute (AttributeIntelligence, -1);
}

void URPGCharacterCreationWizardWidget::HandleIntelligencePlusClicked ()
{
    AdjustAllocatedAttribute (AttributeIntelligence, 1);
}

void URPGCharacterCreationWizardWidget::HandleWisdomMinusClicked ()
{
    AdjustAllocatedAttribute (AttributeWisdom, -1);
}

void URPGCharacterCreationWizardWidget::HandleWisdomPlusClicked ()
{
    AdjustAllocatedAttribute (AttributeWisdom, 1);
}

void URPGCharacterCreationWizardWidget::HandleCharismaMinusClicked ()
{
    AdjustAllocatedAttribute (AttributeCharisma, -1);
}

void URPGCharacterCreationWizardWidget::HandleCharismaPlusClicked ()
{
    AdjustAllocatedAttribute (AttributeCharisma, 1);
}
