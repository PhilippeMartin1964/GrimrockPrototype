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
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
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

    FSlateColor GetOverviewCellColor (const FGridLevelCellData* CellData)
    {
        if (!CellData)
        {
            return FSlateColor (FLinearColor (0.025f, 0.025f, 0.025f, 1.f));
        }

        if (CellData->bBlocksOccupancy)
        {
            return FSlateColor (FLinearColor (0.18f, 0.18f, 0.18f, 1.f));
        }

        const bool bHasWall = CellData->NorthWall != EGridWallType::None ||
            CellData->EastWall != EGridWallType::None ||
            CellData->SouthWall != EGridWallType::None ||
            CellData->WestWall != EGridWallType::None;

        if (bHasWall)
        {
            return FSlateColor (FLinearColor (0.30f, 0.30f, 0.30f, 1.f));
        }

        switch (CellData->CellType)
        {
            case EGridCellType::Floor:
                return FSlateColor (FLinearColor (0.18f, 0.21f, 0.23f, 1.f));

            case EGridCellType::Pit:
                return FSlateColor (FLinearColor (0.08f, 0.07f, 0.09f, 1.f));

            case EGridCellType::StairsUp:
            case EGridCellType::StairsDown:
                return FSlateColor (FLinearColor (0.22f, 0.18f, 0.12f, 1.f));

            case EGridCellType::Teleporter:
                return FSlateColor (FLinearColor (0.12f, 0.20f, 0.28f, 1.f));

            case EGridCellType::Empty:
            default:
                return FSlateColor (FLinearColor (0.05f, 0.055f, 0.06f, 1.f));
        }
    }

    int32 GetOverviewObjectPriority (EGridLevelObjectType Type)
    {
        switch (Type)
        {
            case EGridLevelObjectType::Door:          return 100;
            case EGridLevelObjectType::Receptacle:    return 90;
            case EGridLevelObjectType::Trigger:       return 80;
            case EGridLevelObjectType::Button:        return 70;
            case EGridLevelObjectType::Lever:         return 65;
            case EGridLevelObjectType::PressurePlate: return 60;
            case EGridLevelObjectType::Teleporter:    return 55;
            case EGridLevelObjectType::MonsterSpawn:  return 50;
            case EGridLevelObjectType::ItemSpawn:     return 45;
            case EGridLevelObjectType::Light:         return 40;
            case EGridLevelObjectType::Decoration:    return 30;
            default:                                  return 0;
        }
    }

    FText GetOverviewObjectGlyph (EGridLevelObjectType Type, FName Tag)
    {
        switch (Type)
        {
            case EGridLevelObjectType::Door:
                return Tag == FName (TEXT ("Secret")) || Tag == FName (TEXT ("SecretDoor"))
                    ? FText::FromString (TEXT ("S"))
                    : FText::FromString (TEXT ("D"));

            case EGridLevelObjectType::Button:        return FText::FromString (TEXT ("B"));
            case EGridLevelObjectType::Lever:         return FText::FromString (TEXT ("L"));
            case EGridLevelObjectType::PressurePlate: return FText::FromString (TEXT ("P"));
            case EGridLevelObjectType::Trigger:       return FText::FromString (TEXT ("T"));
            case EGridLevelObjectType::Receptacle:    return FText::FromString (TEXT ("R"));
            case EGridLevelObjectType::Teleporter:    return FText::FromString (TEXT ("X"));
            case EGridLevelObjectType::MonsterSpawn:  return FText::FromString (TEXT ("M"));
            case EGridLevelObjectType::ItemSpawn:     return FText::FromString (TEXT ("I"));
            case EGridLevelObjectType::Light:         return FText::FromString (TEXT ("*"));
            case EGridLevelObjectType::Decoration:    return FText::FromString (TEXT ("o"));
            default:                                  return FText::GetEmpty ();
        }
    }

    FString NameArrayToCommaSeparatedText (const TArray<FName>& Names)
    {
        TArray<FString> Parts;
        Parts.Reserve (Names.Num ());

        for (const FName& Name : Names)
        {
            if (!Name.IsNone ())
            {
                Parts.Add (Name.ToString ());
            }
        }

        return FString::Join (Parts, TEXT (", "));
    }

    TArray<FName> ParseCommaSeparatedNames (const FString& Text)
    {
        TArray<FString> Parts;
        Text.ParseIntoArray (Parts, TEXT (","), true);

        TArray<FName> Names;
        Names.Reserve (Parts.Num ());

        for (FString& Part : Parts)
        {
            Part.TrimStartAndEndInline ();
            if (!Part.IsEmpty ())
            {
                Names.Add (FName (*Part));
            }
        }

        return Names;
    }
}

void FGridLevelEdModeToolkit::Init (const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
    BuildTriggerModeOptions ();
    BuildLinkOptions ();
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
            BuildPanelSection (FText::FromString (TEXT ("OVERVIEW MAP")), BuildOverviewMapSection ())
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
            BuildPanelSection (FText::FromString (TEXT ("SELECTED OBJECT")), BuildObjectInspectorSection ())
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildPanelSection (FText::FromString (TEXT ("VALIDATION")), BuildValidationSection ())
        ];

    if (Obj)
    {
        if (Obj->Type != EGridLevelObjectType::Trigger)
        {
            ToolkitRoot->AddSlot ()
                .AutoHeight ()
                .Padding (0.f, 0.f, 0.f, 8.f)
                [
                    BuildPanelSection (FText::FromString (TEXT ("BEHAVIOR EDITOR")), BuildBehaviorEditorSection ())
                ];
        }

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

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildOverviewMapSection ()
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
    if (!EditorActor || !EditorActor->LevelAsset)
    {
        return SNew (STextBlock)
            .Text (FText::FromString (TEXT ("No editor actor or level asset.")));
    }

    const UGridLevelAsset* LevelAsset = EditorActor->LevelAsset;
    const FGridLevelObjectData* SelectedObject = EditorActor->GetSelectedObjectData ();

    TSharedRef<SUniformGridPanel> GridPanel = SNew (SUniformGridPanel)
        .SlotPadding (FMargin (1.f));

    for (int32 Y = LevelAsset->Height - 1; Y >= 0; --Y)
    {
        const int32 DisplayRow = LevelAsset->Height - 1 - Y;
        for (int32 X = LevelAsset->Width - 1; X >= 0; --X)
        {
            const int32 DisplayColumn = LevelAsset->Width - 1 - X;
            GridPanel->AddSlot (DisplayColumn, DisplayRow)
                [
                    BuildOverviewCell (X, Y, SelectedObject)
                ];
        }
    }

    return SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ()
        [
            SNew (SBox)
                .WidthOverride (512.f)
                [
                    GridPanel
                ]
        ]

    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("Legend: D Door, S Secret Door, B Button, L Lever, P Plate, T Trigger, R Receptacle, X Teleporter, M/I Spawn.")))
                .AutoWrapText (true)
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildOverviewCell (
    int32 CellX,
    int32 CellY,
    const FGridLevelObjectData* SelectedObject)
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
    const UGridLevelAsset* LevelAsset = EditorActor ? EditorActor->LevelAsset : nullptr;
    const bool bValidCell = LevelAsset && LevelAsset->IsValidCoord (CellX, CellY);
    const FGridLevelCellData* CellData = bValidCell ? &LevelAsset->GetCell (CellX, CellY) : nullptr;

    const FGridLevelObjectData* DisplayObject = nullptr;
    int32 BestPriority = INDEX_NONE;
    int32 ObjectCount = 0;
    if (LevelAsset)
    {
        for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
        {
            if (Obj.CellX != CellX || Obj.CellY != CellY)
            {
                continue;
            }

            ++ObjectCount;
            const int32 Priority = GetOverviewObjectPriority (Obj.Type);
            if (!DisplayObject || Priority > BestPriority)
            {
                DisplayObject = &Obj;
                BestPriority = Priority;
            }
        }
    }

    const bool bSelectedCell = EditorActor &&
        EditorActor->SelectedCellX == CellX &&
        EditorActor->SelectedCellY == CellY;
    const bool bSelectedObjectCell = SelectedObject &&
        SelectedObject->CellX == CellX &&
        SelectedObject->CellY == CellY;

    FText GlyphText = DisplayObject
        ? GetOverviewObjectGlyph (DisplayObject->Type, DisplayObject->Tag)
        : FText::GetEmpty ();
    if (ObjectCount > 1 && !GlyphText.IsEmpty ())
    {
        GlyphText = FText::FromString (FString::Printf (TEXT ("%s+"), *GlyphText.ToString ()));
    }

    FSlateColor BorderColor = bSelectedCell
        ? FSlateColor (FLinearColor (1.f, 0.86f, 0.18f, 1.f))
        : bSelectedObjectCell
        ? FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f))
        : GetOverviewCellColor (CellData);

    return SNew (SButton)
        .ButtonColorAndOpacity (BorderColor)
        .ContentPadding (FMargin (0.f))
        .OnClicked_Lambda ([this, CellX, CellY] () -> FReply
    {
        return OnOverviewCellClicked (CellX, CellY);
    })
        [
            SNew (SBox)
                .WidthOverride (14.f)
                .HeightOverride (14.f)
                [
                    SNew (STextBlock)
                        .Text (GlyphText)
                        .Justification (ETextJustify::Center)
                        .ColorAndOpacity (FSlateColor (FLinearColor::White))
                        .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 7))
                ]
        ];
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
        .Padding (8.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ()
                .AutoHeight ()
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("DUNGEON EDITOR")))
                        .Font (FAppStyle::GetFontStyle ("HeadingMedium"))
                ]

                + SVerticalBox::Slot ()
                .AutoHeight ()
                .Padding (0.f, 6.f, 0.f, 0.f)
                [
                    SNew (SWrapBox)

                    + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                    [
                        BuildStatusBadge (
                            FText::FromString (TEXT ("Tool")),
                            GetActiveToolText (),
                            FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f)))
                    ]

                    + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                    [
                        BuildStatusBadge (
                            FText::FromString (TEXT ("Cell")),
                            GetSelectedCellStatusText (),
                            FSlateColor (FLinearColor (0.40f, 0.85f, 0.45f, 1.f)))
                    ]

                    + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                    [
                        BuildStatusBadge (
                            FText::FromString (TEXT ("Edge/Facing")),
                            GetSelectedEdgeStatusText (),
                            FSlateColor (FLinearColor (1.f, 0.72f, 0.20f, 1.f)))
                    ]

                    + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                    [
                        BuildStatusBadge (
                            FText::FromString (TEXT ("Object")),
                            GetSelectedObjectStatusText (),
                            FSlateColor (FLinearColor (0.70f, 0.55f, 1.f, 1.f)))
                    ]

                    + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                    [
                        BuildStatusBadge (
                            FText::FromString (TEXT ("Validation")),
                            GetValidationStatusText (),
                            FSlateColor (bValidationHasRun
                                ? FLinearColor (1.f, 0.72f, 0.20f, 1.f)
                                : FLinearColor (0.50f, 0.50f, 0.50f, 1.f)))
                    ]
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

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildPropertyRow (const FText& Label, TSharedRef<SWidget> ValueWidget) const
{
    return SNew (SHorizontalBox)
        + SHorizontalBox::Slot ()
        .FillWidth (0.35f)
        .VAlign (VAlign_Center)
        .Padding (0.f, 2.f, 8.f, 2.f)
        [
            SNew (STextBlock)
                .Text (Label)
                .ColorAndOpacity (FSlateColor (FLinearColor (0.72f, 0.72f, 0.72f, 1.f)))
        ]
        + SHorizontalBox::Slot ()
        .FillWidth (0.65f)
        .VAlign (VAlign_Center)
        .Padding (0.f, 2.f)
        [
            ValueWidget
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildReadOnlyPropertyRow (const FText& Label, const FText& Value) const
{
    return BuildPropertyRow (
        Label,
        SNew (STextBlock)
            .Text (Value)
            .AutoWrapText (true));
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildActionButton (const FText& Label, const FOnClicked& OnClicked) const
{
    return SNew (SButton)
        .Text (Label)
        .HAlign (HAlign_Center)
        .ContentPadding (FMargin (8.f, 3.f))
        .OnClicked (OnClicked);
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildStatusBadge (
    const FText& Label,
    const FText& Value,
    const FSlateColor& AccentColor) const
{
    return SNew (SBorder)
        .Padding (FMargin (7.f, 4.f))
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SHorizontalBox)

            + SHorizontalBox::Slot ()
            .AutoWidth ()
            .VAlign (VAlign_Center)
            .Padding (0.f, 0.f, 5.f, 0.f)
            [
                SNew (STextBlock)
                    .Text (Label)
                    .ColorAndOpacity (AccentColor)
                    .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 8))
            ]

            + SHorizontalBox::Slot ()
            .AutoWidth ()
            .VAlign (VAlign_Center)
            [
                SNew (STextBlock)
                    .Text (Value)
                    .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8))
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

FReply FGridLevelEdModeToolkit::OnOverviewCellClicked (int32 CellX, int32 CellY)
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        if (EditorActor->SelectCellFromOverview (CellX, CellY))
        {
            RefreshPalette ();
        }
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
    const FText TypeText = GetEnumDisplayText (TypeEnum, static_cast<int64> (Obj->Type));

    return FText::Format (
        FText::FromString (TEXT ("{0} ({1},{2})")),
        TypeText,
        FText::AsNumber (Obj->CellX),
        FText::AsNumber (Obj->CellY));
}

FText FGridLevelEdModeToolkit::GetValidationStatusText () const
{
    if (!bValidationHasRun)
    {
        return FText::FromString (TEXT ("Not run"));
    }

    int32 ErrorCount = 0;
    int32 WarningCount = 0;
    for (const FGridLevelValidationMessage& ValidationMessage : ValidationMessages)
    {
        if (ValidationMessage.Severity == EGridLevelValidationSeverity::Error)
        {
            ++ErrorCount;
        }
        else if (ValidationMessage.Severity == EGridLevelValidationSeverity::Warning)
        {
            ++WarningCount;
        }
    }

    return FText::Format (
        FText::FromString (TEXT ("{0} total, {1} errors, {2} warnings")),
        FText::AsNumber (ValidationMessages.Num ()),
        FText::AsNumber (ErrorCount),
        FText::AsNumber (WarningCount));
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

FReply FGridLevelEdModeToolkit::OnValidateLevelClicked ()
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        bValidationHasRun = true;
        ValidationMessages = EditorActor->ValidateCurrentLevel ();
        RefreshPalette ();
    }

    return FReply::Handled ();
}

FReply FGridLevelEdModeToolkit::OnMoveSelectedObjectToCurrentCellClicked ()
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->MoveSelectedObjectToCurrentSelection ();
        RefreshPalette ();
    }

    return FReply::Handled ();
}

FReply FGridLevelEdModeToolkit::OnRemoveExactLinkClicked (
    FGuid SourceObjectId,
    FGuid TargetObjectId,
    EGridObjectEventType SourceEvent,
    EGridLinkAction Action)
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();
        EditorActor->RemoveExactLink (SourceObjectId, TargetObjectId, SourceEvent, Action);
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

FText FGridLevelEdModeToolkit::GetLinkSourceEventText (EGridObjectEventType SourceEvent) const
{
    const UEnum* Enum = StaticEnum<EGridObjectEventType> ();

    return Enum
        ? Enum->GetDisplayNameTextByValue (static_cast<int64> (SourceEvent))
        : FText::FromString (TEXT ("Unknown"));
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
                    BuildActionButton (
                        FText::FromString (TEXT ("Focus Selected Object")),
                        FOnClicked::CreateLambda ([this] () -> FReply
                    {
                        return OnFocusSelectedObjectClicked ();
                    }))
                ]

            + SHorizontalBox::Slot ().AutoWidth ()
                [
                    BuildActionButton (
                        FText::FromString (TEXT ("Apply Selected Object")),
                        FOnClicked::CreateRaw (this, &FGridLevelEdModeToolkit::OnApplySelectedObjectClicked))
                ]

                + SHorizontalBox::Slot ().AutoWidth ().Padding (4.f, 0.f, 0.f, 0.f)
                [
                    BuildActionButton (
                        FText::FromString (TEXT ("Move To Current Cell")),
                        FOnClicked::CreateRaw (this, &FGridLevelEdModeToolkit::OnMoveSelectedObjectToCurrentCellClicked))
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
                                                .Text (GetObjectGlyph (Obj.Type))
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
                    BuildReadOnlyPropertyRow (FText::FromString (TEXT ("ObjectId")), FText::FromString (ShortObjectId))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildReadOnlyPropertyRow (FText::FromString (TEXT ("Type")), TypeText)
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildReadOnlyPropertyRow (
                        FText::FromString (TEXT ("Cell / Edge")),
                        FText::Format (
                            FText::FromString (TEXT ("X={0} Y={1} Edge={2}")),
                            FText::AsNumber (Obj.CellX),
                            FText::AsNumber (Obj.CellY),
                            EdgeText))
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
                [
                    BuildPropertyRow (
                        FText::FromString (TEXT ("ArchetypeId")),
                        SNew (SEditableTextBox)
                            .Text (FText::FromName (Obj.ArchetypeId))
                            .OnTextCommitted_Lambda ([this] (const FText& NewText, ETextCommit::Type CommitType)
                        {
                            if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                            {
                                EditorActor->SetSelectedObjectArchetypeId (FName (*NewText.ToString ()));
                                RefreshPalette ();
                            }
                        }))
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildPropertyRow (
                        FText::FromString (TEXT ("Tag")),
                        SNew (SEditableTextBox)
                            .Text (FText::FromName (Obj.Tag))
                            .OnTextCommitted_Lambda ([this] (const FText& NewText, ETextCommit::Type CommitType)
                        {
                            if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                            {
                                EditorActor->SetSelectedObjectTag (FName (*NewText.ToString ()));
                                RefreshPalette ();
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
                        if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                        {
                            EditorActor->SetSelectedObjectNotes (NewText.ToString ());
                            RefreshPalette ();
                        }
                    })
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
                                if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                                {
                                    EditorActor->SetSelectedObjectInitiallyEnabled (NewState == ECheckBoxState::Checked);
                                    RefreshPalette ();
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
                                if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                                {
                                    EditorActor->SetSelectedObjectInitiallyActive (NewState == ECheckBoxState::Checked);
                                    RefreshPalette ();
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
                                if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                                {
                                    EditorActor->SetSelectedObjectOverrideBehavior (NewState == ECheckBoxState::Checked);
                                    RefreshPalette ();
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
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildTriggerBehaviorSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
        {
            if (EditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                CachedBehaviorObjectId.Invalidate ();
                RefreshPalette ();
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
        return BuildPropertyRow (
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
                                .OnGenerateWidget (this, &FGridLevelEdModeToolkit::MakeTriggerModeComboWidget)
                                .OnSelectionChanged_Lambda ([Obj, ApplyBehavior] (
                                    TSharedPtr<EGridObjectTriggerMode> NewValue,
                                    ESelectInfo::Type SelectInfo)
                            {
                                if (NewValue.IsValid ())
                                {
                                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                                    NewBehavior.TriggerMode = *NewValue;
                                    ApplyBehavior (NewBehavior);
                                }
                            })
                                [
                                    SNew (STextBlock)
                                        .Text (TriggerModeEnum
                                            ? TriggerModeEnum->GetDisplayNameTextByValue (static_cast<int64> (Behavior.TriggerMode))
                                            : FText::FromString (TEXT ("Unknown")))
                                ]
                        ]
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeNumberRow (
                        FText::FromString (TEXT ("Delay (s)")),
                        Behavior.Delay,
                        [] (FGridObjectBehaviorParams& NewBehavior, float NewValue)
                    {
                        NewBehavior.Delay = NewValue;
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeNumberRow (
                        FText::FromString (TEXT ("Duration (s)")),
                        Behavior.Duration,
                        [] (FGridObjectBehaviorParams& NewBehavior, float NewValue)
                    {
                        NewBehavior.Duration = NewValue;
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            MakeCheckRow (
                                FText::FromString (TEXT ("Fire On Enter")),
                                Behavior.bFireOnEnter,
                                [] (FGridObjectBehaviorParams& NewBehavior, bool bNewValue)
                            {
                                NewBehavior.bFireOnEnter = bNewValue;
                            })
                        ]

                    + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            MakeCheckRow (
                                FText::FromString (TEXT ("Fire On Exit")),
                                Behavior.bFireOnExit,
                                [] (FGridObjectBehaviorParams& NewBehavior, bool bNewValue)
                            {
                                NewBehavior.bFireOnExit = bNewValue;
                            })
                        ]

                    + SHorizontalBox::Slot ().AutoWidth ()
                        [
                            MakeCheckRow (
                                FText::FromString (TEXT ("Invert Links")),
                                Behavior.bInvertLinks,
                                [] (FGridObjectBehaviorParams& NewBehavior, bool bNewValue)
                            {
                                NewBehavior.bInvertLinks = bNewValue;
                            })
                        ]
                ]
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildReceptacleBehaviorSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
        {
            if (EditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                CachedBehaviorObjectId.Invalidate ();
                RefreshPalette ();
            }
        }
    };

    auto MakeTextRow = [this, Obj, ApplyBehavior] (
        const FText& Label,
        const FText& Value,
        TFunction<void (FGridObjectBehaviorParams&, const FString&)> Mutator) -> TSharedRef<SWidget>
    {
        return BuildPropertyRow (
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
                        .IsChecked (Behavior.bAcceptAnyItem ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                        .OnCheckStateChanged_Lambda ([this, Obj] (ECheckBoxState NewState)
                    {
                        FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                        NewBehavior.bAcceptAnyItem = NewState == ECheckBoxState::Checked;

                        if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                        {
                            if (EditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
                            {
                                CachedBehaviorObjectId.Invalidate ();
                                RefreshPalette ();
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
                        FText::FromString (NameArrayToCommaSeparatedText (Behavior.AcceptedArchetypeIds)),
                        [] (FGridObjectBehaviorParams& NewBehavior, const FString& Text)
                    {
                        NewBehavior.AcceptedArchetypeIds = ParseCommaSeparatedNames (Text);
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeTextRow (
                        FText::FromString (TEXT ("Accepted Item Tags")),
                        FText::FromString (NameArrayToCommaSeparatedText (Behavior.AcceptedItemTags)),
                        [] (FGridObjectBehaviorParams& NewBehavior, const FString& Text)
                    {
                        NewBehavior.AcceptedItemTags = ParseCommaSeparatedNames (Text);
                    })
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    MakeTextRow (
                        FText::FromString (TEXT ("Initial Contained Item")),
                        FText::FromName (Behavior.InitialContainedItemArchetypeId),
                        [] (FGridObjectBehaviorParams& NewBehavior, const FString& Text)
                    {
                        FString TrimmedText = Text;
                        TrimmedText.TrimStartAndEndInline ();
                        NewBehavior.InitialContainedItemArchetypeId = TrimmedText.IsEmpty ()
                            ? NAME_None
                            : FName (*TrimmedText);
                    })
                ]
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildValidationSection ()
{
    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    Root->AddSlot ()
        .AutoHeight ()
        [
            BuildActionButton (
                FText::FromString (TEXT ("Validate Level")),
                FOnClicked::CreateRaw (this, &FGridLevelEdModeToolkit::OnValidateLevelClicked))
        ];

    if (!bValidationHasRun)
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

    if (ValidationMessages.Num () == 0)
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

    for (const FGridLevelValidationMessage& ValidationMessage : ValidationMessages)
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

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildLinksSection (const FGridLevelObjectData& SelectedObject)
{
    return SNew (SVerticalBox)

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
            BuildActionButton (
                FText::FromString (TEXT ("Clear All Links For Selected Object")),
                FOnClicked::CreateRaw (this, &FGridLevelEdModeToolkit::OnClearSelectedObjectLinksClicked))
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildLinkCreationSection ()
{
    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 8.f, 0.f)
                [
                    SNew (STextBlock).Text (FText::FromString (TEXT ("New Link Event")))
                ]

                + SHorizontalBox::Slot ().FillWidth (0.5f).Padding (0.f, 0.f, 12.f, 0.f)
                [
                    SNew (SComboBox<TSharedPtr<EGridObjectEventType>>)
                        .OptionsSource (&LinkSourceEventOptions)
                        .OnGenerateWidget (this, &FGridLevelEdModeToolkit::MakeLinkSourceEventComboWidget)
                        .OnSelectionChanged (this, &FGridLevelEdModeToolkit::OnLinkSourceEventSelectionChanged)
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
                    SNew (STextBlock).Text (FText::FromString (TEXT ("Action")))
                ]

                + SHorizontalBox::Slot ().FillWidth (0.5f)
                [
                    SNew (SComboBox<TSharedPtr<EGridLinkAction>>)
                        .OptionsSource (&LinkActionOptions)
                        .OnGenerateWidget (this, &FGridLevelEdModeToolkit::MakeLinkActionComboWidget)
                        .OnSelectionChanged (this, &FGridLevelEdModeToolkit::OnLinkActionSelectionChanged)
                        [
                            SNew (STextBlock)
                                .Text_Lambda ([this] ()
                            {
                                return GetSelectedLinkActionText ();
                            })
                        ]
                ]
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
                                FText::FromString (TEXT ("{0} -> {1} -> {2}")),
                                GetLinkSourceEventText (Link.SourceEvent),
                                GetLinkActionText (Link.Action),
                                GetObjectSummaryText (OtherId)))
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
                                Link.SourceEvent,
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

void FGridLevelEdModeToolkit::BuildLinkOptions ()
{
    LinkSourceEventOptions.Reset ();
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEventType> (EGridObjectEventType::Activated));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEventType> (EGridObjectEventType::Deactivated));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEventType> (EGridObjectEventType::ItemInserted));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEventType> (EGridObjectEventType::ItemRemoved));
    LinkSourceEventOptions.Add (MakeShared<EGridObjectEventType> (EGridObjectEventType::ItemChanged));

    LinkActionOptions.Reset ();
    LinkActionOptions.Add (MakeShared<EGridLinkAction> (EGridLinkAction::Toggle));
    LinkActionOptions.Add (MakeShared<EGridLinkAction> (EGridLinkAction::Open));
    LinkActionOptions.Add (MakeShared<EGridLinkAction> (EGridLinkAction::Close));
    LinkActionOptions.Add (MakeShared<EGridLinkAction> (EGridLinkAction::Activate));
    LinkActionOptions.Add (MakeShared<EGridLinkAction> (EGridLinkAction::Deactivate));
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

TSharedRef<SWidget> FGridLevelEdModeToolkit::MakeLinkSourceEventComboWidget (
    TSharedPtr<EGridObjectEventType> Item) const
{
    if (!Item.IsValid ())
    {
        return SNew (STextBlock).Text (FText::FromString (TEXT ("Invalid")));
    }

    return SNew (STextBlock).Text (GetLinkSourceEventText (*Item));
}

void FGridLevelEdModeToolkit::OnLinkSourceEventSelectionChanged (
    TSharedPtr<EGridObjectEventType> NewValue,
    ESelectInfo::Type SelectInfo)
{
    if (NewValue.IsValid ())
    {
        if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
        {
            EditorActor->Modify ();
            EditorActor->LinkSourceEvent = *NewValue;
            RefreshPalette ();
        }
    }
}

FText FGridLevelEdModeToolkit::GetSelectedLinkSourceEventText () const
{
    if (const AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        return GetLinkSourceEventText (EditorActor->LinkSourceEvent);
    }

    return FText::FromString (TEXT ("Unknown"));
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::MakeLinkActionComboWidget (
    TSharedPtr<EGridLinkAction> Item) const
{
    if (!Item.IsValid ())
    {
        return SNew (STextBlock).Text (FText::FromString (TEXT ("Invalid")));
    }

    return SNew (STextBlock).Text (GetLinkActionText (*Item));
}

void FGridLevelEdModeToolkit::OnLinkActionSelectionChanged (
    TSharedPtr<EGridLinkAction> NewValue,
    ESelectInfo::Type SelectInfo)
{
    if (NewValue.IsValid ())
    {
        if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
        {
            EditorActor->Modify ();
            EditorActor->LinkAction = *NewValue;
            RefreshPalette ();
        }
    }
}

FText FGridLevelEdModeToolkit::GetSelectedLinkActionText () const
{
    if (const AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        return GetLinkActionText (EditorActor->LinkAction);
    }

    return FText::FromString (TEXT ("Unknown"));
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
