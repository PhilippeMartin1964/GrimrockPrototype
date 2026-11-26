#pragma once

#include "CoreMinimal.h"
#include "GridItemWorkshopSession.h"
#include "GridItemWorkshopContext.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SGridItemWorkshopViewport;
class SBox;
class STableViewBase;
class ITableRow;
struct FPropertyChangedEvent;
class FTransactionObjectEvent;

struct FGridWorkshopAssetLink
{
	FText Label;
	FSoftObjectPath Path;
};

/** Editor-only Slate UI; no WBP or runtime author controls. */
class SGridItemWorkshop : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridItemWorkshop) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SGridItemWorkshop() override;
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
	void RefreshRegistry(bool bInitial = false);
	void RefreshList();
	void RefreshSelection(bool bRecenter);
	void UpdateAssets();
	void OnAssetChanged(UObject* Object, FPropertyChangedEvent& Event);
	void OnAssetTransacted(UObject* Object, const FTransactionObjectEvent& Event);
	void OnSelection(TSharedPtr<FGridWorkshopEntry> Entry, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> MakeItemRow(TSharedPtr<FGridWorkshopEntry> Entry, const TSharedRef<STableViewBase>& Owner);
	TSharedRef<ITableRow> MakeAssetRow(TSharedPtr<FGridWorkshopAssetLink> Entry, const TSharedRef<STableViewBase>& Owner);
	void QueueLevel(int32 Level);
	FText BuildDetails(const FGridItemInspectionView& View, const SGridItemWorkshopViewport& Viewport, EGridItemPreviewResult Result) const;
	TSharedRef<SWidget> MakePreviewPane(const TSharedRef<SGridItemWorkshopViewport>& Viewport, bool bComparison);
	void ToggleComparison();
	void LinkViews();
	void RefreshComparison(bool bRecenter);
	void QueueComparisonLevel(int32 Level);
	TSharedRef<SWidget> MakeContextControls();
	void SetContext(EGridWorkshopContext NewContext);
	void SetLighting(EGridWorkshopLighting NewLighting);

	FGridItemWorkshopSession Session;
	TArray<TSharedPtr<FGridWorkshopEntry>> Filtered;
	TArray<TSharedPtr<FGridWorkshopAssetLink>> Assets;
	TSharedPtr<SListView<TSharedPtr<FGridWorkshopEntry>>> ItemList;
	TSharedPtr<SListView<TSharedPtr<FGridWorkshopAssetLink>>> AssetList;
	TSharedPtr<SGridItemWorkshopViewport> PreviewViewport;
	TSharedPtr<SGridItemWorkshopViewport> ComparisonViewport;
	TSharedPtr<SBox> ComparisonHost;
	int32 ComparisonLevel = 20;
	int32 PendingComparisonLevel = INDEX_NONE;
	bool bSynchronizeViews = true;
	bool bShowComparison = false;
	EGridWorkshopContext Context = EGridWorkshopContext::Examination;
	EGridWorkshopLighting Lighting = EGridWorkshopLighting::Studio;
	FText ComparisonDetails;
	FText DetailsText;
	FString Search;
	EGridWorkshopFilter Filter = EGridWorkshopFilter::All;
	EGridItemPreviewResult PreviewResult = EGridItemPreviewResult::InvalidSelection;
	int32 PendingLevel = INDEX_NONE;
	bool bRefreshPending = false;
	FDelegateHandle ChangedHandle;
	FDelegateHandle TransactedHandle;
};
