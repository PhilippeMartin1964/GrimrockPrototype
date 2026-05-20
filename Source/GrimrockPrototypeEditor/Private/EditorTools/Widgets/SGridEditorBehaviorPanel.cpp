#include "EditorTools/Widgets/SGridEditorBehaviorPanel.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"

#include "Core/GridLevelAsset.h"

#include "Styling/CoreStyle.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
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
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Invert Connectors")))
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

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
                        [
                            SNew (STextBlock)
                                .Text (FText::FromString (TEXT ("Item Spawn")))
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SEditableTextBox)
                                .Text (this, &SGridEditorBehaviorPanel::GetEditedSpawnedItemArchetypeIdText)
                                .HintText (FText::FromString (TEXT ("Spawned Item Archetype Id")))
                                .ToolTipText (FText::FromString (TEXT ("Item archetype spawned or represented by this ItemSpawn. Runtime spawning is planned but may not be implemented yet.")))
                                .OnTextCommitted (this, &SGridEditorBehaviorPanel::OnEditedSpawnedItemArchetypeIdCommitted)
                        ]
                ]
        ];
}

void SGridEditorBehaviorPanel::ApplyEditedBehaviorToSelection ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();

        if (CurrentEditorActor->ApplyBehaviorToSelectedObject (EditedBehavior))
        {
            RequestRefresh ();
        }
    }
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
    ApplyEditedBehaviorToSelection ();
}

void SGridEditorBehaviorPanel::OnEditedDurationChanged (float NewValue)
{
    EditedBehavior.Activation.Duration = FMath::Max (0.f, NewValue);
    ApplyEditedBehaviorToSelection ();
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

FText SGridEditorBehaviorPanel::GetEditedSpawnedItemArchetypeIdText () const
{
    return FText::FromName (EditedBehavior.ItemSpawn.SpawnedItemArchetypeId);
}

void SGridEditorBehaviorPanel::OnEditedInvertLinksChanged (ECheckBoxState NewState)
{
    EditedBehavior.Activation.bInvertLinks = NewState == ECheckBoxState::Checked;
    ApplyEditedBehaviorToSelection ();
}

void SGridEditorBehaviorPanel::OnEditedFireOnEnterChanged (ECheckBoxState NewState)
{
    EditedBehavior.Trigger.bFireOnEnter = NewState == ECheckBoxState::Checked;
    ApplyEditedBehaviorToSelection ();
}

void SGridEditorBehaviorPanel::OnEditedFireOnExitChanged (ECheckBoxState NewState)
{
    EditedBehavior.Trigger.bFireOnExit = NewState == ECheckBoxState::Checked;
    ApplyEditedBehaviorToSelection ();
}

void SGridEditorBehaviorPanel::OnEditedSpawnedItemArchetypeIdCommitted (const FText& NewText, ETextCommit::Type CommitType)
{
    FString TrimmedText = NewText.ToString ();
    TrimmedText.TrimStartAndEndInline ();
    EditedBehavior.ItemSpawn.SpawnedItemArchetypeId = TrimmedText.IsEmpty ()
        ? NAME_None
        : FName (*TrimmedText);
    ApplyEditedBehaviorToSelection ();
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
        ApplyEditedBehaviorToSelection ();
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
