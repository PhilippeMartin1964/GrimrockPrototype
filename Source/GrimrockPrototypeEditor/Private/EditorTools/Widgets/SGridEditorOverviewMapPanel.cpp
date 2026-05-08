#include "EditorTools/Widgets/SGridEditorOverviewMapPanel.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Templates/Function.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"

namespace
{
    FText GetEnumDisplayText (const UEnum* Enum, int64 Value)
    {
        return Enum
            ? Enum->GetDisplayNameTextByValue (Value)
            : FText::FromString (TEXT ("Unknown"));
    }

    FText GetBooleanText (bool bValue)
    {
        return bValue
            ? FText::FromString (TEXT ("Yes"))
            : FText::FromString (TEXT ("No"));
    }

    FLinearColor GetOverviewEmptyColor ()
    {
        return FLinearColor (0.006f, 0.006f, 0.008f, 1.f);
    }

    FLinearColor GetOverviewFloorColor ()
    {
        return FLinearColor (0.22f, 0.30f, 0.36f, 1.f);
    }

    FLinearColor GetOverviewWalledFloorColor ()
    {
        return FLinearColor (0.38f, 0.40f, 0.42f, 1.f);
    }

    FLinearColor GetOverviewBlockColor ()
    {
        return FLinearColor (0.26f, 0.16f, 0.12f, 1.f);
    }

    FLinearColor GetOverviewPitColor ()
    {
        return FLinearColor (0.08f, 0.05f, 0.12f, 1.f);
    }

    FLinearColor GetOverviewStairsColor ()
    {
        return FLinearColor (0.36f, 0.28f, 0.14f, 1.f);
    }

    FLinearColor GetOverviewTeleporterColor ()
    {
        return FLinearColor (0.10f, 0.30f, 0.45f, 1.f);
    }

    FLinearColor GetOverviewSelectedCellOutlineColor ()
    {
        return FLinearColor (1.f, 0.86f, 0.18f, 1.f);
    }

    FLinearColor GetOverviewSelectedObjectOutlineColor ()
    {
        return FLinearColor (0.25f, 0.78f, 1.f, 1.f);
    }

    FLinearColor GetOverviewMultiObjectOutlineColor ()
    {
        return FLinearColor (0.35f, 0.85f, 0.45f, 1.f);
    }

    FLinearColor GetOverviewDefaultOutlineColor ()
    {
        return FLinearColor (0.08f, 0.08f, 0.08f, 1.f);
    }

    FSlateColor GetOverviewCellColor (const FGridLevelCellData* CellData)
    {
        if (!CellData)
        {
            return FSlateColor (GetOverviewEmptyColor ());
        }

        if (CellData->bBlocksOccupancy)
        {
            return FSlateColor (GetOverviewBlockColor ());
        }

        const bool bHasWall = CellData->NorthWall != EGridWallType::None ||
            CellData->EastWall != EGridWallType::None ||
            CellData->SouthWall != EGridWallType::None ||
            CellData->WestWall != EGridWallType::None;

        switch (CellData->CellType)
        {
            case EGridCellType::Floor:
                return FSlateColor (bHasWall ? GetOverviewWalledFloorColor () : GetOverviewFloorColor ());

            case EGridCellType::Pit:
                return FSlateColor (GetOverviewPitColor ());

            case EGridCellType::StairsUp:
            case EGridCellType::StairsDown:
                return FSlateColor (GetOverviewStairsColor ());

            case EGridCellType::Teleporter:
                return FSlateColor (GetOverviewTeleporterColor ());

            case EGridCellType::Empty:
            default:
                return FSlateColor (GetOverviewEmptyColor ());
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
}

void SGridEditorOverviewMapPanel::Construct (const FArguments& InArgs)
{
    EditorActor = InArgs._EditorActor;
    OnGetEditorActor = InArgs._OnGetEditorActor;
    OnRequestRefresh = InArgs._OnRequestRefresh;

    ChildSlot
    [
        BuildOverviewMapSection ()
    ];
}

AGridLevelEditorActor* SGridEditorOverviewMapPanel::GetEditorActor () const
{
    if (EditorActor.IsValid ())
    {
        return EditorActor.Get ();
    }

    return OnGetEditorActor.IsBound ()
        ? OnGetEditorActor.Execute ()
        : nullptr;
}

void SGridEditorOverviewMapPanel::RequestRefresh () const
{
    if (OnRequestRefresh.IsBound ())
    {
        OnRequestRefresh.Execute ();
    }
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildOverviewMapSection ()
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
    {
        return SNew (STextBlock)
            .Text (FText::FromString (TEXT ("No editor actor or level asset.")));
    }

    const UGridLevelAsset* LevelAsset = CurrentEditorActor->LevelAsset;
    const FGridLevelObjectData* SelectedObject = CurrentEditorActor->GetSelectedObjectData ();

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
            BuildOverviewColorLegend ()
        ]

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 5.f, 0.f, 0.f)
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("Legend: D Door, S Secret Door, B Button, L Lever, P Plate, T Trigger, R Receptacle, X Teleporter, M/I Spawn.")))
                .AutoWrapText (true)
        ]

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
        [
            BuildSelectedCellSection ()
        ];
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildOverviewCell (
    int32 CellX,
    int32 CellY,
    const FGridLevelObjectData* SelectedObject)
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridLevelAsset* LevelAsset = CurrentEditorActor ? CurrentEditorActor->LevelAsset : nullptr;
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

    const bool bSelectedCell = CurrentEditorActor &&
        CurrentEditorActor->SelectedCellX == CellX &&
        CurrentEditorActor->SelectedCellY == CellY;
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

    const FSlateColor OutlineColor = bSelectedCell
        ? FSlateColor (GetOverviewSelectedCellOutlineColor ())
        : bSelectedObjectCell
        ? FSlateColor (GetOverviewSelectedObjectOutlineColor ())
        : ObjectCount > 1
        ? FSlateColor (GetOverviewMultiObjectOutlineColor ())
        : FSlateColor (GetOverviewDefaultOutlineColor ());

    const FSlateColor FillColor = GetOverviewCellColor (CellData);

    return SNew (SBox)
        .WidthOverride (18.f)
        .HeightOverride (18.f)
        [
            SNew (SBorder)
            .Padding (bSelectedCell || bSelectedObjectCell ? 2.f : 1.f)
            .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
            .BorderBackgroundColor (OutlineColor)
            [
                SNew (SButton)
                    .ButtonColorAndOpacity (FillColor)
                    .ContentPadding (FMargin (0.f))
                    .ToolTipText (GetOverviewCellTooltipText (CellX, CellY))
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
                ]
            ]
        ];
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildOverviewColorLegend () const
{
    return SNew (SWrapBox)

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Empty")), GetOverviewEmptyColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Floor")), GetOverviewFloorColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Wall")), GetOverviewWalledFloorColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Block")), GetOverviewBlockColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Pit")), GetOverviewPitColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Stairs")), GetOverviewStairsColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Teleporter")), GetOverviewTeleporterColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Selected")), GetOverviewSelectedCellOutlineColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Object")), GetOverviewSelectedObjectOutlineColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Multi")), GetOverviewMultiObjectOutlineColor ())
        ];
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildOverviewLegendSwatch (const FText& Label, const FLinearColor& Color) const
{
    return SNew (SHorizontalBox)

        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .VAlign (VAlign_Center)
        .Padding (0.f, 0.f, 3.f, 0.f)
        [
            SNew (SBorder)
                .Padding (0.f)
                .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
                .BorderBackgroundColor (FSlateColor (Color))
                [
                    SNew (SBox)
                        .WidthOverride (10.f)
                        .HeightOverride (10.f)
                ]
        ]

        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .VAlign (VAlign_Center)
        [
            SNew (STextBlock)
                .Text (Label)
                .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 7))
                .ColorAndOpacity (FSlateColor (FLinearColor (0.72f, 0.72f, 0.72f, 1.f)))
        ];
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildSelectedCellSection ()
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const bool bHasValidSelection = CurrentEditorActor && CurrentEditorActor->IsSelectionValidForEditing ();
    const bool bHasSelectedObject = CurrentEditorActor && CurrentEditorActor->GetSelectedObjectData ();
    const bool bHasObjectInCell = CurrentEditorActor && HasObjectAtCell (CurrentEditorActor->SelectedCellX, CurrentEditorActor->SelectedCellY);
    const bool bHasSelectedEdge = CurrentEditorActor && CurrentEditorActor->SelectedEdge != EGridEdge::None;

    auto MakeCellAction = [this] (
        const FText& Label,
        bool bEnabled,
        const FText& DisabledTooltip,
        TFunction<void (AGridLevelEditorActor&)> Action) -> TSharedRef<SWidget>
    {
        return SNew (SButton)
            .Text (Label)
            .HAlign (HAlign_Center)
            .ContentPadding (FMargin (7.f, 2.f))
            .IsEnabled (bEnabled)
            .ToolTipText (bEnabled ? FText::GetEmpty () : DisabledTooltip)
            .OnClicked_Lambda ([this, Action] () -> FReply
        {
            if (AGridLevelEditorActor* MutableEditorActor = GetEditorActor ())
            {
                Action (*MutableEditorActor);
                RequestRefresh ();
            }

            return FReply::Handled ();
        });
    };

    return SNew (SBorder)
        .Padding (FMargin (6.f, 5.f))
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.DarkGroupBorder"))
        [
            SNew (SVerticalBox)

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 5.f)
            [
                SNew (SWrapBox)

                + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 2.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("SELECTED CELL")))
                        .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 9))
                        .ColorAndOpacity (FSlateColor (FLinearColor (0.80f, 0.80f, 0.80f, 1.f)))
                ]

                + SWrapBox::Slot ().Padding (0.f, 0.f, 0.f, 2.f)
                [
                    SNew (STextBlock)
                        .Text (GetSelectedCellSummaryText ())
                        .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8))
                        .AutoWrapText (true)
                ]
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                SNew (SWrapBox)

                + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                [
                    MakeCellAction (
                        FText::FromString (TEXT ("Focus")),
                        bHasSelectedObject || bHasObjectInCell,
                        FText::FromString (TEXT ("Focus requires a selected object or an object on the selected cell.")),
                        [] (AGridLevelEditorActor& MutableEditorActor)
                    {
                        if (!MutableEditorActor.GetSelectedObjectData () && MutableEditorActor.LevelAsset)
                        {
                            for (const FGridLevelObjectData& Obj : MutableEditorActor.LevelAsset->Objects)
                            {
                                if (Obj.CellX == MutableEditorActor.SelectedCellX &&
                                    Obj.CellY == MutableEditorActor.SelectedCellY)
                                {
                                    MutableEditorActor.SelectObjectById (Obj.ObjectId);
                                    break;
                                }
                            }
                        }

                        MutableEditorActor.FocusSelectedObject ();
                    })
                ]

                + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                [
                    MakeCellAction (
                        FText::FromString (TEXT ("Select Object")),
                        bHasObjectInCell,
                        FText::FromString (TEXT ("No object exists on the selected cell.")),
                        [] (AGridLevelEditorActor& MutableEditorActor)
                    {
                        if (!MutableEditorActor.LevelAsset)
                        {
                            return;
                        }

                        for (const FGridLevelObjectData& Obj : MutableEditorActor.LevelAsset->Objects)
                        {
                            if (Obj.CellX == MutableEditorActor.SelectedCellX &&
                                Obj.CellY == MutableEditorActor.SelectedCellY)
                            {
                                MutableEditorActor.SelectObjectById (Obj.ObjectId);
                                return;
                            }
                        }
                    })
                ]

                + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                [
                    MakeCellAction (
                        FText::FromString (TEXT ("Paint Cell")),
                        bHasValidSelection,
                        FText::FromString (TEXT ("Paint Cell requires a valid selected cell.")),
                        [] (AGridLevelEditorActor& MutableEditorActor)
                    {
                        MutableEditorActor.Modify ();
                        MutableEditorActor.ActiveTool = EGridEditorTool::PaintCell;
                        MutableEditorActor.ApplyPrimaryToolAction ();
                    })
                ]

                + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                [
                    MakeCellAction (
                        FText::FromString (TEXT ("Paint Wall")),
                        bHasValidSelection && bHasSelectedEdge,
                        FText::FromString (TEXT ("Paint Wall is disabled when Edge/Facing is None.")),
                        [] (AGridLevelEditorActor& MutableEditorActor)
                    {
                        MutableEditorActor.PaintSelectedWall ();
                    })
                ]

                + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                [
                    MakeCellAction (
                        FText::FromString (TEXT ("Place Object")),
                        bHasValidSelection,
                        FText::FromString (TEXT ("Place Object requires a valid selected cell.")),
                        [] (AGridLevelEditorActor& MutableEditorActor)
                    {
                        MutableEditorActor.PlaceSelectedObject ();
                    })
                ]

                + SWrapBox::Slot ().Padding (0.f, 0.f, 4.f, 4.f)
                [
                    MakeCellAction (
                        FText::FromString (TEXT ("Erase")),
                        bHasValidSelection,
                        FText::FromString (TEXT ("Erase requires a valid selected cell.")),
                        [] (AGridLevelEditorActor& MutableEditorActor)
                    {
                        MutableEditorActor.EraseAtSelection ();
                    })
                ]
            ]
        ];
}

FReply SGridEditorOverviewMapPanel::OnOverviewCellClicked (int32 CellX, int32 CellY)
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        if (CurrentEditorActor->SelectCellFromOverview (CellX, CellY))
        {
            RequestRefresh ();
        }
    }

    return FReply::Handled ();
}

FText SGridEditorOverviewMapPanel::GetOverviewCellTooltipText (int32 CellX, int32 CellY) const
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridLevelAsset* LevelAsset = CurrentEditorActor ? CurrentEditorActor->LevelAsset : nullptr;
    if (!LevelAsset || !LevelAsset->IsValidCoord (CellX, CellY))
    {
        return FText::Format (
            FText::FromString (TEXT ("Cell X={0} Y={1}\nInvalid cell")),
            FText::AsNumber (CellX),
            FText::AsNumber (CellY));
    }

    const FGridLevelCellData& CellData = LevelAsset->GetCell (CellX, CellY);
    const UEnum* CellTypeEnum = StaticEnum<EGridCellType> ();

    return FText::Format (
        FText::FromString (TEXT ("Cell X={0} Y={1}\nType: {2}\nCeiling: {3}\nBlocks Occupancy: {4}\nWalls: {5}\nObjects: {6}")),
        FText::AsNumber (CellX),
        FText::AsNumber (CellY),
        GetEnumDisplayText (CellTypeEnum, static_cast<int64> (CellData.CellType)),
        GetBooleanText (CellData.bHasCeiling),
        GetBooleanText (CellData.bBlocksOccupancy),
        GetCellWallSummaryText (CellData),
        GetCellObjectSummaryText (CellX, CellY));
}

FText SGridEditorOverviewMapPanel::GetSelectedCellSummaryText () const
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridLevelAsset* LevelAsset = CurrentEditorActor ? CurrentEditorActor->LevelAsset : nullptr;
    if (!CurrentEditorActor || !LevelAsset || !CurrentEditorActor->IsSelectionValidForEditing ())
    {
        return FText::FromString (TEXT ("No valid selected cell."));
    }

    const FGridLevelCellData& CellData = LevelAsset->GetCell (CurrentEditorActor->SelectedCellX, CurrentEditorActor->SelectedCellY);
    const UEnum* CellTypeEnum = StaticEnum<EGridCellType> ();

    return FText::Format (
        FText::FromString (TEXT ("X={0} Y={1} | Type: {2} | Walls: {3} | Objects: {4}")),
        FText::AsNumber (CurrentEditorActor->SelectedCellX),
        FText::AsNumber (CurrentEditorActor->SelectedCellY),
        GetEnumDisplayText (CellTypeEnum, static_cast<int64> (CellData.CellType)),
        GetCellWallSummaryText (CellData),
        GetCellObjectSummaryText (CurrentEditorActor->SelectedCellX, CurrentEditorActor->SelectedCellY));
}

FText SGridEditorOverviewMapPanel::GetCellWallSummaryText (const FGridLevelCellData& CellData) const
{
    const UEnum* WallTypeEnum = StaticEnum<EGridWallType> ();
    const FText NorthText = GetEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.NorthWall));
    const FText EastText = GetEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.EastWall));
    const FText SouthText = GetEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.SouthWall));
    const FText WestText = GetEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.WestWall));

    return FText::Format (
        FText::FromString (TEXT ("N={0}, E={1}, S={2}, W={3}")),
        NorthText,
        EastText,
        SouthText,
        WestText);
}

FText SGridEditorOverviewMapPanel::GetCellObjectSummaryText (int32 CellX, int32 CellY) const
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridLevelAsset* LevelAsset = CurrentEditorActor ? CurrentEditorActor->LevelAsset : nullptr;
    if (!LevelAsset)
    {
        return FText::FromString (TEXT ("None"));
    }

    const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
    TArray<FString> ObjectSummaries;
    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.CellX != CellX || Obj.CellY != CellY)
        {
            continue;
        }

        FString Details;
        if (!Obj.Tag.IsNone ())
        {
            Details = FString::Printf (TEXT (" tag=%s"), *Obj.Tag.ToString ());
        }
        else if (!Obj.ArchetypeId.IsNone ())
        {
            Details = FString::Printf (TEXT (" archetype=%s"), *Obj.ArchetypeId.ToString ());
        }
        else if (!Obj.PaletteEntryId.IsNone ())
        {
            Details = FString::Printf (TEXT (" palette=%s"), *Obj.PaletteEntryId.ToString ());
        }

        ObjectSummaries.Add (FString::Printf (
            TEXT ("%s%s"),
            *GetEnumDisplayText (TypeEnum, static_cast<int64> (Obj.Type)).ToString (),
            *Details));
    }

    return ObjectSummaries.Num () > 0
        ? FText::FromString (FString::Join (ObjectSummaries, TEXT ("; ")))
        : FText::FromString (TEXT ("None"));
}

bool SGridEditorOverviewMapPanel::HasObjectAtCell (int32 CellX, int32 CellY) const
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridLevelAsset* LevelAsset = CurrentEditorActor ? CurrentEditorActor->LevelAsset : nullptr;
    if (!LevelAsset)
    {
        return false;
    }

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        if (Obj.CellX == CellX && Obj.CellY == CellY)
        {
            return true;
        }
    }

    return false;
}

#endif
