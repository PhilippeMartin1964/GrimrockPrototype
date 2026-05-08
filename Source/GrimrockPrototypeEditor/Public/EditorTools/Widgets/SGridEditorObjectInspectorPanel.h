#pragma once

#include "CoreMinimal.h"
#include "Widgets/Input/SButton.h"
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
    TSharedRef<SWidget> BuildTriggerBehaviorSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildReceptacleBehaviorSection (const FGridLevelObjectData& Obj);

    TSharedRef<SWidget> BuildPropertyRow (const FText& Label, TSharedRef<SWidget> ValueWidget) const;
    TSharedRef<SWidget> BuildReadOnlyPropertyRow (const FText& Label, const FText& Value) const;
    TSharedRef<SWidget> BuildActionButton (const FText& Label, const FOnClicked& OnClicked) const;
    TSharedRef<SWidget> MakeTriggerModeComboWidget (TSharedPtr<EGridObjectTriggerMode> Item) const;

    FReply OnApplySelectedObjectClicked ();
    FReply OnMoveSelectedObjectToCurrentCellClicked ();
    FReply OnFocusSelectedObjectClicked ();

    void BuildTriggerModeOptions ();

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorObjectInspectorActor OnGetEditorActor;
    FOnGridEditorObjectInspectorRequestRefresh OnRequestRefresh;
    TArray<TSharedPtr<EGridObjectTriggerMode>> TriggerModeOptions;
};

#endif
