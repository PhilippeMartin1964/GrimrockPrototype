#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectBehavior.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
enum class EGridObjectTriggerMode : uint8;

DECLARE_DELEGATE_RetVal (AGridLevelEditorActor*, FOnGetGridEditorBehaviorActor);
DECLARE_DELEGATE (FOnGridEditorBehaviorRequestRefresh);

class SGridEditorBehaviorPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS (SGridEditorBehaviorPanel)
        {
        }
        SLATE_ARGUMENT (TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
        SLATE_EVENT (FOnGetGridEditorBehaviorActor, OnGetEditorActor)
        SLATE_EVENT (FOnGridEditorBehaviorRequestRefresh, OnRequestRefresh)
    SLATE_END_ARGS ()

    void Construct (const FArguments& InArgs);

private:
    AGridLevelEditorActor* GetEditorActor () const;
    void RequestRefresh () const;

    TSharedRef<SWidget> BuildBehaviorEditorSection ();
    void BuildTriggerModeOptions ();
    void SyncEditedBehaviorFromSelection ();

    FReply OnApplyBehaviorClicked ();

    TOptional<float> GetEditedDelay () const;
    TOptional<float> GetEditedDuration () const;

    ECheckBoxState GetEditedInvertLinksCheckState () const;
    ECheckBoxState GetEditedFireOnEnterCheckState () const;
    ECheckBoxState GetEditedFireOnExitCheckState () const;
    FText GetEditedSpawnedItemArchetypeIdText () const;

    void OnEditedDelayChanged (float NewValue);
    void OnEditedDurationChanged (float NewValue);
    void OnEditedInvertLinksChanged (ECheckBoxState NewState);
    void OnEditedFireOnEnterChanged (ECheckBoxState NewState);
    void OnEditedFireOnExitChanged (ECheckBoxState NewState);
    void OnEditedSpawnedItemArchetypeIdCommitted (const FText& NewText, ETextCommit::Type CommitType);

    TSharedRef<SWidget> MakeTriggerModeComboWidget (TSharedPtr<EGridObjectTriggerMode> Item) const;
    void OnTriggerModeSelectionChanged (TSharedPtr<EGridObjectTriggerMode> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedTriggerModeText () const;

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorBehaviorActor OnGetEditorActor;
    FOnGridEditorBehaviorRequestRefresh OnRequestRefresh;

    FGuid CachedBehaviorObjectId;
    FGridObjectBehaviorParams EditedBehavior;
    TArray<TSharedPtr<EGridObjectTriggerMode>> TriggerModeOptions;
};

#endif
