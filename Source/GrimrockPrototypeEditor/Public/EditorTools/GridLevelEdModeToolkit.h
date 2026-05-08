#pragma once

#include "CoreMinimal.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Core/GridTypes.h"

#include "Styling/SlateBrush.h"
#include "Widgets/Input/SButton.h"

#if WITH_EDITOR

#include "Toolkits/BaseToolkit.h"

class SWidget;
class AGridLevelEditorActor;
class UGridObjectPaletteAsset;
struct FGridEditorValidationPanelState;
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

    TSharedRef<SWidget> BuildToolSection ();
    TSharedRef<SWidget> BuildToolTile (const FText& Label, const FText& Glyph, EGridEditorTool ToolValue);
    UTexture2D* GetToolIcon (EGridEditorTool Tool) const;
    TSharedRef<SWidget> BuildIconOrFallback (UTexture2D* Icon, EGridLevelObjectType FallbackType, float Size);

    TSharedRef<SWidget> BuildPaletteSection ();
    TSharedRef<SWidget> BuildPaletteTile (const FGridObjectPaletteEntry& Entry);

    FReply OnToolClicked (int32 ToolValue);
    FReply OnPaletteEntryClicked (FName EntryId);

    FText GetSelectedPaletteEntryText () const;
    FText GetActiveToolText () const;
    FText GetSelectedCellStatusText () const;
    FText GetSelectedEdgeStatusText () const;
    FText GetSelectedObjectStatusText () const;
    FText GetValidationStatusText () const;

    const FSlateBrush* GetOrCreateBrush (UTexture2D* Texture, float Size);

private:
    TSharedPtr<SVerticalBox> ToolkitRoot;
    TSharedPtr<SWidget> ToolkitWidget;

    TSharedPtr<FGridEditorValidationPanelState> ValidationState;

    TMap<FString, TSharedPtr<FSlateBrush>> CachedIconBrushes;
};

#endif
