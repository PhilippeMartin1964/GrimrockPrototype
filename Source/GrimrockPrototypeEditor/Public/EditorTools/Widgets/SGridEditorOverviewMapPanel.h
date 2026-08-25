#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
struct FGridLevelCellData;
struct FGridLevelObjectData;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorActor);
DECLARE_DELEGATE(FOnGridEditorOverviewRequestRefresh);

enum class EGridEditorOverviewObjectAnchor : uint8
{
	None,
	North,
	East,
	South,
	West,
	Center
};

struct FGridEditorOverviewAnchorObjectGroup
{
	EGridEditorOverviewObjectAnchor Anchor = EGridEditorOverviewObjectAnchor::None;
	TArray<const FGridLevelObjectData*> Objects;
};

class SGridEditorOverviewMapPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorOverviewMapPanel)
	{
	}
	SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
	SLATE_EVENT(FOnGetGridEditorActor, OnGetEditorActor)
	SLATE_EVENT(FOnGridEditorOverviewRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	void RequestRefresh() const;

	TSharedRef<SWidget> BuildOverviewMapSection();
	TSharedRef<SWidget> BuildOverviewCell(int32 CellX, int32 CellY, const FGridLevelObjectData* SelectedObject);
	TSharedRef<SWidget> BuildCellObjectMarkers(const TArray<const FGridLevelObjectData*>& CellObjects) const;
	TSharedRef<SWidget> BuildOverviewColorLegend() const;
	TSharedRef<SWidget> BuildOverviewLegendSwatch(const FText& Label, const FLinearColor& Color) const;
	TSharedRef<SWidget> BuildSelectedCellSection();
	TSharedRef<SWidget> BuildObjectsOnSelectedCellSection();
	TSharedRef<SWidget> BuildObjectAnchorGroup(const FGridEditorOverviewAnchorObjectGroup& Group);

	FReply OnOverviewCellClicked(int32 CellX, int32 CellY);
	FReply OnSelectObjectFromSelectedCellClicked(FGuid ObjectId);

	FText GetOverviewCellTooltipText(int32 CellX, int32 CellY) const;
	FText GetSelectedCellSummaryText() const;
	FText GetCellWallSummaryText(const FGridLevelCellData& CellData) const;
	FText GetCellObjectSummaryText(int32 CellX, int32 CellY) const;
	FText GetObjectAnchorLabel(EGridEditorOverviewObjectAnchor Anchor) const;
	FText GetSelectedCellObjectSummaryText(const FGridLevelObjectData& Object) const;
	EGridEditorOverviewObjectAnchor GetObjectAnchor(const FGridLevelObjectData& Object) const;
	TArray<FGridEditorOverviewAnchorObjectGroup> GetObjectsAtSelectedCellGroupedByAnchor() const;
	bool HasObjectAtCell(int32 CellX, int32 CellY) const;

private:
	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorActor OnGetEditorActor;
	FOnGridEditorOverviewRequestRefresh OnRequestRefresh;
};

#endif
