#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
enum class EGridItemType : uint8;
enum class EGridObjectCommand : uint8;
enum class EGridObjectCondition : uint8;
enum class EGridObjectEvent : uint8;
struct FGridLevelObjectData;
struct FGridObjectLink;

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
    TSharedRef<SWidget> BuildConditionCreationSection ();
    TSharedRef<SWidget> BuildObjectLinksList (const FGridLevelObjectData& SelectedObject, bool bOutgoing);
    TSharedRef<SWidget> BuildObjectCombo (const FText& EmptyText, bool bSourceObject);

    void RebuildLinksSection ();
    void BuildObjectOptions ();
    void BuildEventOptions ();
    void BuildCommandOptions ();
    void BuildConditionOptions ();
    void BuildItemTypeOptions ();
    void RefreshConnectorFormOptions ();
    FReply OnRemoveExactLinkClicked (FGridObjectLink Link);
    FReply OnClearSelectedObjectLinksClicked ();
    FReply OnSelectObjectFromLinkClicked (FGuid ObjectId);
    FReply OnToggleAddConnectorClicked ();
    FReply OnCreateConnectorClicked ();
    FReply OnCancelAddConnectorClicked ();

    FText GetObjectSummaryText (const FGuid& ObjectId) const;
    FText GetLinkSourceEventText (EGridObjectEvent SourceEvent) const;
    FText GetLinkCommandText (EGridObjectCommand Command) const;
    FText GetLinkConditionText (EGridObjectCondition Condition) const;
    FText GetLinkConditionSummaryText (const FGridObjectLink& Link) const;
    FText GetItemTypeText (EGridItemType ItemType) const;
    FText GetSelectedObjectOptionText (const TSharedPtr<FGuid>& ObjectId, const FText& EmptyText) const;
    FGridObjectLink BuildLinkFromForm () const;
    bool CanCreateConnector () const;
    bool IsConditionSelected (EGridObjectCondition Condition) const;

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

    TSharedRef<SWidget> MakeLinkConditionComboWidget (TSharedPtr<EGridObjectCondition> Item) const;
    void OnLinkConditionSelectionChanged (TSharedPtr<EGridObjectCondition> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedLinkConditionText () const;

    TSharedRef<SWidget> MakeItemTypeComboWidget (TSharedPtr<EGridItemType> Item) const;
    void OnItemTypeSelectionChanged (TSharedPtr<EGridItemType> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedItemTypeText () const;

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorLinksActor OnGetEditorActor;
    FOnGridEditorLinksRequestRefresh OnRequestRefresh;

    TArray<TSharedPtr<EGridObjectEvent>> LinkSourceEventOptions;
    TArray<TSharedPtr<EGridObjectCommand>> LinkCommandOptions;
    TArray<TSharedPtr<EGridObjectCondition>> LinkConditionOptions;
    TArray<TSharedPtr<EGridItemType>> ItemTypeOptions;
    TArray<TSharedPtr<FGuid>> SourceObjectOptions;
    TArray<TSharedPtr<FGuid>> TargetObjectOptions;

    bool bAddConnectorVisible = false;
    TSharedPtr<FGuid> SelectedSourceObjectId;
    TSharedPtr<FGuid> SelectedTargetObjectId;
    TSharedPtr<EGridObjectEvent> SelectedSourceEvent;
    TSharedPtr<EGridObjectCommand> SelectedCommand;
    TSharedPtr<EGridObjectCondition> SelectedCondition;
    TSharedPtr<EGridItemType> SelectedConditionItemType;

    FName ConditionItemDefinitionId = NAME_None;
    FName ConditionItemTag = NAME_None;
    int32 ConditionCount = 1;
    float ConditionWeight = 0.0f;
    bool bInvertCondition = false;
};

#endif
