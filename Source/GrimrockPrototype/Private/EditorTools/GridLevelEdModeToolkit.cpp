#include "EditorTools/GridLevelEdModeToolkit.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridObjectPaletteAsset.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "EditorModeManager.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"

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
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("Grimrock Grid Editor")))
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildToolSection ()
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            SNew (STextBlock)
                .Text_Lambda ([this] ()
            {
                return FText::Format (
                    FText::FromString (TEXT ("Active Tool: {0}")),
                    GetActiveToolText ());
            })
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 0.f, 0.f, 8.f)
        [
            BuildPaletteSection ()
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        .Padding (0.f, 8.f, 0.f, 8.f)
        [
            BuildObjectInspectorSection ()
        ];

    ToolkitRoot->AddSlot ()
        .AutoHeight ()
        [
            SNew (STextBlock)
                .Text_Lambda ([this] ()
            {
                return FText::Format (
                    FText::FromString (TEXT ("Selected Palette Entry: {0}")),
                    GetSelectedPaletteEntryText ());
            })
        ];
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
    auto MakeToolButton = [this] (const TCHAR* Label, EGridEditorTool ToolValue) -> TSharedRef<SWidget>
    {
        // Use a lambda to bind the parameter to the OnClicked delegate (OnClicked expects no params)
        return SNew (SButton)
            .Text (FText::FromString (Label))
            .OnClicked_Lambda([this, ToolValue]() -> FReply { return OnToolClicked(static_cast<int32>(ToolValue)); });
    };

    return SNew (SVerticalBox)
        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
        [SNew (STextBlock).Text (FText::FromString (TEXT ("Tools")))]
        + SVerticalBox::Slot ().AutoHeight ()
        [SNew (SWrapBox) + SWrapBox::Slot ().Padding (2.f)
                [MakeToolButton (TEXT ("Select"), EGridEditorTool::Select)]
        + SWrapBox::Slot ().Padding (2.f)
                [MakeToolButton (TEXT ("Cell"), EGridEditorTool::PaintCell)]
        + SWrapBox::Slot ().Padding (2.f)
                [MakeToolButton (TEXT ("Wall"), EGridEditorTool::PaintWall)]
        + SWrapBox::Slot ().Padding (2.f)
                [MakeToolButton (TEXT ("Object"), EGridEditorTool::PaintObject)]
        + SWrapBox::Slot ().Padding (2.f)
                [MakeToolButton (TEXT ("Erase"), EGridEditorTool::Erase)]
        + SWrapBox::Slot ().Padding (2.f)
                [MakeToolButton (TEXT ("Link"), EGridEditorTool::Link)]
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildPaletteSection ()
{
    AGridLevelEditorActor* EditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    Root->AddSlot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
        [SNew (STextBlock).Text (FText::FromString (TEXT ("Palette")))];

    if (!EditorActor)
    {
        Root->AddSlot ().AutoHeight ()
            [SNew (STextBlock).Text (FText::FromString (TEXT ("No GridLevelEditorActor found in the world.")))];

        return Root;
    }
    if (!EditorActor->ObjectPalette)
    {
        Root->AddSlot ().AutoHeight ()
            [SNew (STextBlock).Text (FText::FromString (TEXT ("No ObjectPalette assigned on GridLevelEditorActor.")))];

        return Root;
    }
    TMap<FName, TArray<FGridObjectPaletteEntry>> EntriesByCategory;
    for (const FGridObjectPaletteEntry& Entry : EditorActor->ObjectPalette->Entries)
    {
        EntriesByCategory.FindOrAdd (Entry.Category).Add (Entry);
    }
    for (const TPair<FName, TArray<FGridObjectPaletteEntry>>& Pair : EntriesByCategory)
    {
        Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 2.f)
            [SNew (STextBlock).Text (FText::FromName (Pair.Key))];

        TSharedRef<SWrapBox> Wrap = SNew (SWrapBox);

        for (const FGridObjectPaletteEntry& Entry : Pair.Value)
        {
            const FName EntryId = Entry.EntryId;
            Wrap->AddSlot ().Padding (2.f)
                [SNew (SButton)
                        .Text (Entry.DisplayName.IsEmpty () 
                                      ? FText::FromName (Entry.EntryId) 
                                      : Entry.DisplayName)
                        .OnClicked_Lambda([this, EntryId]() -> FReply { return OnPaletteEntryClicked(EntryId); })];
        }
        Root->AddSlot ().AutoHeight () [Wrap];
    }
    return Root;
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

FReply FGridLevelEdModeToolkit::OnApplySelectedObjectClicked ()
{
    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
    {
        EditorActor->Modify ();
        EditorActor->ApplyEditedSelectedObject ();
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

    Root->AddSlot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
        [SNew (STextBlock).Text (FText::FromString (TEXT ("Object Inspector")))];

    if (!EditorActor || !EditorActor->LevelAsset)
    {
        Root->AddSlot ().AutoHeight ()
            [SNew (STextBlock).Text (FText::FromString (TEXT ("No editor actor or level asset.")))];

        return Root;
    }

    const FGridLevelObjectData* Obj = EditorActor->GetSelectedObjectData ();
    if (!Obj)
    {
        Root->AddSlot ().AutoHeight ()
            [SNew (STextBlock).Text (FText::FromString (TEXT ("No selected object.")))];

        return Root;
    }
    SyncEditedBehaviorFromSelection ();
    Root->AddSlot ().AutoHeight ().Padding (0.f, 2.f, 0.f, 2.f)
        [SNew (STextBlock).Text (GetSelectedObjectDetailsText ())];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 8.f, 0.f, 8.f)
        [BuildBehaviorEditorSection ()];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 4.f, 0.f, 8.f)
        [SNew (SButton).Text (FText::FromString (TEXT ("Focus Selected Object"))).OnClicked_Lambda ([this] () -> FReply
    {
        return OnFocusSelectedObjectClicked ();
    })];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 2.f)
        [SNew (STextBlock).Text (FText::FromString (TEXT ("Outgoing Links")))];

    Root->AddSlot ().AutoHeight ()
        [BuildObjectLinksList (*Obj, true)];

    Root->AddSlot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 2.f)
        [SNew (STextBlock).Text (FText::FromString (TEXT ("Incoming Links")))];

    Root->AddSlot ().AutoHeight ()
        [BuildObjectLinksList (*Obj, false)];

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

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildObjectLinksList (
    const FGridLevelObjectData& SelectedObject,
    bool bOutgoing) const
{
    const AGridLevelEditorActor* EditorActor = GetEditorActor ();

    TSharedRef<SVerticalBox> Root = SNew (SVerticalBox);

    if (!EditorActor || !EditorActor->LevelAsset)
    {
        Root->AddSlot ().AutoHeight ()
            [SNew (STextBlock).Text (FText::FromString (TEXT ("No level asset.")))];

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
        const FString Arrow = bOutgoing ? TEXT ("->") : TEXT ("<-");

        Root->AddSlot ().AutoHeight ().Padding (0.f, 1.f, 0.f, 1.f)
            [SNew (SHorizontalBox)
            + SHorizontalBox::Slot ().FillWidth (1.f).VAlign (VAlign_Center)
            [SNew (STextBlock).Text (FText::Format (
                FText::FromString (TEXT ("{0} {1} [{2}]")),
                FText::FromString (Arrow),
                GetObjectSummaryText (OtherId),
                GetLinkActionText (Link.Action)))]
            + SHorizontalBox::Slot ().AutoWidth ().Padding (4.f, 0.f)
            [SNew (SButton).Text (bOutgoing
                                  ? FText::FromString (TEXT ("Select Target"))
                                  : FText::FromString (TEXT ("Select Source")))
            .OnClicked_Lambda ([this, OtherId] () -> FReply
        {
            // We're in a const method; 'this' is const. Call the non-const handler via const_cast.
            return const_cast<FGridLevelEdModeToolkit*>(this)->OnSelectObjectFromLinkClicked (OtherId);
        })
            ]];
        ++Count;
    }

    if (Count == 0)
    {
        Root->AddSlot ().AutoHeight ()
            [SNew (STextBlock).Text (FText::FromString (TEXT ("None")))];
    }
    return Root;
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

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 4.f)
        [
            SNew (STextBlock)
                .Text (FText::FromString (TEXT ("Behavior Editor")))
        ]

        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
        [
            SNew (SHorizontalBox)

                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 6.f, 0.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Trigger Mode")))
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

                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 6.f, 0.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Delay")))
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

                + SHorizontalBox::Slot ().AutoWidth ().VAlign (VAlign_Center).Padding (0.f, 0.f, 6.f, 0.f)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Duration")))
                ]

                + SHorizontalBox::Slot ().FillWidth (1.f)
                [
                    SNew (SNumericEntryBox<float>)
                        .MinValue (0.f)
                        .Value (this, &FGridLevelEdModeToolkit::GetEditedDuration)
                        .OnValueChanged (this, &FGridLevelEdModeToolkit::OnEditedDurationChanged)
                ]
        ]

    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 4.f)
        [
            SNew (SCheckBox)
                .IsChecked (this, &FGridLevelEdModeToolkit::GetEditedInvertLinksCheckState)
                .OnCheckStateChanged (this, &FGridLevelEdModeToolkit::OnEditedInvertLinksChanged)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Invert Links")))
                ]
        ]

    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
        [
            SNew (SCheckBox)
                .IsChecked (this, &FGridLevelEdModeToolkit::GetEditedFireOnEnterCheckState)
                .OnCheckStateChanged (this, &FGridLevelEdModeToolkit::OnEditedFireOnEnterChanged)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Fire On Enter")))
                ]
        ]

    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 2.f)
        [
            SNew (SCheckBox)
                .IsChecked (this, &FGridLevelEdModeToolkit::GetEditedFireOnExitCheckState)
                .OnCheckStateChanged (this, &FGridLevelEdModeToolkit::OnEditedFireOnExitChanged)
                [
                    SNew (STextBlock)
                        .Text (FText::FromString (TEXT ("Fire On Exit")))
                ]
        ]

    + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 6.f, 0.f, 0.f)
        [
            SNew (SButton)
                .Text (FText::FromString (TEXT ("Apply Behavior To Selected Object")))
                .OnClicked (this, &FGridLevelEdModeToolkit::OnApplyBehaviorClicked)
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

#endif