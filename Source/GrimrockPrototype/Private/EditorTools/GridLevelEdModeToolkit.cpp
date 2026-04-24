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

void FGridLevelEdModeToolkit::Init (const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
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
    ToolkitWidget = BuildToolkitWidget ();
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildToolkitWidget ()
{
    return SNew (SBorder).Padding (8.f)
        [
            SNew (SScrollBox) + SScrollBox::Slot ()
                [
                    SNew (SVerticalBox) + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 8.f)
                        [SNew (STextBlock).Text (FText::FromString (TEXT ("Grimrock Grid Editor")))]
                        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 8.f)
                        [BuildToolSection ()]
                        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 8.f)
                        [SNew (STextBlock).Text_Lambda ([this] ()
                    {
                        return FText::Format (FText::FromString (TEXT ("Active Tool: {0}")),
                                              GetActiveToolText ());
                    })]
                        + SVerticalBox::Slot ().AutoHeight ().Padding (0.f, 0.f, 0.f, 8.f)
                        [BuildPaletteSection ()]

                        + SVerticalBox::Slot ().AutoHeight ()
                        [SNew (STextBlock).Text_Lambda ([this] ()
                    {
                        return FText::Format (FText::FromString (TEXT ("Selected Palette Entry: {0}")),
                                              GetSelectedPaletteEntryText ());
                    })]
                ]
        ];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildToolSection ()
{
    auto MakeToolButton = [this] (const TCHAR* Label, EGridEditorTool ToolValue) -> TSharedRef<SWidget>
    {
        return SNew (SButton).Text (FText::FromString (Label))
            .OnClicked (this, &FGridLevelEdModeToolkit::OnToolClicked, static_cast<int32>(ToolValue));
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
            Wrap->AddSlot ().Padding (2.f)
                [SNew (SButton).Text (Entry.DisplayName.IsEmpty () 
                                      ? FText::FromName (Entry.EntryId) 
                                      : Entry.DisplayName)
                        .OnClicked (this, &FGridLevelEdModeToolkit::OnPaletteEntryClicked, Entry.EntryId)];
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

#endif