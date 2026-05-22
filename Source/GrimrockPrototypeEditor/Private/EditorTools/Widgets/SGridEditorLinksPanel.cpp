#include "EditorTools/Widgets/SGridEditorLinksPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridTypes.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"

namespace
{
    const FLinearColor OutgoingConnectorColor (0.25f, 0.75f, 1.f, 1.f);
    const FLinearColor IncomingConnectorColor (0.70f, 0.55f, 1.f, 1.f);
    const FLinearColor BrokenConnectorColor (1.f, 0.18f, 0.16f, 1.f);

    const FGridLevelObjectData* FindObjectById (const UGridLevelAsset* LevelAsset, const FGuid& ObjectId);
    TArray<EGridObjectCommand> GetSupportedCommandsForTarget (const FGridLevelObjectData& Obj, const UGridObjectArchetypeAsset* Archetype);

    bool ArchetypeTextContains (const UGridObjectArchetypeAsset* Archetype, const TCHAR* Needle)
    {
        if (!Archetype)
        {
            return false;
        }

        return Archetype->ArchetypeId.ToString ().Contains (Needle, ESearchCase::IgnoreCase) ||
            Archetype->DisplayName.ToString ().Contains (Needle, ESearchCase::IgnoreCase) ||
            Archetype->Category.ToString ().Contains (Needle, ESearchCase::IgnoreCase);
    }

    bool IsLockLikeObject (const FGridLevelObjectData& Obj, const UGridObjectArchetypeAsset* Archetype)
    {
        return Obj.Type == EGridLevelObjectType::Receptacle &&
            (ArchetypeTextContains (Archetype, TEXT ("Lock")) ||
                ArchetypeTextContains (Archetype, TEXT ("Keyhole")));
    }

    bool CanObjectEmitEvents (const FGridLevelObjectData& Obj, const UGridObjectArchetypeAsset* Archetype)
    {
        switch (Obj.Type)
        {
            case EGridLevelObjectType::Button:
            case EGridLevelObjectType::Lever:
            case EGridLevelObjectType::PressurePlate:
            case EGridLevelObjectType::Trigger:
            case EGridLevelObjectType::Receptacle:
                return true;

            case EGridLevelObjectType::Door:
                return Archetype && Archetype->bIsInteractable;

            case EGridLevelObjectType::ItemSpawn:
            case EGridLevelObjectType::Item:
            case EGridLevelObjectType::Teleporter:
                return Archetype && Archetype->bIsInteractable;

            case EGridLevelObjectType::Decoration:
                return Archetype && Archetype->bIsInteractable && !Archetype->bIsReadable;

            default:
                return false;
        }
    }

    TArray<EGridObjectEvent> GetSupportedEventsForSource (const FGridLevelObjectData& Obj, const UGridObjectArchetypeAsset* Archetype)
    {
        TArray<EGridObjectEvent> Events;

        switch (Obj.Type)
        {
            case EGridLevelObjectType::Button:
                Events = {EGridObjectEvent::Activated, EGridObjectEvent::Used};
                break;

            case EGridLevelObjectType::Lever:
                Events = {EGridObjectEvent::Activated, EGridObjectEvent::Deactivated};
                break;

            case EGridLevelObjectType::PressurePlate:
                Events = {EGridObjectEvent::Activated, EGridObjectEvent::Deactivated, EGridObjectEvent::Entered, EGridObjectEvent::Exited};
                break;

            case EGridLevelObjectType::Trigger:
                Events = {EGridObjectEvent::Entered, EGridObjectEvent::Exited, EGridObjectEvent::Activated, EGridObjectEvent::Deactivated};
                break;

            case EGridLevelObjectType::Receptacle:
                Events = IsLockLikeObject (Obj, Archetype)
                    ? TArray<EGridObjectEvent> {EGridObjectEvent::Activated, EGridObjectEvent::Used}
                    : TArray<EGridObjectEvent> {EGridObjectEvent::ItemInserted, EGridObjectEvent::ItemRemoved};
                break;

            case EGridLevelObjectType::Door:
                Events = {EGridObjectEvent::Opened, EGridObjectEvent::Closed};
                break;

            case EGridLevelObjectType::Teleporter:
                Events = {EGridObjectEvent::Entered, EGridObjectEvent::Activated, EGridObjectEvent::Deactivated};
                break;

            case EGridLevelObjectType::ItemSpawn:
            case EGridLevelObjectType::Item:
                if (Archetype && Archetype->bIsInteractable)
                {
                    Events = {EGridObjectEvent::Activated, EGridObjectEvent::Used};
                }
                break;

            case EGridLevelObjectType::Decoration:
                if (Archetype && Archetype->bIsInteractable && !Archetype->bIsReadable)
                {
                    Events = {EGridObjectEvent::Activated, EGridObjectEvent::Used};
                }
                break;

            default:
                break;
        }

        return Events;
    }

    bool CanObjectReceiveCommands (const FGridLevelObjectData& Obj, const UGridObjectArchetypeAsset* Archetype)
    {
        return GetSupportedCommandsForTarget (Obj, Archetype).Num () > 0;
    }

    TArray<EGridObjectCommand> GetSupportedCommandsForTarget (const FGridLevelObjectData& Obj, const UGridObjectArchetypeAsset* Archetype)
    {
        switch (Obj.Type)
        {
            case EGridLevelObjectType::Door:
                return {EGridObjectCommand::Open, EGridObjectCommand::Close, EGridObjectCommand::Toggle, EGridObjectCommand::Lock, EGridObjectCommand::Unlock};

            case EGridLevelObjectType::Teleporter:
                return {EGridObjectCommand::Activate, EGridObjectCommand::Deactivate, EGridObjectCommand::Toggle};

            case EGridLevelObjectType::Light:
                return {EGridObjectCommand::Activate, EGridObjectCommand::Deactivate, EGridObjectCommand::Toggle};

            default:
                break;
        }

        return {};
    }

    bool IsConnectorBroken (const FGridObjectLink& Link)
    {
        return !Link.SourceObjectId.IsValid () || !Link.TargetObjectId.IsValid ();
    }

    bool IsConnectorBroken (const FGridObjectLink& Link, const UGridLevelAsset* LevelAsset)
    {
        return IsConnectorBroken (Link) ||
            !FindObjectById (LevelAsset, Link.SourceObjectId) ||
            !FindObjectById (LevelAsset, Link.TargetObjectId);
    }

    const FGridLevelObjectData* FindObjectById (const UGridLevelAsset* LevelAsset, const FGuid& ObjectId)
    {
        if (!LevelAsset || !ObjectId.IsValid ())
        {
            return nullptr;
        }

        for (const FGridLevelObjectData& Object : LevelAsset->Objects)
        {
            if (Object.ObjectId == ObjectId)
            {
                return &Object;
            }
        }

        return nullptr;
    }
}

void SGridEditorLinksPanel::Construct (const FArguments& InArgs)
{
    EditorActor = InArgs._EditorActor;
    OnGetEditorActor = InArgs._OnGetEditorActor;
    OnRequestRefresh = InArgs._OnRequestRefresh;
    BuildLinkOptions ();
    RefreshConnectorFormOptions ();

    RebuildLinksSection ();
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

void SGridEditorLinksPanel::RebuildLinksSection ()
{
    RefreshConnectorFormOptions ();

    ChildSlot
    [
        BuildLinksSection ()
    ];
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

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 6.f)
        [
            BuildConnectorsHeader ()
        ]

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, bAddConnectorVisible ? 8.f : 0.f)
        [
            bAddConnectorVisible
                ? BuildLinkCreationSection ()
                : SNullWidget::NullWidget
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

TSharedRef<SWidget> SGridEditorLinksPanel::BuildConnectorsHeader ()
{
    return SNew (SHorizontalBox)

        + SHorizontalBox::Slot ()
        .FillWidth (1.f)
        .VAlign (VAlign_Center)
        [
            BuildConnectorLegend ()
        ]

        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .VAlign (VAlign_Center)
        [
            GridEditorWidgetHelpers::BuildGridActionButton (
                FText::FromString (TEXT ("+")),
                FOnClicked::CreateSP (this, &SGridEditorLinksPanel::OnToggleAddConnectorClicked))
        ];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildConnectorLegend ()
{
    return SNew (SHorizontalBox)

        + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 8.f, 0.f)
        [
            BuildConnectorLegendItem (
                FText::FromString (TEXT ("Cyan = Outgoing")),
                FSlateColor (OutgoingConnectorColor))
        ]

        + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 8.f, 0.f)
        [
            BuildConnectorLegendItem (
                FText::FromString (TEXT ("Purple = Incoming")),
                FSlateColor (IncomingConnectorColor))
        ]

        + SHorizontalBox::Slot ().AutoWidth ()
        [
            BuildConnectorLegendItem (
                FText::FromString (TEXT ("Red = Broken")),
                FSlateColor (BrokenConnectorColor))
        ];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildConnectorLegendItem (
    const FText& Label,
    const FSlateColor& Color) const
{
    return SNew (STextBlock)
        .Text (Label)
        .ColorAndOpacity (Color)
        .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8));
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildLinkCreationSection ()
{
    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Source Object")),
                        BuildObjectCombo (
                            FText::FromString (TEXT ("Select source")),
                            true))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Event")),
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
                            ])
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Target Object")),
                        BuildObjectCombo (
                            FText::FromString (TEXT ("Select target")),
                            false))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Command")),
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
                            ])
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
                [
                    SNew (SHorizontalBox)

                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 4.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (FText::FromString (TEXT ("Create")))
                            .HAlign (HAlign_Center)
                            .ContentPadding (FMargin (8.f, 3.f))
                            .IsEnabled (this, &SGridEditorLinksPanel::CanCreateConnector)
                            .OnClicked (this, &SGridEditorLinksPanel::OnCreateConnectorClicked)
                    ]

                    + SHorizontalBox::Slot ().AutoWidth ()
                    [
                        GridEditorWidgetHelpers::BuildGridActionButton (
                            FText::FromString (TEXT ("Cancel")),
                            FOnClicked::CreateSP (this, &SGridEditorLinksPanel::OnCancelAddConnectorClicked))
                    ]
                ]
        ];
}

TSharedRef<SWidget> SGridEditorLinksPanel::BuildObjectCombo (const FText& EmptyText, bool bSourceObject)
{
    if (bSourceObject)
    {
        return SNew (SComboBox<TSharedPtr<FGuid>>)
            .OptionsSource (&SourceObjectOptions)
            .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeObjectComboWidget)
            .OnSelectionChanged (this, &SGridEditorLinksPanel::OnSourceObjectSelectionChanged)
            [
                SNew (STextBlock)
                    .Text_Lambda ([this, EmptyText] ()
                {
                    return GetSelectedObjectOptionText (SelectedSourceObjectId, EmptyText);
                })
            ];
    }

    return SNew (SComboBox<TSharedPtr<FGuid>>)
        .OptionsSource (&TargetObjectOptions)
        .OnGenerateWidget (this, &SGridEditorLinksPanel::MakeObjectComboWidget)
        .OnSelectionChanged (this, &SGridEditorLinksPanel::OnTargetObjectSelectionChanged)
        [
            SNew (STextBlock)
                .Text_Lambda ([this, EmptyText] ()
            {
                return GetSelectedObjectOptionText (SelectedTargetObjectId, EmptyText);
            })
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

    static const EGridObjectEvent EventOrder[] =
    {
        EGridObjectEvent::Activated,
        EGridObjectEvent::Deactivated,
        EGridObjectEvent::ItemInserted,
        EGridObjectEvent::ItemRemoved,
        EGridObjectEvent::ItemChanged,
        EGridObjectEvent::Used,
        EGridObjectEvent::Entered,
        EGridObjectEvent::Exited,
        EGridObjectEvent::Opened,
        EGridObjectEvent::Closed,
        EGridObjectEvent::Enabled,
        EGridObjectEvent::Disabled
    };

    const auto AddLinkRow = [this, CurrentEditorActor, &Root, &Count, bOutgoing] (const FGridObjectLink& Link)
    {
        const FGuid SourceId = Link.SourceObjectId;
        const FGuid TargetId = Link.TargetObjectId;
        const FGuid OtherId = bOutgoing ? TargetId : SourceId;
        const bool bBroken = IsConnectorBroken (Link, CurrentEditorActor->LevelAsset);
        const FText FlowText = bOutgoing
            ? FText::Format (
                FText::FromString (TEXT ("-> {0} : {1}")),
                FindObjectById (CurrentEditorActor->LevelAsset, TargetId) ? GetObjectSummaryText (TargetId) : FText::FromString (TEXT ("Missing object")),
                GetLinkCommandText (Link.Command))
            : FText::Format (
                FText::FromString (TEXT ("{0} : {1} -> {2}")),
                FindObjectById (CurrentEditorActor->LevelAsset, SourceId) ? GetObjectSummaryText (SourceId) : FText::FromString (TEXT ("Missing object")),
                GetLinkSourceEventText (Link.SourceEvent),
                GetLinkCommandText (Link.Command));

        Root->AddSlot ()
            .AutoHeight ()
            .Padding (bOutgoing ? FMargin (12.f, 2.f, 0.f, 4.f) : FMargin (0.f, 2.f, 0.f, 4.f))
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
                                .ColorAndOpacity (bBroken
                                    ? FSlateColor (BrokenConnectorColor)
                                    : FSlateColor (bOutgoing ? OutgoingConnectorColor : IncomingConnectorColor))
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
    };

    if (bOutgoing)
    {
        for (const EGridObjectEvent Event : EventOrder)
        {
            bool bEventHeaderAdded = false;
            for (const FGridObjectLink& Link : CurrentEditorActor->LevelAsset->Links)
            {
                if (Link.SourceObjectId != SelectedObject.ObjectId || Link.SourceEvent != Event)
                {
                    continue;
                }

                if (!bEventHeaderAdded)
                {
                    Root->AddSlot ().AutoHeight ().Padding (0.f, 5.f, 0.f, 1.f)
                    [
                        SNew (STextBlock)
                            .Text (GetLinkSourceEventText (Event))
                            .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 9))
                    ];
                    bEventHeaderAdded = true;
                }

                AddLinkRow (Link);
            }
        }
    }
    else
    {
        for (const FGridObjectLink& Link : CurrentEditorActor->LevelAsset->Links)
        {
            if (Link.TargetObjectId == SelectedObject.ObjectId)
            {
                AddLinkRow (Link);
            }
        }
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

FReply SGridEditorLinksPanel::OnToggleAddConnectorClicked ()
{
    bAddConnectorVisible = !bAddConnectorVisible;

    if (bAddConnectorVisible)
    {
        BuildObjectOptions ();
    }

    RebuildLinksSection ();
    return FReply::Handled ();
}

FReply SGridEditorLinksPanel::OnCreateConnectorClicked ()
{
    if (!CanCreateConnector ())
    {
        return FReply::Handled ();
    }

    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();

        if (CurrentEditorActor->CreateLink (
            *SelectedSourceObjectId,
            *SelectedTargetObjectId,
            *SelectedSourceEvent,
            *SelectedCommand))
        {
            SelectedTargetObjectId.Reset ();
            bAddConnectorVisible = false;
            RequestRefresh ();
        }
    }

    return FReply::Handled ();
}

FReply SGridEditorLinksPanel::OnCancelAddConnectorClicked ()
{
    SelectedTargetObjectId.Reset ();
    bAddConnectorVisible = false;
    RebuildLinksSection ();
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
        const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId);
        const FString NameText = Archetype && !Archetype->DisplayName.IsEmpty ()
            ? Archetype->DisplayName.ToString ()
            : TypeText;
        const FString SummaryNameText = Obj.Type == EGridLevelObjectType::ItemSpawn
            ? FString::Printf (TEXT ("%s Spawner"), *NameText.Replace (TEXT (" Spawn"), TEXT ("")))
            : Obj.Type == EGridLevelObjectType::MonsterSpawn
                ? FString::Printf (TEXT ("%s Spawner"), *NameText.Replace (TEXT (" Spawn"), TEXT ("")))
                : NameText;
        const FString SuffixText = (Obj.Type == EGridLevelObjectType::ItemSpawn || Obj.Type == EGridLevelObjectType::MonsterSpawn)
            ? TEXT (" [Spawner]")
            : TEXT ("");

        const UEnum* EdgeEnum = StaticEnum<EGridEdge> ();
        const FString EdgeText = EdgeEnum
            ? EdgeEnum->GetDisplayNameTextByValue (static_cast<int64> (Obj.Edge)).ToString ()
            : TEXT ("Unknown");

        return FText::FromString (
            FString::Printf (
                TEXT ("%s @ (%d,%d) %s%s"),
                *SummaryNameText,
                Obj.CellX,
                Obj.CellY,
                *EdgeText,
                *SuffixText));
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

FText SGridEditorLinksPanel::GetSelectedObjectOptionText (
    const TSharedPtr<FGuid>& ObjectId,
    const FText& EmptyText) const
{
    return ObjectId.IsValid ()
        ? GetObjectSummaryText (*ObjectId)
        : EmptyText;
}

bool SGridEditorLinksPanel::CanCreateConnector () const
{
    const auto ContainsGuid = [] (const TArray<TSharedPtr<FGuid>>& Options, const TSharedPtr<FGuid>& Value)
    {
        if (!Value.IsValid ())
        {
            return false;
        }

        for (const TSharedPtr<FGuid>& Option : Options)
        {
            if (Option.IsValid () && *Option == *Value)
            {
                return true;
            }
        }

        return false;
    };

    const auto ContainsEvent = [] (const TArray<TSharedPtr<EGridObjectEvent>>& Options, const TSharedPtr<EGridObjectEvent>& Value)
    {
        if (!Value.IsValid ())
        {
            return false;
        }

        for (const TSharedPtr<EGridObjectEvent>& Option : Options)
        {
            if (Option.IsValid () && *Option == *Value)
            {
                return true;
            }
        }

        return false;
    };

    const auto ContainsCommand = [] (const TArray<TSharedPtr<EGridObjectCommand>>& Options, const TSharedPtr<EGridObjectCommand>& Value)
    {
        if (!Value.IsValid ())
        {
            return false;
        }

        for (const TSharedPtr<EGridObjectCommand>& Option : Options)
        {
            if (Option.IsValid () && *Option == *Value)
            {
                return true;
            }
        }

        return false;
    };

    return ContainsGuid (SourceObjectOptions, SelectedSourceObjectId) &&
        SelectedSourceObjectId->IsValid () &&
        ContainsEvent (LinkSourceEventOptions, SelectedSourceEvent) &&
        ContainsGuid (TargetObjectOptions, SelectedTargetObjectId) &&
        SelectedTargetObjectId->IsValid () &&
        ContainsCommand (LinkCommandOptions, SelectedCommand);
}

void SGridEditorLinksPanel::BuildLinkOptions ()
{
    BuildEventOptions ();
    BuildCommandOptions ();
}

void SGridEditorLinksPanel::BuildObjectOptions ()
{
    SourceObjectOptions.Reset ();
    TargetObjectOptions.Reset ();

    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
    {
        SelectedSourceObjectId.Reset ();
        SelectedTargetObjectId.Reset ();
        return;
    }

    for (const FGridLevelObjectData& Object : CurrentEditorActor->LevelAsset->Objects)
    {
        const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor->FindObjectArchetypeById (Object.ArchetypeId);
        if (CanObjectEmitEvents (Object, Archetype))
        {
            SourceObjectOptions.Add (MakeShared<FGuid> (Object.ObjectId));
        }

        if (CanObjectReceiveCommands (Object, Archetype))
        {
            TargetObjectOptions.Add (MakeShared<FGuid> (Object.ObjectId));
        }
    }

    const auto FindOption = [] (const TArray<TSharedPtr<FGuid>>& Options, const FGuid& ObjectId) -> TSharedPtr<FGuid>
    {
        for (const TSharedPtr<FGuid>& Option : Options)
        {
            if (Option.IsValid () && *Option == ObjectId)
            {
                return Option;
            }
        }

        return nullptr;
    };

    const FGridLevelObjectData* SelectedObject = CurrentEditorActor->GetSelectedObjectData ();
    if (!SelectedSourceObjectId.IsValid () && SelectedObject)
    {
        SelectedSourceObjectId = FindOption (SourceObjectOptions, SelectedObject->ObjectId);
    }
    else if (SelectedSourceObjectId.IsValid ())
    {
        SelectedSourceObjectId = FindOption (SourceObjectOptions, *SelectedSourceObjectId);
    }

    if (SelectedTargetObjectId.IsValid ())
    {
        SelectedTargetObjectId = FindOption (TargetObjectOptions, *SelectedTargetObjectId);
    }
}

void SGridEditorLinksPanel::BuildEventOptions ()
{
    LinkSourceEventOptions.Reset ();

    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const FGridLevelObjectData* SourceObject = CurrentEditorActor && CurrentEditorActor->LevelAsset && SelectedSourceObjectId.IsValid ()
        ? FindObjectById (CurrentEditorActor->LevelAsset, *SelectedSourceObjectId)
        : nullptr;

    if (SourceObject)
    {
        const UGridObjectArchetypeAsset* SourceArchetype = CurrentEditorActor->FindObjectArchetypeById (SourceObject->ArchetypeId);
        for (const EGridObjectEvent Event : GetSupportedEventsForSource (*SourceObject, SourceArchetype))
        {
            LinkSourceEventOptions.Add (MakeShared<EGridObjectEvent> (Event));
        }
    }

    bool bCurrentEventStillValid = false;
    for (const TSharedPtr<EGridObjectEvent>& Option : LinkSourceEventOptions)
    {
        if (Option.IsValid () && SelectedSourceEvent.IsValid () && *Option == *SelectedSourceEvent)
        {
            SelectedSourceEvent = Option;
            bCurrentEventStillValid = true;
            break;
        }
    }

    if (!bCurrentEventStillValid)
    {
        SelectedSourceEvent = LinkSourceEventOptions.Num () > 0 ? LinkSourceEventOptions[0] : nullptr;
    }
}

void SGridEditorLinksPanel::BuildCommandOptions ()
{
    LinkCommandOptions.Reset ();

    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const FGridLevelObjectData* TargetObject = CurrentEditorActor && CurrentEditorActor->LevelAsset && SelectedTargetObjectId.IsValid ()
        ? FindObjectById (CurrentEditorActor->LevelAsset, *SelectedTargetObjectId)
        : nullptr;

    if (TargetObject)
    {
        const UGridObjectArchetypeAsset* TargetArchetype = CurrentEditorActor->FindObjectArchetypeById (TargetObject->ArchetypeId);
        for (const EGridObjectCommand Command : GetSupportedCommandsForTarget (*TargetObject, TargetArchetype))
        {
            LinkCommandOptions.Add (MakeShared<EGridObjectCommand> (Command));
        }
    }

    bool bCurrentCommandStillValid = false;
    for (const TSharedPtr<EGridObjectCommand>& Option : LinkCommandOptions)
    {
        if (Option.IsValid () && SelectedCommand.IsValid () && *Option == *SelectedCommand)
        {
            SelectedCommand = Option;
            bCurrentCommandStillValid = true;
            break;
        }
    }

    if (!bCurrentCommandStillValid)
    {
        SelectedCommand = LinkCommandOptions.Num () > 0 ? LinkCommandOptions[0] : nullptr;
    }
}

void SGridEditorLinksPanel::RefreshConnectorFormOptions ()
{
    BuildObjectOptions ();
    BuildEventOptions ();
    BuildCommandOptions ();
}

TSharedRef<SWidget> SGridEditorLinksPanel::MakeObjectComboWidget (TSharedPtr<FGuid> Item) const
{
    if (!Item.IsValid ())
    {
        return SNew (STextBlock).Text (FText::FromString (TEXT ("Invalid")));
    }

    return SNew (STextBlock).Text (GetObjectSummaryText (*Item));
}

void SGridEditorLinksPanel::OnSourceObjectSelectionChanged (
    TSharedPtr<FGuid> NewValue,
    ESelectInfo::Type SelectInfo)
{
    SelectedSourceObjectId = NewValue;
    BuildEventOptions ();
}

void SGridEditorLinksPanel::OnTargetObjectSelectionChanged (
    TSharedPtr<FGuid> NewValue,
    ESelectInfo::Type SelectInfo)
{
    SelectedTargetObjectId = NewValue;
    BuildCommandOptions ();
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
        SelectedSourceEvent = NewValue;
    }
}

FText SGridEditorLinksPanel::GetSelectedLinkSourceEventText () const
{
    return SelectedSourceEvent.IsValid ()
        ? GetLinkSourceEventText (*SelectedSourceEvent)
        : FText::FromString (TEXT ("Select event"));
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
        SelectedCommand = NewValue;
    }
}

FText SGridEditorLinksPanel::GetSelectedLinkCommandText () const
{
    return SelectedCommand.IsValid ()
        ? GetLinkCommandText (*SelectedCommand)
        : FText::FromString (TEXT ("Select command"));
}

#endif
