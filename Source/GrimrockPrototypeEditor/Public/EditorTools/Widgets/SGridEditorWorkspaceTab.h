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

enum class EGridEditorSelectedObjectPage : uint8
{
	Properties,
	Connectors
};

/**
 * GEUI01 dockable workspace host.
 *
 * The host intentionally composes existing Grid Editor widgets instead of
 * introducing a parallel authoring model. GEUI09 keeps lightweight context
 * observation but scopes it per workspace so unrelated editor changes no
 * longer rebuild every detached Nomad tab.
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
	void PrepareSessionStateForCurrentContext();

	TSharedRef<SWidget> BuildContent();
	TSharedRef<SWidget> BuildDungeonLevelsContent();
	TSharedRef<SWidget> BuildPlaytestValidationContent();
	TSharedRef<SWidget> BuildToolsPaletteContent();
	TSharedRef<SWidget> BuildSelectedObjectContent();
	TSharedRef<SWidget> BuildMigrationNotice(const FText& Text) const;

private:
	EGridEditorWorkspaceTab WorkspaceTab = EGridEditorWorkspaceTab::DungeonLevels;
	EGridEditorSelectedObjectPage SelectedObjectPage = EGridEditorSelectedObjectPage::Properties;
	TSharedPtr<FGridEditorToolPalettePanelState> ToolPaletteState;
	TSharedPtr<FGridEditorValidationPanelState> ValidationState;

	TWeakObjectPtr<AGridLevelEditorActor> ObservedEditorActor;
	bool bObservedHasEditorActor = false;
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
