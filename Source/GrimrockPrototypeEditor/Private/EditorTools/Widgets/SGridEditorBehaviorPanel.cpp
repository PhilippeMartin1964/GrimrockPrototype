#include "EditorTools/Widgets/SGridEditorBehaviorPanel.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"

#include "Core/GridLevelAsset.h"

#include "Styling/CoreStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"

void SGridEditorBehaviorPanel::Construct (const FArguments& InArgs)
{
    EditorActor = InArgs._EditorActor;
    OnGetEditorActor = InArgs._OnGetEditorActor;
    OnRequestRefresh = InArgs._OnRequestRefresh;
    BuildTriggerModeOptions ();

    ChildSlot
    [
        BuildBehaviorEditorSection ()
    ];
}

AGridLevelEditorActor* SGridEditorBehaviorPanel::GetEditorActor () const
{
    if (EditorActor.IsValid ())
    {
        return EditorActor.Get ();
    }

    return OnGetEditorActor.IsBound ()
        ? OnGetEditorActor.Execute ()
        : nullptr;
}

void SGridEditorBehaviorPanel::RequestRefresh () const
{
    if (OnRequestRefresh.IsBound ())
    {
        OnRequestRefresh.Execute ();
    }
}

void SGridEditorBehaviorPanel::BuildTriggerModeOptions ()
{
    TriggerModeOptions.Reset ();

    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Instant));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Hold));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Toggle));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::OneShot));
}

void SGridEditorBehaviorPanel::SyncEditedBehaviorFromSelection ()
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    if (!CurrentEditorActor)
    {
        CachedBehaviorObjectId.Invalidate ();
        EditedBehavior = FGridObjectBehaviorParams ();
        return;
    }

    const FGridLevelObjectData* Obj = CurrentEditorActor->GetSelectedObjectData ();
    if (!Obj)
    {
        CachedBehaviorObjectId.Invalidate ();
        EditedBehavior = FGridObjectBehaviorParams ();
        return;
    }

    if (CachedBehaviorObjectId != Obj->ObjectId)
    {
        CachedBehaviorObjectId = Obj->ObjectId;
        EditedBehavior = Obj->Behavior;
    }
}

TSharedRef<SWidget> SGridEditorBehaviorPanel::BuildBehaviorEditorSection ()
{
    SyncEditedBehaviorFromSelection ();

    return SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().FillWidth (0.55f).Padding (0.f, 0.f, 12.f, 0.f)
                [
                    SNew (SVerticalBox)

                        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SHorizontalBox)

                                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 8.f, 0.f)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Trigger Mode")))
                                ]

                                + SHorizontalBox::Slot ().FillWidth (1.f)
                                [
                                    SNew (SComboBox<TSharedPtr<EGridObjectTriggerMode>>)
                                        .OptionsSource (&TriggerModeOptions)
                                        .OnGenerateWidget (this, &SGridEditorBehaviorPanel::MakeTriggerModeComboWidget)
                                        .OnSelectionChanged (this, &SGridEditorBehaviorPanel::OnTriggerModeSelectionChanged)
                                        [
                                            SNew (STextBlock)
                                                .Text_Lambda ([this] ()
                                            {
                                                return GetSelectedTriggerModeText ();
                                            })
                                        ]
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SHorizontalBox)

                                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 8.f, 0.f)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Delay (s)")))
                                ]

                                + SHorizontalBox::Slot ().FillWidth (1.f)
                                [
                                    SNew (SNumericEntryBox<float>)
                                        .MinValue (0.f)
                                        .Value (this, &SGridEditorBehaviorPanel::GetEditedDelay)
                                        .OnValueChanged (this, &SGridEditorBehaviorPanel::OnEditedDelayChanged)
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SHorizontalBox)

                                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 8.f, 0.f)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Duration (s)")))
                                ]

                                + SHorizontalBox::Slot ().FillWidth (1.f)
                                [
                                    SNew (SNumericEntryBox<float>)
                                        .MinValue (0.f)
                                        .Value (this, &SGridEditorBehaviorPanel::GetEditedDuration)
                                        .OnValueChanged (this, &SGridEditorBehaviorPanel::OnEditedDurationChanged)
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
                        [
                            SNew (SButton)
                                .Text (FText::FromString (TEXT ("APPLY BEHAVIOR")))
                                .HAlign (HAlign_Center)
                                .OnClicked (this, &SGridEditorBehaviorPanel::OnApplyBehaviorClicked)
                        ]
                ]

            + SHorizontalBox::Slot ().FillWidth (0.45f)
                [
                    SNew (SVerticalBox)

                        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (this, &SGridEditorBehaviorPanel::GetEditedInvertLinksCheckState)
                                .OnCheckStateChanged (this, &SGridEditorBehaviorPanel::OnEditedInvertLinksChanged)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Invert Links")))
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (this, &SGridEditorBehaviorPanel::GetEditedFireOnEnterCheckState)
                                .OnCheckStateChanged (this, &SGridEditorBehaviorPanel::OnEditedFireOnEnterChanged)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Fire On Enter")))
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (this, &SGridEditorBehaviorPanel::GetEditedFireOnExitCheckState)
                                .OnCheckStateChanged (this, &SGridEditorBehaviorPanel::OnEditedFireOnExitChanged)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Fire On Exit")))
                                ]
                        ]
                ]
        ];
}

FReply SGridEditorBehaviorPanel::OnApplyBehaviorClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();

        if (CurrentEditorActor->ApplyBehaviorToSelectedObject (EditedBehavior))
        {
            RequestRefresh ();
        }
    }

    return FReply::Handled ();
}

TOptional<float> SGridEditorBehaviorPanel::GetEditedDelay () const
{
    return EditedBehavior.Activation.Delay;
}

TOptional<float> SGridEditorBehaviorPanel::GetEditedDuration () const
{
    return EditedBehavior.Activation.Duration;
}

void SGridEditorBehaviorPanel::OnEditedDelayChanged (float NewValue)
{
    EditedBehavior.Activation.Delay = FMath::Max (0.f, NewValue);
}

void SGridEditorBehaviorPanel::OnEditedDurationChanged (float NewValue)
{
    EditedBehavior.Activation.Duration = FMath::Max (0.f, NewValue);
}

ECheckBoxState SGridEditorBehaviorPanel::GetEditedInvertLinksCheckState () const
{
    return EditedBehavior.Activation.bInvertLinks ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SGridEditorBehaviorPanel::GetEditedFireOnEnterCheckState () const
{
    return EditedBehavior.Trigger.bFireOnEnter ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SGridEditorBehaviorPanel::GetEditedFireOnExitCheckState () const
{
    return EditedBehavior.Trigger.bFireOnExit ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SGridEditorBehaviorPanel::OnEditedInvertLinksChanged (ECheckBoxState NewState)
{
    EditedBehavior.Activation.bInvertLinks = NewState == ECheckBoxState::Checked;
}

void SGridEditorBehaviorPanel::OnEditedFireOnEnterChanged (ECheckBoxState NewState)
{
    EditedBehavior.Trigger.bFireOnEnter = NewState == ECheckBoxState::Checked;
}

void SGridEditorBehaviorPanel::OnEditedFireOnExitChanged (ECheckBoxState NewState)
{
    EditedBehavior.Trigger.bFireOnExit = NewState == ECheckBoxState::Checked;
}

TSharedRef<SWidget> SGridEditorBehaviorPanel::MakeTriggerModeComboWidget (
    TSharedPtr<EGridObjectTriggerMode> Item) const
{
    if (!Item.IsValid ())
    {
        return SNew (STextBlock).Text (FText::FromString (TEXT ("Invalid")));
    }

    const UEnum* Enum = StaticEnum<EGridObjectTriggerMode> ();
    const FText Text = Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (*Item))
        : FText::FromString (TEXT ("Unknown"));

    return SNew (STextBlock).Text (Text);
}

void SGridEditorBehaviorPanel::OnTriggerModeSelectionChanged (
    TSharedPtr<EGridObjectTriggerMode> NewValue,
    ESelectInfo::Type SelectInfo)
{
    if (NewValue.IsValid ())
    {
        EditedBehavior.Activation.TriggerMode = *NewValue;
    }
}

FText SGridEditorBehaviorPanel::GetSelectedTriggerModeText () const
{
    const UEnum* Enum = StaticEnum<EGridObjectTriggerMode> ();

    return Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (EditedBehavior.Activation.TriggerMode))
        : FText::FromString (TEXT ("Unknown"));
}

#endif
