#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
class UGridWorldObjectDefinitionAsset;
struct FGridWorldObjectInstanceConfig;
enum class EGridEdge : uint8;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorObjectInspectorActor);
DECLARE_DELEGATE(FOnGridEditorObjectInspectorRequestRefresh);

class SGridEditorObjectInspectorPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorObjectInspectorPanel)
	{
	}
	SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
	SLATE_EVENT(FOnGetGridEditorObjectInspectorActor, OnGetEditorActor)
	SLATE_EVENT(FOnGridEditorObjectInspectorRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	void RequestRefresh() const;
	void EditWorldObjectConfig(FGuid ObjectId, TFunctionRef<void(FGridWorldObjectInstanceConfig&)> Edit);

	TSharedRef<SWidget> BuildObjectInspectorSection();
	TSharedRef<SWidget> BuildSelectedObjectCard(FGuid ObjectId);
	TSharedRef<SWidget> BuildGameObjectSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildContextualComponentSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildOrientationWidget(FGuid ObjectId);
	TSharedRef<SWidget> BuildAdvancedDebugSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildDoorDetailsSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildLeverDetailsSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildButtonDetailsSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildPressurePlateDetailsSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildPitDetailsSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildTeleporterDetailsSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildTransitionDetailsSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildLightDetailsSection(const UGridWorldObjectDefinitionAsset& Definition);
	TSharedRef<SWidget> BuildItemDefinitionSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildMonsterSpawnSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildTriggerBehaviorSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildReceptacleBehaviorSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildLockBehaviorSection(FGuid ObjectId);
	TSharedRef<SWidget> BuildReadableTextSection(FGuid ObjectId);

	FReply OnApplySelectedObjectClicked();
	FReply OnResetBehaviorFromDefinitionClicked();
	FReply OnMoveSelectedObjectToCurrentCellClicked();
	FReply OnFocusSelectedObjectClicked();
	FReply OnSetSelectedObjectOrientationClicked(EGridEdge Orientation);

private:
	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorObjectInspectorActor OnGetEditorActor;
	FOnGridEditorObjectInspectorRequestRefresh OnRequestRefresh;
};

#endif
