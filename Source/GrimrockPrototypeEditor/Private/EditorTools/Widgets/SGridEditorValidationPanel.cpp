#include "EditorTools/Widgets/SGridEditorValidationPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"

namespace
{
    FText GetValidationSeverityText (EGridLevelValidationSeverity Severity)
    {
        switch (Severity)
        {
            case EGridLevelValidationSeverity::Error:   return FText::FromString (TEXT ("Error"));
            case EGridLevelValidationSeverity::Warning: return FText::FromString (TEXT ("Warning"));
            case EGridLevelValidationSeverity::Info:
            default:                                   return FText::FromString (TEXT ("Info"));
        }
    }

    FSlateColor GetValidationSeverityColor (EGridLevelValidationSeverity Severity)
    {
        switch (Severity)
        {
            case EGridLevelValidationSeverity::Error:
                return FSlateColor (FLinearColor (0.95f, 0.25f, 0.20f, 1.f));

            case EGridLevelValidationSeverity::Warning:
                return FSlateColor (FLinearColor (1.0f, 0.72f, 0.20f, 1.f));

            case EGridLevelValidationSeverity::Info:
            default:
                return FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f));
        }
    }
}

void FGridEditorValidationPanelState::CountValidationErrorsWarnings (int32& OutErrorCount, int32& OutWarningCount) const
{
    OutErrorCount = 0;
    OutWarningCount = 0;

    for (const FGridLevelValidationMessage& ValidationMessage : ValidationMessages)
    {
        if (ValidationMessage.Severity == EGridLevelValidationSeverity::Error)
        {
            ++OutErrorCount;
        }
        else if (ValidationMessage.Severity == EGridLevelValidationSeverity::Warning)
        {
            ++OutWarningCount;
        }
    }
}

FText FGridEditorValidationPanelState::GetValidationStatusText () const
{
    if (!bValidationHasRun)
    {
        return FText::FromString (TEXT ("Not run"));
    }

    int32 ErrorCount = 0;
    int32 WarningCount = 0;
    CountValidationErrorsWarnings (ErrorCount, WarningCount);

    return FText::Format (
        FText::FromString (TEXT ("{0} total, {1} errors, {2} warnings")),
        FText::AsNumber (ValidationMessages.Num ()),
        FText::AsNumber (ErrorCount),
        FText::AsNumber (WarningCount));
}

void SGridEditorValidationPanel::Construct (const FArguments& InArgs)
{
    EditorActor = InArgs._EditorActor;
    ValidationState = InArgs._ValidationState;
    OnGetEditorActor = InArgs._OnGetEditorActor;
    OnRequestRefresh = InArgs._OnRequestRefresh;

    ChildSlot
    [
        BuildValidationSection ()
    ];
}

AGridLevelEditorActor* SGridEditorValidationPanel::GetEditorActor () const
{
    if (EditorActor.IsValid ())
    {
        return EditorActor.Get ();
    }

    return OnGetEditorActor.IsBound ()
        ? OnGetEditorActor.Execute ()
        : nullptr;
}

FGridEditorValidationPanelState& SGridEditorValidationPanel::GetValidationState () const
{
    static FGridEditorValidationPanelState FallbackValidationState;

    return ValidationState.IsValid ()
        ? *ValidationState
        : FallbackValidationState;
}

void SGridEditorValidationPanel::RequestRefresh () const
{
    if (OnRequestRefresh.IsBound ())
    {
        OnRequestRefresh.Execute ();
    }
}

TSharedRef<SWidget> SGridEditorValidationPanel::BuildValidationSection ()
{
    const FGridEditorValidationPanelState& CurrentValidationState = GetValidationState ();
    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    Root->AddSlot ()
        .AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridActionButton (
                FText::FromString (TEXT ("Validate Level")),
                FOnClicked::CreateSP (this, &SGridEditorValidationPanel::OnValidateLevelClicked))
        ];

    if (!CurrentValidationState.bValidationHasRun)
    {
        Root->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 6.f, 0.f, 0.f)
            [
                SNew (STextBlock)
                    .Text (FText::FromString (TEXT ("No validation run yet.")))
                    .AutoWrapText (true)
            ];

        return Root;
    }

    if (CurrentValidationState.ValidationMessages.Num () == 0)
    {
        Root->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 6.f, 0.f, 0.f)
            [
                SNew (STextBlock)
                    .Text (FText::FromString (TEXT ("No validation messages.")))
                    .AutoWrapText (true)
            ];

        return Root;
    }

    for (const FGridLevelValidationMessage& ValidationMessage : CurrentValidationState.ValidationMessages)
    {
        const FString ShortObjectId = ValidationMessage.OptionalObjectId.IsValid ()
            ? ValidationMessage.OptionalObjectId.ToString ().Left (8)
            : FString ();

        Root->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 6.f, 0.f, 0.f)
            [
                SNew (SBorder)
                    .Padding (6.f)
                    .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
                    [
                        SNew (SVerticalBox)

                            + SVerticalBox::Slot ().AutoHeight ()
                            [
                                SNew (SHorizontalBox)

                                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 8.f, 0.f)
                                    [
                                        SNew (STextBlock)
                                            .Text (GetValidationSeverityText (ValidationMessage.Severity))
                                            .ColorAndOpacity (GetValidationSeverityColor (ValidationMessage.Severity))
                                            .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 9))
                                    ]

                                    + SHorizontalBox::Slot ().FillWidth (1.f)
                                    [
                                        SNew (STextBlock)
                                            .Text (FText::FromString (ValidationMessage.Message))
                                            .AutoWrapText (true)
                                    ]
                            ]

                        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, ShortObjectId.IsEmpty () ? 0.f : 4.f, 0.f, 0.f)
                            [
                                SNew (STextBlock)
                                    .Visibility (ShortObjectId.IsEmpty () ? EVisibility::Collapsed : EVisibility::Visible)
                                    .Text (FText::Format (
                                        FText::FromString (TEXT ("Object: {0}")),
                                        FText::FromString (ShortObjectId)))
                            ]
                    ]
            ];
    }

    return Root;
}

FReply SGridEditorValidationPanel::OnValidateLevelClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        FGridEditorValidationPanelState& CurrentValidationState = GetValidationState ();
        CurrentValidationState.bValidationHasRun = true;
        CurrentValidationState.ValidationMessages = CurrentEditorActor->ValidateCurrentLevel ();
        RequestRefresh ();
    }

    return FReply::Handled ();
}

#endif
