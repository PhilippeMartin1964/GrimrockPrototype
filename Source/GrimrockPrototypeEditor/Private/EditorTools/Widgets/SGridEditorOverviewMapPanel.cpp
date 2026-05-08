#include "EditorTools/Widgets/SGridEditorOverviewMapPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
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
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SWrapBox.h"

namespace
{
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

    FLinearColor GetOverviewExistingCellColor ()
    {
        return FLinearColor (0.96f, 0.96f, 0.94f, 1.f);
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

    FLinearColor GetOverviewObjectMarkerColor ()
    {
        return FLinearColor (0.02f, 0.02f, 0.02f, 1.f);
    }

    FSlateColor GetOverviewCellColor (const FGridLevelCellData* CellData)
    {
        if (!CellData)
        {
            return FSlateColor (GetOverviewEmptyColor ());
        }

        return CellData->CellType == EGridCellType::Empty
            ? FSlateColor (GetOverviewEmptyColor ())
            : FSlateColor (GetOverviewExistingCellColor ());
    }

    bool IsOverviewEdgeObject (EGridLevelObjectType Type)
    {
        switch (Type)
        {
            case EGridLevelObjectType::Door:
            case EGridLevelObjectType::Button:
            case EGridLevelObjectType::Lever:
            case EGridLevelObjectType::Receptacle:
                return true;

            default:
                return false;
        }
    }

    EHorizontalAlignment GetOverviewMarkerHorizontalAlignment (const FGridLevelObjectData& Obj)
    {
        if (!IsOverviewEdgeObject (Obj.Type))
        {
            return HAlign_Center;
        }

        switch (Obj.Edge)
        {
            case EGridEdge::East: return HAlign_Right;
            case EGridEdge::West: return HAlign_Left;
            default:              return HAlign_Center;
        }
    }

    EVerticalAlignment GetOverviewMarkerVerticalAlignment (const FGridLevelObjectData& Obj)
    {
        if (!IsOverviewEdgeObject (Obj.Type))
        {
            return VAlign_Center;
        }

        switch (Obj.Edge)
        {
            case EGridEdge::North: return VAlign_Top;
            case EGridEdge::South: return VAlign_Bottom;
            default:               return VAlign_Center;
        }
    }

    FVector2D GetOverviewMarkerSize (const FGridLevelObjectData& Obj)
    {
        if (!IsOverviewEdgeObject (Obj.Type))
        {
            return FVector2D (4.f, 4.f);
        }

        switch (Obj.Edge)
        {
            case EGridEdge::East:
            case EGridEdge::West:
                return FVector2D (3.f, 8.f);

            default:
                return FVector2D (8.f, 3.f);
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

    TArray<const FGridLevelObjectData*> CellObjects;
    if (LevelAsset)
    {
        for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
        {
            if (Obj.CellX != CellX || Obj.CellY != CellY)
            {
                continue;
            }

            CellObjects.Add (&Obj);
        }
    }

    const int32 ObjectCount = CellObjects.Num ();
    const bool bSelectedCell = CurrentEditorActor &&
        CurrentEditorActor->SelectedCellX == CellX &&
        CurrentEditorActor->SelectedCellY == CellY;
    const bool bSelectedObjectCell = SelectedObject &&
        SelectedObject->CellX == CellX &&
        SelectedObject->CellY == CellY;

    const bool bHasSpecialOutline = bSelectedCell || bSelectedObjectCell || ObjectCount > 1;

    const FSlateColor OutlineColor = bSelectedCell
        ? FSlateColor (GetOverviewSelectedCellOutlineColor ())
        : bSelectedObjectCell
        ? FSlateColor (GetOverviewSelectedObjectOutlineColor ())
        : ObjectCount > 1
        ? FSlateColor (GetOverviewMultiObjectOutlineColor ())
        : FSlateColor (GetOverviewDefaultOutlineColor ());

    const FSlateColor FillColor = GetOverviewCellColor (CellData);
    const float OutlinePadding = bHasSpecialOutline ? 2.f : 0.f;
    const float InnerCellSize = bHasSpecialOutline ? 14.f : 18.f;

    return SNew (SBox)
        .WidthOverride (18.f)
        .HeightOverride (18.f)
        [
            SNew (SBorder)
            .Padding (OutlinePadding)
            .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
            .BorderBackgroundColor (bHasSpecialOutline ? OutlineColor : FillColor)
            [
                SNew (SButton)
                    .ButtonStyle (&FCoreStyle::Get ().GetWidgetStyle<FButtonStyle> ("NoBorder"))
                    .ButtonColorAndOpacity (FillColor)
                    .ContentPadding (FMargin (0.f))
                    .ToolTipText (GetOverviewCellTooltipText (CellX, CellY))
                    .OnClicked_Lambda ([this, CellX, CellY] () -> FReply
                {
                    return OnOverviewCellClicked (CellX, CellY);
                })
                [
                    SNew (SBorder)
                        .Padding (0.f)
                        .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
                        .BorderBackgroundColor (FillColor)
                        [
                            SNew (SBox)
                                .WidthOverride (InnerCellSize)
                                .HeightOverride (InnerCellSize)
                                [
                                    BuildCellObjectMarkers (CellObjects)
                                ]
                        ]
                ]
            ]
        ];
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildCellObjectMarkers (const TArray<const FGridLevelObjectData*>& CellObjects) const
{
    TSharedRef<SOverlay> MarkerOverlay = SNew (SOverlay);

    const int32 MaxMarkerCount = FMath::Min (CellObjects.Num (), 3);
    for (int32 MarkerIndex = 0; MarkerIndex < MaxMarkerCount; ++MarkerIndex)
    {
        const FGridLevelObjectData* Obj = CellObjects[MarkerIndex];
        if (!Obj)
        {
            continue;
        }

        const FVector2D MarkerSize = GetOverviewMarkerSize (*Obj);

        MarkerOverlay->AddSlot ()
        .HAlign (GetOverviewMarkerHorizontalAlignment (*Obj))
        .VAlign (GetOverviewMarkerVerticalAlignment (*Obj))
        [
            SNew (SBox)
                .WidthOverride (MarkerSize.X)
                .HeightOverride (MarkerSize.Y)
                [
                    SNew (SBorder)
                        .Padding (0.f)
                        .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
                        .BorderBackgroundColor (FSlateColor (GetOverviewObjectMarkerColor ()))
                ]
        ];
    }

    if (CellObjects.Num () > MaxMarkerCount)
    {
        MarkerOverlay->AddSlot ()
        .HAlign (HAlign_Right)
        .VAlign (VAlign_Bottom)
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("+")))
                .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 7))
                .ColorAndOpacity (FSlateColor (GetOverviewObjectMarkerColor ()))
        ];
    }

    return MarkerOverlay;
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
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Cell")), GetOverviewExistingCellColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Selected")), GetOverviewSelectedCellOutlineColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Selected Object")), GetOverviewSelectedObjectOutlineColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Multiple Objects")), GetOverviewMultiObjectOutlineColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewMarkerLegendSwatch (FText::FromString (TEXT ("Edge marker")), true)
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewMarkerLegendSwatch (FText::FromString (TEXT ("Center marker")), false)
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

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildOverviewMarkerLegendSwatch (const FText& Label, bool bEdgeMarker) const
{
    const FVector2D MarkerSize = bEdgeMarker
        ? FVector2D (8.f, 3.f)
        : FVector2D (4.f, 4.f);

    return SNew (SHorizontalBox)

        + SHorizontalBox::Slot ()
        .AutoWidth ()
        .VAlign (VAlign_Center)
        .Padding (0.f, 0.f, 3.f, 0.f)
        [
            SNew (SBorder)
                .Padding (1.f)
                .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
                .BorderBackgroundColor (FSlateColor (GetOverviewExistingCellColor ()))
                [
                    SNew (SBox)
                        .WidthOverride (10.f)
                        .HeightOverride (10.f)
                        [
                            SNew (SOverlay)

                            + SOverlay::Slot ()
                            .HAlign (HAlign_Center)
                            .VAlign (bEdgeMarker ? VAlign_Top : VAlign_Center)
                            [
                                SNew (SBox)
                                    .WidthOverride (MarkerSize.X)
                                    .HeightOverride (MarkerSize.Y)
                                    [
                                        SNew (SBorder)
                                            .Padding (0.f)
                                            .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
                                            .BorderBackgroundColor (FSlateColor (GetOverviewObjectMarkerColor ()))
                                    ]
                            ]
                        ]
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
        GridEditorWidgetHelpers::GetGridEnumDisplayText (CellTypeEnum, static_cast<int64> (CellData.CellType)),
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
        GridEditorWidgetHelpers::GetGridEnumDisplayText (CellTypeEnum, static_cast<int64> (CellData.CellType)),
        GetCellWallSummaryText (CellData),
        GetCellObjectSummaryText (CurrentEditorActor->SelectedCellX, CurrentEditorActor->SelectedCellY));
}

FText SGridEditorOverviewMapPanel::GetCellWallSummaryText (const FGridLevelCellData& CellData) const
{
    const UEnum* WallTypeEnum = StaticEnum<EGridWallType> ();
    const FText NorthText = GridEditorWidgetHelpers::GetGridEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.NorthWall));
    const FText EastText = GridEditorWidgetHelpers::GetGridEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.EastWall));
    const FText SouthText = GridEditorWidgetHelpers::GetGridEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.SouthWall));
    const FText WestText = GridEditorWidgetHelpers::GetGridEnumDisplayText (WallTypeEnum, static_cast<int64> (CellData.WestWall));

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
            *GridEditorWidgetHelpers::GetGridEnumDisplayText (TypeEnum, static_cast<int64> (Obj.Type)).ToString (),
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
