#include "EditorTools/Widgets/SGridEditorObjectInspectorPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Core/GridObjectPaletteAsset.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Templates/Function.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

namespace
{
    FText GetInitialActiveStateText (const FGridLevelObjectData& Obj, const TCHAR* ActiveText, const TCHAR* InactiveText)
    {
        return FText::FromString (Obj.bInitiallyActive ? FString (ActiveText) : FString (InactiveText));
    }

    FText GetBoolText (bool bValue)
    {
        return FText::FromString (bValue ? TEXT ("Yes") : TEXT ("No"));
    }

    FText GetNameText (const FName& Name)
    {
        return Name.IsNone ()
            ? FText::FromString (TEXT ("None"))
            : FText::FromName (Name);
    }

    FText GetObjectNameText (const UObject* Object)
    {
        return Object
            ? FText::FromString (Object->GetName ())
            : FText::FromString (TEXT ("None"));
    }

    FText GetClassNameText (const UClass* Class)
    {
        return Class
            ? FText::FromString (Class->GetName ())
            : FText::FromString (TEXT ("None"));
    }

    FText GetEdgeOrFacingText (const FGridLevelObjectData& Obj)
    {
        const UEnum* EdgeEnum = StaticEnum<EGridEdge> ();
        if (Obj.Edge != EGridEdge::None)
        {
            return GridEditorWidgetHelpers::GetGridEnumDisplayText (EdgeEnum, static_cast<int64>(Obj.Edge));
        }

        return FText::Format (
            FText::FromString (TEXT ("Facing {0} deg")),
            FText::AsNumber (Obj.LocalYaw));
    }

    FText GetHeaderPlacementText (const FGridLevelObjectData& Obj)
    {
        if (Obj.Edge != EGridEdge::None)
        {
            return FText::Format (
                FText::FromString (TEXT ("@ ({0},{1}) {2}")),
                FText::AsNumber (Obj.CellX),
                FText::AsNumber (Obj.CellY),
                GetEdgeOrFacingText (Obj));
        }

        if (!FMath::IsNearlyZero (Obj.LocalYaw))
        {
            return FText::Format (
                FText::FromString (TEXT ("@ ({0},{1}) {2}")),
                FText::AsNumber (Obj.CellX),
                FText::AsNumber (Obj.CellY),
                GetEdgeOrFacingText (Obj));
        }

        return FText::Format (
            FText::FromString (TEXT ("@ ({0},{1})")),
            FText::AsNumber (Obj.CellX),
            FText::AsNumber (Obj.CellY));
    }

    TSharedRef<SWidget> BuildExplicitConnectorSummary (const FText& EmitsText)
    {
        TSharedRef<SVerticalBox> Root = SNew (SVerticalBox)

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Emits")),
                    EmitsText)
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Connector Rule")),
                    FText::FromString (TEXT ("Only explicit SourceEvent links execute.")))
            ];

        return Root;
    }

    FText GetConnectorEventText (EGridObjectEvent Event)
    {
        const UEnum* EventEnum = StaticEnum<EGridObjectEvent> ();
        const FText EventText = EventEnum
            ? EventEnum->GetDisplayNameTextByValue (static_cast<int64> (Event))
            : FText::FromString (TEXT ("Unknown"));

        return FText::Format (
            FText::FromString (TEXT ("On {0}")),
            EventText);
    }

    FText GetConnectorCommandText (EGridObjectCommand Command)
    {
        const UEnum* CommandEnum = StaticEnum<EGridObjectCommand> ();
        return CommandEnum
            ? CommandEnum->GetDisplayNameTextByValue (static_cast<int64> (Command))
            : FText::FromString (TEXT ("Unknown"));
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

    FText GetConnectorObjectSummaryText (
        const AGridLevelEditorActor* EditorActor,
        const FGridLevelObjectData& Object)
    {
        const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
        const UGridObjectArchetypeAsset* Archetype = EditorActor
            ? EditorActor->FindObjectArchetypeById (Object.ArchetypeId)
            : nullptr;

        const FText NameText = Archetype && !Archetype->DisplayName.IsEmpty ()
            ? Archetype->DisplayName
            : GridEditorWidgetHelpers::GetGridEnumDisplayText (TypeEnum, static_cast<int64>(Object.Type));

        return FText::Format (
            FText::FromString (TEXT ("{0} {1}")),
            NameText,
            GetHeaderPlacementText (Object));
    }

    TSharedRef<SWidget> BuildConnectorTextRow (const FText& Text, bool bWarning)
    {
        return SNew (STextBlock)
            .Text (Text)
            .AutoWrapText (true)
            .ColorAndOpacity (bWarning
                ? FSlateColor (FLinearColor (1.f, 0.55f, 0.18f, 1.f))
                : FSlateColor::UseForeground ());
    }

    bool IsObjectOrientationEditable (const FGridLevelObjectData& Obj, const UGridObjectArchetypeAsset* Archetype)
    {
        if (!Archetype)
        {
            return false;
        }

        const bool bPlacementCanFace =
            Archetype->PlacementKind == EGridObjectPlacementKind::Edge ||
            Archetype->PlacementKind == EGridObjectPlacementKind::Wall ||
            Archetype->PlacementKind == EGridObjectPlacementKind::Floor ||
            Archetype->PlacementKind == EGridObjectPlacementKind::Center;
        if (!bPlacementCanFace)
        {
            return false;
        }

        if (Obj.Type == EGridLevelObjectType::Trigger ||
            Obj.Type == EGridLevelObjectType::ItemSpawn ||
            Obj.Type == EGridLevelObjectType::MonsterSpawn)
        {
            return Archetype->PreviewMesh || Archetype->FixedMesh || Archetype->MovingMesh;
        }

        return Archetype->PreviewMesh ||
            Archetype->FixedMesh ||
            Archetype->MovingMesh ||
            Archetype->RuntimeActorClass ||
            Archetype->ItemActorClass;
    }

    TSharedRef<SWidget> BuildBehaviorFloatSpinBoxRow (
        const FText& Label,
        float Value,
        TFunction<void(float)> ApplyValue)
    {
        return GridEditorWidgetHelpers::BuildGridPropertyRow (
            Label,
            SNew (SSpinBox<float>)
                .Value (Value)
                .MinDesiredWidth (90.f)
                .OnValueCommitted_Lambda ([ApplyValue] (float NewValue, ETextCommit::Type CommitType)
            {
                ApplyValue (NewValue);
            }));
    }
}

void SGridEditorObjectInspectorPanel::Construct (const FArguments& InArgs)
{
    EditorActor = InArgs._EditorActor;
    OnGetEditorActor = InArgs._OnGetEditorActor;
    OnRequestRefresh = InArgs._OnRequestRefresh;

    ChildSlot
    [
        BuildObjectInspectorSection ()
    ];
}

AGridLevelEditorActor* SGridEditorObjectInspectorPanel::GetEditorActor () const
{
    if (EditorActor.IsValid ())
    {
        return EditorActor.Get ();
    }

    return OnGetEditorActor.IsBound ()
        ? OnGetEditorActor.Execute ()
        : nullptr;
}

void SGridEditorObjectInspectorPanel::RequestRefresh () const
{
    if (OnRequestRefresh.IsBound ())
    {
        OnRequestRefresh.Execute ();
    }
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildObjectInspectorSection ()
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (!CurrentEditorActor || !CurrentEditorActor->LevelAsset)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No editor actor or level asset.")))
            ];
        return Root;
    }

    const FGridLevelObjectData* Obj = CurrentEditorActor->GetSelectedObjectData ();
    if (!Obj)
    {
        Root->AddSlot ().AutoHeight ()
            [
                SNew (STextBlock).Text (FText::FromString (TEXT ("No selected object.")))
            ];
        return Root;
    }

    Root->AddSlot ().AutoHeight ()
        [
            BuildSelectedObjectCard (*Obj)
        ];

    const UGridObjectArchetypeAsset* SelectedArchetype = CurrentEditorActor->FindObjectArchetypeById (Obj->ArchetypeId);
    const bool bShowOrientationWidget = IsObjectOrientationEditable (*Obj, SelectedArchetype);

    Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 4.f, 0.f)
                [
                    GridEditorWidgetHelpers::BuildGridActionButton (
                        FText::FromString (TEXT ("Move To Current Cell")),
                        FOnClicked::CreateSP (this, &SGridEditorObjectInspectorPanel::OnMoveSelectedObjectToCurrentCellClicked))
                ]
                + SHorizontalBox::Slot ().AutoWidth ().Padding (4.f, 0.f, 0.f, 0.f)
                [
                    bShowOrientationWidget
                        ? BuildOrientationWidget (*Obj)
                        : SNullWidget::NullWidget
                ]
        ];

    return Root;
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildOrientationWidget (const FGridLevelObjectData& Obj)
{
    const bool bUsesEdge = Obj.Edge != EGridEdge::None;
    const EGridEdge CurrentOrientation = bUsesEdge
        ? Obj.Edge
        : (FMath::IsNearlyEqual (Obj.LocalYaw, 90.f) ? EGridEdge::East
            : FMath::IsNearlyEqual (Obj.LocalYaw, 180.f) ? EGridEdge::South
            : FMath::IsNearlyEqual (Obj.LocalYaw, 270.f) ? EGridEdge::West
            : EGridEdge::North);

    auto MakeButton = [this, CurrentOrientation] (const TCHAR* Label, EGridEdge Orientation) -> TSharedRef<SWidget>
    {
        return SNew (SButton)
            .Text (FText::FromString (Label))
            .ContentPadding (FMargin (6.f, 3.f))
            .ButtonColorAndOpacity (CurrentOrientation == Orientation
                ? FLinearColor (0.25f, 0.45f, 0.75f, 1.f)
                : FLinearColor::White)
            .OnClicked (FOnClicked::CreateSP (this, &SGridEditorObjectInspectorPanel::OnSetSelectedObjectOrientationClicked, Orientation));
    };

    return SNew (SHorizontalBox)
        + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 6.f, 0.f)
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("Orientation")))
                .ColorAndOpacity (FSlateColor (FLinearColor (0.72f, 0.72f, 0.72f, 1.f)))
        ]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 2.f, 0.f)
        [
            MakeButton (TEXT ("North"), EGridEdge::North)
        ]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f, 0.f)
        [
            MakeButton (TEXT ("East"), EGridEdge::East)
        ]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f, 0.f)
        [
            MakeButton (TEXT ("South"), EGridEdge::South)
        ]
        + SHorizontalBox::Slot ().AutoWidth ().Padding (2.f, 0.f, 0.f, 0.f)
        [
            MakeButton (TEXT ("West"), EGridEdge::West)
        ];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildSelectedObjectCard (const FGridLevelObjectData& Obj)
{
    const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType> ();
    const FText TypeText = GridEditorWidgetHelpers::GetGridEnumDisplayText (TypeEnum, static_cast<int64>(Obj.Type));
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId)
        : nullptr;
    const FText TitleText = Archetype && !Archetype->DisplayName.IsEmpty ()
        ? Archetype->DisplayName
        : TypeText;

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
                                                .Text (GridEditorWidgetHelpers::GetGridObjectGlyph (Obj.Type))
                                                .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 36))
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
                                        .Text (TitleText)
                                        .Font (FCoreStyle::GetDefaultFontStyle ("Regular", 20))
                                ]

                                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f, 0.f, 0.f)
                                [
                                    SNew (STextBlock)
                                        .Text (FText::Format (
                                            FText::FromString (TEXT ("{0} {1}")),
                                            TypeText,
                                            GetHeaderPlacementText (Obj)))
                                        .ColorAndOpacity (FSlateColor (FLinearColor (0.72f, 0.72f, 0.72f, 1.f)))
                                ]
                        ]
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
                [
                    BuildGameObjectSection (Obj)
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
                [
                    BuildContextualComponentSection (Obj)
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
                [
                    BuildAdvancedDebugSection (Obj)
                ]
        ];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildGameObjectSection (const FGridLevelObjectData& Obj)
{
    const UEnum* PlacementKindEnum = StaticEnum<EGridObjectPlacementKind> ();
    const UEnum* ObjectCategoryEnum = StaticEnum<EGridObjectCategory> ();
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId)
        : nullptr;

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (Archetype)
    {
        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Placement Kind")),
                GridEditorWidgetHelpers::GetGridEnumDisplayText (PlacementKindEnum, static_cast<int64>(Archetype->PlacementKind)))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Palette Category")),
                GetNameText (Archetype->Category))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Functional Category")),
                GridEditorWidgetHelpers::GetGridEnumDisplayText (ObjectCategoryEnum, static_cast<int64>(Archetype->ObjectCategory)))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Runtime Interactable")),
                GetBoolText (Archetype->bIsInteractable))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Runtime Readable")),
                GetBoolText (Archetype->bIsReadable))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Runtime Light Source")),
                GetBoolText (Archetype->bIsLightSource))
        ];
    }

    Root->AddSlot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
    [
        SNew (SHorizontalBox)
            + SHorizontalBox::Slot ().AutoWidth ().Padding (0.f, 0.f, 12.f, 0.f)
            [
                SNew (SCheckBox)
                    .IsChecked (Obj.bInitiallyEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                {
                    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                    {
                        CurrentEditorActor->SetSelectedObjectInitiallyEnabled (NewState == ECheckBoxState::Checked);
                        RequestRefresh ();
                    }
                })
                    [
                        SNew (STextBlock).Text (FText::FromString (TEXT ("Enabled at Start")))
                    ]
            ]
            + SHorizontalBox::Slot ().AutoWidth ()
            [
                SNew (SCheckBox)
                    .IsChecked (Obj.bInitiallyActive ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                {
                    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                    {
                        CurrentEditorActor->SetSelectedObjectInitiallyActive (NewState == ECheckBoxState::Checked);
                        RequestRefresh ();
                    }
                })
                    [
                        SNew (STextBlock).Text (FText::FromString (TEXT ("Active at Start")))
                    ]
            ]
    ];

    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Game Object")),
        Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildContextualComponentSection (const FGridLevelObjectData& Obj)
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId)
        : nullptr;

    TSharedPtr<SWidget> PrimarySection;
    if (Archetype && Archetype->IsReadable ())
    {
        PrimarySection = BuildReadableTextSection (Obj);
    }
    else
    {
        switch (Obj.Type)
        {
            case EGridLevelObjectType::Door:
                PrimarySection = BuildDoorDetailsSection (Obj);
                break;

            case EGridLevelObjectType::Lever:
                PrimarySection = BuildLeverDetailsSection (Obj);
                break;

            case EGridLevelObjectType::Button:
                PrimarySection = BuildButtonDetailsSection (Obj);
                break;

            case EGridLevelObjectType::PressurePlate:
                PrimarySection = BuildPressurePlateDetailsSection (Obj);
                break;

            case EGridLevelObjectType::Trigger:
                PrimarySection = BuildTriggerBehaviorSection (Obj);
                break;

            case EGridLevelObjectType::Receptacle:
                PrimarySection = BuildReceptacleBehaviorSection (Obj);
                break;

            case EGridLevelObjectType::Teleporter:
                PrimarySection = BuildTeleporterDetailsSection (Obj);
                break;

            default:
                PrimarySection = GridEditorWidgetHelpers::BuildGridPanelSection (
                    FText::FromString (TEXT ("Component")),
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("No contextual component fields are exposed for this object type yet.")))
                        .AutoWrapText (true));
                break;
        }
    }

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox)
        + SVerticalBox::Slot ().AutoHeight ()
        [
            PrimarySection.ToSharedRef ()
        ];

    if (Archetype && Archetype->bIsLightSource)
    {
        Root->AddSlot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 0.f)
        [
            BuildLightDetailsSection (*Archetype)
        ];
    }

    return Root;
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildAdvancedDebugSection (const FGridLevelObjectData& Obj)
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId)
        : nullptr;

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("ObjectId")),
                FText::FromString (Obj.ObjectId.ToString ()))
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("ArchetypeId")),
                FText::FromName (Obj.ArchetypeId))
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Tag")),
                FText::FromName (Obj.Tag))
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
                if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                {
                    CurrentEditorActor->SetSelectedObjectNotes (NewText.ToString ());
                    RequestRefresh ();
                }
            })
        ];

    if (Archetype)
    {
        Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Runtime Actor Class")),
                GetClassNameText (Archetype->RuntimeActorClass.Get ()))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Item Actor Class")),
                GetClassNameText (Archetype->ItemActorClass.Get ()))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Main Mesh / Preview Mesh")),
                GetObjectNameText (Archetype->PreviewMesh.Get ()))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Fixed Mesh")),
                GetObjectNameText (Archetype->FixedMesh.Get ()))
        ];

        Root->AddSlot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Moving Mesh")),
                GetObjectNameText (Archetype->MovingMesh.Get ()))
        ];
    }

    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Advanced / Debug")),
        Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildDoorDetailsSection (const FGridLevelObjectData& Obj)
{
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId)
        : nullptr;
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                RequestRefresh ();
            }
        }
    };

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Initial State")),
                GetInitialActiveStateText (Obj, TEXT ("Open / Active"), TEXT ("Closed / Inactive")))
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Blocks Movement (Generic Object)")),
                Archetype
                    ? GetBoolText (Archetype->bBlocksMovement)
                    : FText::FromString (TEXT ("Runtime door blocking handled by door system")))
        ];

    Root->AddSlot ().AutoHeight ()
    [
        BuildBehaviorFloatSpinBoxRow (
            FText::FromString (TEXT ("Open Height")),
            Obj.Behavior.DoorAnimation.OpenHeight,
            [Obj, ApplyBehavior] (float NewValue)
        {
            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
            NewBehavior.DoorAnimation.OpenHeight = NewValue;
            ApplyBehavior (NewBehavior);
        })
    ];

    Root->AddSlot ().AutoHeight ()
    [
        BuildBehaviorFloatSpinBoxRow (
            FText::FromString (TEXT ("Move Duration")),
            Obj.Behavior.DoorAnimation.MoveDuration,
            [Obj, ApplyBehavior] (float NewValue)
        {
            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
            NewBehavior.DoorAnimation.MoveDuration = NewValue;
            ApplyBehavior (NewBehavior);
        })
    ];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 1.f, 0.f, 3.f)
    [
        SNew (STextBlock)
            .Text (FText::FromString (TEXT ("Door passage blocking is handled by the door system. This archetype flag is mainly for generic non-door blocking.")))
            .AutoWrapText (true)
            .ColorAndOpacity (FSlateColor (FLinearColor (0.65f, 0.65f, 0.65f)))
    ];

    if (Archetype)
    {
        Root->AddSlot ().AutoHeight ()
        [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Moving Mesh")),
                    GetBoolText (Archetype->MovingMesh.Get () != nullptr))
        ];

        Root->AddSlot ().AutoHeight ()
        [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Fixed Mesh")),
                    GetBoolText (Archetype->FixedMesh.Get () != nullptr))
        ];
    }

    Root->AddSlot ().AutoHeight ()
    [
        GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
            FText::FromString (TEXT ("Supported Commands")),
            FText::FromString (TEXT ("Open, Close, Toggle, Lock, Unlock")))
    ];

    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Door")),
        Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildLeverDetailsSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                RequestRefresh ();
            }
        }
    };

    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Lever")),
        SNew (SVerticalBox)

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Initial State")),
                    GetInitialActiveStateText (Obj, TEXT ("Activated"), TEXT ("Deactivated")))
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildBehaviorFloatSpinBoxRow (
                    FText::FromString (TEXT ("Off Pitch")),
                    Obj.Behavior.LeverAnimation.LeverOffPitch,
                    [Obj, ApplyBehavior] (float NewValue)
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    NewBehavior.LeverAnimation.LeverOffPitch = NewValue;
                    ApplyBehavior (NewBehavior);
                })
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildBehaviorFloatSpinBoxRow (
                    FText::FromString (TEXT ("On Pitch")),
                    Obj.Behavior.LeverAnimation.LeverOnPitch,
                    [Obj, ApplyBehavior] (float NewValue)
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    NewBehavior.LeverAnimation.LeverOnPitch = NewValue;
                    ApplyBehavior (NewBehavior);
                })
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildBehaviorFloatSpinBoxRow (
                    FText::FromString (TEXT ("Toggle Duration")),
                    Obj.Behavior.LeverAnimation.ToggleDuration,
                    [Obj, ApplyBehavior] (float NewValue)
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    NewBehavior.LeverAnimation.ToggleDuration = NewValue;
                    ApplyBehavior (NewBehavior);
                })
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildExplicitConnectorSummary (FText::FromString (TEXT ("Activated, Deactivated, Toggled")))
            ]);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildButtonDetailsSection (const FGridLevelObjectData& Obj)
{
    FString ButtonType = TEXT ("Generic");
    const FString ArchetypeIdText = Obj.ArchetypeId.ToString ();

    if (ArchetypeIdText.Contains (TEXT ("Button_Secret"), ESearchCase::IgnoreCase))
    {
        ButtonType = TEXT ("Secret");
    } else if (ArchetypeIdText.Contains (TEXT ("Button_Wall"), ESearchCase::IgnoreCase))
    {
        ButtonType = TEXT ("Wall");
    } else if (ArchetypeIdText.Contains (TEXT ("Button_Normal"), ESearchCase::IgnoreCase))
    {
        ButtonType = TEXT ("Normal");
    }

    auto ApplyBehavior = [this](const FGridObjectBehaviorParams& NewBehavior)
        {
            if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
            {
                if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
                {
                    RequestRefresh ();
                }
            }
        };

    auto BuildFloatBehaviorRow =
        [this, Obj, ApplyBehavior] (
            const FText& Label,
            float CurrentValue,
            float MinValue,
            float MaxValue,
            float Delta,
            TFunction<void (FGridObjectBehaviorParams&, float)> AssignValue) -> TSharedRef<SWidget>
        {
            return GridEditorWidgetHelpers::BuildGridPropertyRow (
                Label,
                SNew (SSpinBox<float>)
                .Value (CurrentValue)
                .MinValue (MinValue)
                .MaxValue (MaxValue)
                .Delta (Delta)
                .MinSliderValue (MinValue)
                .MaxSliderValue (MaxValue)
                .OnValueCommitted_Lambda (
                    [Obj, ApplyBehavior, AssignValue](float NewValue, ETextCommit::Type CommitType)
                    {
                        FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                        AssignValue (NewBehavior, NewValue);
                        ApplyBehavior (NewBehavior);
                    }));
        };

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Button Type")),
                FText::FromString (ButtonType))
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Initial State")),
                GetInitialActiveStateText (Obj, TEXT ("Pressed"), TEXT ("Released")))
        ]

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 2.f)
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("Animation")))
                .Font (FCoreStyle::GetDefaultFontStyle ("Bold", 9))
                .ColorAndOpacity (FSlateColor (FLinearColor (0.72f, 0.72f, 0.72f, 1.f)))
        ]

        + SVerticalBox::Slot ().AutoHeight ()
        [
            BuildFloatBehaviorRow (
                FText::FromString (TEXT ("Press Distance")),
                Obj.Behavior.ButtonAnimation.ButtonPressDistance,
                0.f,
                50.f,
                0.5f,
                [](FGridObjectBehaviorParams& Behavior, float NewValue)
                {
                    Behavior.ButtonAnimation.ButtonPressDistance = NewValue;
                })
        ]

    + SVerticalBox::Slot ().AutoHeight ()
        [
            BuildFloatBehaviorRow (
                FText::FromString (TEXT ("Press Duration")),
                Obj.Behavior.ButtonAnimation.ButtonPressDuration,
                0.01f,
                5.f,
                0.01f,
                [](FGridObjectBehaviorParams& Behavior, float NewValue)
                {
                    Behavior.ButtonAnimation.ButtonPressDuration = NewValue;
                })
        ]

    + SVerticalBox::Slot ().AutoHeight ()
        [
            BuildFloatBehaviorRow (
                FText::FromString (TEXT ("Release Duration")),
                Obj.Behavior.ButtonAnimation.ButtonReleaseDuration,
                0.01f,
                5.f,
                0.01f,
                [](FGridObjectBehaviorParams& Behavior, float NewValue)
                {
                    Behavior.ButtonAnimation.ButtonReleaseDuration = NewValue;
                })
        ]

    + SVerticalBox::Slot ().AutoHeight ()
        [
            BuildFloatBehaviorRow (
                FText::FromString (TEXT ("Hold Time")),
                Obj.Behavior.ButtonAnimation.ButtonHoldTime,
                0.f,
                10.f,
                0.05f,
                [](FGridObjectBehaviorParams& Behavior, float NewValue)
                {
                    Behavior.ButtonAnimation.ButtonHoldTime = NewValue;
                })
        ]

    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                FText::FromString (TEXT ("Emits")),
                FText::FromString (TEXT ("Activated, Used")))
        ];

    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Button")),
        Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildPressurePlateDetailsSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                RequestRefresh ();
            }
        }
    };

    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Pressure Plate / Floor Trigger")),
        SNew (SVerticalBox)

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Initial State")),
                    GetInitialActiveStateText (Obj, TEXT ("Activated"), TEXT ("Deactivated")))
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildBehaviorFloatSpinBoxRow (
                    FText::FromString (TEXT ("Released Height")),
                    Obj.Behavior.PressurePlateAnimation.ReleasedHeightAboveFloor,
                    [Obj, ApplyBehavior] (float NewValue)
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    NewBehavior.PressurePlateAnimation.ReleasedHeightAboveFloor = NewValue;
                    ApplyBehavior (NewBehavior);
                })
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildBehaviorFloatSpinBoxRow (
                    FText::FromString (TEXT ("Pressed Height")),
                    Obj.Behavior.PressurePlateAnimation.PressedHeightAboveFloor,
                    [Obj, ApplyBehavior] (float NewValue)
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    NewBehavior.PressurePlateAnimation.PressedHeightAboveFloor = NewValue;
                    ApplyBehavior (NewBehavior);
                })
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildBehaviorFloatSpinBoxRow (
                    FText::FromString (TEXT ("Move Duration")),
                    Obj.Behavior.PressurePlateAnimation.MoveDuration,
                    [Obj, ApplyBehavior] (float NewValue)
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    NewBehavior.PressurePlateAnimation.MoveDuration = NewValue;
                    ApplyBehavior (NewBehavior);
                })
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                BuildExplicitConnectorSummary (FText::FromString (TEXT ("Activated, Deactivated, Entered, Exited")))
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                SNew (STextBlock)
                    .Text (FText::FromString (TEXT ("Use explicit Activated and Deactivated connectors to control what happens on press and release.")))
                    .AutoWrapText (true)
            ]);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildTeleporterDetailsSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this](const FGridObjectBehaviorParams& NewBehavior)
        {
            if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
            {
                if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
                {
                    RequestRefresh ();
                }
            }
        };
    auto BuildIntBehaviorRow =
        [Obj, ApplyBehavior] (const FText& Label, int32 CurrentValue, int32 MinValue, int32 MaxValue,
            TFunction<void (FGridObjectBehaviorParams&, int32)> AssignValue) -> TSharedRef<SWidget>
        {
            return GridEditorWidgetHelpers::BuildGridPropertyRow (
                Label,
                SNew (SSpinBox<int32>).Value (CurrentValue).MinValue (MinValue)
                .MaxValue (MaxValue).MinSliderValue (MinValue).MaxSliderValue (MaxValue)
                .Delta (1).OnValueCommitted_Lambda (
                    [Obj, ApplyBehavior, AssignValue](int32 NewValue, ETextCommit::Type CommitType)
                    {
                        FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                        AssignValue (NewBehavior, NewValue);
                        ApplyBehavior (NewBehavior);
                    }));
        };
    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox)

        + SVerticalBox::Slot ().AutoHeight ()
        [
            BuildIntBehaviorRow (
                FText::FromString (TEXT ("Target Cell X")),
                Obj.Behavior.Teleporter.TargetCellX, -1, 31,
                [](FGridObjectBehaviorParams& Behavior, int32 NewValue)
                {
                    Behavior.Teleporter.TargetCellX = NewValue;
                })
        ]
    + SVerticalBox::Slot ().AutoHeight ()
        [
            BuildIntBehaviorRow (
                FText::FromString (TEXT ("Target Cell Y")),
                Obj.Behavior.Teleporter.TargetCellY, -1, 31,
                [](FGridObjectBehaviorParams& Behavior, int32 NewValue)
                {
                    Behavior.Teleporter.TargetCellY = NewValue;
                })
        ]
    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("Use -1 / -1 to mark an unset destination.")))
                .AutoWrapText (true)
                .ColorAndOpacity (FSlateColor (FLinearColor (0.65f, 0.65f, 0.65f)))
        ];

    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Teleporter")),
        Root);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildLightDetailsSection (const UGridObjectArchetypeAsset& Archetype)
{
    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Light")),
        SNew (SVerticalBox)

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Light Color")),
                    FText::FromString (Archetype.LightColor.ToString ()))
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Intensity")),
                    FText::AsNumber (Archetype.LightIntensity))
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Radius")),
                    FText::AsNumber (Archetype.LightRadius))
            ]

            + SVerticalBox::Slot ().AutoHeight ()
            [
                GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                    FText::FromString (TEXT ("Use Light Flicker (if supported)")),
                    GetBoolText (Archetype.bUseLightFlicker))
            ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 1.f, 0.f, 0.f)
            [
                SNew (STextBlock)
                    .Text (FText::FromString (TEXT ("Actual flicker support depends on the runtime light component path.")))
                    .AutoWrapText (true)
                    .ColorAndOpacity (FSlateColor (FLinearColor (0.65f, 0.65f, 0.65f)))
            ]);
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildReadableTextSection (const FGridLevelObjectData& Obj)
{
    return GridEditorWidgetHelpers::BuildGridPanelSection (
        FText::FromString (TEXT ("Readable Text")),
        SNew (SMultiLineEditableTextBox)
            .Text (Obj.OverrideReadableText)
            .AutoWrapText (true)
            .HintText (FText::FromString (TEXT ("Text displayed when the player presses Use.")))
            .OnTextCommitted_Lambda ([this] (const FText& NewText, ETextCommit::Type CommitType)
        {
            if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
            {
                CurrentEditorActor->SetSelectedObjectReadableText (NewText);
                RequestRefresh ();
            }
        }));
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildTriggerBehaviorSection (const FGridLevelObjectData& Obj)
{
    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Trigger")))
                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 6.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Triggers emit explicit connector events. Add connectors for Activated or Deactivated to control enter and exit behavior.")))
                        .AutoWrapText (true)
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildExplicitConnectorSummary (FText::FromString (TEXT ("Activated, Deactivated")))
                ]
        ];
}

TSharedRef<SWidget> SGridEditorObjectInspectorPanel::BuildReceptacleBehaviorSection (const FGridLevelObjectData& Obj)
{
    auto ApplyBehavior = [this] (const FGridObjectBehaviorParams& NewBehavior)
    {
        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
        {
            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
            {
                RequestRefresh ();
            }
        }
    };

    const FGridObjectBehaviorParams& Behavior = Obj.Behavior;
    const AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ();
    const UGridObjectArchetypeAsset* Archetype = CurrentEditorActor
        ? CurrentEditorActor->FindObjectArchetypeById (Obj.ArchetypeId)
        : nullptr;
    const UEnum* PlacementKindEnum = StaticEnum<EGridObjectPlacementKind> ();
    const TArray<GridEditorWidgetHelpers::FGridArchetypeOption> ItemOptions =
        GridEditorWidgetHelpers::GetItemArchetypeOptions (CurrentEditorActor ? CurrentEditorActor->ObjectPalette : nullptr);

    auto FindItemLabel = [&ItemOptions] (FName ArchetypeId) -> FText
    {
        if (ArchetypeId.IsNone ())
        {
            return FText::FromString (TEXT ("None"));
        }

        for (const GridEditorWidgetHelpers::FGridArchetypeOption& Option : ItemOptions)
        {
            if (Option.ArchetypeId == ArchetypeId)
            {
                return Option.Label;
            }
        }

        return FText::FromName (ArchetypeId);
    };

    const bool bInitialContentCompatible =
        Behavior.Receptacle.InitialContainedItemArchetypeId.IsNone () ||
        Behavior.Receptacle.bAcceptAnyItem ||
        Behavior.Receptacle.AcceptedArchetypeIds.Contains (Behavior.Receptacle.InitialContainedItemArchetypeId);

    TSharedRef<SVerticalBox> AcceptedItemsList = SNew (SVerticalBox);
    if (ItemOptions.Num () == 0)
    {
        AcceptedItemsList->AddSlot ().AutoHeight ()
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("No item archetypes are available in the palette.")))
                .AutoWrapText (true)
        ];
    }
    else
    {
        for (const GridEditorWidgetHelpers::FGridArchetypeOption& Option : ItemOptions)
        {
            AcceptedItemsList->AddSlot ().AutoHeight ().Padding (0.f, 1.f)
            [
                SNew (SCheckBox)
                    .IsChecked (Behavior.Receptacle.AcceptedArchetypeIds.Contains (Option.ArchetypeId)
                        ? ECheckBoxState::Checked
                        : ECheckBoxState::Unchecked)
                    .OnCheckStateChanged_Lambda ([Obj, ApplyBehavior, Option] (ECheckBoxState NewState)
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    if (NewState == ECheckBoxState::Checked)
                    {
                        NewBehavior.Receptacle.AcceptedArchetypeIds.AddUnique (Option.ArchetypeId);
                    }
                    else
                    {
                        NewBehavior.Receptacle.AcceptedArchetypeIds.Remove (Option.ArchetypeId);
                    }
                    ApplyBehavior (NewBehavior);
                })
                    [
                        SNew (STextBlock)
                            .Text (Option.Label)
                    ]
            ];
        }
    }

    auto BuildInitialContentMenu = [Obj, ApplyBehavior, ItemOptions] () -> TSharedRef<SWidget>
    {
        TSharedRef<SVerticalBox> Menu = SNew (SVerticalBox);

        Menu->AddSlot ().AutoHeight ()
        [
            SNew (SButton)
                .Text (FText::FromString (TEXT ("None")))
                .OnClicked_Lambda ([Obj, ApplyBehavior] ()
            {
                FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                NewBehavior.Receptacle.InitialContainedItemArchetypeId = NAME_None;
                ApplyBehavior (NewBehavior);
                return FReply::Handled ();
            })
        ];

        for (const GridEditorWidgetHelpers::FGridArchetypeOption& Option : ItemOptions)
        {
            Menu->AddSlot ().AutoHeight ()
            [
                SNew (SButton)
                    .Text (Option.Label)
                    .OnClicked_Lambda ([Obj, ApplyBehavior, Option] ()
                {
                    FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                    NewBehavior.Receptacle.InitialContainedItemArchetypeId = Option.ArchetypeId;
                    ApplyBehavior (NewBehavior);
                    return FReply::Handled ();
                })
            ];
        }

        return Menu;
    };

    return SNew (SBorder)
        .Padding (6.f)
        .BorderImage (FAppStyle::GetBrush ("ToolPanel.GroupBorder"))
        [
            SNew (SVerticalBox)

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Receptacle")))
                        .Font (FAppStyle::GetFontStyle ("DetailsView.CategoryFontStyle"))
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    Archetype
                        ? GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                            FText::FromString (TEXT ("Interactable")),
                            GetBoolText (Archetype->bIsInteractable))
                        : SNullWidget::NullWidget
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    Archetype
                        ? GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                            FText::FromString (TEXT ("Placement Kind")),
                            GridEditorWidgetHelpers::GetGridEnumDisplayText (PlacementKindEnum, static_cast<int64>(Archetype->PlacementKind)))
                        : SNullWidget::NullWidget
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    Archetype && Archetype->bIsLightSource
                        ? GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow (
                            FText::FromString (TEXT ("Runtime Light Source")),
                            GetBoolText (Archetype->bIsLightSource))
                        : SNullWidget::NullWidget
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f, 0.f, 2.f)
                [
                    SNew (SCheckBox)
                        .IsChecked (Behavior.Receptacle.bAcceptAnyItem ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                        .OnCheckStateChanged_Lambda ([this, Obj] (ECheckBoxState NewState)
                    {
                        FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                        NewBehavior.Receptacle.bAcceptAnyItem = NewState == ECheckBoxState::Checked;

                        if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
                        {
                            if (CurrentEditorActor->ApplyBehaviorToSelectedObject (NewBehavior))
                            {
                                RequestRefresh ();
                            }
                        }
                    })
                        [
                            SNew (STextBlock).Text (FText::FromString (TEXT ("Accept Any Item")))
                        ]
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f, 0.f, 2.f)
                [
                    SNew (SCheckBox)
                        .IsChecked (Behavior.Receptacle.bUsePhysicalPlacement ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                        .OnCheckStateChanged_Lambda ([Obj, ApplyBehavior] (ECheckBoxState NewState)
                    {
                        FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                        NewBehavior.Receptacle.bUsePhysicalPlacement = NewState == ECheckBoxState::Checked;
                        ApplyBehavior (NewBehavior);
                    })
                        [
                            SNew (STextBlock).Text (FText::FromString (TEXT ("Physical Placement")))
                        ]
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Max Contained Items")),
                        SNew (SSpinBox<int32>)
                            .Value (Behavior.Receptacle.MaxContainedItems)
                            .MinValue (1)
                            .MinSliderValue (1)
                            .Delta (1)
                            .MinDesiredWidth (90.f)
                            .OnValueCommitted_Lambda ([Obj, ApplyBehavior] (int32 NewValue, ETextCommit::Type CommitType)
                        {
                            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                            NewBehavior.Receptacle.MaxContainedItems = FMath::Max (1, NewValue);
                            ApplyBehavior (NewBehavior);
                        }))
                ]

                + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f, 0.f, 2.f)
                [
                    SNew (SCheckBox)
                        .IsChecked (Behavior.Receptacle.bExtinguishItemOnPhysicalPlacement ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
                        .OnCheckStateChanged_Lambda ([Obj, ApplyBehavior] (ECheckBoxState NewState)
                    {
                        FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                        NewBehavior.Receptacle.bExtinguishItemOnPhysicalPlacement = NewState == ECheckBoxState::Checked;
                        ApplyBehavior (NewBehavior);
                    })
                        [
                            SNew (STextBlock).Text (FText::FromString (TEXT ("Extinguish Item On Physical Placement")))
                        ]
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildBehaviorFloatSpinBoxRow (
                        FText::FromString (TEXT ("Physical Placement Surface Offset")),
                        Behavior.Receptacle.PhysicalPlacementSurfaceOffset,
                        [Obj, ApplyBehavior] (float NewValue)
                        {
                            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                            NewBehavior.Receptacle.PhysicalPlacementSurfaceOffset = FMath::Max (0.f, NewValue);
                            ApplyBehavior (NewBehavior);
                        })
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildBehaviorFloatSpinBoxRow (
                        FText::FromString (TEXT ("Physical Rotation Pitch")),
                        Behavior.Receptacle.PhysicalPlacementInitialRotationOffset.Pitch,
                        [Obj, ApplyBehavior] (float NewValue)
                        {
                            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                            NewBehavior.Receptacle.PhysicalPlacementInitialRotationOffset.Pitch = NewValue;
                            ApplyBehavior (NewBehavior);
                        })
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildBehaviorFloatSpinBoxRow (
                        FText::FromString (TEXT ("Physical Rotation Yaw")),
                        Behavior.Receptacle.PhysicalPlacementInitialRotationOffset.Yaw,
                        [Obj, ApplyBehavior] (float NewValue)
                        {
                            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                            NewBehavior.Receptacle.PhysicalPlacementInitialRotationOffset.Yaw = NewValue;
                            ApplyBehavior (NewBehavior);
                        })
                ]

                + SVerticalBox::Slot ().AutoHeight ()
                [
                    BuildBehaviorFloatSpinBoxRow (
                        FText::FromString (TEXT ("Physical Rotation Roll")),
                        Behavior.Receptacle.PhysicalPlacementInitialRotationOffset.Roll,
                        [Obj, ApplyBehavior] (float NewValue)
                        {
                            FGridObjectBehaviorParams NewBehavior = Obj.Behavior;
                            NewBehavior.Receptacle.PhysicalPlacementInitialRotationOffset.Roll = NewValue;
                            ApplyBehavior (NewBehavior);
                        })
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Accepted Items")),
                        AcceptedItemsList)
                ]

            + SVerticalBox::Slot ().AutoHeight ()
                [
                    GridEditorWidgetHelpers::BuildGridPropertyRow (
                        FText::FromString (TEXT ("Initial Content")),
                        SNew (SComboButton)
                            .ButtonContent ()
                            [
                                SNew (STextBlock)
                                    .Text (FindItemLabel (Behavior.Receptacle.InitialContainedItemArchetypeId))
                            ]
                            .MenuContent ()
                            [
                                BuildInitialContentMenu ()
                            ])
                ]

            + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 0.f)
                [
                    bInitialContentCompatible
                        ? SNullWidget::NullWidget
                        : StaticCastSharedRef<SWidget> (SNew (STextBlock)
                            .Text (FText::FromString (TEXT ("Warning: Initial Content is not included in Accepted Items.")))
                            .AutoWrapText (true)
                            .ColorAndOpacity (FSlateColor (FLinearColor (1.f, 0.55f, 0.18f, 1.f))))
                ]
        ];
}

FReply SGridEditorObjectInspectorPanel::OnApplySelectedObjectClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();

        if (CurrentEditorActor->ApplyEditedSelectedObject ())
        {
            RequestRefresh ();
        }
    }

    return FReply::Handled ();
}

FReply SGridEditorObjectInspectorPanel::OnResetBehaviorFromArchetypeClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->Modify ();

        if (CurrentEditorActor->ResetSelectedObjectBehaviorFromArchetype ())
        {
            RequestRefresh ();
        }
    }

    return FReply::Handled ();
}

FReply SGridEditorObjectInspectorPanel::OnMoveSelectedObjectToCurrentCellClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->MoveSelectedObjectToCurrentSelection ();
        RequestRefresh ();
    }

    return FReply::Handled ();
}

FReply SGridEditorObjectInspectorPanel::OnFocusSelectedObjectClicked ()
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        CurrentEditorActor->FocusSelectedObject ();
        RequestRefresh ();
    }

    return FReply::Handled ();
}

FReply SGridEditorObjectInspectorPanel::OnSetSelectedObjectOrientationClicked (EGridEdge Orientation)
{
    if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor ())
    {
        if (CurrentEditorActor->SetSelectedObjectOrientation (Orientation))
        {
            RequestRefresh ();
        }
    }
    return FReply::Handled ();
}

#endif
