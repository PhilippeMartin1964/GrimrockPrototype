#pragma once

#include "CoreMinimal.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;

struct FGridEditorValidationPanelState
{
    TArray<FGridLevelValidationMessage> ValidationMessages;
    bool bValidationHasRun = false;

    void CountValidationErrorsWarnings (int32& OutErrorCount, int32& OutWarningCount) const;
    FText GetValidationStatusText () const;
};

DECLARE_DELEGATE_RetVal (AGridLevelEditorActor*, FOnGetGridEditorValidationActor);
DECLARE_DELEGATE (FOnGridEditorValidationRequestRefresh);

class SGridEditorValidationPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS (SGridEditorValidationPanel)
        {
        }
        SLATE_ARGUMENT (TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
        SLATE_ARGUMENT (TSharedPtr<FGridEditorValidationPanelState>, ValidationState)
        SLATE_EVENT (FOnGetGridEditorValidationActor, OnGetEditorActor)
        SLATE_EVENT (FOnGridEditorValidationRequestRefresh, OnRequestRefresh)
    SLATE_END_ARGS ()

    void Construct (const FArguments& InArgs);

private:
    AGridLevelEditorActor* GetEditorActor () const;
    FGridEditorValidationPanelState& GetValidationState () const;
    void RequestRefresh () const;

    TSharedRef<SWidget> BuildValidationSection ();
    FReply OnValidateLevelClicked ();

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    TSharedPtr<FGridEditorValidationPanelState> ValidationState;
    FOnGetGridEditorValidationActor OnGetEditorActor;
    FOnGridEditorValidationRequestRefresh OnRequestRefresh;
};

#endif
