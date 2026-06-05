#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "UI/GridPartyMemberWidget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridInventoryWidget.generated.h"

class AGrimrockPartyPawn;
class UGridPartyInventoryComponent;
class UGridInventorySlotWidget;
class UGridPartyMemberWidget;
class UButton;
class UHorizontalBox;
class UTexture2D;
class UUniformGridPanel;
class UWidgetSwitcher;

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridInventoryWidget : public UUserWidget
{
    GENERATED_BODY ()

public:
    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots")
    TSubclassOf<UGridInventorySlotWidget> InventorySlotWidgetClass;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots", meta = (ClampMin = "1"))
    int32 InventorySlotColumnCount = 6;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots", meta = (ClampMin = "0"))
    int32 InventorySlotCountOverride = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory UI|Slots")
    TArray<TObjectPtr<UGridInventorySlotWidget>> GeneratedInventorySlotWidgets;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Slots")
    TObjectPtr<UUniformGridPanel> InventorySlotsGridPanel;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UHorizontalBox> HorizontalBox_MainContent;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UWidgetSwitcher> WidgetSwitcher_MainContent;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UButton> Button_TabInventory;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UButton> Button_TabSkills;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UButton> Button_TabJournal;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UButton> Button_TabMap;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UButton> Button_TabRecipes;

    UPROPERTY (meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    TObjectPtr<UButton> Button_TabCodex;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory UI|Top Tabs")
    EInventoryTopTab CurrentTopTab = EInventoryTopTab::Inventory;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TArray<TObjectPtr<UGridInventorySlotWidget>> RegisteredInventorySlots;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TObjectPtr<UGridInventorySlotWidget> MainHandSlotWidget;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TObjectPtr<UGridInventorySlotWidget> OffHandSlotWidget;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Slots")
    TObjectPtr<UGridInventorySlotWidget> CursorSlotWidget;

    UPROPERTY (BlueprintReadOnly, Category = "Inventory|Party")
    TArray<TObjectPtr<UGridPartyMemberWidget>> RegisteredPartyMemberWidgets;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    void InitializeInventoryWidget (AGrimrockPartyPawn* InPartyPawn);

    UFUNCTION (BlueprintCallable, BlueprintNativeEvent, Category = "Inventory")
    void RefreshInventory ();

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    int32 GetSelectedCharacterIndex () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    int32 GetInventorySlotCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetInventoryItemAtSlot (int32 SlotIndex, FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetMainHandItem (FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetOffHandItem (FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool GetCursorItem (FGridItemInstance& OutItem) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory")
    bool HasCursorItem () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetItemDisplayString (const FGridItemInstance& Item) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetCursorItemDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetMainHandDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetOffHandDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Display")
    FString GetInventorySlotDisplayText (int32 SlotIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetActiveCharacterCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    int32 GetMaxActiveCharacterCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool GetCharacterSummary (int32 CharacterIndex, FGridInventoryCharacterSummary& OutSummary) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    bool SelectCharacter (int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    FString GetCharacterDisplayText (int32 CharacterIndex) const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    FString GetSelectedCharacterDisplayText () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void RegisterPartyMemberWidget (UGridPartyMemberWidget* MemberWidget, int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void RefreshRegisteredPartyMemberWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Party")
    void HandleRegisteredPartyMemberClicked (int32 CharacterIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void RegisterInventorySlotWidget (UGridInventorySlotWidget* SlotWidget, EGridInventoryUiSlotType SlotType, int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void RebuildInventorySlotWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void ClearGeneratedInventorySlotWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    int32 ResolveInventorySlotWidgetCount () const;

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void SetInventorySlotWidgetClass (TSubclassOf<UGridInventorySlotWidget> InClass);

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Slots")
    void SetInventorySlotsGridPanel (UUniformGridPanel* InGridPanel);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void RefreshRegisteredSlotWidgets ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Slots")
    void HandleRegisteredSlotClicked (EGridInventoryUiSlotType SlotType, int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Drag")
    bool HandleSlotDrop (
        EGridInventoryUiSlotType SourceType,
        int32 SourceIndex,
        EGridInventoryUiSlotType TargetType,
        int32 TargetIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleInventorySlotClicked (int32 SlotIndex);

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleMainHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleOffHandClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory|Actions")
    bool HandleCursorReturnToInventoryClicked ();

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Top Tabs")
    void SetActiveTopTab (EInventoryTopTab NewTab);

    UFUNCTION (BlueprintCallable, Category = "Inventory UI|Top Tabs")
    void UpdateTopTabButtonStyles ();

protected:
    virtual void NativeConstruct () override;

private:
    void BuildMainContentSwitcher ();
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

    void RemoveGeneratedInventorySlotsFromRegistry ();

    UPROPERTY (Transient)
    TMap<TObjectPtr<UButton>, FButtonStyle> DefaultTopTabButtonStyles;

    UPROPERTY (Transient)
    TObjectPtr<UTexture2D> SelectedTopTabTexture;

    UPROPERTY (Transient)
    bool bInventorySlotsBuilt = false;

    UPROPERTY (Transient)
    int32 LastBuiltSlotCount = 0;

    UPROPERTY (Transient)
    int32 LastBuiltColumnCount = 0;

    UPROPERTY (Transient)
    TSubclassOf<UGridInventorySlotWidget> LastBuiltSlotWidgetClass;

    UPROPERTY (Transient)
    TObjectPtr<UUniformGridPanel> LastBuiltGridPanel;
};
