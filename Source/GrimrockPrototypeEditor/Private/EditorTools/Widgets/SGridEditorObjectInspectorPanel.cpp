#include "EditorTools/Widgets/SGridEditorObjectInspectorPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridObjectArchetypeAsset.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

#include "Templates/Function.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

void SGridEditorObjectInspectorPanel::Construct (const FArguments& InArgs)
{
    EditorActor = InArgs._EditorActor;
    OnGetEditorActor = InArgs._OnGetEditorActor;
    OnRequestRefresh = InArgs._OnRequestRefresh;
    BuildTriggerModeOptions ();

    ChildSlot
    [
        BuildObjectInspectorSection ()
    ];
}

AGridLevelEditorActor* SGridEditorObjectInspectorPanel::GetEditorActor () const
{
    if (EditorActor.IsValid ())
    {
        return EditorActor.Get ();
    }

    return OnGetEditorActor.IsBound ()
        ? OnGetEditorActor.Execute ()
        : nullptr;
}

void SGridEditorObjectInspectorPanel::RequestRefresh () const
{
    if (OnRequestRefresh.IsBound ())
    {
        OnRequestRefresh.Execute ();
    }
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildObjectInspectorSection ()
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No editor actor or level asset.")))
            ];
        return Root;
    }

    const FGridLevelObjectData* Obj = CurrentEditorActor->GetSelectedObjectData ();
    if (!Obj)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No selected object.")))
            ];
        return Root;
    }

    Root->AddSlot ().AutoHeight ()
        [
            BuildSelectedObjectCard (*Obj)
        ];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 4.f, 0.f)
                [
                    GridEditorWidgetHelpers::BuildGridActionButton (
                        FText::FromString (TEXT ("Focus Selected Object")),
                        FOnClicked::CreateSP (this, &SGridEditorObjectInspectorPanel::OnFocusSelectedObjectClicked))
                ]

            + SHorizontalBox::Slot ().AutoWidth ()
                [
                    GridEditorWidgetHelpers::BuildGridActionButton (
                        FText::FromString (TEXT ("Apply Selected Object")),
                        FOnClicked::CreateSP (this, &SGridEditorObjectInspectorPanel::OnApplySelectedObjectClicked))
                ]

                + SHorizontalBox::Slot ().AutoWidth ().Padding (4.f, 0.f, 0.f, 0.f)
                [
                    GridEditorWidgetHelpers::BuildGridActionButton (
                        FText::FromString (TEXT ("Move To Current Cell")),
                        FOnClicked::CreateSP (this, &SGridEditorObjectInspectorPanel::OnMoveSelectedObjectToCurrentCellClicked))
                ]
                + SHorizontalBox::Slot ().AutoWidth ().Padding (4.f, 0.f, 0.f, 0.f)
                [
                    GridEditorWidgetHelpers::BuildGridActionButton (
                        FText::FromString (TEXT ("Rotate 90°")),
                        FOnClicked::CreateSP (this, &SGridEditorObjectInspectorPanel::OnRotateSelectedObjectYawClicked))
                ]
        ];

    return Root;
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildSelectedObjectCard (const FGridLevelObjectData& Obj)
{
    const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
    const UEnum* EdgeEnum = StaticEnum<EGridEdge> ();

    const FText TypeText = GridEditorWidgetHelpers::GetGridEnumDisplayText (TypeEnum, static_cast<int64>(Obj.Type));
    const FText EdgeText = GridEditorWidgetHelpers::GetGridEnumDisplayText (EdgeEnum, static_cast<int64>(Obj.Edge));
    const FString ShortObjectId = Obj.ObjectId.ToString ().Left (8);

    auto BuildOptionalReceptacleBehavior = [this, &Obj] () -> TSharedRef<SWidget>
    {
        if (Obj.Type == EGridLevelObjectType::Receptacle)
        {
            return BuildReceptacleBehaviorSection (Obj);
        }

        return SNullWidget::NullWidget;
    };

    auto BuildOptionalTriggerBehavior = [this, &Obj] () -> TSharedRef<SWidget>
    {
        if (Obj.Type == EGridLevelObjectType::Trigger)
        {
            return BuildTriggerBehaviorSection (Obj);
        }

        return SNullWidget::NullWidget;
    };

    auto BuildOptionalItemSpawnBehavior = [this, &Obj] () -> TSharedRef<SWidget>
    {
        if (Obj.Type == EGridLevelObjectType::ItemSpawn)
        {
            return BuildItemSpawnBehaviorSection (Obj);
        }

        return SNullWidget::NullWidget;
    };

    auto BuildOptionalReadableTextSection = [this, &Obj] () -> TSharedRef<SWidget>
    {
        const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
        const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
            ? CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId)
            : nullptr;

        const bool bLooksReadable = Archetype && Archetype->IsReadable ();
        if (!bLooksReadable)
        {
            return SNullWidget::NullWidget;
        }

        return SNew (SVerticalBox)

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
            [
                SNew (STextBlock)
                    .Text (FText::FromString (TEXT ("Readable Text")))
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                SNew (SMultiLineEditableTextBox)
                    .Text (Obj.OverrideReadableText)
                    .AutoWrapText (true)
                    .HintText (FText::FromString (TEXT ("Text displayed when the player presses Use.")))
                    .OnTextCommitted_Lambda ([this] (const FText& NewText, ETextCommit::Type CommitType)
                {
                    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                    {
                        CurrentEditorActor->SetSelectedObjectReadableText (NewText);
                        RequestRefresh ();
                    }
                })
            ];
    };

    return SNew (SBorder)
        .Padding (8.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .VAlign (VAlign_Center)
                        .Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            SNew (SBox)
                                .WidthOverride (88.f)
                                .HeightOverride (72.f)
                                [
                                    SNew (SBorder)
                                        .Padding (4.f)
                                        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
                                        [
                                            SNew (STextBlock)
                                                .Text (GridEditorWidgetHelpers::GetGridObjectGlyph (Obj.Type))
                                                .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 36))
                                                .Justification (ETextJustify::Center)
                                        ]
                                ]
                        ]

                    + SHorizontalBox::Slot ()
                        .FillWidth (1.f)
                        .VAlign (VAlign_Center)
                        [
                            SNew (STextBlock)
                                .Text (FText::Format (
                                    FText::FromString (TEXT ("{0} @ ({1},{2}) {3}  [{4}]")),
                                    TypeText,
                                    FText::AsNumber (Obj.CellX),
                                    FText::AsNumber (Obj.CellY),
                                    EdgeText,
                                    FText::FromString (ShortObjectId)))
                                .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 20))
                        ]
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
                [
                    GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (FText::FromString (TEXT ("ObjectId")), FText::FromString (ShortObjectId))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (FText::FromString (TEXT ("Type")), TypeText)
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                        FText::FromString (TEXT ("Cell / Edge")),
                        FText::Format (
                            FText::FromString (TEXT ("X={0} Y={1} Edge={2}")),
                            FText::AsNumber (Obj.CellX),
                            FText::AsNumber (Obj.CellY),
                            EdgeText))
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("ArchetypeId")),
                        SNew (SEditableTextBox)
                            .Text (FText::FromName (Obj.ArchetypeId))
                            .OnTextCommitted_Lambda ([this] (const FText& NewText, ETextCommit::Type CommitType)
                        {
                            if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                            {
                                CurrentEditorActor->SetSelectedObjectArchetypeId (FName (*NewText.ToString ()));
                                RequestRefresh ();
                            }
                        }))
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Tag")),
                        SNew (SEditableTextBox)
                            .Text (FText::FromName (Obj.Tag))
                            .OnTextCommitted_Lambda ([this] (const FText& NewText, ETextCommit::Type CommitType)
                        {
                            if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                            {
                                CurrentEditorActor->SetSelectedObjectTag (FName (*NewText.ToString ()));
                                RequestRefresh ();
                            }
                        }))
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (STextBlock).Text (FText::FromString (TEXT ("Notes")))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    SNew (SMultiLineEditableTextBox)
                        .Text (FText::FromString (Obj.Notes))
                        .AutoWrapText (true)
                        .OnTextCommitted_Lambda ([this] (const FText& NewText, ETextCommit::Type CommitType)
                    {
                        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                        {
                            CurrentEditorActor->SetSelectedObjectNotes (NewText.ToString ());
                            RequestRefresh ();
                        }
                    })
                ]
            + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildOptionalReadableTextSection ()
                ]
            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
                [
                    SNew (SHorizontalBox)
                        + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (Obj.bInitiallyEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                            {
                                if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                                {
                                    CurrentEditorActor->SetSelectedObjectInitiallyEnabled (NewState == ECheckBoxState::Checked);
                                    RequestRefresh ();
                                }
                            })
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Initially Enabled")))
                                ]
                        ]
                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (Obj.bInitiallyActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                            {
                                if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                                {
                                    CurrentEditorActor->SetSelectedObjectInitiallyActive (NewState == ECheckBoxState::Checked);
                                    RequestRefresh ();
                                }
                            })
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Initially Active")))
                                ]
                        ]
                    + SHorizontalBox::Slot ().AutoWidth ()
                        [
                            SNew (SCheckBox)
                                .IsChecked (Obj.bOverrideBehavior ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                            {
                                if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                                {
                                    CurrentEditorActor->SetSelectedObjectOverrideBehavior (NewState == ECheckBoxState::Checked);
                                    RequestRefresh ();
                                }
                            })
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Override Behavior")))
                                ]
                        ]
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (
                Obj.Type == EGridLevelObjectType::Receptacle ? FMargin (0.f, 8.f, 0.f, 0.f) : FMargin (0.f))
                [
                    BuildOptionalReceptacleBehavior ()
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (
                    Obj.Type == EGridLevelObjectType::Trigger ? FMargin (0.f, 8.f, 0.f, 0.f) : FMargin (0.f))
                [
                    BuildOptionalTriggerBehavior ()
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (
                    Obj.Type == EGridLevelObjectType::ItemSpawn ? FMargin (0.f, 8.f, 0.f, 0.f) : FMargin (0.f))
                [
                    BuildOptionalItemSpawnBehavior ()
                ]
        ];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildTriggerBehaviorSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                RequestRefresh ();
            }
        }
    };

    auto MakeCheckRow = [Obj, ApplyBehavior] (
        const FText& Label,
        bool bValue,
        TFunction<void (FGridObjectBehaviorParams&, bool)> Mutator) -> TSharedRef<SWidget>
    {
        return SNew (SCheckBox)
            .IsChecked (bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            .OnCheckStateChanged_Lambda ([Obj, ApplyBehavior, Mutator] (ECheckBoxState NewState)
        {
            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
            Mutator (NewBehavior, NewState == ECheckBoxState::Checked);
            ApplyBehavior (NewBehavior);
        })
            [
                SNew (STextBlock).Text (Label)
            ];
    };

    auto MakeNumberRow = [this, Obj, ApplyBehavior] (
        const FText& Label,
        float Value,
        TFunction<void (FGridObjectBehaviorParams&, float)> Mutator) -> TSharedRef<SWidget>
    {
        return GridEditorWidgetHelpers::BuildGridPropertyRow (
            Label,
            SNew (SNumericEntryBox<float>)
                .MinValue (0.f)
                .Value (TOptional<float> (Value))
                .OnValueCommitted_Lambda ([Obj, ApplyBehavior, Mutator] (float NewValue, ETextCommit::Type CommitType)
            {
                FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                Mutator (NewBehavior, FMath::Max (0.f, NewValue));
                ApplyBehavior (NewBehavior);
            }));
    };

    const FGridObjectBehaviorParams& Behavior = Obj.Behavior;
    const UEnum* TriggerModeEnum = StaticEnum<EGridObjectTriggerMode> ();

    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Trigger Behavior")))
                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 6.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Use SourceEvent = Enter or Exit in links to react to trigger events.")))
                        .AutoWrapText (true)
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                [
                    SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ().FillWidth (0.35f).VAlign (VAlign_Center).Padding (0.f, 2.f, 8.f, 2.f)
                        [
                            SNew (STextBlock).Text (FText::FromString (TEXT ("Trigger Mode")))
                        ]

                        + SHorizontalBox::Slot ().FillWidth (0.65f).Padding (0.f, 2.f)
                        [
                            SNew (SComboBox<TSharedPtr<EGridObjectTriggerMode>>)
                                .OptionsSource (&TriggerModeOptions)
                                .OnGenerateWidget (this, &SGridEditorObjectInspectorPanel::MakeTriggerModeComboWidget)
                                .OnSelectionChanged_Lambda ([Obj, ApplyBehavior] (
                                    TSharedPtr<EGridObjectTriggerMode> NewValue,
                                    ESelectInfo::Type SelectInfo)
                            {
                                if (NewValue.IsValid ())
                                {
                                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                                    NewBehavior.Activation.TriggerMode = *NewValue;
                                    ApplyBehavior (NewBehavior);
                                }
                            })
                                [
                                    SNew (STextBlock)
                                        .Text (TriggerModeEnum
                                            ? TriggerModeEnum->GetDisplayNameTextByValue (static_cast<int64> (Behavior.Activation.TriggerMode))
                                            : FText::FromString (TEXT ("Unknown")))
                                ]
                        ]
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeNumberRow (
                        FText::FromString (TEXT ("Delay (s)")),
                        Behavior.Activation.Delay,
                        [] (FGridObjectBehaviorParams& NewBehavior, float NewValue)
                    {
                        NewBehavior.Activation.Delay = NewValue;
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeNumberRow (
                        FText::FromString (TEXT ("Duration (s)")),
                        Behavior.Activation.Duration,
                        [] (FGridObjectBehaviorParams& NewBehavior, float NewValue)
                    {
                        NewBehavior.Activation.Duration = NewValue;
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            MakeCheckRow (
                                FText::FromString (TEXT ("Fire On Enter")),
                                Behavior.Trigger.bFireOnEnter,
                                [] (FGridObjectBehaviorParams& NewBehavior, bool bNewValue)
                            {
                                NewBehavior.Trigger.bFireOnEnter = bNewValue;
                            })
                        ]

                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            MakeCheckRow (
                                FText::FromString (TEXT ("Fire On Exit")),
                                Behavior.Trigger.bFireOnExit,
                                [] (FGridObjectBehaviorParams& NewBehavior, bool bNewValue)
                            {
                                NewBehavior.Trigger.bFireOnExit = bNewValue;
                            })
                        ]

                    + SHorizontalBox::Slot ().AutoWidth ()
                        [
                            MakeCheckRow (
                                FText::FromString (TEXT ("Invert Links")),
                                Behavior.Activation.bInvertLinks,
                                [] (FGridObjectBehaviorParams& NewBehavior, bool bNewValue)
                            {
                                NewBehavior.Activation.bInvertLinks = bNewValue;
                            })
                        ]
                ]
        ];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildReceptacleBehaviorSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                RequestRefresh ();
            }
        }
    };

    auto MakeTextRow = [this, Obj, ApplyBehavior] (
        const FText& Label,
        const FText& Value,
        TFunction<void (FGridObjectBehaviorParams&, const FString&)> Mutator) -> TSharedRef<SWidget>
    {
        return GridEditorWidgetHelpers::BuildGridPropertyRow (
            Label,
            SNew (SEditableTextBox)
                .Text (Value)
                .OnTextCommitted_Lambda ([Obj, ApplyBehavior, Mutator] (const FText& NewText, ETextCommit::Type CommitType)
            {
                FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                Mutator (NewBehavior, NewText.ToString ());
                ApplyBehavior (NewBehavior);
            }));
    };

    const FGridObjectBehaviorParams& Behavior = Obj.Behavior;

    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Receptacle Behavior")))
                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f, 0.f, 2.f)
                [
                    SNew (SCheckBox)
                        .IsChecked (Behavior.Receptacle.bAcceptAnyItem ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                        .OnCheckStateChanged_Lambda ([this, Obj] (ECheckBoxState NewState)
                    {
                        FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                        NewBehavior.Receptacle.bAcceptAnyItem = NewState == ECheckBoxState::Checked;

                        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                        {
                            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
                            {
                                RequestRefresh ();
                            }
                        }
                    })
                        [
                            SNew (STextBlock).Text (FText::FromString (TEXT ("Accept Any Item")))
                        ]
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeTextRow (
                        FText::FromString (TEXT ("Accepted Archetype Ids")),
                        FText::FromString (GridEditorWidgetHelpers::NameArrayToCommaSeparatedText (Behavior.Receptacle.AcceptedArchetypeIds)),
                        [] (FGridObjectBehaviorParams& NewBehavior, const FString& Text)
                    {
                        NewBehavior.Receptacle.AcceptedArchetypeIds = GridEditorWidgetHelpers::ParseCommaSeparatedNames (Text);
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeTextRow (
                        FText::FromString (TEXT ("Accepted Item Tags")),
                        FText::FromString (GridEditorWidgetHelpers::NameArrayToCommaSeparatedText (Behavior.Receptacle.AcceptedItemTags)),
                        [] (FGridObjectBehaviorParams& NewBehavior, const FString& Text)
                    {
                        NewBehavior.Receptacle.AcceptedItemTags = GridEditorWidgetHelpers::ParseCommaSeparatedNames (Text);
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeTextRow (
                        FText::FromString (TEXT ("Initial Contained Item")),
                        FText::FromName (Behavior.Receptacle.InitialContainedItemArchetypeId),
                        [] (FGridObjectBehaviorParams& NewBehavior, const FString& Text)
                    {
                        FString TrimmedText = Text;
                        TrimmedText.TrimStartAndEndInline ();
                        NewBehavior.Receptacle.InitialContainedItemArchetypeId = TrimmedText.IsEmpty ()
                            ? NAME_None
                            : FName (*TrimmedText);
                    })
                ]
        ];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildItemSpawnBehaviorSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                RequestRefresh ();
            }
        }
    };

    const FGridObjectBehaviorParams& Behavior = Obj.Behavior;

    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Item Spawn Behavior")))
                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 6.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Runtime item spawning is planned but may not be implemented yet.")))
                        .AutoWrapText (true)
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Spawned Item Archetype Id")),
                        SNew (SEditableTextBox)
                            .Text (FText::FromName (Behavior.ItemSpawn.SpawnedItemArchetypeId))
                            .ToolTipText (FText::FromString (TEXT ("Item archetype spawned or represented by this ItemSpawn. Runtime spawning is planned but may not be implemented yet.")))
                            .OnTextCommitted_Lambda ([Obj, ApplyBehavior] (const FText& NewText, ETextCommit::Type CommitType)
                        {
                            FString TrimmedText = NewText.ToString ();
                            TrimmedText.TrimStartAndEndInline ();

                            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                            NewBehavior.ItemSpawn.SpawnedItemArchetypeId = TrimmedText.IsEmpty ()
                                ? NAME_None
                                : FName (*TrimmedText);
                            ApplyBehavior (NewBehavior);
                        }))
                ]
        ];
}

FReply SGridEditorObjectInspectorPanel::OnApplySelectedObjectClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();

        if (CurrentEditorActor->ApplyEditedSelectedObject ())
        {
            RequestRefresh ();
        }
    }

    return FReply::Handled ();
}

FReply SGridEditorObjectInspectorPanel::OnMoveSelectedObjectToCurrentCellClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->MoveSelectedObjectToCurrentSelection ();
        RequestRefresh ();
    }

    return FReply::Handled ();
}

FReply SGridEditorObjectInspectorPanel::OnFocusSelectedObjectClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->FocusSelectedObject ();
        RequestRefresh ();
    }

    return FReply::Handled ();
}

FReply SGridEditorObjectInspectorPanel::OnRotateSelectedObjectYawClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        if (CurrentEditorActor->RotateSelectedObjectYawStep ())
        {
            RequestRefresh ();
        }
    }
    return FReply::Handled ();
}

void SGridEditorObjectInspectorPanel::BuildTriggerModeOptions ()
{
    TriggerModeOptions.Reset ();

    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Instant));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Hold));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Toggle));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::OneShot));
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::MakeTriggerModeComboWidget (
    TSharedPtr<EGridObjectTriggerMode> Item) const
{
    const UEnum* Enum = StaticEnum<EGridObjectTriggerMode> ();
    const FText Label = Item.IsValid () && Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (*Item))
        : FText::FromString (TEXT ("Unknown"));

    return SNew (STextBlock)
        .Text (Label);
}

#endif
