#include "EditorTools/GridLevelEdModeToolkit.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/Widgets/SGridEditorBehaviorPanel.h"
#include "EditorTools/Widgets/SGridEditorLinksPanel.h"
#include "EditorTools/Widgets/SGridEditorObjectInspectorPanel.h"
#include "EditorTools/Widgets/SGridEditorOverviewMapPanel.h"
#include "EditorTools/Widgets/SGridEditorToolPalettePanel.h"
#include "EditorTools/Widgets/SGridEditorValidationPanel.h"
#include "Core/GridTypes.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "EditorModeManager.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"


void FGridLevelEdModeToolkit::Init (const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
    ToolPaletteState = MakeShared<FGridEditorToolPalettePanelState> ();
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

    const FMargin PanelSpacing (0.f, 0.f, 0.f, 6.f);
    const auto AddToolkitPanel = [this, PanelSpacing] (TSharedRef<SWidget> Panel)
    {
        ToolkitRoot->AddSlot ()
            .AutoHeight ()
            .Padding (PanelSpacing)
            [
                Panel
            ];
    };

    AddToolkitPanel (
        BuildHeaderSection ());

    AddToolkitPanel (
        BuildCollapsiblePanelSection (
            FText::FromString (TEXT ("TOOLS / PALETTE")),
            [this] () -> TSharedRef<SWidget>
            {
                return SNew (SGridEditorToolPalettePanel)
                .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                .ToolPaletteState (ToolPaletteState)
                .OnGetEditorActor (FOnGetGridEditorToolPaletteActor::CreateLambda ([this] ()
                {
                    return GetEditorActor ();
                }))
                .OnRequestRefresh (FOnGridEditorToolPaletteRequestRefresh::CreateLambda ([this] ()
                {
                    RefreshPalette ();
                }));
            },
            PanelExpansionState.bToolsExpanded));

    const AGridLevelEditorActor* EditorActor = GetEditorActor ();

    AddToolkitPanel (
        BuildCollapsiblePanelSection (
            FText::FromString (TEXT ("OVERVIEW MAP")),
            [this] () -> TSharedRef<SWidget>
            {
                return SNew (SGridEditorOverviewMapPanel)
                .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                .OnGetEditorActor (FOnGetGridEditorActor::CreateLambda ([this] ()
                {
                    return GetEditorActor ();
                }))
                .OnRequestRefresh (FOnGridEditorOverviewRequestRefresh::CreateLambda ([this] ()
                {
                    RefreshPalette ();
                }));
            },
            PanelExpansionState.bOverviewExpanded));

    const FGridLevelObjectData* Obj = EditorActor ? EditorActor->GetSelectedObjectData () : nullptr;

    AddToolkitPanel (
        BuildCollapsiblePanelSection (
            FText::FromString (TEXT ("SELECTED OBJECT")),
            [this] () -> TSharedRef<SWidget>
            {
                return SNew (SGridEditorObjectInspectorPanel)
                .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                .OnGetEditorActor (FOnGetGridEditorObjectInspectorActor::CreateLambda ([this] ()
                {
                    return GetEditorActor ();
                }))
                .OnRequestRefresh (FOnGridEditorObjectInspectorRequestRefresh::CreateLambda ([this] ()
                {
                    RefreshPalette ();
                }));
            },
            PanelExpansionState.bSelectedObjectExpanded));

    if (Obj)
    {
        AddToolkitPanel (
            BuildCollapsiblePanelSection (
                FText::FromString (TEXT ("LINKS")),
                [this] () -> TSharedRef<SWidget>
                {
                    return SNew (SGridEditorLinksPanel)
                    .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                    .OnGetEditorActor (FOnGetGridEditorLinksActor::CreateLambda ([this] ()
                    {
                        return GetEditorActor ();
                    }))
                    .OnRequestRefresh (FOnGridEditorLinksRequestRefresh::CreateLambda ([this] ()
                    {
                        RefreshPalette ();
                    }));
                },
                PanelExpansionState.bLinksExpanded));

        if (Obj->Type != EGridLevelObjectType::Trigger)
        {
            AddToolkitPanel (
                BuildCollapsiblePanelSection (
                    FText::FromString (TEXT ("BEHAVIOR EDITOR")),
                    [this] () -> TSharedRef<SWidget>
                    {
                        return SNew (SGridEditorBehaviorPanel)
                        .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                        .OnGetEditorActor (FOnGetGridEditorBehaviorActor::CreateLambda ([this] ()
                        {
                            return GetEditorActor ();
                        }))
                        .OnRequestRefresh (FOnGridEditorBehaviorRequestRefresh::CreateLambda ([this] ()
                        {
                            RefreshPalette ();
                        }));
                    },
                    PanelExpansionState.bBehaviorExpanded));
        }
    }

    AddToolkitPanel (
        BuildCollapsiblePanelSection (
            FText::FromString (TEXT ("VALIDATION")),
            [this] () -> TSharedRef<SWidget>
            {
                return SNew (SGridEditorValidationPanel)
                .EditorActor (TWeakObjectPtr<AGridLevelEditorActor> (GetEditorActor ()))
                .ValidationState (ValidationState)
                .OnGetEditorActor (FOnGetGridEditorValidationActor::CreateLambda ([this] ()
                {
                    return GetEditorActor ();
                }))
                .OnRequestRefresh (FOnGridEditorValidationRequestRefresh::CreateLambda ([this] ()
                {
                    ExpandValidationIfMessagesNeedAttention ();
                    RefreshPalette ();
                }));
            },
            PanelExpansionState.bValidationExpanded));
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildToolkitWidget ()
{
    ToolkitRoot = SNew (SVerticalBox);

    TSharedRef<SWidget> Widget = SNew (SBorder).Padding (8.f)
        [SNew (SScrollBox) + SScrollBox::Slot ()[ToolkitRoot.ToSharedRef ()]];

    RefreshPalette ();

    return Widget;
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildHeaderSection ()
{
    return SNew (SBorder)
        .Padding (FMargin (6.f, 4.f))
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ()
                .AutoHeight ()
                .HAlign (HAlign_Left)
                .Padding (0.f, 0.f, 0.f, 4.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("DUNGEON EDITOR")))
                        .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 18))
                ]

                + SVerticalBox::Slot ()
                .AutoHeight ()
                [
                    SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .Padding (0.f, 0.f, 4.f, 0.f)
                        [
                            GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                                FText::FromString (TEXT ("Tool :")),
                                GetActiveToolText (),
                                FSlateColor (FLinearColor (0.25f, 0.75f, 1.f, 1.f)))
                        ]

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .Padding (0.f, 0.f, 4.f, 0.f)
                        [
                            GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                                FText::FromString (TEXT ("Cell :")),
                                GetSelectedCellStatusText (),
                                FSlateColor (FLinearColor (0.40f, 0.85f, 0.45f, 1.f)))
                        ]

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .Padding (0.f, 0.f, 4.f, 0.f)
                        [
                            GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                                FText::FromString (TEXT ("Edge/Facing :")),
                                GetSelectedEdgeStatusText (),
                                FSlateColor (FLinearColor (1.f, 0.72f, 0.20f, 1.f)))
                        ]

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .Padding (0.f, 0.f, 4.f, 0.f)
                        [
                            GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                                FText::FromString (TEXT ("Object :")),
                                GetSelectedObjectStatusText (),
                                FSlateColor (FLinearColor (0.70f, 0.55f, 1.f, 1.f)))
                        ]

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .Padding (0.f, 0.f, 0.f, 0.f)
                        [
                            GridEditorWidgetHelpers::BuildGridCompactStatusBadge (
                                FText::FromString (TEXT ("Validation :")),
                                GetValidationStatusText (),
                                FSlateColor (ValidationState.IsValid () && ValidationState->bValidationHasRun
                                    ? FLinearColor (1.f, 0.72f, 0.20f, 1.f)
                                    : FLinearColor (0.50f, 0.50f, 0.50f, 1.f)))
                        ]
                    ]
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildCollapsiblePanelSection (
    const FText& Title,
    const TFunctionRef<TSharedRef<SWidget> ()>& BuildContent,
    bool& bExpanded)
{
    return GridEditorWidgetHelpers::BuildGridCollapsiblePanelSection (
        Title,
        BuildContent,
        bExpanded,
        FOnClicked::CreateRaw (this, &FGridLevelEdModeToolkit::TogglePanelExpansion, &bExpanded));
}

FReply FGridLevelEdModeToolkit::TogglePanelExpansion (bool* bExpanded)
{
    if (bExpanded)
    {
        *bExpanded = !*bExpanded;
        RefreshPalette ();
    }

    return FReply::Handled ();
}

void FGridLevelEdModeToolkit::ExpandValidationIfMessagesNeedAttention ()
{
    if (!ValidationState.IsValid ())
    {
        return;
    }

    int32 ErrorCount = 0;
    int32 WarningCount = 0;
    ValidationState->CountValidationErrorsWarnings (ErrorCount, WarningCount);

    if (ErrorCount > 0 || WarningCount > 0)
    {
        PanelExpansionState.bValidationExpanded = true;
    }
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
#endif
