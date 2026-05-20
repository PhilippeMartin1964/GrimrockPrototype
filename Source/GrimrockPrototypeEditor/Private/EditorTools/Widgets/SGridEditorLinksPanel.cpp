#include "EditorTools/Widgets/SGridEditorLinksPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridTypes.h"

#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"

namespace
{
    int32 CountLinksForObject (const UGridLevelAsset& LevelAsset, const FGuid& ObjectId, bool bOutgoing)
    {
        int32 Count = 0;
        for (const FGridObjectLink& Link : LevelAsset.Links)
        {
            const bool bMatches = bOutgoing
                ? Link.SourceObjectId == ObjectId
                : Link.TargetObjectId == ObjectId;

            if (bMatches)
            {
                ++Count;
            }
        }

        return Count;
    }

    FSlateColor GetLinkCommandSlateColor (EGridObjectCommand Command)
    {
        switch (Command)
        {
            case EGridObjectCommand::Open:
            case EGridObjectCommand::Activate:
                return FSlateColor (FLinearColor (0.25f, 0.85f, 0.35f, 1.f));

            case EGridObjectCommand::Close:
            case EGridObjectCommand::Deactivate:
                return FSlateColor (FLinearColor (0.95f, 0.25f, 0.20f, 1.f));

            case EGridObjectCommand::Toggle:
            default:
                return FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f));
        }
    }
}

void SGridEditorLinksPanel::Construct (const FArguments& InArgs)
{
    EditorActor = InArgs._EditorActor;
    OnGetEditorActor = InArgs._OnGetEditorActor;
    OnRequestRefresh = InArgs._OnRequestRefresh;
    BuildLinkOptions ();

    ChildSlot
    [
        BuildLinksSection ()
    ];
}

AGridLevelEditorActor* SGridEditorLinksPanel::GetEditorActor () const
{
    if (EditorActor.IsValid ())
    {
        return EditorActor.Get ();
    }

    return OnGetEditorActor.IsBound ()
        ? OnGetEditorActor.Execute ()
        : nullptr;
}

void SGridEditorLinksPanel::RequestRefresh () const
{
    if (OnRequestRefresh.IsBound ())
    {
        OnRequestRefresh.Execute ();
    }
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildLinksSection ()
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const FGridLevelObjectData* SelectedObject = CurrentEditorActor ? CurrentEditorActor->GetSelectedObjectData () : nullptr;

    if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
    {
        return SNew (STextBlock)
            .Text (FText::FromString (TEXT ("No editor actor or level asset.")));
    }

    if (!SelectedObject)
    {
        return SNew (STextBlock)
            .Text (FText::FromString (TEXT ("No selected object.")));
    }

    return SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 8.f)
        [
            SNew (SHorizontalBox)

            + SHorizontalBox::Slot ()
            .AutoWidth ()
            .Padding (0.f, 0.f, 6.f, 0.f)
            [
                GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                    FText::FromString (TEXT ("Outgoing")),
                    FText::AsNumber (CountLinksForObject (*CurrentEditorActor->LevelAsset, SelectedObject->ObjectId, true)),
                    FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f)))
            ]

            + SHorizontalBox::Slot ()
            .AutoWidth ()
            [
                GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                    FText::FromString (TEXT ("Incoming")),
                    FText::AsNumber (CountLinksForObject (*CurrentEditorActor->LevelAsset, SelectedObject->ObjectId, false)),
                    FSlateColor (FLinearColor (0.70f, 0.55f, 1.f, 1.f)))
            ]
        ]

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildLinkCreationSection ()
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (0.f, 0.f, 4.f, 0.f)
                [
                    SNew (SBorder)
                        .Padding (6.f)
                        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
                        [
                            SNew (SVerticalBox)

                                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
                                [
                                    SNew (STextBlock)
                                        .Text (FText::FromString (TEXT ("OUTGOING CONNECTORS")))
                                        .ColorAndOpacity (FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f)))
                                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                                ]

                                + SVerticalBox::Slot ().AutoHeight ()
                                [
                                    BuildObjectLinksList (*SelectedObject, true)
                                ]
                        ]
                ]

            + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (4.f, 0.f, 0.f, 0.f)
                [
                    SNew (SBorder)
                        .Padding (6.f)
                        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
                        [
                            SNew (SVerticalBox)

                                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
                                [
                                    SNew (STextBlock)
                                        .Text (FText::FromString (TEXT ("INCOMING CONNECTORS")))
                                        .ColorAndOpacity (FSlateColor (FLinearColor (0.70f, 0.55f, 1.f, 1.f)))
                                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                                ]

                                + SVerticalBox::Slot ().AutoHeight ()
                                [
                                    BuildObjectLinksList (*SelectedObject, false)
                                ]
                        ]
                ]
        ];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildLinkCreationSection ()
{
    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 8.f, 0.f)
                [
                    SNew (STextBlock).Text (FText::FromString (TEXT ("New Connector Event")))
                ]

                + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (0.f, 0.f, 12.f, 0.f)
                [
                    SNew (SComboBox<TSharedPtr<EGridObjectEvent>>)
                        .OptionsSource (&LinkSourceEventOptions)
                        .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeLinkSourceEventComboWidget)
                        .OnSelectionChanged (this, &SGridEditorLinksPanel::OnLinkSourceEventSelectionChanged)
                        [
                            SNew (STextBlock)
                                .Text_Lambda ([this] ()
                            {
                                return GetSelectedLinkSourceEventText ();
                            })
                        ]
                ]

            + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 8.f, 0.f)
                [
                    SNew (STextBlock).Text (FText::FromString (TEXT ("Command")))
                ]

                + SHorizontalBox::Slot ().FillWidth (0.5f)
                [
                    SNew (SComboBox<TSharedPtr<EGridObjectCommand>>)
                        .OptionsSource (&LinkCommandOptions)
                        .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeLinkCommandComboWidget)
                        .OnSelectionChanged (this, &SGridEditorLinksPanel::OnLinkCommandSelectionChanged)
                        [
                            SNew (STextBlock)
                                .Text_Lambda ([this] ()
                            {
                                return GetSelectedLinkCommandText ();
                            })
                        ]
                ]
        ];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildObjectLinksList (
    const FGridLevelObjectData& SelectedObject,
    bool bOutgoing)
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No level asset.")))
            ];
        return Root;
    }

    int32 Count = 0;

    for (const FGridObjectLink& Link : CurrentEditorActor->LevelAsset->Links)
    {
        const bool bMatches = bOutgoing
            ? Link.SourceObjectId == SelectedObject.ObjectId
            : Link.TargetObjectId == SelectedObject.ObjectId;

        if (!bMatches)
        {
            continue;
        }

        const FGuid SourceId = Link.SourceObjectId;
        const FGuid TargetId = Link.TargetObjectId;
        const FGuid OtherId = bOutgoing ? TargetId : SourceId;
        const FText FlowText = bOutgoing
            ? FText::Format (
                FText::FromString (TEXT ("{0} -> {1} -> {2}")),
                GetLinkSourceEventText (Link.SourceEvent),
                GetLinkCommandText (Link.Command),
                GetObjectSummaryText (TargetId))
            : FText::Format (
                FText::FromString (TEXT ("{0} -> {1} -> {2} -> {3}")),
                GetObjectSummaryText (SourceId),
                GetLinkSourceEventText (Link.SourceEvent),
                GetLinkCommandText (Link.Command),
                GetObjectSummaryText (TargetId));

        Root->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 2.f, 0.f, 4.f)
            [
                SNew (SBorder)
                    .Padding (5.f)
                    .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
                    [
                        SNew (SVerticalBox)

                        + SVerticalBox::Slot ()
                        .AutoHeight ()
                        .Padding (0.f, 0.f, 0.f, 4.f)
                        [
                            SNew (STextBlock)
                                .Text (FlowText)
                                .ColorAndOpacity (GetLinkCommandSlateColor (Link.Command))
                                .AutoWrapText (true)
                        ]

                        + SVerticalBox::Slot ()
                        .AutoHeight ()
                        [
                            SNew (SHorizontalBox)

                            + SHorizontalBox::Slot ()
                            .AutoWidth ()
                            .Padding (0.f, 0.f, 4.f, 0.f)
                            [
                                GridEditorWidgetHelpers::BuildGridActionButton (
                                    bOutgoing
                                        ? FText::FromString (TEXT ("Go To Target"))
                                        : FText::FromString (TEXT ("Go To Source")),
                                    FOnClicked::CreateLambda ([this, OtherId] () -> FReply
                                    {
                                        return OnSelectObjectFromLinkClicked (OtherId);
                                    }))
                            ]

                            + SHorizontalBox::Slot ()
                            .AutoWidth ()
                            .Padding (0.f, 0.f, 4.f, 0.f)
                            [
                                GridEditorWidgetHelpers::BuildGridActionButton (
                                    FText::FromString (TEXT ("Remove")),
                                    FOnClicked::CreateLambda ([this, Link] () -> FReply
                                    {
                                        return OnRemoveExactLinkClicked (
                                            Link.SourceObjectId,
                                            Link.TargetObjectId,
                                            Link.SourceEvent,
                                            Link.Command);
                                    }))
                            ]
                        ]
                    ]
            ];

        ++Count;
    }

    if (Count == 0)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("None")))
            ];
    }

    return Root;
}

FReply SGridEditorLinksPanel::OnRemoveExactLinkClicked (
    FGuid SourceObjectId,
    FGuid TargetObjectId,
    EGridObjectEvent SourceEvent,
    EGridObjectCommand Command)
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();
        CurrentEditorActor->RemoveExactLink (SourceObjectId, TargetObjectId, SourceEvent, Command);
        RequestRefresh ();
    }

    return FReply::Handled ();
}

FReply SGridEditorLinksPanel::OnClearSelectedObjectLinksClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();
        CurrentEditorActor->RemoveAllLinksForSelectedObject ();
        RequestRefresh ();
    }

    return FReply::Handled ();
}

FReply SGridEditorLinksPanel::OnSelectObjectFromLinkClicked (FGuid ObjectId)
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();

        if (CurrentEditorActor->SelectObjectById (ObjectId))
        {
            RequestRefresh ();
        }
    }

    return FReply::Handled ();
}

FText SGridEditorLinksPanel::GetObjectSummaryText (const FGuid& ObjectId) const
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();

    if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset || !ObjectId.IsValid ())
    {
        return FText::FromString (TEXT ("Invalid object"));
    }

    for (const FGridLevelObjectData& Obj : CurrentEditorActor->LevelAsset->Objects)
    {
        if (Obj.ObjectId != ObjectId)
        {
            continue;
        }

        const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
        const FString TypeText = TypeEnum
            ? TypeEnum->GetDisplayNameTextByValue (static_cast<int64> (Obj.Type)).ToString ()
            : TEXT ("Object");

        const UEnum* EdgeEnum = StaticEnum<EGridEdge> ();
        const FString EdgeText = EdgeEnum
            ? EdgeEnum->GetDisplayNameTextByValue (static_cast<int64> (Obj.Edge)).ToString ()
            : TEXT ("Unknown");

        return FText::FromString (
            FString::Printf (
                TEXT ("%s (%d,%d Edge=%s)"),
                *TypeText,
                Obj.CellX,
                Obj.CellY,
                *EdgeText));
    }

    return FText::FromString (TEXT ("Missing object"));
}

FText SGridEditorLinksPanel::GetLinkSourceEventText (EGridObjectEvent SourceEvent) const
{
    const UEnum* Enum = StaticEnum<EGridObjectEvent> ();

    return Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (SourceEvent))
        : FText::FromString (TEXT ("Unknown"));
}

FText SGridEditorLinksPanel::GetLinkCommandText (EGridObjectCommand Command) const
{
    const UEnum* Enum = StaticEnum<EGridObjectCommand> ();

    return Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (Command))
        : FText::FromString (TEXT ("Unknown"));
}

void SGridEditorLinksPanel::BuildLinkOptions ()
{
    LinkSourceEventOptions.Reset ();
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Activated));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Deactivated));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::ItemInserted));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::ItemRemoved));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::ItemChanged));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Used));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Entered));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Exited));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Opened));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Closed));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Enabled));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (EGridObjectEvent::Disabled));

    LinkCommandOptions.Reset ();
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Toggle));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Open));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Close));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Activate));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Deactivate));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Enable));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Disable));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Lock));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Unlock));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Spawn));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Despawn));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::Teleport));
    LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (EGridObjectCommand::ShowMessage));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeLinkSourceEventComboWidget (
    TSharedPtr<EGridObjectEvent> Item) const
{
    if (!Item.IsValid ())
    {
        return SNew (STextBlock).Text (FText::FromString (TEXT ("Invalid")));
    }

    return SNew (STextBlock).Text (GetLinkSourceEventText (*Item));
}

void SGridEditorLinksPanel::OnLinkSourceEventSelectionChanged (
    TSharedPtr<EGridObjectEvent> NewValue,
    ESelectInfo::Type SelectInfo)
{
    if (NewValue.IsValid ())
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            CurrentEditorActor->Modify ();
            CurrentEditorActor->LinkSourceEvent = *NewValue;
            RequestRefresh ();
        }
    }
}

FText SGridEditorLinksPanel::GetSelectedLinkSourceEventText () const
{
    if (const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        return GetLinkSourceEventText (CurrentEditorActor->LinkSourceEvent);
    }

    return FText::FromString (TEXT ("Unknown"));
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeLinkCommandComboWidget (
    TSharedPtr<EGridObjectCommand> Item) const
{
    if (!Item.IsValid ())
    {
        return SNew (STextBlock).Text (FText::FromString (TEXT ("Invalid")));
    }

    return SNew (STextBlock).Text (GetLinkCommandText (*Item));
}

void SGridEditorLinksPanel::OnLinkCommandSelectionChanged (
    TSharedPtr<EGridObjectCommand> NewValue,
    ESelectInfo::Type SelectInfo)
{
    if (NewValue.IsValid ())
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            CurrentEditorActor->Modify ();
            CurrentEditorActor->LinkCommand = *NewValue;
            RequestRefresh ();
        }
    }
}

FText SGridEditorLinksPanel::GetSelectedLinkCommandText () const
{
    if (const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        return GetLinkCommandText (CurrentEditorActor->LinkCommand);
    }

    return FText::FromString (TEXT ("Unknown"));
}

#endif
