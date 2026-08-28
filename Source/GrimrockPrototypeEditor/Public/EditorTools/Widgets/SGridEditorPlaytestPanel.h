#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorPlaytestActor);
DECLARE_DELEGATE(FOnGridEditorPlaytestRequestRefresh);

/**
 * Shared PlayTest controls for the Grid Editor.
 *
 * GEUI03 extracts the existing PIE preparation UI into this widget so the
 * legacy Toolkit and the dockable PlayTest & Validation workspace use the same
 * behavior and the same AGridLevelEditorActor state.
 */
class SGridEditorPlaytestPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorPlaytestPanel)
	{
	}
	SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
	SLATE_EVENT(FOnGetGridEditorPlaytestActor, OnGetEditorActor)
	SLATE_EVENT(FOnGridEditorPlaytestRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	void RequestRefresh() const;
	TSharedRef<SWidget> BuildPanel();

private:
	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorPlaytestActor OnGetEditorActor;
	FOnGridEditorPlaytestRequestRefresh OnRequestRefresh;
};

#endif
