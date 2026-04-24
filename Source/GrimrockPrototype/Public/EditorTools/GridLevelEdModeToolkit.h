#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"

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

    FReply OnApplySelectedObjectClicked ();

    TSharedRef<SWidget> BuildObjectInspectorSection ();

    FText GetSelectedObjectDetailsText () const;

    TSharedRef<SWidget> BuildObjectLinksList (const FGridLevelObjectData& SelectedObject, bool bOutgoing) const;

    FText GetObjectSummaryText (const FGuid& ObjectId) const;
    FText GetLinkActionText (EGridLinkAction Action) const;

    FReply OnRemoveExactLinkClicked (FGuid SourceObjectId, FGuid TargetObjectId, EGridLinkAction Action);
    FReply OnClearSelectedObjectLinksClicked ();
    FReply OnSelectObjectFromLinkClicked (FGuid ObjectId);

private:
    TSharedPtr<SVerticalBox> ToolkitRoot;
    TSharedPtr<SWidget> ToolkitWidget;
};

#endif