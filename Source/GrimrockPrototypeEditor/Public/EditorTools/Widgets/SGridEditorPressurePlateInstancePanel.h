#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorPressurePlateInstanceActor);
DECLARE_DELEGATE(FOnGridEditorPressurePlateInstanceRequestRefresh);

/**
 * Selected Object companion panel for pressure-plate instance activation rules.
 *
 * A pressure plate has no authored "pressed at start" state: runtime pressure is
 * derived from actual party/monster occupancy and item weight. This companion
 * exposes the optional monster-occupancy rule without introducing generic state.
 */
class SGridEditorPressurePlateInstancePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorPressurePlateInstancePanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
		SLATE_EVENT(FOnGetGridEditorPressurePlateInstanceActor, OnGetEditorActor)
		SLATE_EVENT(FOnGridEditorPressurePlateInstanceRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	void RequestRefresh() const;
	TSharedRef<SWidget> BuildContent();

	void SetMonsterActivates(FGuid ObjectId, bool bMonsterActivates);

	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorPressurePlateInstanceActor OnGetEditorActor;
	FOnGridEditorPressurePlateInstanceRequestRefresh OnRequestRefresh;
};

#endif
