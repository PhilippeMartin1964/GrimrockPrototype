#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorDungeonLevelsActor);
DECLARE_DELEGATE(FOnGridEditorDungeonLevelsRequestRefresh);

/**
 * Authoritative Slate presentation for dungeon-level navigation and management.
 *
 * The widget mutates only the existing AGridLevelEditorActor / UGridDungeonAsset
 * workflow. It is shared by the legacy inline Toolkit and the GEUI02 Nomad tab.
 */
class SGridEditorDungeonLevelsPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorDungeonLevelsPanel)
	{
	}
	SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
	SLATE_EVENT(FOnGetGridEditorDungeonLevelsActor, OnGetEditorActor)
	SLATE_EVENT(FOnGridEditorDungeonLevelsRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	void RequestRefresh() const;
	TSharedRef<SWidget> BuildPanel();

	FReply HandleSelectDungeonLevel(FName LevelId);
	FReply HandleCreateDungeonLevelClicked();

private:
	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorDungeonLevelsActor OnGetEditorActor;
	FOnGridEditorDungeonLevelsRequestRefresh OnRequestRefresh;
};

#endif
