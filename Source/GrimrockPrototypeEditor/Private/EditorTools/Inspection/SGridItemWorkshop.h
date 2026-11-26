#pragma once

#include "CoreMinimal.h"
#include "GridItemWorkshopSession.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SGridItemWorkshopViewport;
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
	FText BuildDetails() const;

	FGridItemWorkshopSession Session;
	TArray<TSharedPtr<FGridWorkshopEntry>> Filtered;
	TArray<TSharedPtr<FGridWorkshopAssetLink>> Assets;
	TSharedPtr<SListView<TSharedPtr<FGridWorkshopEntry>>> ItemList;
	TSharedPtr<SListView<TSharedPtr<FGridWorkshopAssetLink>>> AssetList;
	TSharedPtr<SGridItemWorkshopViewport> PreviewViewport;
	FText DetailsText;
	FString Search;
	EGridWorkshopFilter Filter = EGridWorkshopFilter::All;
	EGridItemPreviewResult PreviewResult = EGridItemPreviewResult::InvalidSelection;
	int32 PendingLevel = INDEX_NONE;
	bool bRefreshPending = false;
	FDelegateHandle ChangedHandle;
	FDelegateHandle TransactedHandle;
};
