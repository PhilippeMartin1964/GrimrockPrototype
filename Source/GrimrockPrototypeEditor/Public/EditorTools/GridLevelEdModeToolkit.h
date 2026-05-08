#pragma once

#include "CoreMinimal.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridTypes.h"
#include "Core/GridObjectBehavior.h"

#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"

#if WITH_EDITOR

#include "Toolkits/BaseToolkit.h"

class SWidget;
class AGridLevelEditorActor;
class UGridObjectPaletteAsset;
struct FGridObjectPaletteEntry;

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
    TSharedRef<SWidget> BuildPropertyRow (const FText& Label, TSharedRef<SWidget> ValueWidget) const;
    TSharedRef<SWidget> BuildReadOnlyPropertyRow (const FText& Label, const FText& Value) const;
    TSharedRef<SWidget> BuildActionButton (const FText& Label, const FOnClicked& OnClicked) const;
    TSharedRef<SWidget> BuildStatusBadge (const FText& Label, const FText& Value, const FSlateColor& AccentColor) const;

    TSharedRef<SWidget> BuildOverviewMapSection ();
    TSharedRef<SWidget> BuildOverviewCell (int32 CellX, int32 CellY, const FGridLevelObjectData* SelectedObject);
    TSharedRef<SWidget> BuildOverviewCellTooltip (int32 CellX, int32 CellY) const;
    FText GetOverviewCellTooltipText (int32 CellX, int32 CellY) const;
    FText GetCellWallSummaryText (const FGridLevelCellData& CellData) const;
    FText GetCellObjectSummaryText (int32 CellX, int32 CellY) const;

    TSharedRef<SWidget> BuildToolSection ();
    TSharedRef<SWidget> BuildToolTile (const FText& Label, const FText& Glyph, EGridEditorTool ToolValue);
    UTexture2D* GetToolIcon (EGridEditorTool Tool) const;
    TSharedRef<SWidget> BuildIconOrFallback (UTexture2D* Icon, EGridLevelObjectType FallbackType, float Size);

    TSharedRef<SWidget> BuildPaletteSection ();
    TSharedRef<SWidget> BuildPaletteTile (const FGridObjectPaletteEntry& Entry);

    TSharedRef<SWidget> BuildObjectInspectorSection ();
    TSharedRef<SWidget> BuildSelectedObjectCard (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildTriggerBehaviorSection (const FGridLevelObjectData& Obj);
    TSharedRef<SWidget> BuildReceptacleBehaviorSection (const FGridLevelObjectData& Obj);

    TSharedRef<SWidget> BuildBehaviorEditorSection ();

    TSharedRef<SWidget> BuildValidationSection ();

    TSharedRef<SWidget> BuildLinksSection (const FGridLevelObjectData& SelectedObject);
    TSharedRef<SWidget> BuildLinkCreationSection ();
    TSharedRef<SWidget> BuildObjectLinksList (const FGridLevelObjectData& SelectedObject, bool bOutgoing) const;

    FReply OnToolClicked (int32 ToolValue);
    FReply OnOverviewCellClicked (int32 CellX, int32 CellY);
    FReply OnPaletteEntryClicked (FName EntryId);
    FReply OnApplySelectedObjectClicked ();
    FReply OnMoveSelectedObjectToCurrentCellClicked ();
    FReply OnValidateLevelClicked ();

    FReply OnRemoveExactLinkClicked (FGuid SourceObjectId, FGuid TargetObjectId, EGridObjectEventType SourceEvent, EGridLinkAction Action);
    FReply OnClearSelectedObjectLinksClicked ();
    FReply OnSelectObjectFromLinkClicked (FGuid ObjectId);
    FReply OnFocusSelectedObjectClicked ();

    FText GetSelectedPaletteEntryText () const;
    FText GetActiveToolText () const;
    FText GetSelectedCellStatusText () const;
    FText GetSelectedEdgeStatusText () const;
    FText GetSelectedObjectStatusText () const;
    FText GetValidationStatusText () const;
    FText GetSelectedObjectDetailsText () const;
    FText GetObjectSummaryText (const FGuid& ObjectId) const;
    FText GetLinkSourceEventText (EGridObjectEventType SourceEvent) const;
    FText GetLinkActionText (EGridLinkAction Action) const;

private:
    void BuildTriggerModeOptions ();
    void BuildLinkOptions ();
    void SyncEditedBehaviorFromSelection ();

    FReply OnApplyBehaviorClicked ();

    TOptional<float> GetEditedDelay () const;
    TOptional<float> GetEditedDuration () const;

    ECheckBoxState GetEditedInvertLinksCheckState () const;
    ECheckBoxState GetEditedFireOnEnterCheckState () const;
    ECheckBoxState GetEditedFireOnExitCheckState () const;

    void OnEditedDelayChanged (float NewValue);
    void OnEditedDurationChanged (float NewValue);
    void OnEditedInvertLinksChanged (ECheckBoxState NewState);
    void OnEditedFireOnEnterChanged (ECheckBoxState NewState);
    void OnEditedFireOnExitChanged (ECheckBoxState NewState);

    TSharedRef<SWidget> MakeTriggerModeComboWidget (TSharedPtr<EGridObjectTriggerMode> Item) const;
    void OnTriggerModeSelectionChanged (TSharedPtr<EGridObjectTriggerMode> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedTriggerModeText () const;

    TSharedRef<SWidget> MakeLinkSourceEventComboWidget (TSharedPtr<EGridObjectEventType> Item) const;
    void OnLinkSourceEventSelectionChanged (TSharedPtr<EGridObjectEventType> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedLinkSourceEventText () const;

    TSharedRef<SWidget> MakeLinkActionComboWidget (TSharedPtr<EGridLinkAction> Item) const;
    void OnLinkActionSelectionChanged (TSharedPtr<EGridLinkAction> NewValue, ESelectInfo::Type SelectInfo);
    FText GetSelectedLinkActionText () const;

    const FSlateBrush* GetOrCreateBrush (UTexture2D* Texture, float Size);

private:
    TSharedPtr<SVerticalBox> ToolkitRoot;
    TSharedPtr<SWidget> ToolkitWidget;

    FGuid CachedBehaviorObjectId;
    FGridObjectBehaviorParams EditedBehavior;
    TArray<FGridLevelValidationMessage> ValidationMessages;
    bool bValidationHasRun = false;

    TArray<TSharedPtr<EGridObjectTriggerMode>> TriggerModeOptions;
    TArray<TSharedPtr<EGridObjectEventType>> LinkSourceEventOptions;
    TArray<TSharedPtr<EGridLinkAction>> LinkActionOptions;
    TMap<FString, TSharedPtr<FSlateBrush>> CachedIconBrushes;
};

#endif
