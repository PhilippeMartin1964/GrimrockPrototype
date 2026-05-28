#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Templates/Function.h"

#if WITH_EDITOR

#include "Toolkits/BaseToolkit.h"

class SWidget;
class SVerticalBox;
class AGridLevelEditorActor;
struct FGridEditorToolPalettePanelState;
struct FGridEditorValidationPanelState;

struct FGridEditorPanelExpansionState
{
    bool bDungeonLevelsExpanded = true;
    bool bToolsExpanded = true;
    bool bOverviewExpanded = true;
    bool bSelectedObjectExpanded = true;
    bool bLinksExpanded = false;
    bool bValidationExpanded = false;
};

class FGridLevelEdModeToolkit : public FModeToolkit
{
public:
    virtual void Init (const TSharedPtr<IToolkitHost>& InitToolkitHost) override;

    virtual FName GetToolkitFName () const override;
    virtual FText GetBaseToolkitName () const override;
    virtual class FEdMode* GetEditorMode () const override;
    virtual TSharedPtr<SWidget> GetInlineContent () const override;

    void RefreshPalette ();

private:
    AGridLevelEditorActor* GetEditorActor () const;

    TSharedRef<SWidget> BuildToolkitWidget ();

    TSharedRef<SWidget> BuildHeaderSection ();
    TSharedRef<SWidget> BuildDungeonLevelsPanel ();
    TSharedRef<SWidget> BuildCollapsiblePanelSection (
        const FText& Title,
        const TFunctionRef<TSharedRef<SWidget> ()>& BuildContent,
        bool& bExpanded);
    FReply HandleSelectDungeonLevel (FName LevelId);
    FReply TogglePanelExpansion (bool* bExpanded);
    void ExpandValidationIfMessagesNeedAttention ();

    FText GetActiveToolText () const;
    FText GetSelectedCellStatusText () const;
    FText GetSelectedEdgeStatusText () const;
    FText GetSelectedObjectStatusText () const;
    FText GetValidationStatusText () const;

private:
    TSharedPtr<SVerticalBox> ToolkitRoot;
    TSharedPtr<SWidget> ToolkitWidget;

    TSharedPtr<FGridEditorToolPalettePanelState> ToolPaletteState;
    TSharedPtr<FGridEditorValidationPanelState> ValidationState;
    FGridEditorPanelExpansionState PanelExpansionState;
};

#endif
