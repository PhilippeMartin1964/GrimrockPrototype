#include "EditorTools/GridLevelEdModeToolkit.h"

#if WITH_EDITOR

#include "EditorTools/GridEditorWorkspaceTabs.h"
#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "Core/GridTypes.h"

#include "Editor.h"
#include "EditorModeManager.h"
#include "EngineUtils.h"
#include "Framework/Docking/TabManager.h"

#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void FGridLevelEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	ToolkitWidget = BuildToolkitWidget();
	FModeToolkit::Init(InitToolkitHost);
}

FName FGridLevelEdModeToolkit::GetToolkitFName() const
{
	return FName("GridLevelEdModeToolkit");
}

FText FGridLevelEdModeToolkit::GetBaseToolkitName() const
{
	return FText::FromString(TEXT("Grimrock Grid Editor"));
}

FEdMode* FGridLevelEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FGridLevelEdMode::EM_GridLevelEdModeId);
}

TSharedPtr<SWidget> FGridLevelEdModeToolkit::GetInlineContent() const
{
	return ToolkitWidget;
}

AGridLevelEditorActor* FGridLevelEdModeToolkit::GetEditorActor() const
{
	if (!GEditor)
	{
		return nullptr;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AGridLevelEditorActor> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

void FGridLevelEdModeToolkit::RefreshPalette()
{
	if (!ToolkitRoot.IsValid())
	{
		return;
	}

	ToolkitRoot->ClearChildren();

	ToolkitRoot->AddSlot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			BuildHeaderSection()
		];

	ToolkitRoot->AddSlot()
		.AutoHeight()
		[
			BuildWorkspaceLauncherSection()
		];
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildToolkitWidget()
{
	ToolkitRoot = SNew(SVerticalBox);

	TSharedRef<SWidget> Widget = SNew(SBorder).Padding(8.f)[SNew(SScrollBox) + SScrollBox::Slot()[ToolkitRoot.ToSharedRef()]];

	RefreshPalette();

	return Widget;
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildWorkspaceLauncherSection()
{
	TSharedRef<SVerticalBox> Buttons = SNew(SVerticalBox);

	const auto AddWorkspaceButton = [this, &Buttons](const FText& Label, const FText& Tooltip, const FName& TabName)
	{
		Buttons->AddSlot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SButton)
					.Text(Label)
					.ToolTipText(Tooltip)
					.HAlign(HAlign_Left)
					.ContentPadding(FMargin(10.f, 5.f))
					.OnClicked(FOnClicked::CreateRaw(this, &FGridLevelEdModeToolkit::OpenWorkspaceTab, TabName))
			];
	};

	AddWorkspaceButton(
		FText::FromString(TEXT("Dungeon Levels")),
		FText::FromString(TEXT("Open dungeon level navigation and the 32x32 overview map.")),
		GridEditorWorkspaceTabs::DungeonLevels());

	AddWorkspaceButton(
		FText::FromString(TEXT("PlayTest & Validation")),
		FText::FromString(TEXT("Open playtest preparation and level validation.")),
		GridEditorWorkspaceTabs::PlaytestValidation());

	AddWorkspaceButton(
		FText::FromString(TEXT("Tools & Palette")),
		FText::FromString(TEXT("Open Grid Editor tools and the searchable object palette.")),
		GridEditorWorkspaceTabs::ToolsPalette());

	AddWorkspaceButton(
		FText::FromString(TEXT("Selected Object")),
		FText::FromString(TEXT("Open selected-object properties and connectors.")),
		GridEditorWorkspaceTabs::SelectedObject());

	AddWorkspaceButton(
		FText::FromString(TEXT("Grimrock Lua Scripts")),
		FText::FromString(TEXT("Open level Lua scripts and event bindings.")),
		GridEditorWorkspaceTabs::LuaScripts());

	return SNew(SBorder)
		.Padding(FMargin(6.f))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 5.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("WORKSPACE")))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("Open the authoring window you need.")))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.68f, 0.68f, 1.f)))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				Buttons
			]
		];
}

FReply FGridLevelEdModeToolkit::OpenWorkspaceTab(FName TabName)
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(TabName));
	return FReply::Handled();
}

TSharedRef<SWidget> FGridLevelEdModeToolkit::BuildHeaderSection()
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
                                FText::FromString (TEXT ("Level :")),
                                GetCurrentLevelStatusText (),
                                FSlateColor (FLinearColor (0.50f, 0.75f, 1.f, 1.f)))
                        ]
                    ]

                + SVerticalBox::Slot ()
                .AutoHeight ()
                .Padding (0.f, 6.f, 0.f, 0.f)
                [
                    SNew (SHorizontalBox)

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked_Lambda ([this] ()
                                {
                                    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
                                    return EditorActor && EditorActor->bShowOutgoingConnectors
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                                })
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                                {
                                    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                                    {
                                        EditorActor->bShowOutgoingConnectors = NewState == ECheckBoxState::Checked;
                                        if (GEditor)
                                        {
                                            GEditor->RedrawAllViewports ();
                                        }
                                    }
                                })
                                [
                                    SNew (STextBlock)
                                        .Text (FText::FromString (TEXT ("Show Outgoing Connectors")))
                                ]
                        ]

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        .Padding (0.f, 0.f, 12.f, 0.f)
                        [
                            SNew (SCheckBox)
                                .IsChecked_Lambda ([this] ()
                                {
                                    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
                                    return EditorActor && EditorActor->bShowIncomingConnectors
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                                })
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                                {
                                    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                                    {
                                        EditorActor->bShowIncomingConnectors = NewState == ECheckBoxState::Checked;
                                        if (GEditor)
                                        {
                                            GEditor->RedrawAllViewports ();
                                        }
                                    }
                                })
                                [
                                    SNew (STextBlock)
                                        .Text (FText::FromString (TEXT ("Show Incoming Connectors")))
                                ]
                        ]

                        + SHorizontalBox::Slot ()
                        .AutoWidth ()
                        [
                            SNew (SCheckBox)
                                .IsChecked_Lambda ([this] ()
                                {
                                    const AGridLevelEditorActor* EditorActor = GetEditorActor ();
                                    return EditorActor && EditorActor->bShowConnectorLabels
                                        ? ECheckBoxState::Checked
                                        : ECheckBoxState::Unchecked;
                                })
                                .OnCheckStateChanged_Lambda ([this] (ECheckBoxState NewState)
                                {
                                    if (AGridLevelEditorActor* EditorActor = GetEditorActor ())
                                    {
                                        EditorActor->bShowConnectorLabels = NewState == ECheckBoxState::Checked;
                                        if (GEditor)
                                        {
                                            GEditor->RedrawAllViewports ();
                                        }
                                    }
                                })
                                [
                                    SNew (STextBlock)
                                        .Text (FText::FromString (TEXT ("Show Connector Labels")))
                                ]
                        ]
                ]
        ];
}

FText FGridLevelEdModeToolkit::GetCurrentLevelStatusText() const
{
	if (const AGridLevelEditorActor* EditorActor = GetEditorActor())
	{
		if (!EditorActor->CurrentDungeonLevelId.IsNone())
		{
			return FText::FromName(EditorActor->CurrentDungeonLevelId);
		}

		if (EditorActor->LevelAsset)
		{
			return FText::FromString(EditorActor->LevelAsset->GetName());
		}
	}

	return FText::FromString(TEXT("None"));
}

FText FGridLevelEdModeToolkit::GetActiveToolText() const
{
	if (const AGridLevelEditorActor* EditorActor = GetEditorActor())
	{
		const UEnum* Enum = StaticEnum<EGridEditorTool>();
		if (Enum)
		{
			return Enum->GetDisplayNameTextByValue(static_cast<int64>(EditorActor->ActiveTool));
		}
	}
	return FText::FromString(TEXT("Unknown"));
}

FText FGridLevelEdModeToolkit::GetSelectedCellStatusText() const
{
	if (const AGridLevelEditorActor* EditorActor = GetEditorActor())
	{
		return FText::Format(FText::FromString(TEXT("X={0} Y={1}")), FText::AsNumber(EditorActor->SelectedCellX), FText::AsNumber(EditorActor->SelectedCellY));
	}

	return FText::FromString(TEXT("None"));
}

FText FGridLevelEdModeToolkit::GetSelectedEdgeStatusText() const
{
	if (const AGridLevelEditorActor* EditorActor = GetEditorActor())
	{
		const UEnum* EdgeEnum = StaticEnum<EGridEdge>();
		return EdgeEnum ? EdgeEnum->GetDisplayNameTextByValue(static_cast<int64>(EditorActor->SelectedEdge)) : FText::FromString(TEXT("Unknown"));
	}

	return FText::FromString(TEXT("None"));
}

FText FGridLevelEdModeToolkit::GetSelectedObjectStatusText() const
{
	const AGridLevelEditorActor* EditorActor = GetEditorActor();
	const FGridLevelObjectData* Obj = EditorActor ? EditorActor->GetSelectedObjectData() : nullptr;
	if (!Obj)
	{
		return FText::FromString(TEXT("None"));
	}

	const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
	const FText TypeText = GridEditorWidgetHelpers::GetGridEnumDisplayText(TypeEnum, static_cast<int64>(Obj->Type));

	return FText::Format(FText::FromString(TEXT("{0} ({1},{2})")), TypeText, FText::AsNumber(Obj->CellX), FText::AsNumber(Obj->CellY));
}

#endif
