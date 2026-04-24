#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

#include "Toolkits/BaseToolkit.h"

class IDetailsView;
class SWidget;
class AGridLevelEditorActor;
class UGridObjectPaletteAsset;

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
    TSharedRef<SWidget> BuildToolSection ();
    TSharedRef<SWidget> BuildPaletteSection ();

    FReply OnToolClicked (int32 ToolValue);
    FReply OnPaletteEntryClicked (FName EntryId);

    FText GetSelectedPaletteEntryText () const;
    FText GetActiveToolText () const;

private:
    TSharedPtr<SVerticalBox> ToolkitRoot;
    TSharedPtr<SWidget> ToolkitWidget;
};

#endif