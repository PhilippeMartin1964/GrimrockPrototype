#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
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
    TSharedRef<SWidget> BuildConnectorsHeader (bool bAllowAddConnector);
    TSharedRef<SWidget> BuildConnectorLegend ();
    TSharedRef<SWidget> BuildConnectorLegendItem (const FText& Label, const FSlateColor& Color) const;
    TSharedRef<SWidget> BuildLinkCreationSection ();
    TSharedRef<SWidget> BuildObjectLinksList (const FGridLevelObjectData& SelectedObject, bool bOutgoing);
    TSharedRef<SWidget> BuildObjectCombo (const FText& EmptyText, bool bSourceObject);

    void RebuildLinksSection ();
    void BuildObjectOptions ();
    void BuildEventOptions ();
    void BuildCommandOptions ();
    void RefreshConnectorFormOptions ();
    FReply OnRemoveExactLinkClicked (FGuid SourceObjectId, FGuid TargetObjectId, EGridObjectEvent SourceEvent, EGridObjectCommand Command);
    FReply OnClearSelectedObjectLinksClicked ();
    FReply OnSelectObjectFromLinkClicked (FGuid ObjectId);
    FReply OnToggleAddConnectorClicked ();
    FReply OnCreateConnectorClicked ();
    FReply OnCancelAddConnectorClicked ();

    FText GetObjectSummaryText (const FGuid& ObjectId) const;
    FText GetLinkSourceEventText (EGridObjectEvent SourceEvent) const;
    FText GetLinkCommandText (EGridObjectCommand Command) const;
    FText GetSelectedObjectOptionText (const TSharedPtr<FGuid>& ObjectId, const FText& EmptyText) const;
    bool CanCreateConnector () const;

    void BuildLinkOptions ();

    TSharedRef<SWidget> MakeObjectComboWidget (TSharedPtr<FGuid> Item) const;
    void OnSourceObjectSelectionChanged (TSharedPtr<FGuid> NewValue, ESelectInfo::Type SelectInfo);
    void OnTargetObjectSelectionChanged (TSharedPtr<FGuid> NewValue, ESelectInfo::Type SelectInfo);

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
    TArray<TSharedPtr<FGuid>> SourceObjectOptions;
    TArray<TSharedPtr<FGuid>> TargetObjectOptions;

    bool bAddConnectorVisible = false;
    TSharedPtr<FGuid> SelectedSourceObjectId;
    TSharedPtr<FGuid> SelectedTargetObjectId;
    TSharedPtr<EGridObjectEvent> SelectedSourceEvent;
    TSharedPtr<EGridObjectCommand> SelectedCommand;
};

#endif
