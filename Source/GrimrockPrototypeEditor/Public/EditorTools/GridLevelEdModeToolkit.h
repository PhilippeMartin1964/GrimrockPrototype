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

    void CountValidationErrorsWarnings (int32& OutErrorCount, int32& OutWarningCount) const;

    TSharedRef<SWidget> BuildToolSection ();
    TSharedRef<SWidget> BuildToolTile (const FText& Label, const FText& Glyph, EGridEditorTool ToolValue);
    UTexture2D* GetToolIcon (EGridEditorTool Tool) const;
    TSharedRef<SWidget> BuildIconOrFallback (UTexture2D* Icon, EGridLevelObjectType FallbackType, float Size);

    TSharedRef<SWidget> BuildPaletteSection ();
    TSharedRef<SWidget> BuildPaletteTile (const FGridObjectPaletteEntry& Entry);

    TSharedRef<SWidget> BuildBehaviorEditorSection ();

    TSharedRef<SWidget> BuildValidationSection ();

    FReply OnToolClicked (int32 ToolValue);
    FReply OnPaletteEntryClicked (FName EntryId);
    FReply OnValidateLevelClicked ();

    FText GetSelectedPaletteEntryText () const;
    FText GetActiveToolText () const;
    FText GetSelectedCellStatusText () const;
    FText GetSelectedEdgeStatusText () const;
    FText GetSelectedObjectStatusText () const;
    FText GetValidationStatusText () const;

private:
    void BuildTriggerModeOptions ();
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

    const FSlateBrush* GetOrCreateBrush (UTexture2D* Texture, float Size);

private:
    TSharedPtr<SVerticalBox> ToolkitRoot;
    TSharedPtr<SWidget> ToolkitWidget;

    FGuid CachedBehaviorObjectId;
    FGridObjectBehaviorParams EditedBehavior;
    TArray<FGridLevelValidationMessage> ValidationMessages;
    bool bValidationHasRun = false;

    TArray<TSharedPtr<EGridObjectTriggerMode>> TriggerModeOptions;
    TMap<FString, TSharedPtr<FSlateBrush>> CachedIconBrushes;
};

#endif
