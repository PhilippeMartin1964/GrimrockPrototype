#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
struct FGridLevelObjectData;

DECLARE_DELEGATE_RetVal (AGridLevelEditorActor*, FOnGetGridEditorLinksActor);
DECLARE_DELEGATE (FOnGridEditorLinksRequestRefresh);

class SGridEditorLinksPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS (SGridEditorLinksPanel)
        {
        }
        SLATE_ARGUMENT (TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
        SLATE_EVENT (FOnGetGridEditorLinksActor, OnGetEditorActor)
        SLATE_EVENT (FOnGridEditorLinksRequestRefresh, OnRequestRefresh)
    SLATE_END_ARGS ()

    void Construct (const FArguments& InArgs);

private:
    AGridLevelEditorActor* GetEditorActor () const;
    void RequestRefresh () const;

    TSharedRef<SWidget> BuildLinksSection ();
    TSharedRef<SWidget> BuildLinkCreationSection ();
    TSharedRef<SWidget> BuildObjectLinksList (const FGridLevelObjectData& SelectedObject, bool bOutgoing);

    FReply OnRemoveExactLinkClicked (FGuid SourceObjectId, FGuid TargetObjectId, EGridObjectEventType SourceEvent, EGridLinkAction Action);
    FReply OnClearSelectedObjectLinksClicked ();
    FReply OnSelectObjectFromLinkClicked (FGuid ObjectId);

    FText GetObjectSummaryText (const FGuid& ObjectId) const;
    FText GetLinkSourceEventText (EGridObjectEventType SourceEvent) const;
    FText GetLinkActionText (EGridLinkAction Action) const;

    void BuildLinkOptions ();

    TSharedRef<SWidget> MakeLinkSourceEventComboWidget (TSharedPtr<EGridObjectEventType> Item) const;
    void OnLinkSourceEventSelectionChanged (TSharedPtr<EGridObjectEventType> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedLinkSourceEventText () const;

    TSharedRef<SWidget> MakeLinkActionComboWidget (TSharedPtr<EGridLinkAction> Item) const;
    void OnLinkActionSelectionChanged (TSharedPtr<EGridLinkAction> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedLinkActionText () const;

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorLinksActor OnGetEditorActor;
    FOnGridEditorLinksRequestRefresh OnRequestRefresh;

    TArray<TSharedPtr<EGridObjectEventType>> LinkSourceEventOptions;
    TArray<TSharedPtr<EGridLinkAction>> LinkActionOptions;
};

#endif
