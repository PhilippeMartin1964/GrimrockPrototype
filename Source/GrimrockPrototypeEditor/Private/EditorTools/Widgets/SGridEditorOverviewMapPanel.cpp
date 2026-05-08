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
        return FLinearColor (0.019f, 0.019f, 0.019f, 1.f);
    }

    FLinearColor GetOverviewExistingCellColor ()
    {
        return FLinearColor (0.957f, 0.957f, 0.957f, 1.f);
    }

    FLinearColor GetOverviewSelectedCellOutlineColor ()
    {
        return FLinearColor (0.125f, 0.125f, 0.125f, 1.f);
    }

    FLinearColor GetOverviewSelectedCellFillColor ()
    {
        return FLinearColor (1.f, 0.847f, 0.29f, 1.f);
    }

    FLinearColor GetOverviewSelectedObjectOutlineColor ()
    {
        return FLinearColor (0.263f, 0.812f, 1.f, 1.f);
    }

    FLinearColor GetOverviewSelectedObjectFillColor ()
    {
        return FLinearColor (0.333f, 0.839f, 1.f, 1.f);
    }

    FLinearColor GetOverviewMultiObjectOutlineColor ()
    {
        return FLinearColor (0.318f, 0.847f, 0.416f, 1.f);
    }

    FLinearColor GetOverviewObjectMarkerColor ()
    {
        return FLinearColor (0.125f, 0.125f, 0.125f, 1.f);
    }

    FSlateColor GetOverviewCellColor (
        const FGridLevelCellData* CellData,
        bool bSelectedCell,
        bool bSelectedObjectCell)
    {
        if (!CellData)
        {
            return FSlateColor (GetOverviewEmptyColor ());
        }

        if (bSelectedCell)
        {
            return FSlateColor (GetOverviewSelectedCellFillColor ());
        }

        if (CellData->CellType == EGridCellType::Empty)
        {
            return FSlateColor (GetOverviewEmptyColor ());
        }

        if (bSelectedObjectCell)
        {
            return FSlateColor (GetOverviewSelectedObjectFillColor ());
        }

        return FSlateColor (GetOverviewExistingCellColor ());
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

    enum class EOverviewMarkerAnchor : uint8
    {
        North,
        East,
        South,
        West,
        Center
    };

    EOverviewMarkerAnchor GetOverviewMarkerAnchor (const FGridLevelObjectData& Obj)
    {
        if (!IsOverviewEdgeObject (Obj.Type))
        {
            return EOverviewMarkerAnchor::Center;
        }

        switch (Obj.Edge)
        {
            case EGridEdge::North: return EOverviewMarkerAnchor::North;
            case EGridEdge::East:  return EOverviewMarkerAnchor::East;
            case EGridEdge::South: return EOverviewMarkerAnchor::South;
            case EGridEdge::West:  return EOverviewMarkerAnchor::West;
            default:               return EOverviewMarkerAnchor::Center;
        }
    }

    bool IsDoorMarker (const FGridLevelObjectData& Obj)
    {
        return Obj.Type == EGridLevelObjectType::Door;
    }

    FMargin GetOverviewMarkerPadding (EOverviewMarkerAnchor Anchor, bool bDoorMarker)
    {
        switch (Anchor)
        {
            case EOverviewMarkerAnchor::North:
                return bDoorMarker ? FMargin (3.f, 2.f, 0.f, 0.f) : FMargin (6.f, 2.f, 0.f, 0.f);

            case EOverviewMarkerAnchor::East:
                return bDoorMarker ? FMargin (14.f, 3.f, 0.f, 0.f) : FMargin (14.f, 6.f, 0.f, 0.f);

            case EOverviewMarkerAnchor::South:
                return bDoorMarker ? FMargin (3.f, 14.f, 0.f, 0.f) : FMargin (6.f, 14.f, 0.f, 0.f);

            case EOverviewMarkerAnchor::West:
                return bDoorMarker ? FMargin (2.f, 3.f, 0.f, 0.f) : FMargin (2.f, 6.f, 0.f, 0.f);

            case EOverviewMarkerAnchor::Center:
            default:
                return FMargin (7.f, 7.f, 0.f, 0.f);
        }
    }

    FVector2D GetOverviewMarkerSize (EOverviewMarkerAnchor Anchor, bool bDoorMarker)
    {
        switch (Anchor)
        {
            case EOverviewMarkerAnchor::North:
            case EOverviewMarkerAnchor::South:
                return bDoorMarker ? FVector2D (12.f, 2.f) : FVector2D (6.f, 2.f);

            case EOverviewMarkerAnchor::East:
            case EOverviewMarkerAnchor::West:
                return bDoorMarker ? FVector2D (2.f, 12.f) : FVector2D (2.f, 6.f);

            case EOverviewMarkerAnchor::Center:
            default:
                return FVector2D (4.f, 4.f);
        }
    }

    void AddOverviewOutline (const TSharedRef<SOverlay>& CellOverlay, const FSlateColor& OutlineColor)
    {
        auto AddOutlineStrip = [&CellOverlay, &OutlineColor] (float X, float Y, float Width, float Height)
        {
            CellOverlay->AddSlot ()
            .HAlign (HAlign_Left)
            .VAlign (VAlign_Top)
            .Padding (FMargin (X, Y, 0.f, 0.f))
            [
                SNew (SBox)
                    .WidthOverride (Width)
                    .HeightOverride (Height)
                    [
                        SNew (SBorder)
                            .Padding (0.f)
                            .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
                            .BorderBackgroundColor (OutlineColor)
                    ]
            ];
        };

        AddOutlineStrip (0.f, 0.f, 18.f, 1.f);
        AddOutlineStrip (0.f, 17.f, 18.f, 1.f);
        AddOutlineStrip (0.f, 0.f, 1.f, 18.f);
        AddOutlineStrip (17.f, 0.f, 1.f, 18.f);
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
    const bool bExistingCell = CellData && CellData->CellType != EGridCellType::Empty;

    const bool bHasSpecialOutline = CellData && (bSelectedCell || (bExistingCell && (bSelectedObjectCell || ObjectCount > 1)));
    const FSlateColor FillColor = GetOverviewCellColor (CellData, bSelectedCell, bSelectedObjectCell);
    const FSlateColor OutlineColor = bSelectedCell
        ? FSlateColor (GetOverviewSelectedCellOutlineColor ())
        : bSelectedObjectCell
        ? FSlateColor (GetOverviewSelectedObjectOutlineColor ())
        : ObjectCount > 1
        ? FSlateColor (GetOverviewMultiObjectOutlineColor ())
        : FillColor;

    TSharedRef<SOverlay> CellOverlay = SNew (SOverlay);

    CellOverlay->AddSlot ()
    [
        SNew (SBorder)
            .Padding (0.f)
            .BorderImage (FCoreStyle::Get ().GetBrush ("WhiteBrush"))
            .BorderBackgroundColor (FillColor)
    ];

    if (bExistingCell)
    {
        CellOverlay->AddSlot ()
        [
            BuildCellObjectMarkers (CellObjects)
        ];
    }

    if (bHasSpecialOutline)
    {
        AddOverviewOutline (CellOverlay, OutlineColor);
    }

    return SNew (SBox)
        .WidthOverride (18.f)
        .HeightOverride (18.f)
        [
            SNew (SButton)
            .ButtonStyle (&FCoreStyle::Get ().GetWidgetStyle<FButtonStyle> ("NoBorder"))
            .ButtonColorAndOpacity (FLinearColor::White)
            .ContentPadding (FMargin (0.f))
            .ToolTipText (GetOverviewCellTooltipText (CellX, CellY))
            .OnClicked_Lambda ([this, CellX, CellY] () -> FReply
            {
                return OnOverviewCellClicked (CellX, CellY);
            })
            [
                SNew (SBox)
                    .WidthOverride (18.f)
                    .HeightOverride (18.f)
                    [
                        CellOverlay
                    ]
            ]
        ];
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildCellObjectMarkers (const TArray<const FGridLevelObjectData*>& CellObjects) const
{
    TSharedRef<SOverlay> MarkerOverlay = SNew (SOverlay);

    bool bHasNorthMarker = false;
    bool bHasEastMarker = false;
    bool bHasSouthMarker = false;
    bool bHasWestMarker = false;
    bool bHasCenterMarker = false;
    bool bNorthMarkerIsDoor = false;
    bool bEastMarkerIsDoor = false;
    bool bSouthMarkerIsDoor = false;
    bool bWestMarkerIsDoor = false;

    for (const FGridLevelObjectData* Obj : CellObjects)
    {
        if (!Obj)
        {
            continue;
        }

        const EOverviewMarkerAnchor MarkerAnchor = GetOverviewMarkerAnchor (*Obj);
        bool* bAnchorAlreadyUsed = nullptr;
        bool* bAnchorUsesDoorGeometry = nullptr;
        switch (MarkerAnchor)
        {
            case EOverviewMarkerAnchor::North:
                bAnchorAlreadyUsed = &bHasNorthMarker;
                bAnchorUsesDoorGeometry = &bNorthMarkerIsDoor;
                break;

            case EOverviewMarkerAnchor::East:
                bAnchorAlreadyUsed = &bHasEastMarker;
                bAnchorUsesDoorGeometry = &bEastMarkerIsDoor;
                break;

            case EOverviewMarkerAnchor::South:
                bAnchorAlreadyUsed = &bHasSouthMarker;
                bAnchorUsesDoorGeometry = &bSouthMarkerIsDoor;
                break;

            case EOverviewMarkerAnchor::West:
                bAnchorAlreadyUsed = &bHasWestMarker;
                bAnchorUsesDoorGeometry = &bWestMarkerIsDoor;
                break;

            case EOverviewMarkerAnchor::Center:
            default:
                bAnchorAlreadyUsed = &bHasCenterMarker;
                break;
        }

        *bAnchorAlreadyUsed = true;
        if (bAnchorUsesDoorGeometry && IsDoorMarker (*Obj))
        {
            *bAnchorUsesDoorGeometry = true;
        }
    }

    auto AddMarker = [&MarkerOverlay] (EOverviewMarkerAnchor MarkerAnchor, bool bDoorMarker)
    {
        const FVector2D MarkerSize = GetOverviewMarkerSize (MarkerAnchor, bDoorMarker);

        MarkerOverlay->AddSlot ()
        .HAlign (HAlign_Left)
        .VAlign (VAlign_Top)
        .Padding (GetOverviewMarkerPadding (MarkerAnchor, bDoorMarker))
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
    };

    if (bHasNorthMarker)
    {
        AddMarker (EOverviewMarkerAnchor::North, bNorthMarkerIsDoor);
    }

    if (bHasEastMarker)
    {
        AddMarker (EOverviewMarkerAnchor::East, bEastMarkerIsDoor);
    }

    if (bHasSouthMarker)
    {
        AddMarker (EOverviewMarkerAnchor::South, bSouthMarkerIsDoor);
    }

    if (bHasWestMarker)
    {
        AddMarker (EOverviewMarkerAnchor::West, bWestMarkerIsDoor);
    }

    if (bHasCenterMarker)
    {
        AddMarker (EOverviewMarkerAnchor::Center, false);
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
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Selected")), GetOverviewSelectedCellFillColor ())
        ]

        + SWrapBox::Slot ().Padding (0.f, 0.f, 6.f, 4.f)
        [
            BuildOverviewLegendSwatch (FText::FromString (TEXT ("Selected Object")), GetOverviewSelectedObjectFillColor ())
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
        ? GetOverviewMarkerSize (EOverviewMarkerAnchor::North, false)
        : FVector2D (4.f, 4.f);
    const FMargin MarkerPadding = bEdgeMarker
        ? GetOverviewMarkerPadding (EOverviewMarkerAnchor::North, false)
        : GetOverviewMarkerPadding (EOverviewMarkerAnchor::Center, false);

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
                        .WidthOverride (18.f)
                        .HeightOverride (18.f)
                        [
                            SNew (SOverlay)

                            + SOverlay::Slot ()
                            .HAlign (HAlign_Left)
                            .VAlign (VAlign_Top)
                            .Padding (MarkerPadding)
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
