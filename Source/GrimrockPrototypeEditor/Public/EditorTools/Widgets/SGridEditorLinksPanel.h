#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
class UGridLevelAsset;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorLinksActor);
DECLARE_DELEGATE(FOnGridEditorLinksRequestRefresh);

/**
 * LUA-UX01/02: Selected Object Event -> Action authoring.
 *
 * Object identity is authored exclusively on the Properties page. This panel
 * only maps a selected object's events to unconditional commands or Lua
 * callbacks. Conditional/puzzle logic belongs in Lua.
 *
 * Historical conditional links remain readable/removable for asset
 * compatibility, but this panel never creates new conditional links.
 */
class SGridEditorLinksPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorLinksPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
		SLATE_EVENT(FOnGetGridEditorLinksActor, OnGetEditorActor)
		SLATE_EVENT(FOnGridEditorLinksRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	UGridLevelAsset* GetLevelAsset() const;
	void RequestRefresh() const;
	void Rebuild();
	void RefreshOptions();
	void BuildEventOptions();
	void BuildTargetOptions();
	void BuildCommandOptions();
	void BuildScriptOptions();
	void BuildCallbackOptions();

	TSharedRef<SWidget> BuildRoot();
	TSharedRef<SWidget> BuildActionCreationSection();
	TSharedRef<SWidget> BuildActionsListSection();

	TSharedRef<SWidget> MakeEventOptionWidget(TSharedPtr<EGridObjectEvent> Item) const;
	TSharedRef<SWidget> MakeCommandOptionWidget(TSharedPtr<EGridObjectCommand> Item) const;
	TSharedRef<SWidget> MakeNameOptionWidget(TSharedPtr<FName> Item) const;
	TSharedRef<SWidget> MakeObjectOptionWidget(TSharedPtr<FGuid> Item) const;

	void OnEventSelectionChanged(TSharedPtr<EGridObjectEvent> Item, ESelectInfo::Type SelectInfo);
	void OnActionTypeSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo);
	void OnTargetSelectionChanged(TSharedPtr<FGuid> Item, ESelectInfo::Type SelectInfo);
	void OnCommandSelectionChanged(TSharedPtr<EGridObjectCommand> Item, ESelectInfo::Type SelectInfo);
	void OnScriptSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo);
	void OnCallbackSelectionChanged(TSharedPtr<FName> Item, ESelectInfo::Type SelectInfo);

	FReply OnCreateActionClicked();
	FReply OnRemoveActionClicked(FGridObjectLink Link);
	FReply OnGoToObjectClicked(FGuid ObjectId);

	bool IsLuaActionSelected() const;
	bool CanCreateAction() const;
	bool IsActionBroken(const FGridObjectLink& Link) const;
	FString GetObjectSummary(FGuid ObjectId) const;
	FString GetActionSummary(const FGridObjectLink& Link, bool bOutgoing) const;
	FText GetEventText(EGridObjectEvent Event) const;
	FText GetCommandText(EGridObjectCommand Command) const;

private:
	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorLinksActor OnGetEditorActor;
	FOnGridEditorLinksRequestRefresh OnRequestRefresh;

	TArray<TSharedPtr<EGridObjectEvent>> EventOptions;
	TArray<TSharedPtr<FName>> ActionTypeOptions;
	TArray<TSharedPtr<FGuid>> TargetOptions;
	TArray<TSharedPtr<EGridObjectCommand>> CommandOptions;
	TArray<TSharedPtr<FName>> ScriptOptions;
	TArray<TSharedPtr<FName>> CallbackOptions;

	TSharedPtr<EGridObjectEvent> SelectedEvent;
	TSharedPtr<FName> SelectedActionType;
	TSharedPtr<FGuid> SelectedTarget;
	TSharedPtr<EGridObjectCommand> SelectedCommand;
	TSharedPtr<FName> SelectedScript;
	TSharedPtr<FName> SelectedCallback;
};

#endif
