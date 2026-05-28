#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
class UGridObjectArchetypeAsset;
struct FGridLevelObjectData;
enum class EGridEdge : uint8;

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
    TSharedRef<SWidget> BuildOrientationWidget (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildAdvancedDebugSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildDoorDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildLeverDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildButtonDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildPressurePlateDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildTeleporterDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildTransitionDetailsSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildLightDetailsSection (const UGridObjectArchetypeAsset& Archetype);
    TSharedRef<SWidget> BuildTriggerBehaviorSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildReceptacleBehaviorSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildReadableTextSection (const FGridLevelObjectData& Obj);

    FReply OnApplySelectedObjectClicked ();
    FReply OnResetBehaviorFromArchetypeClicked ();
    FReply OnMoveSelectedObjectToCurrentCellClicked ();
    FReply OnFocusSelectedObjectClicked ();
    FReply OnSetSelectedObjectOrientationClicked (EGridEdge Orientation);

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorObjectInspectorActor OnGetEditorActor;
    FOnGridEditorObjectInspectorRequestRefresh OnRequestRefresh;
};

#endif
