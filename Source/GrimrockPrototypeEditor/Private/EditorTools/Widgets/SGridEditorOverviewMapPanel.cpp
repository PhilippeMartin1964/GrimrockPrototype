#include "EditorTools/Widgets/SGridEditorOverviewMapPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"

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

    bool IsDoorMarker (const FGridLevelObjectData& Obj)
    {
        return Obj.Type == EGridLevelObjectType::Door;
    }

    FMargin GetOverviewMarkerPadding (EGridEditorOverviewObjectAnchor Anchor, bool bDoorMarker)
    {
        switch (Anchor)
        {
            case EGridEditorOverviewObjectAnchor::North:
                return bDoorMarker ? FMargin (3.f, 2.f, 0.f, 0.f) : FMargin (6.f, 2.f, 0.f, 0.f);

            case EGridEditorOverviewObjectAnchor::East:
                return bDoorMarker ? FMargin (14.f, 3.f, 0.f, 0.f) : FMargin (14.f, 6.f, 0.f, 0.f);

            case EGridEditorOverviewObjectAnchor::South:
                return bDoorMarker ? FMargin (3.f, 14.f, 0.f, 0.f) : FMargin (6.f, 14.f, 0.f, 0.f);

            case EGridEditorOverviewObjectAnchor::West:
                return bDoorMarker ? FMargin (2.f, 3.f, 0.f, 0.f) : FMargin (2.f, 6.f, 0.f, 0.f);

            case EGridEditorOverviewObjectAnchor::Center:
            default:
                return FMargin (7.f, 7.f, 0.f, 0.f);
        }
    }

    FVector2D GetOverviewMarkerSize (EGridEditorOverviewObjectAnchor Anchor, bool bDoorMarker)
    {
        switch (Anchor)
        {
            case EGridEditorOverviewObjectAnchor::North:
            case EGridEditorOverviewObjectAnchor::South:
                return bDoorMarker ? FVector2D (12.f, 2.f) : FVector2D (6.f, 2.f);

            case EGridEditorOverviewObjectAnchor::East:
            case EGridEditorOverviewObjectAnchor::West:
                return bDoorMarker ? FVector2D (2.f, 12.f) : FVector2D (2.f, 6.f);

            case EGridEditorOverviewObjectAnchor::Center:
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

        const EGridEditorOverviewObjectAnchor MarkerAnchor = GetObjectAnchor (*Obj);
        bool* bAnchorAlreadyUsed = nullptr;
        bool* bAnchorUsesDoorGeometry = nullptr;
        switch (MarkerAnchor)
        {
            case EGridEditorOverviewObjectAnchor::North:
                bAnchorAlreadyUsed = &bHasNorthMarker;
                bAnchorUsesDoorGeometry = &bNorthMarkerIsDoor;
                break;

            case EGridEditorOverviewObjectAnchor::East:
                bAnchorAlreadyUsed = &bHasEastMarker;
                bAnchorUsesDoorGeometry = &bEastMarkerIsDoor;
                break;

            case EGridEditorOverviewObjectAnchor::South:
                bAnchorAlreadyUsed = &bHasSouthMarker;
                bAnchorUsesDoorGeometry = &bSouthMarkerIsDoor;
                break;

            case EGridEditorOverviewObjectAnchor::West:
                bAnchorAlreadyUsed = &bHasWestMarker;
                bAnchorUsesDoorGeometry = &bWestMarkerIsDoor;
                break;

            case EGridEditorOverviewObjectAnchor::Center:
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

    auto AddMarker = [&MarkerOverlay] (EGridEditorOverviewObjectAnchor MarkerAnchor, bool bDoorMarker)
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
        AddMarker (EGridEditorOverviewObjectAnchor::North, bNorthMarkerIsDoor);
    }

    if (bHasEastMarker)
    {
        AddMarker (EGridEditorOverviewObjectAnchor::East, bEastMarkerIsDoor);
    }

    if (bHasSouthMarker)
    {
        AddMarker (EGridEditorOverviewObjectAnchor::South, bSouthMarkerIsDoor);
    }

    if (bHasWestMarker)
    {
        AddMarker (EGridEditorOverviewObjectAnchor::West, bWestMarkerIsDoor);
    }

    if (bHasCenterMarker)
    {
        AddMarker (EGridEditorOverviewObjectAnchor::Center, false);
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
        ? GetOverviewMarkerSize (EGridEditorOverviewObjectAnchor::North, false)
        : FVector2D (4.f, 4.f);
    const FMargin MarkerPadding = bEdgeMarker
        ? GetOverviewMarkerPadding (EGridEditorOverviewObjectAnchor::North, false)
        : GetOverviewMarkerPadding (EGridEditorOverviewObjectAnchor::Center, false);

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

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 1.f, 0.f, 0.f)
            [
                BuildObjectsOnSelectedCellSection ()
            ]
        ];
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildObjectsOnSelectedCellSection ()
{
    const TArray<FGridEditorOverviewAnchorObjectGroup> ObjectGroups = GetObjectsAtSelectedCellGroupedByAnchor ();

    TSharedRef<SVerticalBox> ObjectsBox = SNew (SVerticalBox);

    ObjectsBox->AddSlot ()
    .AutoHeight ()
    .Padding (0.f, 0.f, 0.f, 4.f)
    [
        SNew (STextBlock)
            .Text (FText::FromString (TEXT ("OBJECTS ON CELL")))
            .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 8))
            .ColorAndOpacity (FSlateColor (FLinearColor (0.72f, 0.72f, 0.72f, 1.f)))
    ];

    bool bHasObjects = false;
    for (const FGridEditorOverviewAnchorObjectGroup& Group : ObjectGroups)
    {
        if (Group.Objects.Num () == 0)
        {
            continue;
        }

        bHasObjects = true;
        ObjectsBox->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 4.f)
        [
            BuildObjectAnchorGroup (Group)
        ];
    }

    if (!bHasObjects)
    {
        ObjectsBox->AddSlot ()
        .AutoHeight ()
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("No objects on selected cell.")))
                .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8))
                .ColorAndOpacity (FSlateColor (FLinearColor (0.62f, 0.62f, 0.62f, 1.f)))
        ];
    }

    return ObjectsBox;
}

TSharedRef<SWidget> SGridEditorOverviewMapPanel::BuildObjectAnchorGroup (
    const FGridEditorOverviewAnchorObjectGroup& Group)
{
    TSharedRef<SVerticalBox> GroupBox = SNew (SVerticalBox);

    GroupBox->AddSlot ()
    .AutoHeight ()
    .Padding (0.f, 0.f, 0.f, 2.f)
    [
        SNew (STextBlock)
            .Text (GetObjectAnchorLabel (Group.Anchor))
            .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 8))
            .ColorAndOpacity (FSlateColor (FLinearColor (0.86f, 0.86f, 0.86f, 1.f)))
    ];

    for (const FGridLevelObjectData* Object : Group.Objects)
    {
        if (!Object)
        {
            continue;
        }

        const FGuid ObjectId = Object->ObjectId;

        GroupBox->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 1.f, 0.f, 1.f)
        [
            SNew (SHorizontalBox)

            + SHorizontalBox::Slot ()
            .FillWidth (1.f)
            .VAlign (VAlign_Center)
            .Padding (0.f, 0.f, 5.f, 0.f)
            [
                SNew (STextBlock)
                    .Text (GetSelectedCellObjectSummaryText (*Object))
                    .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 8))
                    .AutoWrapText (true)
            ]

            + SHorizontalBox::Slot ()
            .AutoWidth ()
            .VAlign (VAlign_Center)
            [
                SNew (SButton)
                    .Text (FText::FromString (TEXT ("Select")))
                    .HAlign (HAlign_Center)
                    .ContentPadding (FMargin (6.f, 1.f))
                    .ToolTipText (FText::FromString (TEXT ("Select this object.")))
                    .OnClicked_Lambda ([this, ObjectId] () -> FReply
                    {
                        return OnSelectObjectFromSelectedCellClicked (ObjectId);
                    })
            ]
        ];
    }

    return GroupBox;
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

FReply SGridEditorOverviewMapPanel::OnSelectObjectFromSelectedCellClicked (FGuid ObjectId)
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        if (CurrentEditorActor->SelectObjectById (ObjectId))
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

FText SGridEditorOverviewMapPanel::GetObjectAnchorLabel (EGridEditorOverviewObjectAnchor Anchor) const
{
    switch (Anchor)
    {
        case EGridEditorOverviewObjectAnchor::North:
            return FText::FromString (TEXT ("North"));

        case EGridEditorOverviewObjectAnchor::East:
            return FText::FromString (TEXT ("East"));

        case EGridEditorOverviewObjectAnchor::South:
            return FText::FromString (TEXT ("South"));

        case EGridEditorOverviewObjectAnchor::West:
            return FText::FromString (TEXT ("West"));

        case EGridEditorOverviewObjectAnchor::Center:
            return FText::FromString (TEXT ("Center"));

        case EGridEditorOverviewObjectAnchor::None:
        default:
            return FText::FromString (TEXT ("Unknown"));
    }
}

FText SGridEditorOverviewMapPanel::GetSelectedCellObjectSummaryText (const FGridLevelObjectData& Object) const
{
    const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
    const FString TypeText = GridEditorWidgetHelpers::GetGridEnumDisplayText (
        TypeEnum,
        static_cast<int64> (Object.Type)).ToString ();
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Object.ArchetypeId)
        : nullptr;

    FString IdentifierText;
    if (!Object.Tag.IsNone ())
    {
        IdentifierText = FString::Printf (TEXT ("Tag=%s"), *Object.Tag.ToString ());
    }
    else if (!Object.ArchetypeId.IsNone ())
    {
        IdentifierText = FString::Printf (TEXT ("Archetype=%s"), *Object.ArchetypeId.ToString ());
    }
    else if (!Object.PaletteEntryId.IsNone ())
    {
        IdentifierText = FString::Printf (TEXT ("Palette=%s"), *Object.PaletteEntryId.ToString ());
    }
    else
    {
        IdentifierText = FString::Printf (TEXT ("Id=%s"), *Object.ObjectId.ToString ().Left (8));
    }

    FString ArchetypeDetails;
    if (Archetype)
    {
        const UEnum* CategoryEnum = StaticEnum<EGridObjectCategory> ();
        const UEnum* PlacementEnum = StaticEnum<EGridObjectPlacementKind> ();
        ArchetypeDetails = FString::Printf (
            TEXT (" | %s/%s"),
            *GridEditorWidgetHelpers::GetGridEnumDisplayText (
                CategoryEnum,
                static_cast<int64> (Archetype->ObjectCategory)).ToString (),
            *GridEditorWidgetHelpers::GetGridEnumDisplayText (
                PlacementEnum,
                static_cast<int64> (Archetype->PlacementKind)).ToString ());
    }

    return FText::FromString (FString::Printf (TEXT ("%s | %s%s"), *TypeText, *IdentifierText, *ArchetypeDetails));
}

EGridEditorOverviewObjectAnchor SGridEditorOverviewMapPanel::GetObjectAnchor (
    const FGridLevelObjectData& Object) const
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Object.ArchetypeId)
        : nullptr;
    if (Archetype)
    {
        if (!Archetype->IsEdgePlaced ())
        {
            return EGridEditorOverviewObjectAnchor::Center;
        }

        switch (Object.Edge)
        {
            case EGridEdge::North:
                return EGridEditorOverviewObjectAnchor::North;

            case EGridEdge::East:
                return EGridEditorOverviewObjectAnchor::East;

            case EGridEdge::South:
                return EGridEditorOverviewObjectAnchor::South;

            case EGridEdge::West:
                return EGridEditorOverviewObjectAnchor::West;

            case EGridEdge::None:
            default:
                return EGridEditorOverviewObjectAnchor::Center;
        }
    }

    if (!IsOverviewEdgeObject (Object.Type))
    {
        return EGridEditorOverviewObjectAnchor::Center;
    }

    switch (Object.Edge)
    {
        case EGridEdge::North:
            return EGridEditorOverviewObjectAnchor::North;

        case EGridEdge::East:
            return EGridEditorOverviewObjectAnchor::East;

        case EGridEdge::South:
            return EGridEditorOverviewObjectAnchor::South;

        case EGridEdge::West:
            return EGridEditorOverviewObjectAnchor::West;

        case EGridEdge::None:
        default:
            return EGridEditorOverviewObjectAnchor::Center;
    }
}

TArray<FGridEditorOverviewAnchorObjectGroup> SGridEditorOverviewMapPanel::GetObjectsAtSelectedCellGroupedByAnchor () const
{
    TArray<FGridEditorOverviewAnchorObjectGroup> Groups;
    Groups.Reserve (5);

    auto AddGroup = [&Groups] (EGridEditorOverviewObjectAnchor Anchor)
    {
        FGridEditorOverviewAnchorObjectGroup& Group = Groups.AddDefaulted_GetRef ();
        Group.Anchor = Anchor;
    };

    AddGroup (EGridEditorOverviewObjectAnchor::North);
    AddGroup (EGridEditorOverviewObjectAnchor::East);
    AddGroup (EGridEditorOverviewObjectAnchor::South);
    AddGroup (EGridEditorOverviewObjectAnchor::West);
    AddGroup (EGridEditorOverviewObjectAnchor::Center);

    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridLevelAsset* LevelAsset = CurrentEditorActor ? CurrentEditorActor->LevelAsset : nullptr;
    if (!CurrentEditorActor || !LevelAsset || !CurrentEditorActor->IsSelectionValidForEditing ())
    {
        return Groups;
    }

    for (const FGridLevelObjectData& Object : LevelAsset->Objects)
    {
        if (Object.CellX != CurrentEditorActor->SelectedCellX ||
            Object.CellY != CurrentEditorActor->SelectedCellY)
        {
            continue;
        }

        const EGridEditorOverviewObjectAnchor Anchor = GetObjectAnchor (Object);
        for (FGridEditorOverviewAnchorObjectGroup& Group : Groups)
        {
            if (Group.Anchor == Anchor)
            {
                Group.Objects.Add (&Object);
                break;
            }
        }
    }

    return Groups;
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
