#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorDoorMotionOverrideActor);
DECLARE_DELEGATE(FOnGridEditorDoorMotionOverrideRequestRefresh);

/**
 * Selected Object companion panel exposing the sparse MovingPartOverrides that are
 * intentionally allowed at level-instance scope.
 *
 * Shared meshes, motion type, axis, pivot and reverse duration remain Definition-owned.
 * Only Motion.Amount and Motion.Duration can be overridden here per moving-part slot.
 */
class SGridEditorDoorMotionOverridePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorDoorMotionOverridePanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
		SLATE_EVENT(FOnGetGridEditorDoorMotionOverrideActor, OnGetEditorActor)
		SLATE_EVENT(FOnGridEditorDoorMotionOverrideRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	void RequestRefresh() const;
	TSharedRef<SWidget> BuildContent();
	TSharedRef<SWidget> BuildMovingPartSection(FGuid ObjectId, int32 PartIndex);

	void SetAmountOverrideEnabled(FGuid ObjectId, int32 PartIndex, bool bEnabled, float DefinitionAmount);
	void SetAmountOverrideValue(FGuid ObjectId, int32 PartIndex, float NewValue);
	void SetDurationOverrideEnabled(FGuid ObjectId, int32 PartIndex, bool bEnabled, float DefinitionDuration);
	void SetDurationOverrideValue(FGuid ObjectId, int32 PartIndex, float NewValue);

	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorDoorMotionOverrideActor OnGetEditorActor;
	FOnGridEditorDoorMotionOverrideRequestRefresh OnRequestRefresh;
};

#endif
