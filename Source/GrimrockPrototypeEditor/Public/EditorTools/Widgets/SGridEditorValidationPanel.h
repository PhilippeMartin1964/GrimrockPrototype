#pragma once

#include "CoreMinimal.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
class SSearchBox;
class SVerticalBox;

struct FGridEditorValidationPanelState
{
	TArray<FGridLevelValidationMessage> ValidationMessages;
	bool bValidationHasRun = false;
	bool bShowErrors = true;
	bool bShowWarnings = true;
	bool bShowInfos = true;
	FString SearchText;

	void CountValidationErrorsWarnings(int32& OutErrorCount, int32& OutWarningCount) const;
	void CountValidationMessages(int32& OutErrorCount, int32& OutWarningCount, int32& OutInfoCount) const;
	FText GetValidationStatusText() const;
};

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorValidationActor);
DECLARE_DELEGATE(FOnGridEditorValidationRequestRefresh);

class SGridEditorValidationPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorValidationPanel)
	{
	}
	SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
	SLATE_ARGUMENT(TSharedPtr<FGridEditorValidationPanelState>, ValidationState)
	SLATE_EVENT(FOnGetGridEditorValidationActor, OnGetEditorActor)
	SLATE_EVENT(FOnGridEditorValidationRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	FGridEditorValidationPanelState& GetValidationState() const;
	void RequestRefresh() const;

	TSharedRef<SWidget> BuildValidationSection();
	TSharedRef<SWidget> BuildValidationResults();
	void RebuildValidationResults();
	FReply OnValidateLevelClicked();
	FReply OnCopySummaryClicked() const;
	FReply OnClearFiltersClicked();
	FReply OnSelectObjectClicked(FGuid ObjectId, bool bFocus);
	FReply OnSelectCellClicked(int32 CellX, int32 CellY);
	void OnSearchTextChanged(const FText& NewText);
	bool ShouldShowMessage(const FGridLevelValidationMessage& ValidationMessage) const;
	bool DoesMessageMatchSearch(const FGridLevelValidationMessage& ValidationMessage) const;

private:
	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	TSharedPtr<FGridEditorValidationPanelState> ValidationState;
	FOnGetGridEditorValidationActor OnGetEditorActor;
	FOnGridEditorValidationRequestRefresh OnRequestRefresh;
	TSharedPtr<SSearchBox> ValidationSearchBox;
	TSharedPtr<SVerticalBox> ValidationResultsRoot;
};

#endif
