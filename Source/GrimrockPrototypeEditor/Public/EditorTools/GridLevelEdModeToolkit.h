#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

#include "Toolkits/BaseToolkit.h"

class SWidget;
class SVerticalBox;
class AGridLevelEditorActor;
struct FGridEditorToolPalettePanelState;
struct FGridEditorValidationPanelState;

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
    TSharedRef<SWidget> BuildPanelSection (const FText& Title, TSharedRef<SWidget> Content);

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
};

#endif
