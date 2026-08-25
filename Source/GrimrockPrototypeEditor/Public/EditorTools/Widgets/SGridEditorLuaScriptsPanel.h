#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h"

#if WITH_EDITOR

enum class EGridLogicIntComparison : uint8;
enum class EGridObjectCondition : uint8;
enum class EGridObjectEvent : uint8;
class AGridLevelEditorActor;
class UGridLevelAsset;

/** Standalone MON19.6 editor for level Lua scripts and targetless bindings. */
class SGridEditorLuaScriptsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorLuaScriptsPanel)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* FindEditorActor() const;
	UGridLevelAsset* GetLevelAsset() const;
	void Rebuild();
	void RebuildBindingOptions();
	void LoadSelectedScriptDraft();

	TSharedRef<SWidget> BuildRoot();
	TSharedRef<SWidget> BuildScriptsSection();
	TSharedRef<SWidget> BuildBindingsSection();
	TSharedRef<SWidget> BuildStatusSection() const;

	FReply OnRefreshClicked();
	FReply OnAddScriptClicked();
	FReply OnApplyScriptClicked();
	FReply OnRevertScriptClicked();
	FReply OnRemoveScriptClicked();
	FReply OnValidateClicked();
	FReply OnCreateBindingClicked();
	FReply OnRemoveBindingClicked(int32 LinkIndex);
	FReply OnSelectScriptClicked(FName ScriptId);

	void SetStatus(const FString& Text, bool bSuccess);

	TSharedRef<SWidget> MakeEventOptionWidget(TSharedPtr<EGridObjectEvent> Item) const;
	TSharedRef<SWidget> MakeNameOptionWidget(TSharedPtr<FName> Item) const;
	TSharedRef<SWidget> MakeConditionOptionWidget(TSharedPtr<EGridObjectCondition> Item) const;
	TSharedRef<SWidget> MakeComparisonOptionWidget(TSharedPtr<EGridLogicIntComparison> Item) const;

	void OnEventChanged(TSharedPtr<EGridObjectEvent> Item, ESelectInfo::Type);
	void OnBindingScriptChanged(TSharedPtr<FName> Item, ESelectInfo::Type);
	void OnCallbackChanged(TSharedPtr<FName> Item, ESelectInfo::Type);
	void OnConditionChanged(TSharedPtr<EGridObjectCondition> Item, ESelectInfo::Type);
	void OnVariableChanged(TSharedPtr<FName> Item, ESelectInfo::Type);
	void OnComparisonChanged(TSharedPtr<EGridLogicIntComparison> Item, ESelectInfo::Type);

	FText GetEventText(EGridObjectEvent Event) const;
	FText GetConditionText(EGridObjectCondition Condition) const;
	FText GetComparisonText(EGridLogicIntComparison Comparison) const;
	FString GetLuaBindingSummary(const struct FGridObjectLink& Link) const;

private:
	FName SelectedScriptId = NAME_None;
	FString DraftScriptId;
	FString DraftSource;

	bool bStatusSuccess = true;
	FString StatusText;
	TArray<FName> LastValidatedCallbacks;

	TArray<TSharedPtr<EGridObjectEvent>> EventOptions;
	TArray<TSharedPtr<FName>> BindingScriptOptions;
	TArray<TSharedPtr<FName>> CallbackOptions;
	TArray<TSharedPtr<EGridObjectCondition>> ConditionOptions;
	TArray<TSharedPtr<FName>> VariableOptions;
	TArray<TSharedPtr<EGridLogicIntComparison>> ComparisonOptions;

	TSharedPtr<EGridObjectEvent> SelectedEvent;
	TSharedPtr<FName> SelectedBindingScript;
	TSharedPtr<FName> SelectedCallback;
	TSharedPtr<EGridObjectCondition> SelectedCondition;
	TSharedPtr<FName> SelectedVariable;
	TSharedPtr<EGridLogicIntComparison> SelectedComparison;

	bool ConditionBoolValue = true;
	int32 ConditionIntValue = 0;
	bool bInvertCondition = false;
};

#endif
