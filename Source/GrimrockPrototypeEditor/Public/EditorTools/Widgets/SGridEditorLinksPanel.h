#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
enum class EGridObjectCommand : uint8;
enum class EGridObjectEvent : uint8;
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

    FReply OnRemoveExactLinkClicked (FGuid SourceObjectId, FGuid TargetObjectId, EGridObjectEvent SourceEvent, EGridObjectCommand Command);
    FReply OnClearSelectedObjectLinksClicked ();
    FReply OnSelectObjectFromLinkClicked (FGuid ObjectId);

    FText GetObjectSummaryText (const FGuid& ObjectId) const;
    FText GetLinkSourceEventText (EGridObjectEvent SourceEvent) const;
    FText GetLinkCommandText (EGridObjectCommand Command) const;

    void BuildLinkOptions ();

    TSharedRef<SWidget> MakeLinkSourceEventComboWidget (TSharedPtr<EGridObjectEvent> Item) const;
    void OnLinkSourceEventSelectionChanged (TSharedPtr<EGridObjectEvent> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedLinkSourceEventText () const;

    TSharedRef<SWidget> MakeLinkCommandComboWidget (TSharedPtr<EGridObjectCommand> Item) const;
    void OnLinkCommandSelectionChanged (TSharedPtr<EGridObjectCommand> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedLinkCommandText () const;

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorLinksActor OnGetEditorActor;
    FOnGridEditorLinksRequestRefresh OnRequestRefresh;

    TArray<TSharedPtr<EGridObjectEvent>> LinkSourceEventOptions;
    TArray<TSharedPtr<EGridObjectCommand>> LinkCommandOptions;
};

#endif
