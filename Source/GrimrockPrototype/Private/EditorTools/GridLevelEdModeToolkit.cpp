#include "EditorTools/GridLevelEdModeToolkit.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Core/GridObjectArchetypeAsset.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "EditorModeManager.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SNumericEntryBox.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"

#include "Framework/Application/SlateApplication.h"

#include "Engine/Texture2D.h"
#include "Brushes/SlateImageBrush.h"
#include "Widgets/Images/SImage.h"

namespace
{
    FSlateColor GetActionSlateColor (EGridLinkAction Action)
    {
        switch (Action)
        {
            case EGridLinkAction::Open:
            case EGridLinkAction::Activate:
            return FSlateColor (FLinearColor (0.25f, 0.85f, 0.35f, 1.f));

            case EGridLinkAction::Close:
            case EGridLinkAction::Deactivate:
            return FSlateColor (FLinearColor (0.95f, 0.25f, 0.20f, 1.f));

            case EGridLinkAction::Toggle:
            default:
            return FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f));
        }
    }

    FText GetToolGlyph (EGridEditorTool Tool)
    {
        switch (Tool)
        {
            case EGridEditorTool::Select:      return FText::FromString (TEXT ("➤"));
            case EGridEditorTool::PaintCell:   return FText::FromString (TEXT ("▦"));
            case EGridEditorTool::PaintWall:   return FText::FromString (TEXT ("▤"));
            case EGridEditorTool::PaintObject: return FText::FromString (TEXT ("▣"));
            case EGridEditorTool::Erase:       return FText::FromString (TEXT ("⌫"));
            case EGridEditorTool::Link:        return FText::FromString (TEXT ("⛓"));
            default:                           return FText::FromString (TEXT ("?"));
        }
    }

    FText GetObjectGlyph (EGridLevelObjectType Type)
    {
        switch (Type)
        {
            case EGridLevelObjectType::Door:          return FText::FromString (TEXT ("▥"));
            case EGridLevelObjectType::Button:        return FText::FromString (TEXT ("●"));
            case EGridLevelObjectType::Lever:         return FText::FromString (TEXT ("◒"));
            case EGridLevelObjectType::PressurePlate: return FText::FromString (TEXT ("▧"));
            case EGridLevelObjectType::Teleporter:    return FText::FromString (TEXT ("◎"));
            case EGridLevelObjectType::Trigger:       return FText::FromString (TEXT ("⌖"));
            case EGridLevelObjectType::MonsterSpawn:  return FText::FromString (TEXT ("☠"));
            case EGridLevelObjectType::ItemSpawn:     return FText::FromString (TEXT ("◈"));
            case EGridLevelObjectType::Decoration:    return FText::FromString (TEXT ("◉"));
            case EGridLevelObjectType::Light:         return FText::FromString (TEXT ("♨"));
            default:                                  return FText::FromString (TEXT ("?"));
        }
    }

    FText GetEnumDisplayText (const UEnum* Enum, int64 Value)
    {
        return Enum
            ? Enum->GetDisplayNameTextByValue (Value)
            : FText::FromString (TEXT ("Unknown"));
    }
}

void FGridLevelEdModeToolkit::Init (const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
    BuildTriggerModeOptions ();
    ToolkitWidget = BuildToolkitWidget ();
    FModeToolkit::Init (InitToolkitHost);
}

FName FGridLevelEdModeToolkit::GetToolkitFName () const
{
    return FName ("GridLevelEdModeToolkit");
}

FText FGridLevelEdModeToolkit::GetBaseToolkitName () const
{
    return FText::FromString (TEXT ("Grimrock Grid Palette"));
}

FEdMode* FGridLevelEdModeToolkit::GetEditorMode () const
{
    return GLevelEditorModeTools ().GetActiveMode (FGridLevelEdMode::EM_GridLevelEdModeId);
}

TSharedPtr<SWidget> FGridLevelEdModeToolkit::GetInlineContent () const
{
    return ToolkitWidget;
}

AGridLevelEditorActor* FGridLevelEdModeToolkit::GetEditorActor () const
{
    if (!GEditor)
    {
        return nullptr;
    }

    UWorld* World = GEditor->GetEditorWorldContext ().World ();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AGridLevelEditorActor> It (World); It; ++It)
    {
        return *It;
    }
    return nullptr;
}

void FGridLevelEdModeToolkit::RefreshPalette ()
{
    if (!ToolkitRoot.IsValid ())
    {
        return;
    }

    ToolkitRoot->ClearChildren ();

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildHeaderSection ()
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildPanelSection (FText::FromString (TEXT ("TOOLS")), BuildToolSection ())
        ];

    const AGridLevelEditorActor* EditorActor = GetEditorActor ();

    if (EditorActor && EditorActor->ActiveTool == EGridEditorTool::PaintObject)
    {
        ToolkitRoot->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 0.f, 0.f, 8.f)
            [
                BuildPanelSection (FText::FromString (TEXT ("PALETTE")), BuildPaletteSection ())
            ];
    }

    const FGridLevelObjectData* Obj = EditorActor ? EditorActor->GetSelectedObjectData () : nullptr;

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildPanelSection (FText::FromString (TEXT ("SELECTED OBJECT")), BuildObjectInspectorSection ())
        ];

    if (Obj)
    {
        ToolkitRoot->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 0.f, 0.f, 8.f)
            [
                BuildPanelSection (FText::FromString (TEXT ("BEHAVIOR EDITOR")), BuildBehaviorEditorSection ())
            ];

        ToolkitRoot->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 0.f, 0.f, 8.f)
            [
                BuildPanelSection (FText::FromString (TEXT ("LINKS")), BuildLinksSection (*Obj))
            ];
    }
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildToolkitWidget ()
{
    ToolkitRoot = SNew (SVerticalBox);

    TSharedRef<SWidget> Widget = SNew (SBorder).Padding (8.f)
        [SNew (SScrollBox) + SScrollBox::Slot ()[ToolkitRoot.ToSharedRef ()]];

    RefreshPalette ();

    return Widget;
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildToolSection ()
{
    return SNew (SHorizontalBox)
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f)
        [BuildToolTile (FText::FromString (TEXT ("Select")), GetToolGlyph (EGridEditorTool::Select), EGridEditorTool::Select)]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f)
        [BuildToolTile (FText::FromString (TEXT ("Paint Cell")), GetToolGlyph (EGridEditorTool::PaintCell), EGridEditorTool::PaintCell)]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f)
        [BuildToolTile (FText::FromString (TEXT ("Paint Wall")), GetToolGlyph (EGridEditorTool::PaintWall), EGridEditorTool::PaintWall)]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f)
        [BuildToolTile (FText::FromString (TEXT ("Paint Object")), GetToolGlyph (EGridEditorTool::PaintObject), EGridEditorTool::PaintObject)]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f)
        [BuildToolTile (FText::FromString (TEXT ("Erase")), GetToolGlyph (EGridEditorTool::Erase), EGridEditorTool::Erase)]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f)
        [BuildToolTile (FText::FromString (TEXT ("Link")), GetToolGlyph (EGridEditorTool::Link), EGridEditorTool::Link)];
}

UTexture2D* FGridLevelEdModeToolkit::GetToolIcon (EGridEditorTool Tool) const
{
    switch (Tool)
    {
        case EGridEditorTool::Select:
        return LoadObject<UTexture2D> (nullptr, TEXT ("/Game/GrimrockPrototype/Icons/T_Tool_Select.T_Tool_Select"));

        case EGridEditorTool::PaintCell:
        return LoadObject<UTexture2D> (nullptr, TEXT ("/Game/GrimrockPrototype/Icons/T_Tool_PaintCell.T_Tool_PaintCell"));

        case EGridEditorTool::PaintWall:
        return LoadObject<UTexture2D> (nullptr, TEXT ("/Game/GrimrockPrototype/Icons/T_Tool_PaintWall.T_Tool_PaintWall"));

        case EGridEditorTool::PaintObject:
        return LoadObject<UTexture2D> (nullptr, TEXT ("/Game/GrimrockPrototype/Icons/T_Tool_PaintObject.T_Tool_PaintObject"));

        case EGridEditorTool::Erase:
        return LoadObject<UTexture2D> (nullptr, TEXT ("/Game/GrimrockPrototype/Icons/T_Tool_Erase.T_Tool_Erase"));

        case EGridEditorTool::Link:
        return LoadObject<UTexture2D> (nullptr, TEXT ("/Game/GrimrockPrototype/Icons/T_Tool_Link.T_Tool_Link"));

        default:
        return nullptr;
    }
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildToolTile (const FText& Label, const FText& Glyph, EGridEditorTool ToolValue)
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
    const bool bSelected = EditorActor && EditorActor->ActiveTool == ToolValue;

    UTexture2D* IconTexture = GetToolIcon (ToolValue);

    const float TileWidth = 80.f;
    const float TileHeight = 80.f;
    const float IconSize = 64.f;

    const FLinearColor NormalColor = FLinearColor (0.25f, 0.22f, 0.18f, 1.f);
    const FLinearColor HoverColor = FLinearColor (0.52f, 0.46f, 0.38f, 1.f);
    const FLinearColor SelectedColor = FLinearColor (0.f, 0.85f, 1.f, 0.85f);

    TSharedPtr<SBorder> TileBorder;

    TSharedRef<SWidget> ButtonContent =
        SAssignNew (TileBorder, SBorder).Padding (2.f).BorderImage (bSelected
            ? FAppStyle::GetBrush ("FocusRectangle")
            : FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        .BorderBackgroundColor (bSelected ? SelectedColor : NormalColor)
        [SNew (SBox).WidthOverride (TileWidth).HeightOverride (TileHeight)
        [SNew (SVerticalBox) + SVerticalBox::Slot ()
        .FillHeight (1.f)
        .HAlign (HAlign_Center)
        .VAlign (VAlign_Center)
        [IconTexture 
        ? StaticCastSharedRef<SWidget> (SNew (SImage)
            .Image (GetOrCreateBrush (IconTexture, IconSize)))
        : StaticCastSharedRef<SWidget> (SNew (STextBlock).Text (Glyph)
            .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 34))
        )]
    + SVerticalBox::Slot ()
        .AutoHeight ()
        .HAlign (HAlign_Center)
        .Padding (0.f, 2.f, 0.f, 2.f)
        [SNew (STextBlock).Text (Label).Justification (ETextJustify::Center)]
        ]];
    return SNew (SButton).ButtonStyle (FAppStyle::Get (), "NoBorder").ContentPadding (0.f)
        .Cursor (EMouseCursor::Hand).OnHovered_Lambda ([TileBorder, bSelected, HoverColor] ()
    {
        if (TileBorder.IsValid () && !bSelected)
        {
            TileBorder->SetBorderBackgroundColor (HoverColor);
        }
    })
        .OnUnhovered_Lambda ([TileBorder, bSelected, NormalColor] ()
    {
        if (TileBorder.IsValid () && !bSelected)
        {
            TileBorder->SetBorderBackgroundColor (NormalColor);
        }
    })
        .OnClicked_Lambda ([this, ToolValue] () -> FReply
    {
        return OnToolClicked (static_cast<int32>(ToolValue));
    })
        [ButtonContent];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildIconOrFallback (UTexture2D* Icon, EGridLevelObjectType FallbackType, float Size) const
{
    if (Icon)
    {
        return SNew (SBox).WidthOverride (Size).HeightOverride (Size)
            [SNew (SImage)
            .Image (new FSlateImageBrush (Icon, FVector2D (Size, Size)))
            ];
    }

    return SNew (SBox).WidthOverride (Size).HeightOverride (Size)
        .HAlign (HAlign_Center).VAlign (VAlign_Center)
        [SNew (STextBlock).Text (GetObjectGlyph (FallbackType))
        .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 30))
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildPaletteSection ()
{
    AGridLevelEditorActor* EditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (!EditorActor)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No GridLevelEditorActor found.")))
            ];
        return Root;
    }

    if (!EditorActor->ObjectPalette)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No ObjectPalette assigned.")))
            ];
        return Root;
    }

    TSharedRef<SUniformGridPanel> Grid = SNew (SUniformGridPanel)
        .SlotPadding (FMargin (4.f));

    int32 Index = 0;

    for (const FGridObjectPaletteEntry& Entry : EditorActor->ObjectPalette->Entries)
    {
        const int32 Row = Index / 5;
        const int32 Column = Index % 5;

        Grid->AddSlot (Column, Row)
            [
                BuildPaletteTile (Entry)
            ];

        ++Index;
    }

    Root->AddSlot ().AutoHeight ()
        [
            Grid
        ];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            SNew (STextBlock)
                .Text_Lambda ([this] ()
            {
                return FText::Format (
                    FText::FromString (TEXT ("Selected Palette Entry: {0}")),
                    GetSelectedPaletteEntryText ());
            })
        ];

    return Root;
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildPaletteTile (const FGridObjectPaletteEntry& Entry)
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();

    const bool bSelected =
        EditorActor &&
        EditorActor->SelectedPaletteEntryId == Entry.EntryId;

    const FText Label = Entry.DisplayName.IsEmpty ()
        ? FText::FromName (Entry.EntryId)
        : Entry.DisplayName;

    const FName EntryId = Entry.EntryId;
    UTexture2D* IconTexture = Entry.Icon.Get ();

    return SNew (SButton)
        .ContentPadding (6.f)
        .ButtonColorAndOpacity (bSelected
            ? FLinearColor (0.10f, 0.45f, 0.55f, 1.f)
            : FLinearColor (0.07f, 0.07f, 0.07f, 1.f))
        .OnClicked_Lambda ([this, EntryId] () -> FReply
    {
        return OnPaletteEntryClicked (EntryId);
    })
        [
            SNew (SBox)
                .WidthOverride (112.f)
                .HeightOverride (96.f)
                [
                    SNew (SVerticalBox)

                        + SVerticalBox::Slot ()
                        .FillHeight (1.f)
                        .HAlign (HAlign_Center)
                        .VAlign (VAlign_Center)
                        [
                            BuildIconOrFallback (IconTexture, Entry.ObjectType, 52.f)
                        ]

                        + SVerticalBox::Slot ()
                        .AutoHeight ()
                        .HAlign (HAlign_Center)
                        .Padding (0.f, 4.f, 0.f, 0.f)
                        [
                            SNew (STextBlock)
                                .Text (Label)
                                .Justification (ETextJustify::Center)
                        ]
                ]
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildHeaderSection ()
{
    return SNew (SBorder)
        .Padding (8.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ()
                .FillWidth (1.f)
                .VAlign (VAlign_Center)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("DUNGEON EDITOR")))
                        .Font (FAppStyle::GetFontStyle ("HeadingMedium"))
                ]

                + SHorizontalBox::Slot ()
                .AutoWidth ()
                .VAlign (VAlign_Center)
                [
                    SNew (STextBlock)
                        .Text_Lambda ([this] ()
                    {
                        return FText::Format (
                            FText::FromString (TEXT ("Active Tool: {0}")),
                            GetActiveToolText ());
                    })
                ]];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildPanelSection (
    const FText& Title,
    TSharedRef<SWidget> Content)
{
    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ()
                .AutoHeight ()
                .Padding (0.f, 0.f, 0.f, 6.f)
                [
                    SNew (STextBlock)
                        .Text (Title)
                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                ]

                + SVerticalBox::Slot ()
                .AutoHeight ()
                [
                    Content
                ]
        ];
}

FReply FGridLevelEdModeToolkit::OnToolClicked (int32 ToolValue)
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();
        EditorActor->ActiveTool = static_cast<EGridEditorTool>(ToolValue);
        RefreshPalette ();
    }

    return FReply::Handled ();
}

FReply FGridLevelEdModeToolkit::OnPaletteEntryClicked (FName EntryId)
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();
        EditorActor->ApplyPaletteEntry (EntryId);
        EditorActor->ActiveTool = EGridEditorTool::PaintObject;
        RefreshPalette ();
    }
    return FReply::Handled ();
}

FText FGridLevelEdModeToolkit::GetSelectedPaletteEntryText () const
{
    if (const AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        return EditorActor->SelectedPaletteEntryId.IsNone ()
            ? FText::FromString (TEXT ("None"))
            : FText::FromName (EditorActor->SelectedPaletteEntryId);
    }
    return FText::FromString (TEXT ("None"));
}

FText FGridLevelEdModeToolkit::GetActiveToolText () const
{
    if (const AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        const UEnum* Enum = StaticEnum<EGridEditorTool> ();
        if (Enum)
        {
            return Enum->GetDisplayNameTextByValue (static_cast<int64>(EditorActor->ActiveTool));
        }
    }
    return FText::FromString (TEXT ("Unknown"));
}

FReply FGridLevelEdModeToolkit::OnApplyBehaviorClicked ()
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();

        if (EditorActor->ApplyBehaviorToSelectedObject (EditedBehavior))
        {
            RefreshPalette ();
        }
    }

    return FReply::Handled ();
}

FReply FGridLevelEdModeToolkit::OnRemoveExactLinkClicked (
    FGuid SourceObjectId,
    FGuid TargetObjectId,
    EGridLinkAction Action)
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();
        EditorActor->RemoveExactLink (SourceObjectId, TargetObjectId, Action);
		RefreshPalette ();
    }

    return FReply::Handled ();
}

FReply FGridLevelEdModeToolkit::OnClearSelectedObjectLinksClicked ()
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();
        EditorActor->RemoveAllLinksForSelectedObject ();
		RefreshPalette ();
    }

    return FReply::Handled ();
}

FText FGridLevelEdModeToolkit::GetObjectSummaryText (const FGuid& ObjectId) const
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();

    if (!EditorActor || !EditorActor->LevelAsset || !ObjectId.IsValid ())
    {
        return FText::FromString (TEXT ("Invalid object"));
    }
    for (const FGridLevelObjectData& Obj : EditorActor->LevelAsset->Objects)
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

FText FGridLevelEdModeToolkit::GetLinkActionText (EGridLinkAction Action) const
{
    const UEnum* Enum = StaticEnum<EGridLinkAction> ();

    return Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (Action))
        : FText::FromString (TEXT ("Unknown"));
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildObjectInspectorSection ()
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (!EditorActor || !EditorActor->LevelAsset)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No editor actor or level asset.")))
            ];
        return Root;
    }

    const FGridLevelObjectData* Obj = EditorActor->GetSelectedObjectData ();
    if (!Obj)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No selected object.")))
            ];
        return Root;
    }

    SyncEditedBehaviorFromSelection ();

    Root->AddSlot ().AutoHeight ()
        [
            BuildSelectedObjectCard (*Obj)
        ];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 4.f, 0.f)
                [
                    SNew (SButton)
                        .Text (FText::FromString (TEXT ("Focus Selected Object")))
                        .OnClicked_Lambda ([this] () -> FReply
                    {
                        return OnFocusSelectedObjectClicked ();
                    })
                ]

            + SHorizontalBox::Slot ().AutoWidth ()
                [
                    SNew (SButton)
                        .Text (FText::FromString (TEXT ("Apply Selected Object")))
                        .OnClicked (this, &FGridLevelEdModeToolkit::OnApplySelectedObjectClicked)
                ]
        ];

    return Root;
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildSelectedObjectCard (const FGridLevelObjectData& Obj)
{
    const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
    const UEnum* EdgeEnum = StaticEnum<EGridEdge> ();

    const FText TypeText = GetEnumDisplayText (TypeEnum, static_cast<int64>(Obj.Type));
    const FText EdgeText = GetEnumDisplayText (EdgeEnum, static_cast<int64>(Obj.Edge));

    return SNew (SBorder)
        .Padding (8.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ()
                .AutoWidth ()
                .VAlign (VAlign_Center)
                .Padding (0.f, 0.f, 12.f, 0.f)
                [
                    SNew (SBox)
                        .WidthOverride (112.f)
                        .HeightOverride (88.f)
                        [
                            SNew (SBorder)
                                .Padding (4.f)
                                .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
                                [
                                    SNew (STextBlock)
                                        .Text (GetObjectGlyph (Obj.Type))
                                        .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 42))
                                        .Justification (ETextJustify::Center)
                                ]
                        ]
                ]

            + SHorizontalBox::Slot ()
                .FillWidth (1.f)
                .VAlign (VAlign_Center)
                [
                    SNew (SVerticalBox)

                        + SVerticalBox::Slot ().AutoHeight ()
                        [
                            SNew (STextBlock)
                                .Text (FText::Format (
                                    FText::FromString (TEXT ("{0} @ ({1},{2}) {3}")),
                                    TypeText,
                                    FText::AsNumber (Obj.CellX),
                                    FText::AsNumber (Obj.CellY),
                                    EdgeText))
                                .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 20))
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
                        [
                            SNew (STextBlock)
                                .Text (FText::Format (
                                    FText::FromString (
                                        TEXT ("Type: {0}\nPosition: ({1},{2})\nFacing: {3}\nTag: {4}\nArchetype: {5}")),
                                    TypeText,
                                    FText::AsNumber (Obj.CellX),
                                    FText::AsNumber (Obj.CellY),
                                    EdgeText,
                                    FText::FromName (Obj.Tag),
                                    FText::FromName (Obj.ArchetypeId)))
                        ]
                ]
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildLinksSection (const FGridLevelObjectData& SelectedObject)
{
    return SNew (SVerticalBox)

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
                                        .Text (FText::FromString (TEXT ("OUTGOING LINKS")))
                                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                                ]

                                + SVerticalBox::Slot ().AutoHeight ()
                                [
                                    BuildObjectLinksList (SelectedObject, true)
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
                                        .Text (FText::FromString (TEXT ("INCOMING LINKS")))
                                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                                ]

                                + SVerticalBox::Slot ().AutoHeight ()
                                [
                                    BuildObjectLinksList (SelectedObject, false)
                                ]
                        ]
                ]
        ]

    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
        [
            SNew (SButton)
                .Text (FText::FromString (TEXT ("Clear All Links For Selected Object")))
                .OnClicked (this, &FGridLevelEdModeToolkit::OnClearSelectedObjectLinksClicked)
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildObjectLinksList (
    const FGridLevelObjectData& SelectedObject,
    bool bOutgoing) const
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (!EditorActor || !EditorActor->LevelAsset)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No level asset.")))
            ];
        return Root;
    }

    int32 Count = 0;

    for (const FGridLevelLinkData& Link : EditorActor->LevelAsset->Links)
    {
        const bool bMatches = bOutgoing
            ? Link.SourceObjectId == SelectedObject.ObjectId
            : Link.TargetObjectId == SelectedObject.ObjectId;

        if (!bMatches)
        {
            continue;
        }

        const FGuid OtherId = bOutgoing ? Link.TargetObjectId : Link.SourceObjectId;
        const FString ArrowText = bOutgoing ? TEXT ("→") : TEXT ("←");

        Root->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 2.f)
            [
                SNew (SHorizontalBox)

                    + SHorizontalBox::Slot ()
                    .AutoWidth ()
                    .VAlign (VAlign_Center)
                    .Padding (0.f, 0.f, 6.f, 0.f)
                    [
                        SNew (STextBlock)
                            .Text (FText::FromString (ArrowText))
                            .ColorAndOpacity (GetActionSlateColor (Link.Action))
                            .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 20))
                    ]

                    + SHorizontalBox::Slot ()
                    .FillWidth (1.f)
                    .VAlign (VAlign_Center)
                    [
                        SNew (STextBlock)
                            .Text (FText::Format (
                                FText::FromString (TEXT ("{0}   [{1}]")),
                                GetObjectSummaryText (OtherId),
                                GetLinkActionText (Link.Action)))
                            .ColorAndOpacity (GetActionSlateColor (Link.Action))
                    ]

                + SHorizontalBox::Slot ()
                    .AutoWidth ()
                    .Padding (4.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (bOutgoing
                                ? FText::FromString (TEXT ("Target"))
                                : FText::FromString (TEXT ("Source")))
                            .OnClicked_Lambda ([this, OtherId] () -> FReply
                        {
                            return const_cast<FGridLevelEdModeToolkit*>(this)->OnSelectObjectFromLinkClicked (OtherId);
                        })
                    ]

                + SHorizontalBox::Slot ()
                    .AutoWidth ()
                    .Padding (4.f, 0.f)
                    [
                        SNew (SButton)
                            .Text (FText::FromString (TEXT ("X")))
                            .OnClicked_Lambda ([this, Link] () -> FReply
                        {
                            return const_cast<FGridLevelEdModeToolkit*>(this)->OnRemoveExactLinkClicked (
                                Link.SourceObjectId,
                                Link.TargetObjectId,
                                Link.Action);
                        })
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

FText FGridLevelEdModeToolkit::GetSelectedObjectDetailsText () const
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
    if (!EditorActor)
    {
        return FText::FromString (TEXT ("No selected object."));
    }

    const FGridLevelObjectData* Obj = EditorActor->GetSelectedObjectData ();
    if (!Obj)
    {
        return FText::FromString (TEXT ("No selected object."));
    }

    const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
    const FString TypeText = TypeEnum
        ? TypeEnum->GetDisplayNameTextByValue (static_cast<int64> (Obj->Type)).ToString ()
        : TEXT ("Unknown");

    const UEnum* EdgeEnum = StaticEnum<EGridEdge> ();
    const FString EdgeText = EdgeEnum
        ? EdgeEnum->GetDisplayNameTextByValue (static_cast<int64> (Obj->Edge)).ToString ()
        : TEXT ("Unknown");

    const UEnum* BehaviorEnum = StaticEnum<EGridObjectTriggerMode> ();
    const FString BehaviorText = BehaviorEnum
        ? BehaviorEnum->GetDisplayNameTextByValue (static_cast<int64> (Obj->Behavior.TriggerMode)).ToString ()
        : TEXT ("Unknown");

    return FText::FromString (
        FString::Printf (
            TEXT ("Type: %s\nId: %s\nCell: X=%d Y=%d\nEdge: %s\nArchetype: %s\nPalette: %s\nEnabled: %s\nActive: %s\nTag: %s\nNotes: %s\nBehavior: %s\nDelay: %.2f\nDuration: %.2f\nInvert Links: %s\nFire On Enter: %s\nFire On Exit: %s"),
            *TypeText,
            *Obj->ObjectId.ToString (),
            Obj->CellX,
            Obj->CellY,
            *EdgeText,
            *Obj->ArchetypeId.ToString (),
            *Obj->PaletteEntryId.ToString (),
            Obj->bInitiallyEnabled ? TEXT ("true") : TEXT ("false"),
            Obj->bInitiallyActive ? TEXT ("true") : TEXT ("false"),
            *Obj->Tag.ToString (),
            *Obj->Notes,
            *BehaviorText,
            Obj->Behavior.Delay,
            Obj->Behavior.Duration,
            Obj->Behavior.bInvertLinks ? TEXT ("true") : TEXT ("false"),
            Obj->Behavior.bFireOnEnter ? TEXT ("true") : TEXT ("false"),
            Obj->Behavior.bFireOnExit ? TEXT ("true") : TEXT ("false")));
}

FReply FGridLevelEdModeToolkit::OnSelectObjectFromLinkClicked (FGuid ObjectId)
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();

        if (EditorActor->SelectObjectById (ObjectId))
        {
            RefreshPalette ();
        }
    }

    return FReply::Handled ();
}

FReply FGridLevelEdModeToolkit::OnFocusSelectedObjectClicked ()
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->FocusSelectedObject ();
        RefreshPalette ();
    }

    return FReply::Handled ();
}

void FGridLevelEdModeToolkit::BuildTriggerModeOptions ()
{
    TriggerModeOptions.Reset ();

    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Instant));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Hold));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::Toggle));
    TriggerModeOptions.Add (MakeShared<EGridObjectTriggerMode> (EGridObjectTriggerMode::OneShot));
}

void FGridLevelEdModeToolkit::SyncEditedBehaviorFromSelection ()
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
    if (!EditorActor)
    {
        CachedBehaviorObjectId.Invalidate ();
        EditedBehavior = FGridObjectBehaviorParams ();
        return;
    }

    const FGridLevelObjectData* Obj = EditorActor->GetSelectedObjectData ();
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

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildBehaviorEditorSection ()
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
                                        .OnGenerateWidget (this, &FGridLevelEdModeToolkit::MakeTriggerModeComboWidget)
                                        .OnSelectionChanged (this, &FGridLevelEdModeToolkit::OnTriggerModeSelectionChanged)
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
                                        .Value (this, &FGridLevelEdModeToolkit::GetEditedDelay)
                                        .OnValueChanged (this, &FGridLevelEdModeToolkit::OnEditedDelayChanged)
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
                                        .Value (this, &FGridLevelEdModeToolkit::GetEditedDuration)
                                        .OnValueChanged (this, &FGridLevelEdModeToolkit::OnEditedDurationChanged)
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
                        [
                            SNew (SButton)
                                .Text (FText::FromString (TEXT ("APPLY BEHAVIOR")))
                                .HAlign (HAlign_Center)
                                .OnClicked (this, &FGridLevelEdModeToolkit::OnApplyBehaviorClicked)
                        ]
                ]

            + SHorizontalBox::Slot ().FillWidth (0.45f)
                [
                    SNew (SVerticalBox)

                        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (this, &FGridLevelEdModeToolkit::GetEditedInvertLinksCheckState)
                                .OnCheckStateChanged (this, &FGridLevelEdModeToolkit::OnEditedInvertLinksChanged)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Invert Links")))
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (this, &FGridLevelEdModeToolkit::GetEditedFireOnEnterCheckState)
                                .OnCheckStateChanged (this, &FGridLevelEdModeToolkit::OnEditedFireOnEnterChanged)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Fire On Enter")))
                                ]
                        ]

                    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked (this, &FGridLevelEdModeToolkit::GetEditedFireOnExitCheckState)
                                .OnCheckStateChanged (this, &FGridLevelEdModeToolkit::OnEditedFireOnExitChanged)
                                [
                                    SNew (STextBlock).Text (FText::FromString (TEXT ("Fire On Exit")))
                                ]
                        ]
                ]
        ];
}

TOptional<float> FGridLevelEdModeToolkit::GetEditedDelay () const
{
    return EditedBehavior.Delay;
}

TOptional<float> FGridLevelEdModeToolkit::GetEditedDuration () const
{
    return EditedBehavior.Duration;
}

void FGridLevelEdModeToolkit::OnEditedDelayChanged (float NewValue)
{
    EditedBehavior.Delay = FMath::Max (0.f, NewValue);
}

void FGridLevelEdModeToolkit::OnEditedDurationChanged (float NewValue)
{
    EditedBehavior.Duration = FMath::Max (0.f, NewValue);
}

ECheckBoxState FGridLevelEdModeToolkit::GetEditedInvertLinksCheckState () const
{
    return EditedBehavior.bInvertLinks ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState FGridLevelEdModeToolkit::GetEditedFireOnEnterCheckState () const
{
    return EditedBehavior.bFireOnEnter ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState FGridLevelEdModeToolkit::GetEditedFireOnExitCheckState () const
{
    return EditedBehavior.bFireOnExit ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FGridLevelEdModeToolkit::OnEditedInvertLinksChanged (ECheckBoxState NewState)
{
    EditedBehavior.bInvertLinks = NewState == ECheckBoxState::Checked;
}

void FGridLevelEdModeToolkit::OnEditedFireOnEnterChanged (ECheckBoxState NewState)
{
    EditedBehavior.bFireOnEnter = NewState == ECheckBoxState::Checked;
}

void FGridLevelEdModeToolkit::OnEditedFireOnExitChanged (ECheckBoxState NewState)
{
    EditedBehavior.bFireOnExit = NewState == ECheckBoxState::Checked;
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::MakeTriggerModeComboWidget (
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

void FGridLevelEdModeToolkit::OnTriggerModeSelectionChanged (
    TSharedPtr<EGridObjectTriggerMode> NewValue,
    ESelectInfo::Type SelectInfo)
{
    if (NewValue.IsValid ())
    {
        EditedBehavior.TriggerMode = *NewValue;
    }
}

FText FGridLevelEdModeToolkit::GetSelectedTriggerModeText () const
{
    const UEnum* Enum = StaticEnum<EGridObjectTriggerMode> ();

    return Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (EditedBehavior.TriggerMode))
        : FText::FromString (TEXT ("Unknown"));
}


FReply FGridLevelEdModeToolkit::OnApplySelectedObjectClicked ()
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();

        if (EditorActor->ApplyEditedSelectedObject ())
        {
            RefreshPalette ();
        }
    }

    return FReply::Handled ();
}

const FSlateBrush* FGridLevelEdModeToolkit::GetOrCreateBrush (UTexture2D* Texture, float Size)
{
    if (!Texture)
    {
        return nullptr;
    }
    if (TSharedPtr<FSlateBrush>* Existing = CachedIconBrushes.Find (Texture))
    {
        return Existing->Get ();
    }
    TSharedPtr<FSlateBrush> NewBrush = MakeShared<FSlateImageBrush> (Texture, FVector2D (Size, Size));
    CachedIconBrushes.Add (Texture, NewBrush);
    return NewBrush.Get ();
}

#endif