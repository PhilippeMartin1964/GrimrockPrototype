#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SNullWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;

DECLARE_DELEGATE_RetVal(AGridLevelEditorActor*, FOnGetGridEditorObjectIdentityActor);
DECLARE_DELEGATE(FOnGridEditorObjectIdentityRequestRefresh);

/**
 * LUA-UX02: single authoring surface for a placed object's human-readable identity.
 *
 * LogicId is the only author-facing identity. Legacy serialized Tag data is
 * intentionally not exposed here and remains only for backward compatibility.
 */
class SGridEditorObjectIdentityPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGridEditorObjectIdentityPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
		SLATE_EVENT(FOnGetGridEditorObjectIdentityActor, OnGetEditorActor)
		SLATE_EVENT(FOnGridEditorObjectIdentityRequestRefresh, OnRequestRefresh)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	AGridLevelEditorActor* GetEditorActor() const;
	void RequestRefresh() const;
	void Rebuild();
	TSharedRef<SWidget> BuildRoot();
	FText GetTypeText(FGuid ObjectId) const;
	FText GetPositionText(FGuid ObjectId) const;
	FText GetDefinitionText(FGuid ObjectId) const;
	void SetStatus(const FString& Text, bool bSuccess);

private:
	TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
	FOnGetGridEditorObjectIdentityActor OnGetEditorActor;
	FOnGridEditorObjectIdentityRequestRefresh OnRequestRefresh;
	FString StatusText;
	bool bStatusSuccess = true;
};

#endif
