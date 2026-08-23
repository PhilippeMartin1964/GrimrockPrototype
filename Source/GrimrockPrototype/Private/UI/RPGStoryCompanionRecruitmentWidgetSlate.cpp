#include "UI/RPGStoryCompanionRecruitmentWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "RPGStoryCompanionRecruitmentWidgetSlate"

TSharedRef<SWidget> URPGStoryCompanionRecruitmentWidget::RebuildWidget ()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return Super::RebuildWidget ();
    }

    return SNew (SBorder)
        .Padding (FMargin (32.0f))
        .HAlign (HAlign_Center)
        .VAlign (VAlign_Center)
        [
            SNew (SBox)
            .WidthOverride (720.0f)
            [
                SNew (SVerticalBox)
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.0f, 0.0f, 0.0f, 12.0f)
                [
                    SAssignNew (NativeNameText, STextBlock)
                    .Justification (ETextJustify::Center)
                    .Text (View.DisplayName)
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.0f, 0.0f, 0.0f, 12.0f)
                [
                    SAssignNew (NativeIdentityText, STextBlock)
                    .Justification (ETextJustify::Center)
                    .Text (BuildIdentityLine ())
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.0f, 0.0f, 0.0f, 16.0f)
                [
                    SAssignNew (NativeDescriptionText, STextBlock)
                    .AutoWrapText (true)
                    .Text (View.ShortDescription)
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.0f, 0.0f, 0.0f, 12.0f)
                [
                    SAssignNew (NativeDetailsButton, SButton)
                    .HAlign (HAlign_Center)
                    .Text (LOCTEXT ("ShowDetails", "Voir la fiche"))
                    .OnClicked (
                        FOnClicked::CreateUObject (
                            this,
                            &URPGStoryCompanionRecruitmentWidget::HandleNativeDetailsClicked))
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.0f, 0.0f, 0.0f, 16.0f)
                [
                    SAssignNew (NativeDetailsText, STextBlock)
                    .AutoWrapText (true)
                    .Text (BuildDetailsText ())
                    .Visibility (
                        View.bDetailsVisible
                            ? EVisibility::Visible
                            : EVisibility::Collapsed)
                ]
                + SVerticalBox::Slot ().AutoHeight ().Padding (0.0f, 0.0f, 0.0f, 14.0f)
                [
                    SAssignNew (NativeStatusText, STextBlock)
                    .Justification (ETextJustify::Center)
                    .AutoWrapText (true)
                    .Text (View.StatusText)
                ]
                + SVerticalBox::Slot ().AutoHeight ()
                [
                    SNew (SHorizontalBox)
                    + SHorizontalBox::Slot ().FillWidth (1.0f).Padding (0.0f, 0.0f, 8.0f, 0.0f)
                    [
                        SAssignNew (NativeRecruitButton, SButton)
                        .HAlign (HAlign_Center)
                        .IsEnabled (View.bCanRecruit)
                        .Text (LOCTEXT ("Recruit", "Recruter"))
                        .OnClicked (
                            FOnClicked::CreateUObject (
                                this,
                                &URPGStoryCompanionRecruitmentWidget::HandleNativeRecruitClicked))
                    ]
                    + SHorizontalBox::Slot ().FillWidth (1.0f).Padding (8.0f, 0.0f, 0.0f, 0.0f)
                    [
                        SNew (SButton)
                        .HAlign (HAlign_Center)
                        .Text (LOCTEXT ("Decline", "Refuser"))
                        .OnClicked (
                            FOnClicked::CreateUObject (
                                this,
                                &URPGStoryCompanionRecruitmentWidget::HandleNativeDeclineClicked))
                    ]
                ]
            ]
        ];
}

void URPGStoryCompanionRecruitmentWidget::ReleaseSlateResources (
    bool bReleaseChildren)
{
    Super::ReleaseSlateResources (bReleaseChildren);
    NativeNameText.Reset ();
    NativeIdentityText.Reset ();
    NativeDescriptionText.Reset ();
    NativeStatusText.Reset ();
    NativeDetailsText.Reset ();
    NativeRecruitButton.Reset ();
    NativeDetailsButton.Reset ();
}

void URPGStoryCompanionRecruitmentWidget::RefreshBoundWidgets ()
{
    if (Text_Name)
    {
        Text_Name->SetText (View.DisplayName);
    }
    if (Text_Identity)
    {
        Text_Identity->SetText (BuildIdentityLine ());
    }
    if (Text_Description)
    {
        Text_Description->SetText (View.ShortDescription);
    }
    if (Text_Status)
    {
        Text_Status->SetText (View.StatusText);
    }
    if (Text_Details)
    {
        Text_Details->SetText (BuildDetailsText ());
    }
    if (Text_ShowDetailsAction)
    {
        Text_ShowDetailsAction->SetText (
            View.bDetailsVisible
                ? LOCTEXT ("HideDetails", "Masquer la fiche")
                : LOCTEXT ("ShowDetails", "Voir la fiche"));
    }
    if (Button_Recruit)
    {
        Button_Recruit->SetIsEnabled (View.bCanRecruit);
    }
    if (Panel_Details)
    {
        Panel_Details->SetVisibility (
            View.bDetailsVisible
                ? ESlateVisibility::Visible
                : ESlateVisibility::Collapsed);
    }
    if (Image_Portrait)
    {
        Image_Portrait->SetBrushFromSoftTexture (View.Portrait, false);
    }
    if (Image_FullBody)
    {
        Image_FullBody->SetBrushFromSoftTexture (View.FullBody, false);
    }
    if (Image_ClassIcon)
    {
        Image_ClassIcon->SetBrushFromSoftTexture (View.ClassIcon, false);
    }
}

void URPGStoryCompanionRecruitmentWidget::RefreshNativeSlate ()
{
    if (NativeNameText.IsValid ())
    {
        NativeNameText->SetText (View.DisplayName);
    }
    if (NativeIdentityText.IsValid ())
    {
        NativeIdentityText->SetText (BuildIdentityLine ());
    }
    if (NativeDescriptionText.IsValid ())
    {
        NativeDescriptionText->SetText (View.ShortDescription);
    }
    if (NativeStatusText.IsValid ())
    {
        NativeStatusText->SetText (View.StatusText);
    }
    if (NativeDetailsText.IsValid ())
    {
        NativeDetailsText->SetText (BuildDetailsText ());
        NativeDetailsText->SetVisibility (
            View.bDetailsVisible
                ? EVisibility::Visible
                : EVisibility::Collapsed);
    }
    if (NativeRecruitButton.IsValid ())
    {
        NativeRecruitButton->SetEnabled (View.bCanRecruit);
    }
    if (NativeDetailsButton.IsValid ())
    {
        NativeDetailsButton->SetContent (
            SNew (STextBlock)
            .Text (
                View.bDetailsVisible
                    ? LOCTEXT ("HideDetailsNative", "Masquer la fiche")
                    : LOCTEXT ("ShowDetailsNative", "Voir la fiche")));
    }
}

FReply URPGStoryCompanionRecruitmentWidget::HandleNativeRecruitClicked ()
{
    TryRecruit ();
    return FReply::Handled ();
}

FReply URPGStoryCompanionRecruitmentWidget::HandleNativeDeclineClicked ()
{
    DeclineRecruitment ();
    return FReply::Handled ();
}

FReply URPGStoryCompanionRecruitmentWidget::HandleNativeDetailsClicked ()
{
    ToggleDetails ();
    return FReply::Handled ();
}

#undef LOCTEXT_NAMESPACE
