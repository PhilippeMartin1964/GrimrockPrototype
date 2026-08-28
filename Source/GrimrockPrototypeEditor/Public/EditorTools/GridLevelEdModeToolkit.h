#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"

#if WITH_EDITOR

#include "Toolkits/BaseToolkit.h"

class AGridLevelEditorActor;
class SVerticalBox;
class SWidget;

class FGridLevelEdModeToolkit : public FModeToolkit
{
public:
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual class FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<SWidget> GetInlineContent() const override;

	// Kept as the public refresh entry point used by FGridLevelEdMode.
	void RefreshPalette();

private:
	AGridLevelEditorActor* GetEditorActor() const;

	TSharedRef<SWidget> BuildToolkitWidget();
	TSharedRef<SWidget> BuildHeaderSection();
	TSharedRef<SWidget> BuildWorkspaceLauncherSection();

	FReply OpenWorkspaceTab(FName TabName);

	FText GetCurrentLevelStatusText() const;
	FText GetActiveToolText() const;
	FText GetSelectedCellStatusText() const;
	FText GetSelectedEdgeStatusText() const;
	FText GetSelectedObjectStatusText() const;

private:
	TSharedPtr<SVerticalBox> ToolkitRoot;
	TSharedPtr<SWidget> ToolkitWidget;
};

#endif
