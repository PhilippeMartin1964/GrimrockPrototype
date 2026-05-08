#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
class UTexture2D;
struct FGridObjectPaletteEntry;

struct FGridEditorToolPalettePanelState
{
    TMap<FString, TSharedPtr<FSlateBrush>> CachedIconBrushes;
};

DECLARE_DELEGATE_RetVal (AGridLevelEditorActor*, FOnGetGridEditorToolPaletteActor);
DECLARE_DELEGATE (FOnGridEditorToolPaletteRequestRefresh);

class SGridEditorToolPalettePanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS (SGridEditorToolPalettePanel)
        {
        }
        SLATE_ARGUMENT (TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
        SLATE_ARGUMENT (TSharedPtr<FGridEditorToolPalettePanelState>, ToolPaletteState)
        SLATE_EVENT (FOnGetGridEditorToolPaletteActor, OnGetEditorActor)
        SLATE_EVENT (FOnGridEditorToolPaletteRequestRefresh, OnRequestRefresh)
    SLATE_END_ARGS ()

    void Construct (const FArguments& InArgs);

private:
    AGridLevelEditorActor* GetEditorActor () const;
    FGridEditorToolPalettePanelState& GetToolPaletteState () const;
    void RequestRefresh () const;

    TSharedRef<SWidget> BuildToolPalettePanel ();
    TSharedRef<SWidget> BuildPanelSection (const FText& Title, TSharedRef<SWidget> Content) const;
    TSharedRef<SWidget> BuildToolSection ();
    TSharedRef<SWidget> BuildToolTile (const FText& Label, const FText& Glyph, EGridEditorTool ToolValue);
    UTexture2D* GetToolIcon (EGridEditorTool Tool) const;
    TSharedRef<SWidget> BuildPaletteSection ();
    TSharedRef<SWidget> BuildPaletteTile (const FGridObjectPaletteEntry& Entry);
    TSharedRef<SWidget> BuildIconOrFallback (UTexture2D* Icon, EGridLevelObjectType FallbackType, float Size);

    FReply OnToolClicked (int32 ToolValue);
    FReply OnPaletteEntryClicked (FName EntryId);

    FText GetSelectedPaletteEntryText () const;
    const FSlateBrush* GetOrCreateBrush (UTexture2D* Texture, float Size);

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    TSharedPtr<FGridEditorToolPalettePanelState> ToolPaletteState;
    FOnGetGridEditorToolPaletteActor OnGetEditorActor;
    FOnGridEditorToolPaletteRequestRefresh OnRequestRefresh;
};

#endif
