#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "UI/GridInventoryUiTypes.h"
#include "GrimrockMenuWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UCanvasPanel;
class UGridInventoryWidget;
class UScaleBox;
class USizeBox;
class UTexture2D;
class UWidget;
class UWidgetSwitcher;

UCLASS ()
class GRIMROCKPROTOTYPE_API UGrimrockMenuWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UFUNCTION (BlueprintCallable, Category = "Menu")
    void InitializeMenuWidget (AGrimrockPartyPawn* InPartyPawn);

    UFUNCTION (BlueprintCallable, Category = "Menu")
    void RefreshInventory ();

    UFUNCTION (BlueprintCallable, Category = "Menu|Top Tabs")
    void SetActiveTopTab (EInventoryTopTab NewTab);

    UFUNCTION (BlueprintCallable, Category = "Menu|Top Tabs")
    void UpdateTopTabButtonStyles ();

    UFUNCTION (BlueprintCallable, Category = "Menu")
    UGridInventoryWidget* GetInventoryWidget () const;

    UPROPERTY (BlueprintReadOnly, Category = "Menu")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "Menu|Top Tabs")
    EInventoryTopTab CurrentTopTab = EInventoryTopTab::Inventory;

protected:
    virtual void NativeConstruct () override;

    virtual void NativeTick (
        const FGeometry& MyGeometry,
        float InDeltaTime) override;

private:
    void ApplyMenuViewportLimit ();
    UWidget* GetTopTabPage (EInventoryTopTab Tab) const;
    void BindTopTabButtons ();
    void ApplyTopTabButtonStyle (UButton* Button, EInventoryTopTab Tab);

    UFUNCTION ()
    void HandleInventoryTopTabClicked ();

    UFUNCTION ()
    void HandleSkillsTopTabClicked ();

    UFUNCTION ()
    void HandleJournalTopTabClicked ();

    UFUNCTION ()
    void HandleMapTopTabClicked ();

    UFUNCTION ()
    void HandleRecipesTopTabClicked ();

    UFUNCTION ()
    void HandleCodexTopTabClicked ();

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> CanvasPanel_Root;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UScaleBox> ScaleBox_MenuRoot;

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> SizeBox_MenuDesign;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UWidgetSwitcher> WidgetSwitcher_MainContent;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UButton> Button_TabInventory;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UButton> Button_TabSkills;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UButton> Button_TabJournal;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UButton> Button_TabMap;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UButton> Button_TabRecipes;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UButton> Button_TabCodex;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UGridInventoryWidget> Page_Inventory;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UWidget> Page_Skills;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UWidget> Page_Journal;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UWidget> Page_Map;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UWidget> Page_Recipes;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UWidget> Page_Codex;

    UPROPERTY (Transient)
    TMap<TObjectPtr<UButton>, FButtonStyle> DefaultTopTabButtonStyles;

    UPROPERTY (Transient)
    TObjectPtr<UTexture2D> SelectedTopTabTexture;

    UPROPERTY (Transient)
    bool bTopTabsInitialized = false;

    UPROPERTY (Transient)
    FVector2D LastAppliedViewportPx = FVector2D::ZeroVector;

    UPROPERTY (Transient)
    float LastAppliedViewportScale = 0.0f;
};
