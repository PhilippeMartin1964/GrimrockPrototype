#include "UI/RPGCharacterCreationWizardWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"

namespace
{
    constexpr int32 CharacterCreationWizardStepCount = 5;

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
            return FText::FromString (TEXT ("Caracteristiques"));
        case ERPGCharacterCreationWizardStep::Identity:
            return FText::FromString (TEXT ("Identite"));
        case ERPGCharacterCreationWizardStep::Summary:
            return FText::FromString (TEXT ("Resume"));
        default:
            return FText::GetEmpty ();
        }
    }

    ESlateVisibility GetVisibleWhen (bool bShouldBeVisible)
    {
        return bShouldBeVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
    }
}

void URPGCharacterCreationWizardWidget::NativeConstruct ()
{
    Super::NativeConstruct ();

    CurrentWizardStep = InitialWizardStep;
    BindWizardButtons ();
    ApplyWizardStepToSwitcher ();
    RefreshWizardShell ();
}

void URPGCharacterCreationWizardWidget::BindWizardButtons ()
{
    if (Button_Previous)
    {
        Button_Previous->OnClicked.RemoveDynamic (
            this,
            &URPGCharacterCreationWizardWidget::HandlePreviousClicked);
        Button_Previous->OnClicked.AddDynamic (
            this,
            &URPGCharacterCreationWizardWidget::HandlePreviousClicked);
    }

    if (Button_Next)
    {
        Button_Next->OnClicked.RemoveDynamic (
            this,
            &URPGCharacterCreationWizardWidget::HandleNextClicked);
        Button_Next->OnClicked.AddDynamic (
            this,
            &URPGCharacterCreationWizardWidget::HandleNextClicked);
    }

    if (Button_Cancel)
    {
        Button_Cancel->OnClicked.RemoveDynamic (
            this,
            &URPGCharacterCreationWizardWidget::HandleCancelClicked);
        Button_Cancel->OnClicked.AddDynamic (
            this,
            &URPGCharacterCreationWizardWidget::HandleCancelClicked);
    }
}

void URPGCharacterCreationWizardWidget::SetCurrentWizardStep (
    ERPGCharacterCreationWizardStep NewStep)
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
        UE_LOG (
            LogTemp,
            Log,
            TEXT ("CharacterCreationWizard Cancel Ignored Widget=%s Reason=CancelDisabled"),
            *GetName ());
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

void URPGCharacterCreationWizardWidget::RefreshWizardShell ()
{
    if (Text_StepTitle)
    {
        Text_StepTitle->SetText (GetCurrentWizardStepTitle ());
    }

    if (Text_StepCounter)
    {
        Text_StepCounter->SetText (FText::Format (
            FText::FromString (TEXT ("{0} / {1}")),
            FText::AsNumber (GetCurrentWizardStepNumber ()),
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

    UE_LOG (
        LogTemp,
        Verbose,
        TEXT ("CharacterCreationWizard Refreshed Widget=%s Step=%d StepName=%s"),
        *GetName (),
        GetCurrentWizardStepIndex (),
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

UWidget* URPGCharacterCreationWizardWidget::GetPanelForWizardStep (
    ERPGCharacterCreationWizardStep Step) const
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
