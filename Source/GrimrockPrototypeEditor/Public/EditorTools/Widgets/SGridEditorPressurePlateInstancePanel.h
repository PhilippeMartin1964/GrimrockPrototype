#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorPressurePlateInstanceActor);
DECLARE_DELEGATE(FOnGridEditorPressurePlateInstanceRequestRefresh);

/**
 * Selected Object companion panel for pressure-plate instance state that is
 * intentionally authored at level scope.
 *
 * It exposes the initial pressed state and the optional monster-occupancy
 * activation rule without introducing a second runtime data model.
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

	void SetPressedAtStart(FGuid ObjectId, bool bPressed);
	void SetActivationRulesOverrideEnabled(FGuid ObjectId, bool bEnabled);
	void SetMonsterActivates(FGuid ObjectId, bool bMonsterActivates);

	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorPressurePlateInstanceActor OnGetEditorActor;
	FOnGridEditorPressurePlateInstanceRequestRefresh OnRequestRefresh;
};

#endif
