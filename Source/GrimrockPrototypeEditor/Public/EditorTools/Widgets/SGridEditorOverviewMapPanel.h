#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#if WITH_EDITOR

class AGridLevelEditorActor;
struct FGridLevelCellData;
struct FGridLevelObjectData;

DECLARE_DELEGATE_RetVal (AGridLevelEditorActor*, FOnGetGridEditorActor);
DECLARE_DELEGATE (FOnGridEditorOverviewRequestRefresh);

class SGridEditorOverviewMapPanel : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS (SGridEditorOverviewMapPanel)
        {
        }
        SLATE_ARGUMENT (TWeakObjectPtr<AGridLevelEditorActor>, EditorActor)
        SLATE_EVENT (FOnGetGridEditorActor, OnGetEditorActor)
        SLATE_EVENT (FOnGridEditorOverviewRequestRefresh, OnRequestRefresh)
    SLATE_END_ARGS ()

    void Construct (const FArguments& InArgs);

private:
    AGridLevelEditorActor* GetEditorActor () const;
    void RequestRefresh () const;

    TSharedRef<SWidget> BuildOverviewMapSection ();
    TSharedRef<SWidget> BuildOverviewCell (int32 CellX, int32 CellY, const FGridLevelObjectData* SelectedObject);
    TSharedRef<SWidget> BuildOverviewColorLegend () const;
    TSharedRef<SWidget> BuildOverviewLegendSwatch (const FText& Label, const FLinearColor& Color) const;
    TSharedRef<SWidget> BuildSelectedCellSection ();

    FReply OnOverviewCellClicked (int32 CellX, int32 CellY);

    FText GetOverviewCellTooltipText (int32 CellX, int32 CellY) const;
    FText GetSelectedCellSummaryText () const;
    FText GetCellWallSummaryText (const FGridLevelCellData& CellData) const;
    FText GetCellObjectSummaryText (int32 CellX, int32 CellY) const;
    bool HasObjectAtCell (int32 CellX, int32 CellY) const;

private:
    TWeakObjectPtr<AGridLevelEditorActor> EditorActor;
    FOnGetGridEditorActor OnGetEditorActor;
    FOnGridEditorOverviewRequestRefresh OnRequestRefresh;
};

#endif
