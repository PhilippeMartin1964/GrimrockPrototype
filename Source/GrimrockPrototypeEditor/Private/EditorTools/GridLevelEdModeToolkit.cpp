#include "EditorTools/GridLevelEdModeToolkit.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/Widgets/SGridEditorBehaviorPanel.h"
#include "EditorTools/Widgets/SGridEditorLinksPanel.h"
#include "EditorTools/Widgets/SGridEditorObjectInspectorPanel.h"
#include "EditorTools/Widgets/SGridEditorOverviewMapPanel.h"
#include "EditorTools/Widgets/SGridEditorValidationPanel.h"
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

}

void FGridLevelEdModeToolkit::Init (const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
    ValidationState = MakeShared<FGridEditorValidationPanelState> ();
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

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildPanelSection (
                FText::FromString (TEXT ("OVERVIEW MAP")),
                SNew (SGridEditorOverviewMapPanel)
                    .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                    .OnGetEditorActor (FOnGetGridEditorActor::CreateLambda ([this] ()
                    {
                        return GetEditorActor ();
                    }))
                    .OnRequestRefresh (FOnGridEditorOverviewRequestRefresh::CreateLambda ([this] ()
                    {
                        RefreshPalette ();
                    })))
        ];

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
            BuildPanelSection (
                FText::FromString (TEXT ("SELECTED OBJECT")),
                SNew (SGridEditorObjectInspectorPanel)
                    .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                    .OnGetEditorActor (FOnGetGridEditorObjectInspectorActor::CreateLambda ([this] ()
                    {
                        return GetEditorActor ();
                    }))
                    .OnRequestRefresh (FOnGridEditorObjectInspectorRequestRefresh::CreateLambda ([this] ()
                    {
                        RefreshPalette ();
                    })))
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildPanelSection (
                FText::FromString (TEXT ("VALIDATION")),
                SNew (SGridEditorValidationPanel)
                    .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                    .ValidationState (ValidationState)
                    .OnGetEditorActor (FOnGetGridEditorValidationActor::CreateLambda ([this] ()
                    {
                        return GetEditorActor ();
                    }))
                    .OnRequestRefresh (FOnGridEditorValidationRequestRefresh::CreateLambda ([this] ()
                    {
                        RefreshPalette ();
                    })))
        ];

    if (Obj)
    {
        if (Obj->Type != EGridLevelObjectType::Trigger)
        {
            ToolkitRoot->AddSlot ()
                .AutoHeight ()
                .Padding (0.f, 0.f, 0.f, 8.f)
                [
                    BuildPanelSection (
                        FText::FromString (TEXT ("BEHAVIOR EDITOR")),
                        SNew (SGridEditorBehaviorPanel)
                            .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                            .OnGetEditorActor (FOnGetGridEditorBehaviorActor::CreateLambda ([this] ()
                            {
                                return GetEditorActor ();
                            }))
                            .OnRequestRefresh (FOnGridEditorBehaviorRequestRefresh::CreateLambda ([this] ()
                            {
                                RefreshPalette ();
                            })))
                ];
        }

        ToolkitRoot->AddSlot ()
            .AutoHeight ()
            .Padding (0.f, 0.f, 0.f, 8.f)
            [
                BuildPanelSection (
                    FText::FromString (TEXT ("LINKS")),
                    SNew (SGridEditorLinksPanel)
                        .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                        .OnGetEditorActor (FOnGetGridEditorLinksActor::CreateLambda ([this] ()
                        {
                            return GetEditorActor ();
                        }))
                        .OnRequestRefresh (FOnGridEditorLinksRequestRefresh::CreateLambda ([this] ()
                        {
                            RefreshPalette ();
                        })))
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

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildIconOrFallback (UTexture2D* Icon, EGridLevelObjectType FallbackType, float Size)
{
    if (Icon)
    {
        return SNew (SBox).WidthOverride (Size).HeightOverride (Size)
            [SNew (SImage)
            .Image (GetOrCreateBrush (Icon, Size))
            ];
    }

    return SNew (SBox).WidthOverride (Size).HeightOverride (Size)
        .HAlign (HAlign_Center).VAlign (VAlign_Center)
        [SNew (STextBlock).Text (GridEditorWidgetHelpers::GetGridObjectGlyph (FallbackType))
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
            [SNew (STextBlock).Text (FText::FromString (TEXT ("No GridLevelEditorActor found.")))];
        return Root;
    }
    if (!EditorActor->ObjectPalette)
    {
        Root->AddSlot ().AutoHeight ()
            [SNew (STextBlock).Text (FText::FromString (TEXT ("No ObjectPalette assigned.")))];
        return Root;
    }
    TSharedRef<SUniformGridPanel> Grid = SNew (SUniformGridPanel).SlotPadding (FMargin (4.f));
    int32 Index = 0;

    for (const FGridObjectPaletteEntry& Entry : EditorActor->ObjectPalette->Entries)
    {
        const int32 Row = Index / 5;
        const int32 Column = Index % 5;

        Grid->AddSlot (Column, Row)[BuildPaletteTile (Entry)];
        ++Index;
    }
    Root->AddSlot ().AutoHeight ()[Grid];
    Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [SNew (STextBlock).Text_Lambda ([this] ()
    {
        return FText::Format (FText::FromString (TEXT ("Selected Palette Entry: {0}")),
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
        .Padding (FMargin (6.f, 4.f))
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SWrapBox)

            + SWrapBox::Slot ().Padding (0.f, 0.f, 8.f, 3.f)
            [
                SNew (STextBlock)
                    .Text (FText::FromString (TEXT ("DUNGEON EDITOR")))
                    .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 13))
            ]

            + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 3.f)
            [
                GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                    FText::FromString (TEXT ("Tool")),
                    GetActiveToolText (),
                    FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f)))
            ]

            + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 3.f)
            [
                GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                    FText::FromString (TEXT ("Cell")),
                    GetSelectedCellStatusText (),
                    FSlateColor (FLinearColor (0.40f, 0.85f, 0.45f, 1.f)))
            ]

            + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 3.f)
            [
                GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                    FText::FromString (TEXT ("Edge/Facing")),
                    GetSelectedEdgeStatusText (),
                    FSlateColor (FLinearColor (1.f, 0.72f, 0.20f, 1.f)))
            ]

            + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 3.f)
            [
                GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                    FText::FromString (TEXT ("Object")),
                    GetSelectedObjectStatusText (),
                    FSlateColor (FLinearColor (0.70f, 0.55f, 1.f, 1.f)))
            ]

            + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 3.f)
            [
                GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                    FText::FromString (TEXT ("Validation")),
                    GetValidationStatusText (),
                    FSlateColor (ValidationState.IsValid () && ValidationState->bValidationHasRun
                        ? FLinearColor (1.f, 0.72f, 0.20f, 1.f)
                        : FLinearColor (0.50f, 0.50f, 0.50f, 1.f)))
            ]
        ];
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

FText FGridLevelEdModeToolkit::GetSelectedCellStatusText () const
{
    if (const AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        return FText::Format (
            FText::FromString (TEXT ("X={0} Y={1}")),
            FText::AsNumber (EditorActor->SelectedCellX),
            FText::AsNumber (EditorActor->SelectedCellY));
    }

    return FText::FromString (TEXT ("None"));
}

FText FGridLevelEdModeToolkit::GetSelectedEdgeStatusText () const
{
    if (const AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        const UEnum* EdgeEnum = StaticEnum<EGridEdge> ();
        return EdgeEnum
            ? EdgeEnum->GetDisplayNameTextByValue (static_cast<int64> (EditorActor->SelectedEdge))
            : FText::FromString (TEXT ("Unknown"));
    }

    return FText::FromString (TEXT ("None"));
}

FText FGridLevelEdModeToolkit::GetSelectedObjectStatusText () const
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
    const FGridLevelObjectData* Obj = EditorActor ? EditorActor->GetSelectedObjectData () : nullptr;
    if (!Obj)
    {
        return FText::FromString (TEXT ("None"));
    }

    const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
    const FText TypeText = GridEditorWidgetHelpers::GetGridEnumDisplayText (TypeEnum, static_cast<int64> (Obj->Type));

    return FText::Format (
        FText::FromString (TEXT ("{0} ({1},{2})")),
        TypeText,
        FText::AsNumber (Obj->CellX),
        FText::AsNumber (Obj->CellY));
}

FText FGridLevelEdModeToolkit::GetValidationStatusText () const
{
    if (!ValidationState.IsValid ())
    {
        return FText::FromString (TEXT ("Not run"));
    }

    return ValidationState->GetValidationStatusText ();
}



const FSlateBrush* FGridLevelEdModeToolkit::GetOrCreateBrush (UTexture2D* Texture, float Size)
{
    if (!Texture)
    {
        return nullptr;
    }

    const FString CacheKey = FString::Printf (
        TEXT ("%s@%.2f"),
        *Texture->GetPathName (),
        Size);

    if (TSharedPtr<FSlateBrush>* Existing = CachedIconBrushes.Find (CacheKey))
    {
        return Existing->Get ();
    }

    TSharedPtr<FSlateBrush> NewBrush = MakeShared<FSlateImageBrush> (Texture, FVector2D (Size, Size));
    CachedIconBrushes.Add (CacheKey, NewBrush);
    return NewBrush.Get ();
}

#endif
