#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemActionTypes.h"
#include "UI/GridPartyMemberWidget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "GridInventoryWidget.generated.h"

class AGrimrockPartyPawn;
class UBorder;
class UGridPartyInventoryComponent;
class UGridInventorySlotWidget;
class UGridPartyMemberWidget;
class UImage;
class UProgressBar;
class URPGClassVisualAsset;
class UTextBlock;
class UUniformGridPanel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGridInventoryContextActionsRequested, EGridInventoryUiSlotType, SlotType, int32, SlotIndex);

UCLASS()
class GRIMROCKPROTOTYPE_API UGridInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<AGrimrockPartyPawn> OwningPartyPawn;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UGridPartyInventoryComponent> InventoryComponent;

	/**
	 * UI-CHAR01 canonical six portrait selectors. They exist on CharacterSheet
	 * and remain optional in this shared base because InventoryBag does not own them.
	 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UGridPartyMemberWidget> PartyMember_1;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UGridPartyMemberWidget> PartyMember_2;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UGridPartyMemberWidget> PartyMember_3;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UGridPartyMemberWidget> PartyMember_4;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UGridPartyMemberWidget> PartyMember_5;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Party")
	TObjectPtr<UGridPartyMemberWidget> PartyMember_6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Character Details|Visuals")
	TArray<TObjectPtr<URPGClassVisualAsset>> AvailableClassVisuals;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UImage> Image_CharacterPortrait;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UImage> Image_CharacterClassIcon;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UBorder> Border_CharacterClassAccent;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterName;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterRace;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterClass;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterLevel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterExperience;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterStrength;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterDexterity;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterConstitution;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterIntelligence;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterWisdom;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterCharisma;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterHealth;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UProgressBar> ProgressBar_CharacterHealth;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterMana;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UProgressBar> ProgressBar_CharacterMana;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterCarryWeight;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UProgressBar> ProgressBar_CharacterCarryWeight;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details")
	TObjectPtr<UTextBlock> Text_CharacterInventorySlots;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Combat")
	TObjectPtr<UTextBlock> Text_CharacterPhysicalArmor;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Combat")
	TObjectPtr<UTextBlock> Text_CharacterMagicalArmor;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Combat")
	TObjectPtr<UTextBlock> Text_CharacterInitiative;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Combat")
	TObjectPtr<UTextBlock> Text_CharacterAccuracy;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Combat")
	TObjectPtr<UTextBlock> Text_CharacterEvasion;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistancePhysical;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistanceFire;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistanceIce;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistanceLightning;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistancePoison;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistanceHoly;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistanceNecrotic;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Character Details|Resistances")
	TObjectPtr<UTextBlock> Text_ResistanceArcane;

	/**
	 * UI-INV01 selected-character bag presentation. The bag does not own data:
	 * every value is projected from the same selected character summary.
	 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Bag")
	TObjectPtr<UTextBlock> Text_InventoryBagTitle;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Bag")
	TObjectPtr<UTextBlock> Text_InventoryBagSlotUsage;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Bag")
	TObjectPtr<UTextBlock> Text_InventoryBagWeight;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|Bag")
	TObjectPtr<UProgressBar> ProgressBar_InventoryBagWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots")
	TSubclassOf<UGridInventorySlotWidget> InventorySlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots", meta = (ClampMin = "1"))
	int32 InventorySlotColumnCount = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory UI|Slots", meta = (ClampMin = "0"))
	int32 InventorySlotCountOverride = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory UI|Slots")
	TArray<TObjectPtr<UGridInventorySlotWidget>> GeneratedInventorySlotWidgets;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory UI|Slots")
	TObjectPtr<UUniformGridPanel> InventorySlotsGridPanel;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Head;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Face;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Amulet;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Shoulders;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Shirt;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Chest;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Cloak;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Bracers;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Gloves;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Belt;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Legs;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Feet;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Ring1;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Ring2;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Earring1;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_Earring2;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_MainHand;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Inventory|PaperDoll")
	TObjectPtr<UGridInventorySlotWidget> SlotWidget_OffHand;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Slots")
	TArray<TObjectPtr<UGridInventorySlotWidget>> RegisteredInventorySlots;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Slots")
	TMap<EGridEquipmentSlot, TObjectPtr<UGridInventorySlotWidget>> RegisteredEquipmentSlotWidgets;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Slots")
	TObjectPtr<UGridInventorySlotWidget> MainHandSlotWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Slots")
	TObjectPtr<UGridInventorySlotWidget> OffHandSlotWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Slots")
	TObjectPtr<UGridInventorySlotWidget> CursorSlotWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Party")
	TArray<TObjectPtr<UGridPartyMemberWidget>> RegisteredPartyMemberWidgets;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Context Actions")
	FGridItemInstance LastContextItem;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Context Actions")
	FGridFacingTargetContext LastFacingTargetContext;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Context Actions")
	TArray<FGridItemContextAction> LastContextActions;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Context Actions")
	FOnGridInventoryContextActionsRequested OnContextActionsRequested;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventoryWidget(AGrimrockPartyPawn* InPartyPawn);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetSelectedCharacterIndex() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetInventorySlotCount() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetInventoryItemAtSlot(int32 SlotIndex, FGridItemInstance& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetMainHandItem(FGridItemInstance& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetOffHandItem(FGridItemInstance& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	bool GetEquipmentItem(EGridEquipmentSlot EquipmentSlot, FGridItemInstance& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetCursorItem(FGridItemInstance& OutItem) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasCursorItem() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	int32 GetActiveCharacterCount() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	int32 GetMaxActiveCharacterCount() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	bool GetCharacterSummary(int32 CharacterIndex, FGridInventoryCharacterSummary& OutSummary) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	bool SelectCharacter(int32 CharacterIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void RegisterPartyMemberWidget(UGridPartyMemberWidget* MemberWidget, int32 CharacterIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void RefreshRegisteredPartyMemberWidgets();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Character Details")
	void RefreshSelectedCharacterDetails();

	/** UI-WEIGHT01 presentation hook. Blueprint chooses colors; C++ owns the derived state. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Weight")
	void PresentInventoryWeightState(EGridInventoryWeightState WeightState);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Party")
	void HandleRegisteredPartyMemberClicked(int32 CharacterIndex);

	/** UI-INV02: drop one inventory drag operation on another party portrait. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Party|Transfer")
	bool HandlePartyMemberItemDrop(UGridInventoryDragDropOperation* Operation, int32 TargetCharacterIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	void RegisterInventorySlotWidget(UGridInventorySlotWidget* SlotWidget, EGridInventoryUiSlotType SlotType, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	void RegisterEquipmentSlotWidget(UGridInventorySlotWidget* SlotWidget, EGridEquipmentSlot EquipmentSlot);

	/** True only for views that actually author a paper-doll surface/slot. */
	UFUNCTION(BlueprintPure, Category = "Inventory|PaperDoll")
	bool HasPaperDollPresentation() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory|PaperDoll")
	bool ValidatePaperDollEquipmentRegistration() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory UI|Slots")
	void RebuildInventorySlotWidgets();

	UFUNCTION(BlueprintCallable, Category = "Inventory UI|Slots")
	void ClearGeneratedInventorySlotWidgets();

	UFUNCTION(BlueprintCallable, Category = "Inventory UI|Slots")
	int32 ResolveInventorySlotWidgetCount() const;

	UFUNCTION(BlueprintCallable, Category = "Inventory UI|Slots")
	void SetInventorySlotWidgetClass(TSubclassOf<UGridInventorySlotWidget> InClass);

	UFUNCTION(BlueprintCallable, Category = "Inventory UI|Slots")
	void SetInventorySlotsGridPanel(UUniformGridPanel* InGridPanel);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	void RefreshRegisteredSlotWidgets();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Slots")
	void HandleRegisteredSlotClicked(EGridInventoryUiSlotType SlotType, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	bool HandleItemSlotRightClicked(EGridInventoryUiSlotType SlotType, int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	bool BuildContextActionsForSlot(
		EGridInventoryUiSlotType SlotType, int32 SlotIndex, FGridFacingTargetContext& OutFacingTarget, TArray<FGridItemContextAction>& OutActions);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	bool ExecuteInventoryContextAction(EGridItemActionType ActionType, EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	bool ExecuteInventoryContextActionByIndex(EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex, int32 ActionIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Context Actions")
	void CloseItemActionMenu(FName Reason);

	bool IsItemActionMenuOpen() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Context Actions")
	void OnItemActionMenuCloseRequested(FName Reason);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Reading")
	void CloseItemReadPanel(FName Reason);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Reading")
	void OnItemReadPanelCloseRequested(FName Reason);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Context Actions")
	void PresentItemExamination(const FGridItemInstance& Item, const FText& ExaminationText);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Context Actions")
	void PresentItemReading(const FGridItemInstance& Item, const FText& Title, const FText& ReadText);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Drag")
	bool HandleSlotDrop(EGridInventoryUiSlotType SourceType, int32 SourceIndex, EGridInventoryUiSlotType TargetType, int32 TargetIndex,
		bool bSplitStack = false, int32 RequestedQuantity = 0);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Actions")
	bool HandleInventorySlotClicked(int32 SlotIndex, bool bSplitStack = false);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Actions")
	bool HandleEquipmentSlotClicked(EGridEquipmentSlot EquipmentSlot);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Actions")
	bool HandleMainHandClicked();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Actions")
	bool HandleOffHandClicked();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Actions")
	bool HandleCursorReturnToInventoryClicked();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandlePartyInventoryChanged(int32 CharacterIndex);

	void RegisterBoundPartyMemberWidgets();
	void RefreshSelectedInventoryBagPresentation();
	void EnsureSelectedInventorySlotLayout();

	const URPGClassVisualAsset* FindClassVisualForClass(FName ClassId) const;
	void RefreshSelectedCharacterClassIcon();
	void RemoveGeneratedInventorySlotsFromRegistry();
	void RegisterPaperDollEquipmentSlotWidget(UGridInventorySlotWidget* SlotWidget, EGridEquipmentSlot EquipmentSlot, const TCHAR* WidgetName);
	void RegisterPaperDollEquipmentSlotWidgets();
	UGridInventorySlotWidget* FindRegisteredSlotWidget(EGridInventoryUiSlotType SlotType, int32 SlotIndex) const;
	bool ExecuteResolvedInventoryContextAction(
		const FGridItemContextAction& Action, const FGridFacingTargetContext& FacingTarget, EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex);
	EGridEquipmentSlot ResolveSourceEquipmentSlot(const FGridItemContextAction& Action, EGridInventoryUiSlotType SourceSlotType) const;
	bool DropContextItemToGround(const FGridItemContextAction& Action, EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex);

	UPROPERTY(Transient)
	bool bItemActionMenuCloseRequested = false;

	UPROPERTY(Transient)
	bool bInventorySlotsBuilt = false;

	UPROPERTY(Transient)
	int32 LastBuiltSlotCount = 0;

	UPROPERTY(Transient)
	int32 LastBuiltColumnCount = 0;

	UPROPERTY(Transient)
	TSubclassOf<UGridInventorySlotWidget> LastBuiltSlotWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> LastBuiltGridPanel;

};
