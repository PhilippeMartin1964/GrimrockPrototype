#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
class UGridDungeonAsset;
class UGridLevelAsset;
class UGridObjectPaletteAsset;
struct FGridEditorToolPalettePanelState;
struct FGridEditorValidationPanelState;

enum class EGridEditorWorkspaceTab : uint8
{
	DungeonLevels,
	PlaytestValidation,
	ToolsPalette,
	SelectedObject
};

/**
 * GEUI01 dockable workspace host.
 *
 * The host intentionally composes existing Grid Editor widgets instead of
 * introducing a parallel authoring model. It observes only lightweight editor
 * context so detached Nomad tabs follow viewport selection/tool changes.
 */
class SGridEditorWorkspaceTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorWorkspaceTab)
	{
	}
	SLATE_ARGUMENT(EGridEditorWorkspaceTab, WorkspaceTab)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	AGridLevelEditorActor* FindEditorActor() const;
	void Rebuild();
	void CaptureObservedContext();
	bool HasObservedContextChanged() const;

	TSharedRef<SWidget> BuildContent();
	TSharedRef<SWidget> BuildDungeonLevelsContent();
	TSharedRef<SWidget> BuildPlaytestValidationContent();
	TSharedRef<SWidget> BuildToolsPaletteContent();
	TSharedRef<SWidget> BuildSelectedObjectContent();
	TSharedRef<SWidget> BuildMigrationNotice(const FText& Text) const;

private:
	EGridEditorWorkspaceTab WorkspaceTab = EGridEditorWorkspaceTab::DungeonLevels;
	TSharedPtr<FGridEditorToolPalettePanelState> ToolPaletteState;
	TSharedPtr<FGridEditorValidationPanelState> ValidationState;

	TWeakObjectPtr<AGridLevelEditorActor> ObservedEditorActor;
	TWeakObjectPtr<UGridDungeonAsset> ObservedDungeonAsset;
	TWeakObjectPtr<UGridLevelAsset> ObservedLevelAsset;
	TWeakObjectPtr<UGridObjectPaletteAsset> ObservedObjectPalette;
	FName ObservedDungeonLevelId = NAME_None;
	FName ObservedPaletteEntryId = NAME_None;
	FGuid ObservedSelectedObjectId;
	int32 ObservedSelectedCellX = INDEX_NONE;
	int32 ObservedSelectedCellY = INDEX_NONE;
	int32 ObservedSelectedEdge = INDEX_NONE;
	int32 ObservedActiveTool = INDEX_NONE;
	int32 ObservedObjectCount = INDEX_NONE;
	int32 ObservedLinkCount = INDEX_NONE;
	bool bObservedPatrolRouteEditMode = false;
	int32 ObservedPatrolWaypointIndex = INDEX_NONE;
};

#endif
