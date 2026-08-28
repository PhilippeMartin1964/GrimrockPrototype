#include "EditorTools/Widgets/SGridEditorWorkspaceTab.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/Widgets/SGridEditorDungeonLevelsPanel.h"
#include "EditorTools/Widgets/SGridEditorLinksPanel.h"
#include "EditorTools/Widgets/SGridEditorObjectInspectorPanel.h"
#include "EditorTools/Widgets/SGridEditorOverviewMapPanel.h"
#include "EditorTools/Widgets/SGridEditorToolPalettePanel.h"
#include "EditorTools/Widgets/SGridEditorValidationPanel.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SGridEditorWorkspaceTab::Construct(const FArguments& InArgs)
{
	WorkspaceTab = InArgs._WorkspaceTab;
	ToolPaletteState = MakeShared<FGridEditorToolPalettePanelState>();
	ValidationState = MakeShared<FGridEditorValidationPanelState>();

	ChildSlot[BuildContent()];
	CaptureObservedContext();
}

void SGridEditorWorkspaceTab::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (HasObservedContextChanged())
	{
		Rebuild();
	}
}

AGridLevelEditorActor* SGridEditorWorkspaceTab::FindEditorActor() const
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

void SGridEditorWorkspaceTab::Rebuild()
{
	ChildSlot[BuildContent()];
	CaptureObservedContext();
}

void SGridEditorWorkspaceTab::CaptureObservedContext()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();

	ObservedEditorActor = EditorActor;
	ObservedDungeonAsset = EditorActor ? EditorActor->DungeonAsset.Get() : nullptr;
	ObservedLevelAsset = EditorActor ? EditorActor->LevelAsset.Get() : nullptr;
	ObservedObjectPalette = EditorActor ? EditorActor->ObjectPalette.Get() : nullptr;
	ObservedDungeonLevelId = EditorActor ? EditorActor->CurrentDungeonLevelId : NAME_None;
	ObservedPaletteEntryId = EditorActor ? EditorActor->SelectedPaletteEntryId : NAME_None;
	ObservedSelectedObjectId = EditorActor ? EditorActor->LastSelectedObjectId : FGuid();
	ObservedSelectedCellX = EditorActor ? EditorActor->SelectedCellX : INDEX_NONE;
	ObservedSelectedCellY = EditorActor ? EditorActor->SelectedCellY : INDEX_NONE;
	ObservedSelectedEdge = EditorActor ? static_cast<int32>(EditorActor->SelectedEdge) : INDEX_NONE;
	ObservedActiveTool = EditorActor ? static_cast<int32>(EditorActor->ActiveTool) : INDEX_NONE;
	ObservedObjectCount = EditorActor && EditorActor->LevelAsset ? EditorActor->LevelAsset->Objects.Num() : INDEX_NONE;
	ObservedLinkCount = EditorActor && EditorActor->LevelAsset ? EditorActor->LevelAsset->Links.Num() : INDEX_NONE;
	bObservedPatrolRouteEditMode = EditorActor ? EditorActor->bPatrolRouteEditMode : false;
	ObservedPatrolWaypointIndex = EditorActor ? EditorActor->SelectedPatrolWaypointIndex : INDEX_NONE;
}

bool SGridEditorWorkspaceTab::HasObservedContextChanged() const
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();

	if (ObservedEditorActor.Get() != EditorActor)
	{
		return true;
	}

	if (!EditorActor)
	{
		return false;
	}

	const int32 ObjectCount = EditorActor->LevelAsset ? EditorActor->LevelAsset->Objects.Num() : INDEX_NONE;
	const int32 LinkCount = EditorActor->LevelAsset ? EditorActor->LevelAsset->Links.Num() : INDEX_NONE;

	return ObservedDungeonAsset.Get() != EditorActor->DungeonAsset.Get() || ObservedLevelAsset.Get() != EditorActor->LevelAsset.Get() ||
		ObservedObjectPalette.Get() != EditorActor->ObjectPalette.Get() || ObservedDungeonLevelId != EditorActor->CurrentDungeonLevelId ||
		ObservedPaletteEntryId != EditorActor->SelectedPaletteEntryId || ObservedSelectedObjectId != EditorActor->LastSelectedObjectId ||
		ObservedSelectedCellX != EditorActor->SelectedCellX || ObservedSelectedCellY != EditorActor->SelectedCellY ||
		ObservedSelectedEdge != static_cast<int32>(EditorActor->SelectedEdge) || ObservedActiveTool != static_cast<int32>(EditorActor->ActiveTool) ||
		ObservedObjectCount != ObjectCount || ObservedLinkCount != LinkCount || bObservedPatrolRouteEditMode != EditorActor->bPatrolRouteEditMode ||
		ObservedPatrolWaypointIndex != EditorActor->SelectedPatrolWaypointIndex;
}

TSharedRef<SWidget> SGridEditorWorkspaceTab::BuildMigrationNotice(const FText& Text) const
{
	return SNew(SBorder)
		.Padding(FMargin(8.f, 6.f))
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(STextBlock)
				.Text(Text)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.75f, 0.82f, 1.f)))
		];
}

TSharedRef<SWidget> SGridEditorWorkspaceTab::BuildContent()
{
	switch (WorkspaceTab)
	{
		case EGridEditorWorkspaceTab::DungeonLevels:
			return BuildDungeonLevelsContent();
		case EGridEditorWorkspaceTab::PlaytestValidation:
			return BuildPlaytestValidationContent();
		case EGridEditorWorkspaceTab::ToolsPalette:
			return BuildToolsPaletteContent();
		case EGridEditorWorkspaceTab::SelectedObject:
			return BuildSelectedObjectContent();
		default:
			return SNew(STextBlock).Text(FText::FromString(TEXT("Unknown Grid Editor workspace tab.")));
	}
}

TSharedRef<SWidget> SGridEditorWorkspaceTab::BuildDungeonLevelsContent()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();

	return SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Horizontal)

			+ SSplitter::Slot()
			.Value(0.36f)
			[
				SNew(SBorder)
				.Padding(6.f)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SGridEditorDungeonLevelsPanel)
							.EditorActor(TWeakObjectPtr<AGridLevelEditorActor>(EditorActor))
							.OnGetEditorActor(FOnGetGridEditorDungeonLevelsActor::CreateSP(this, &SGridEditorWorkspaceTab::FindEditorActor))
							.OnRequestRefresh(FOnGridEditorDungeonLevelsRequestRefresh::CreateSP(this, &SGridEditorWorkspaceTab::Rebuild))
					]
				]
			]

			+ SSplitter::Slot()
			.Value(0.64f)
			[
				SNew(SBorder)
				.Padding(6.f)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(SGridEditorOverviewMapPanel)
							.EditorActor(TWeakObjectPtr<AGridLevelEditorActor>(EditorActor))
							.OnGetEditorActor(FOnGetGridEditorActor::CreateSP(this, &SGridEditorWorkspaceTab::FindEditorActor))
							.OnRequestRefresh(FOnGridEditorOverviewRequestRefresh::CreateSP(this, &SGridEditorWorkspaceTab::Rebuild))
					]
				]
			]
		];
}

TSharedRef<SWidget> SGridEditorWorkspaceTab::BuildPlaytestValidationContent()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	Root->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
	[
		BuildMigrationNotice(FText::FromString(
			TEXT("GEUI01 foundation: Validation is already hosted here. PlayTest controls remain in the main Grid Editor toolkit until GEUI03.")))
	];

	if (EditorActor && EditorActor->LevelAsset)
	{
		Root->AddSlot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Current LevelAsset")), FText::FromString(EditorActor->LevelAsset->GetName()))
		];
		Root->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Start Cell")),
				FText::Format(FText::FromString(TEXT("({0}, {1})")), FText::AsNumber(EditorActor->LevelAsset->StartCellX),
					FText::AsNumber(EditorActor->LevelAsset->StartCellY)))
		];
	}

	Root->AddSlot().AutoHeight()
	[
		SNew(SGridEditorValidationPanel)
			.EditorActor(TWeakObjectPtr<AGridLevelEditorActor>(EditorActor))
			.ValidationState(ValidationState)
			.OnGetEditorActor(FOnGetGridEditorValidationActor::CreateSP(this, &SGridEditorWorkspaceTab::FindEditorActor))
			.OnRequestRefresh(FOnGridEditorValidationRequestRefresh::CreateSP(this, &SGridEditorWorkspaceTab::Rebuild))
	];

	return SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				Root
			]
		];
}

TSharedRef<SWidget> SGridEditorWorkspaceTab::BuildToolsPaletteContent()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();

	return SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SGridEditorToolPalettePanel)
					.EditorActor(TWeakObjectPtr<AGridLevelEditorActor>(EditorActor))
					.ToolPaletteState(ToolPaletteState)
					.OnGetEditorActor(FOnGetGridEditorToolPaletteActor::CreateSP(this, &SGridEditorWorkspaceTab::FindEditorActor))
					.OnRequestRefresh(FOnGridEditorToolPaletteRequestRefresh::CreateSP(this, &SGridEditorWorkspaceTab::Rebuild))
			]
		];
}

TSharedRef<SWidget> SGridEditorWorkspaceTab::BuildSelectedObjectContent()
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 8.f)
		[
			GridEditorWidgetHelpers::BuildGridPanelSection(
				FText::FromString(TEXT("PROPERTIES")),
				SNew(SGridEditorObjectInspectorPanel)
					.EditorActor(TWeakObjectPtr<AGridLevelEditorActor>(EditorActor))
					.OnGetEditorActor(FOnGetGridEditorObjectInspectorActor::CreateSP(this, &SGridEditorWorkspaceTab::FindEditorActor))
					.OnRequestRefresh(FOnGridEditorObjectInspectorRequestRefresh::CreateSP(this, &SGridEditorWorkspaceTab::Rebuild)))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridPanelSection(
				FText::FromString(TEXT("CONNECTORS")),
				SNew(SGridEditorLinksPanel)
					.EditorActor(TWeakObjectPtr<AGridLevelEditorActor>(EditorActor))
					.OnGetEditorActor(FOnGetGridEditorLinksActor::CreateSP(this, &SGridEditorWorkspaceTab::FindEditorActor))
					.OnRequestRefresh(FOnGridEditorLinksRequestRefresh::CreateSP(this, &SGridEditorWorkspaceTab::Rebuild)))
		];

	return SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				Root
			]
		];
}

#endif
