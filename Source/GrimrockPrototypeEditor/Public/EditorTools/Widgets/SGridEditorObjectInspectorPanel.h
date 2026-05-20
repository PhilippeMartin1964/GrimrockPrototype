#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
enum class EGridObjectTriggerMode : uint8;
struct FGridLevelObjectData;

DECLARE_DELEGATE_RetVal (AGridLevelEditorActor*, FOnGetGridEditorObjectInspectorActor);
DECLARE_DELEGATE (FOnGridEditorObjectInspectorRequestRefresh);

class SGridEditorObjectInspectorPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS (SGridEditorObjectInspectorPanel)
        {
        }
        SLATE_ARGUMENT (TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
        SLATE_EVENT (FOnGetGridEditorObjectInspectorActor, OnGetEditorActor)
        SLATE_EVENT (FOnGridEditorObjectInspectorRequestRefresh, OnRequestRefresh)
    SLATE_END_ARGS ()

    void Construct (const FArguments& InArgs);

private:
    AGridLevelEditorActor* GetEditorActor () const;
    void RequestRefresh () const;

    TSharedRef<SWidget> BuildObjectInspectorSection ();
    TSharedRef<SWidget> BuildSelectedObjectCard (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildGameObjectSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildContextualComponentSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildAdvancedDebugSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildDoorDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildLeverDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildButtonDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildPressurePlateDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildTriggerBehaviorSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildReceptacleBehaviorSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildItemSpawnBehaviorSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildReadableTextSection (const FGridLevelObjectData& Obj);

    TSharedRef<SWidget> MakeTriggerModeComboWidget (TSharedPtr<EGridObjectTriggerMode> Item) const;

    FReply OnApplySelectedObjectClicked ();
    FReply OnResetBehaviorFromArchetypeClicked ();
    FReply OnMoveSelectedObjectToCurrentCellClicked ();
    FReply OnFocusSelectedObjectClicked ();
    FReply OnRotateSelectedObjectYawClicked ();

    void BuildTriggerModeOptions ();

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorObjectInspectorActor OnGetEditorActor;
    FOnGridEditorObjectInspectorRequestRefresh OnRequestRefresh;
    TArray<TSharedPtr<EGridObjectTriggerMode>> TriggerModeOptions;
};

#endif
