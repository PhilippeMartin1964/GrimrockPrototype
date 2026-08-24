#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "UI/GrimrockDesignSurfaceWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GrimrockMenuWidget.generated.h"

class AGrimrockPartyPawn;
class UButton;
class UGridInventoryWidget;
class UGridSkillsWidget;
class UGridSpellbookWidget;
class UTexture2D;
class UWidget;
class UWidgetSwitcher;

UCLASS ()
class GRIMROCKPROTOTYPE_API UGrimrockMenuWidget : public UGrimrockDesignSurfaceWidget
{
    GENERATED_BODY ()

public:
    UFUNCTION (BlueprintCallable, Category = "Menu")
    void InitializeMenuWidget (AGrimrockPartyPawn* InPartyPawn);

    UFUNCTION (BlueprintCallable, Category = "Menu")
    void RefreshInventory ();

    UFUNCTION (BlueprintCallable, Category = "Menu")
    void RefreshSkills ();

    UFUNCTION (BlueprintCallable, Category = "Menu")
    void RefreshSpellbook ();

    UFUNCTION (BlueprintCallable, Category = "Menu|Top Tabs")
    void SetActiveTopTab (EInventoryTopTab NewTab);

    UFUNCTION (BlueprintCallable, Category = "Menu|Top Tabs")
    void UpdateTopTabButtonStyles ();

    UFUNCTION (BlueprintCallable, Category = "Menu")
    UGridInventoryWidget* GetInventoryWidget () const;

    UFUNCTION (BlueprintCallable, Category = "Menu")
    UGridSkillsWidget* GetSkillsWidget () const;

    UFUNCTION (BlueprintCallable, Category = "Menu")
    UGridSpellbookWidget* GetSpellbookWidget () const;

    UPROPERTY (BlueprintReadOnly, Category = "Menu")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "Menu|Top Tabs")
    EInventoryTopTab CurrentTopTab = EInventoryTopTab::Inventory;

protected:
    virtual void NativeConstruct () override;

private:
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

    UFUNCTION ()
    void HandleSpellbookTopTabClicked ();

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

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_TabSpellbook;

    UPROPERTY (meta = (BindWidget))
    TObjectPtr<UGridInventoryWidget> Page_Inventory;

    /** Kept generic until WBP_GridSkills is reparented after C++ validation. */
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

    UPROPERTY (meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Page_Spellbook;

    UPROPERTY (Transient)
    TMap<TObjectPtr<UButton>, FButtonStyle> DefaultTopTabButtonStyles;

    UPROPERTY (Transient)
    TObjectPtr<UTexture2D> SelectedTopTabTexture;

    UPROPERTY (Transient)
    bool bTopTabsInitialized = false;
};
