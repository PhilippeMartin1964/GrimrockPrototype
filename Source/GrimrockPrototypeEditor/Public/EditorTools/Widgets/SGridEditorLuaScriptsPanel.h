#pragma once

#include "CoreMinimal.h"
#include "GridLuaVm.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
class UGridLevelAsset;

/**
 * LUA-UX01: scripts-only Lua authoring surface.
 *
 * Object identity and Event -> Action bindings deliberately live in the
 * Selected Object workspace. This panel edits, validates and explains only
 * the level Lua scripts themselves.
 */
class SGridEditorLuaScriptsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorLuaScriptsPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* FindEditorActor() const;
	UGridLevelAsset* GetLevelAsset() const;
	void SetStatus(const FString& Text, bool bSuccess);
	void LoadSelectedScriptDraft();
	void RefreshSelectedScriptMetadata();
	void Rebuild();

	TSharedRef<SWidget> BuildRoot();
	TSharedRef<SWidget> BuildStatusSection() const;
	TSharedRef<SWidget> BuildScriptsSection();
	TSharedRef<SWidget> BuildPersistentStateSection() const;

	FReply OnAddScriptClicked();
	FReply OnSelectScriptClicked(FName ScriptId);
	FReply OnApplyScriptClicked();
	FReply OnRevertScriptClicked();
	FReply OnRemoveScriptClicked();
	FReply OnValidateClicked();

private:
	FName SelectedScriptId = NAME_None;
	FString DraftScriptId;
	FString DraftSource;
	TArray<FName> LastValidatedCallbacks;
	TArray<FGridLuaPersistentVariableDefinition> PersistentDefinitions;
	FString StatusText;
	bool bStatusSuccess = true;
};

#endif
