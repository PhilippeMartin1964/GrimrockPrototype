#include "UI/GridInventoryWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "RPG/StatusEffects/GridStatusEffectPresentation.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridInventoryProjectionEntry
	{
		int32 SourceSlotIndex = INDEX_NONE;
		FString DisplayName;
		EGridItemType ItemType = EGridItemType::None;
		float TotalWeight = 0.0f;
	};

	void SetInventoryOptionalText(UTextBlock* TextBlock, const FText& Value)
	{
		if (TextBlock)
		{
			TextBlock->SetText(Value);
		}
	}

	void SetInventoryOptionalProgress(UProgressBar* ProgressBar, float CurrentValue, float MaximumValue)
	{
		if (!ProgressBar)
		{
			return;
		}

		const float Ratio = MaximumValue > 0.0f ? CurrentValue / MaximumValue : 0.0f;
		ProgressBar->SetPercent(FMath::Clamp(Ratio, 0.0f, 1.0f));
	}

	FText FormatInventorySlotUsage(int32 UsedSlots, int32 MaximumSlots)
	{
		return FText::FromString(FString::Printf(TEXT("%d / %d"), UsedSlots, MaximumSlots));
	}

	FText ResolveCharacterDisplayName(const FText& DisplayName, FName Id, const TCHAR* Fallback)
	{
		if (!DisplayName.IsEmpty())
		{
			return DisplayName;
		}
		return Id.IsNone() ? FText::FromString(Fallback) : FText::FromName(Id);
	}

	FText FormatIntWithBonus(int32 FinalValue, int32 Bonus)
	{
		if (Bonus == 0)
		{
			return FText::AsNumber(FinalValue);
		}

		return FText::FromString(FString::Printf(TEXT("%d (%+d)"), FinalValue, Bonus));
	}

	FText FormatFloatWithBonus(float FinalValue, float Bonus, int32 FractionDigits = 1)
	{
		const FString FinalValueText = FString::Printf(TEXT("%.*f"), FractionDigits, FinalValue);
		if (FMath::IsNearlyZero(Bonus))
		{
			return FText::FromString(FinalValueText);
		}

		return FText::FromString(FString::Printf(TEXT("%s (%+.*f)"), *FinalValueText, FractionDigits, Bonus));
	}

	FText FormatCurrentMaxWithMaxBonus(int32 CurrentValue, int32 FinalMaxValue, int32 MaxBonus)
	{
		if (MaxBonus == 0)
		{
			return FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentValue, FinalMaxValue));
		}

		return FText::FromString(FString::Printf(TEXT("%d / %d (%+d)"), CurrentValue, FinalMaxValue, MaxBonus));
	}

	FText FormatInventoryBagOwner(const FGridInventoryCharacterSummary& Summary)
	{
		const FText OwnerName = Summary.DisplayName.IsEmpty()
			? FText::FromString(FString::Printf(TEXT("Hero_%02d"), Summary.CharacterIndex + 1))
			: Summary.DisplayName;
		return FText::Format(NSLOCTEXT("GridInventoryBag", "Owner", "Sac de : {0}"), OwnerName);
	}

	FText FormatInventoryBagWeight(float CurrentWeight, float MaximumWeight)
	{
		return FText::FromString(
			FString::Printf(TEXT("Poids : %.1f / %.1f"), FMath::Max(0.0f, CurrentWeight), FMath::Max(0.0f, MaximumWeight)));
	}

	const TCHAR* GetContextActionName(EGridItemActionType ActionType)
	{
		switch (ActionType)
		{
			case EGridItemActionType::Equip:
				return TEXT("Equip");
			case EGridItemActionType::Unequip:
				return TEXT("Unequip");
			case EGridItemActionType::Consume:
				return TEXT("Consume");
			case EGridItemActionType::Read:
				return TEXT("Read");
			case EGridItemActionType::Examine:
				return TEXT("Examine");
			case EGridItemActionType::Use:
				return TEXT("Use");
			case EGridItemActionType::UseOnTarget:
				return TEXT("UseOnTarget");
			case EGridItemActionType::InsertIntoTarget:
				return TEXT("InsertIntoTarget");
			case EGridItemActionType::PlaceOnTarget:
				return TEXT("PlaceOnTarget");
			case EGridItemActionType::DropToGround:
				return TEXT("DropToGround");
			case EGridItemActionType::Throw:
				return TEXT("Throw");
			case EGridItemActionType::Combine:
				return TEXT("Combine");
			case EGridItemActionType::SplitStack:
				return TEXT("SplitStack");
			case EGridItemActionType::ToggleLight:
				return TEXT("ToggleLight");
			case EGridItemActionType::AddToHotbar:
				return TEXT("AddToHotbar");
			case EGridItemActionType::None:
			default:
				return TEXT("None");
		}
	}

	const TCHAR* GetContextTargetTypeName(EGridFacingTargetType TargetType)
	{
		switch (TargetType)
		{
			case EGridFacingTargetType::WallLock:
				return TEXT("WallLock");
			case EGridFacingTargetType::Receptacle:
				return TEXT("Receptacle");
			case EGridFacingTargetType::TorchHolder:
				return TEXT("TorchHolder");
			case EGridFacingTargetType::Readable:
				return TEXT("Readable");
			case EGridFacingTargetType::Door:
				return TEXT("Door");
			case EGridFacingTargetType::Mechanism:
				return TEXT("Mechanism");
			case EGridFacingTargetType::None:
			default:
				return TEXT("None");
		}
	}

	const TCHAR* GetContextEquipmentSlotName(EGridEquipmentSlot EquipmentSlot)
	{
		switch (EquipmentSlot)
		{
			case EGridEquipmentSlot::MainHand:
				return TEXT("MainHand");
			case EGridEquipmentSlot::OffHand:
				return TEXT("OffHand");
			case EGridEquipmentSlot::Head:
				return TEXT("Head");
			case EGridEquipmentSlot::Chest:
				return TEXT("Chest");
			case EGridEquipmentSlot::Legs:
				return TEXT("Legs");
			case EGridEquipmentSlot::Feet:
				return TEXT("Feet");
			case EGridEquipmentSlot::Amulet:
				return TEXT("Amulet");
			case EGridEquipmentSlot::Ring1:
				return TEXT("Ring1");
			case EGridEquipmentSlot::Ring2:
				return TEXT("Ring2");
			case EGridEquipmentSlot::Shoulders:
				return TEXT("Shoulders");
			case EGridEquipmentSlot::Gloves:
				return TEXT("Gloves");
			case EGridEquipmentSlot::Belt:
				return TEXT("Belt");
			case EGridEquipmentSlot::Cloak:
				return TEXT("Cloak");
			case EGridEquipmentSlot::Talisman:
				return TEXT("Talisman");
			case EGridEquipmentSlot::QuickSlot1:
				return TEXT("QuickSlot1");
			case EGridEquipmentSlot::QuickSlot2:
				return TEXT("QuickSlot2");
			case EGridEquipmentSlot::Shirt:
				return TEXT("Chemise");
			case EGridEquipmentSlot::Bracers:
				return TEXT("Brassards");
			case EGridEquipmentSlot::None:
			default:
				return TEXT("None");
		}
	}

	const TCHAR* GetPaperDollEquipmentSlotName(EGridEquipmentSlot EquipmentSlot)
	{
		switch (EquipmentSlot)
		{
			case EGridEquipmentSlot::MainHand:
				return TEXT("MainHand");
			case EGridEquipmentSlot::OffHand:
				return TEXT("OffHand");
			case EGridEquipmentSlot::Head:
				return TEXT("Head");
			case EGridEquipmentSlot::Chest:
				return TEXT("Chest");
			case EGridEquipmentSlot::Legs:
				return TEXT("Legs");
			case EGridEquipmentSlot::Feet:
				return TEXT("Feet");
			case EGridEquipmentSlot::Amulet:
				return TEXT("Amulet");
			case EGridEquipmentSlot::Ring1:
				return TEXT("Ring1");
			case EGridEquipmentSlot::Ring2:
				return TEXT("Ring2");
			case EGridEquipmentSlot::Shoulders:
				return TEXT("Shoulders");
			case EGridEquipmentSlot::Gloves:
				return TEXT("Gloves");
			case EGridEquipmentSlot::Belt:
				return TEXT("Belt");
			case EGridEquipmentSlot::Cloak:
				return TEXT("Cloak");
			case EGridEquipmentSlot::Talisman:
				return TEXT("Talisman");
			case EGridEquipmentSlot::QuickSlot1:
				return TEXT("QuickSlot1");
			case EGridEquipmentSlot::QuickSlot2:
				return TEXT("QuickSlot2");
			case EGridEquipmentSlot::Shirt:
				return TEXT("Shirt");
			case EGridEquipmentSlot::Bracers:
				return TEXT("Bracers");
			case EGridEquipmentSlot::None:
			default:
				return TEXT("None");
		}
	}

	constexpr EGridEquipmentSlot PaperDollEquipmentSlots[] = { EGridEquipmentSlot::Head, EGridEquipmentSlot::Amulet, EGridEquipmentSlot::Shoulders,
		EGridEquipmentSlot::Shirt, EGridEquipmentSlot::Chest, EGridEquipmentSlot::Cloak, EGridEquipmentSlot::Bracers, EGridEquipmentSlot::Gloves,
		EGridEquipmentSlot::Belt, EGridEquipmentSlot::Legs, EGridEquipmentSlot::Feet, EGridEquipmentSlot::Ring1, EGridEquipmentSlot::Ring2,
		EGridEquipmentSlot::MainHand, EGridEquipmentSlot::OffHand };

	constexpr EGridEquipmentSlot ForbiddenPaperDollEquipmentSlots[] = { EGridEquipmentSlot::Talisman, EGridEquipmentSlot::QuickSlot1,
		EGridEquipmentSlot::QuickSlot2 };

	bool IsGridInventoryHandEquipmentSlot(EGridEquipmentSlot EquipmentSlot)
	{
		return EquipmentSlot == EGridEquipmentSlot::MainHand || EquipmentSlot == EGridEquipmentSlot::OffHand;
	}

	FObjectPropertyBase* FindCurrentItemActionMenuProperty(const UGridInventoryWidget* InventoryWidget)
	{
		if (!InventoryWidget)
		{
			return nullptr;
		}

		FProperty* Property = InventoryWidget->GetClass()->FindPropertyByName(FName(TEXT("CurrentItemActionMenu")));
		return CastField<FObjectPropertyBase>(Property);
	}

	UUserWidget* GetCurrentItemActionMenuWidget(const UGridInventoryWidget* InventoryWidget)
	{
		FObjectPropertyBase* Property = FindCurrentItemActionMenuProperty(InventoryWidget);
		if (!Property || !InventoryWidget)
		{
			return nullptr;
		}

		return Cast<UUserWidget>(Property->GetObjectPropertyValue_InContainer(InventoryWidget));
	}

	void ClearCurrentItemActionMenuWidget(UGridInventoryWidget* InventoryWidget)
	{
		FObjectPropertyBase* Property = FindCurrentItemActionMenuProperty(InventoryWidget);
		if (Property && InventoryWidget)
		{
			Property->SetObjectPropertyValue_InContainer(InventoryWidget, nullptr);
		}
	}

	bool IsItemActionMenuDetached(const UUserWidget* ItemActionMenu)
	{
		return ItemActionMenu && !ItemActionMenu->IsInViewport() && !ItemActionMenu->GetParent();
	}

	EGridEquipmentSlot ResolveUiEquipmentSlot(EGridInventoryUiSlotType SlotType, int32 SlotIndex)
	{
		switch (SlotType)
		{
			case EGridInventoryUiSlotType::Equipment:
				return static_cast<EGridEquipmentSlot>(SlotIndex);
			case EGridInventoryUiSlotType::MainHand:
				return EGridEquipmentSlot::MainHand;
			case EGridInventoryUiSlotType::OffHand:
				return EGridEquipmentSlot::OffHand;
			default:
				return EGridEquipmentSlot::None;
		}
	}

}

void UGridInventoryWidget::InitializeInventoryWidget(AGrimrockPartyPawn* InPartyPawn)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridInventoryWidget::HandlePartyInventoryChanged);
	}

	OwningPartyPawn = InPartyPawn;
	InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;

	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.AddUniqueDynamic(this, &UGridInventoryWidget::HandlePartyInventoryChanged);
	}

	RefreshInventory();
}

void UGridInventoryWidget::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	if (InventoryComponent && CharacterIndex != INDEX_NONE && CharacterIndex != InventoryComponent->GetSelectedCharacterIndex())
	{
		RefreshRegisteredPartyMemberWidgets();
		return;
	}

	RefreshInventory();
}

void UGridInventoryWidget::RefreshInventory()
{
	UE_LOG(LogTemp, Verbose, TEXT("GridInventory UI Refresh Pawn=%s InventoryComponent=%s"), *GetNameSafe(OwningPartyPawn), *GetNameSafe(InventoryComponent));
	RefreshSelectedCharacterDetails();
	RefreshRegisteredPartyMemberWidgets();
	RefreshSelectedInventoryBagPresentation();
	EnsureSelectedInventorySlotLayout();
	RefreshRegisteredSlotWidgets();
}

void UGridInventoryWidget::RefreshSelectedInventoryBagPresentation()
{
	FGridInventoryCharacterSummary Summary;
	if (!InventoryComponent || !InventoryComponent->GetCharacterSummary(InventoryComponent->GetSelectedCharacterIndex(), Summary))
	{
		SetInventoryOptionalText(Text_InventoryBagOwner, FText::GetEmpty());
		SetInventoryOptionalText(Text_InventoryBagWeight, FText::GetEmpty());
		if (Text_InventoryEmptyState)
		{
			Text_InventoryEmptyState->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	const int32 VisibleItemCount = GetVisibleInventoryItemCount();
	SetInventoryOptionalText(Text_InventoryBagOwner, FormatInventoryBagOwner(Summary));
	SetInventoryOptionalText(Text_InventoryBagWeight, FormatInventoryBagWeight(Summary.CurrentWeight, Summary.MaxWeight));
	if (Text_InventoryEmptyState)
	{
		Text_InventoryEmptyState->SetVisibility(VisibleItemCount == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGridInventoryWidget::EnsureSelectedInventorySlotLayout()
{
	if (!InventorySlotsGridPanel || !InventorySlotWidgetClass)
	{
		return;
	}

	// One shared bag grid. RebuildInventorySlotWidgets() already skips work when
	// count/columns/class/panel did not change.
	RebuildInventorySlotWidgets();
}

int32 UGridInventoryWidget::GetSelectedCharacterIndex() const
{
	return InventoryComponent ? InventoryComponent->GetSelectedCharacterIndex() : INDEX_NONE;
}

int32 UGridInventoryWidget::GetInventorySlotCount() const
{
	if (!InventoryComponent)
	{
		return 0;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
	return State.ActiveCharacters.IsValidIndex(CharacterIndex) ? State.ActiveCharacters[CharacterIndex].InventorySlots.Num() : 0;
}

bool UGridInventoryWidget::GetInventoryItemAtSlot(int32 SlotIndex, FGridItemInstance& OutItem) const
{
	OutItem = FGridItemInstance();
	if (!InventoryComponent)
	{
		return false;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
	if (!State.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& CharacterState = State.ActiveCharacters[CharacterIndex];
	if (!CharacterState.InventorySlots.IsValidIndex(SlotIndex) || CharacterState.InventorySlots[SlotIndex].IsEmpty())
	{
		return false;
	}

	OutItem = CharacterState.InventorySlots[SlotIndex].Item;
	return true;
}

bool UGridInventoryWidget::GetMainHandItem(FGridItemInstance& OutItem) const
{
	OutItem = FGridItemInstance();
	if (!InventoryComponent)
	{
		return false;
	}
	return InventoryComponent->GetEquippedItem(InventoryComponent->GetSelectedCharacterIndex(), EGridEquipmentSlot::MainHand, OutItem);
}

bool UGridInventoryWidget::GetOffHandItem(FGridItemInstance& OutItem) const
{
	OutItem = FGridItemInstance();
	if (!InventoryComponent)
	{
		return false;
	}
	return InventoryComponent->GetEquippedItem(InventoryComponent->GetSelectedCharacterIndex(), EGridEquipmentSlot::OffHand, OutItem);
}

bool UGridInventoryWidget::GetEquipmentItem(EGridEquipmentSlot EquipmentSlot, FGridItemInstance& OutItem) const
{
	OutItem = FGridItemInstance();
	return InventoryComponent && EquipmentSlot != EGridEquipmentSlot::None &&
		InventoryComponent->GetEquippedItem(InventoryComponent->GetSelectedCharacterIndex(), EquipmentSlot, OutItem);
}

bool UGridInventoryWidget::GetCursorItem(FGridItemInstance& OutItem) const
{
	OutItem = FGridItemInstance();
	if (!InventoryComponent || !InventoryComponent->HasCursorItem())
	{
		return false;
	}

	OutItem = InventoryComponent->GetCursorItem();
	return true;
}

bool UGridInventoryWidget::HasCursorItem() const
{
	return InventoryComponent && InventoryComponent->HasCursorItem();
}

int32 UGridInventoryWidget::GetActiveCharacterCount() const
{
	return InventoryComponent ? InventoryComponent->GetActiveCharacterCount() : 0;
}

int32 UGridInventoryWidget::GetMaxActiveCharacterCount() const
{
	return InventoryComponent ? InventoryComponent->GetMaxActiveCharacterCount() : 0;
}

bool UGridInventoryWidget::GetCharacterSummary(int32 CharacterIndex, FGridInventoryCharacterSummary& OutSummary) const
{
	OutSummary = FGridInventoryCharacterSummary();
	return InventoryComponent && InventoryComponent->GetCharacterSummary(CharacterIndex, OutSummary);
}

bool UGridInventoryWidget::SelectCharacter(int32 CharacterIndex)
{
	const bool bResult = InventoryComponent && InventoryComponent->SetSelectedCharacterIndex(CharacterIndex);
	UE_LOG(LogTemp, Log, TEXT("GridInventory UI SelectCharacter Index=%d Result=%s"), CharacterIndex, bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}

void UGridInventoryWidget::RegisterBoundPartyMemberWidgets()
{
	RegisterPartyMemberWidget(PartyMember_1, 0);
	RegisterPartyMemberWidget(PartyMember_2, 1);
	RegisterPartyMemberWidget(PartyMember_3, 2);
	RegisterPartyMemberWidget(PartyMember_4, 3);
	RegisterPartyMemberWidget(PartyMember_5, 4);
	RegisterPartyMemberWidget(PartyMember_6, 5);
}

void UGridInventoryWidget::RegisterPartyMemberWidget(UGridPartyMemberWidget* MemberWidget, int32 CharacterIndex)
{
	if (!MemberWidget)
	{
		return;
	}

	MemberWidget->InitializePartyMember(CharacterIndex);
	MemberWidget->SetOwnerInventoryWidget(this);
	MemberWidget->OnPartyMemberClicked.RemoveDynamic(this, &UGridInventoryWidget::HandleRegisteredPartyMemberClicked);
	MemberWidget->OnPartyMemberClicked.AddDynamic(this, &UGridInventoryWidget::HandleRegisteredPartyMemberClicked);
	RegisteredPartyMemberWidgets.AddUnique(MemberWidget);
	RefreshRegisteredPartyMemberWidgets();
}

void UGridInventoryWidget::RefreshRegisteredPartyMemberWidgets()
{
	for (UGridPartyMemberWidget* MemberWidget : RegisteredPartyMemberWidgets)
	{
		if (!MemberWidget)
		{
			continue;
		}

		FGridInventoryCharacterSummary Summary;
		if (GetCharacterSummary(MemberWidget->CharacterIndex, Summary))
		{
			MemberWidget->SetVisibility(ESlateVisibility::Visible);
			MemberWidget->SetCharacterSummary(Summary);

			TArray<FGridStatusEffectPresentationView> StatusViews;
			if (InventoryComponent &&
				InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(MemberWidget->CharacterIndex))
			{
				const FGridCharacterInventoryState& Character =
					InventoryComponent->PartyInventoryState.ActiveCharacters[MemberWidget->CharacterIndex];
				FGridStatusEffectPresentationBuilder::Build(Character.StatusEffects, StatusViews);
			}
			MemberWidget->SetStatusEffects(StatusViews);
		}
		else
		{
			MemberWidget->SetStatusEffects(TArray<FGridStatusEffectPresentationView>());
			MemberWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UGridInventoryWidget::HandleRegisteredPartyMemberClicked(int32 CharacterIndex)
{
	SelectCharacter(CharacterIndex);
}

bool UGridInventoryWidget::HandlePartyMemberItemDrop(UGridInventoryDragDropOperation* Operation, int32 TargetCharacterIndex)
{
	if (!InventoryComponent || !Operation || !Operation->bHasItem || Operation->SourceSlotType != EGridInventoryUiSlotType::Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory PartyDrop Failed Reason=InvalidOperation"));
		return false;
	}

	const int32 SourceCharacterIndex = Operation->SourceCharacterIndex;
	if (!InventoryComponent->IsValidCharacterIndex(SourceCharacterIndex) || !InventoryComponent->IsValidCharacterIndex(TargetCharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory PartyDrop Failed Reason=InvalidCharacters Source=%d Target=%d"), SourceCharacterIndex, TargetCharacterIndex);
		return false;
	}
	if (SourceCharacterIndex == TargetCharacterIndex)
	{
		UE_LOG(LogTemp, Verbose, TEXT("GridInventory PartyDrop Ignored Reason=SameCharacter Character=%d"), SourceCharacterIndex);
		return false;
	}

	const FGridPartyInventoryState& PartyState = InventoryComponent->PartyInventoryState;
	if (!PartyState.ActiveCharacters.IsValidIndex(SourceCharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& SourceCharacter = PartyState.ActiveCharacters[SourceCharacterIndex];
	if (!SourceCharacter.InventorySlots.IsValidIndex(Operation->SourceSlotIndex) || SourceCharacter.InventorySlots[Operation->SourceSlotIndex].IsEmpty())
	{
		UE_LOG(LogTemp, Verbose, TEXT("GridInventory PartyDrop Rejected Reason=SourceSlotChanged Source=%d Slot=%d"),
			SourceCharacterIndex, Operation->SourceSlotIndex);
		return false;
	}

	const FGridItemInstance& CurrentSourceItem = SourceCharacter.InventorySlots[Operation->SourceSlotIndex].Item;
	if (CurrentSourceItem.RuntimeObjectId != Operation->SourceRuntimeObjectId || CurrentSourceItem.ItemDefinitionId != Operation->SourceItemDefinitionId)
	{
		UE_LOG(LogTemp, Verbose, TEXT("GridInventory PartyDrop Rejected Reason=SourceIdentityChanged Source=%d Slot=%d"), SourceCharacterIndex,
			Operation->SourceSlotIndex);
		return false;
	}

	const int32 TransferQuantity = FMath::Max(1, CurrentSourceItem.Quantity);
	const FGridItemTransferResult TransferResult =
		UGridItemTransferService::TransferInventorySlotToCharacter(InventoryComponent, SourceCharacterIndex, Operation->SourceSlotIndex, TargetCharacterIndex, 0);

	if (TransferResult.bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory PartyDrop Result=true Source=%d Target=%d Slot=%d Item=%s Quantity=%d Message=%s"),
			SourceCharacterIndex, TargetCharacterIndex, Operation->SourceSlotIndex, *Operation->SourceItemDefinitionId.ToString(), TransferQuantity,
			*TransferResult.Message.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory PartyDrop Result=false Source=%d Target=%d Slot=%d Item=%s Quantity=%d Message=%s"),
			SourceCharacterIndex, TargetCharacterIndex, Operation->SourceSlotIndex, *Operation->SourceItemDefinitionId.ToString(), TransferQuantity,
			*TransferResult.Message.ToString());
	}

	if (TransferResult.bSuccess)
	{
		FString OwnershipError;
		if (!InventoryComponent->ValidateInventoryOwnership(OwnershipError))
		{
			UE_LOG(LogTemp, Error, TEXT("GridInventory PartyDrop Ownership Failed Error=%s"), *OwnershipError);
		}
	}

	// Keep the current selection unchanged: dropping on a portrait transfers the
	// item, it does not navigate to the target character. Inventory notifications
	// refresh the affected UI surfaces.
	return TransferResult.bSuccess;
}

void UGridInventoryWidget::RefreshSelectedCharacterDetails()
{
	FGridInventoryCharacterSummary Summary;
	if (!GetCharacterSummary(GetSelectedCharacterIndex(), Summary))
	{
		SetInventoryOptionalText(Text_CharacterName, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterRace, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterClass, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterLevel, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterExperience, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterStrength, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterDexterity, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterConstitution, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterIntelligence, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterWisdom, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterCharisma, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterHealth, FText::GetEmpty());
		SetInventoryOptionalProgress(ProgressBar_CharacterHealth, 0.0f, 0.0f);
		SetInventoryOptionalText(Text_CharacterMana, FText::GetEmpty());
		SetInventoryOptionalProgress(ProgressBar_CharacterMana, 0.0f, 0.0f);
		SetInventoryOptionalText(Text_CharacterInventorySlots, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterPhysicalArmor, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterMagicalArmor, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterInitiative, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterAccuracy, FText::GetEmpty());
		SetInventoryOptionalText(Text_CharacterEvasion, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistancePhysical, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistanceFire, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistanceIce, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistanceLightning, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistancePoison, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistanceHoly, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistanceNecrotic, FText::GetEmpty());
		SetInventoryOptionalText(Text_ResistanceArcane, FText::GetEmpty());
		if (Image_CharacterPortrait)
		{
			Image_CharacterPortrait->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	SetInventoryOptionalText(Text_CharacterName, Summary.DisplayName);
	SetInventoryOptionalText(Text_CharacterRace, ResolveCharacterDisplayName(Summary.RaceDisplayName, Summary.RaceId, TEXT("Race inconnue")));
	SetInventoryOptionalText(Text_CharacterClass, ResolveCharacterDisplayName(Summary.ClassDisplayName, Summary.ClassId, TEXT("Classe inconnue")));
	SetInventoryOptionalText(Text_CharacterLevel, FText::AsNumber(Summary.Level));
	SetInventoryOptionalText(Text_CharacterExperience, FText::AsNumber(Summary.Experience));
	SetInventoryOptionalText(Text_CharacterStrength, FormatIntWithBonus(Summary.Attributes.Strength, Summary.EquipmentStatBonus.StrengthBonus));
	SetInventoryOptionalText(Text_CharacterDexterity, FormatIntWithBonus(Summary.Attributes.Dexterity, Summary.EquipmentStatBonus.DexterityBonus));
	SetInventoryOptionalText(Text_CharacterConstitution, FormatIntWithBonus(Summary.Attributes.Constitution, Summary.EquipmentStatBonus.ConstitutionBonus));
	SetInventoryOptionalText(Text_CharacterIntelligence, FormatIntWithBonus(Summary.Attributes.Intelligence, Summary.EquipmentStatBonus.IntelligenceBonus));
	SetInventoryOptionalText(Text_CharacterWisdom, FormatIntWithBonus(Summary.Attributes.Wisdom, Summary.EquipmentStatBonus.WisdomBonus));
	SetInventoryOptionalText(Text_CharacterCharisma, FormatIntWithBonus(Summary.Attributes.Charisma, Summary.EquipmentStatBonus.CharismaBonus));
	SetInventoryOptionalText(Text_CharacterHealth,
		FormatCurrentMaxWithMaxBonus(Summary.Resources.CurrentHealth, Summary.DerivedStats.MaxHealth, Summary.EquipmentStatBonus.MaxHealthBonus));
	SetInventoryOptionalProgress(
		ProgressBar_CharacterHealth, static_cast<float>(Summary.Resources.CurrentHealth), static_cast<float>(Summary.DerivedStats.MaxHealth));
	SetInventoryOptionalText(
		Text_CharacterMana, FormatCurrentMaxWithMaxBonus(Summary.Resources.CurrentMana, Summary.DerivedStats.MaxMana, Summary.EquipmentStatBonus.MaxManaBonus));
	SetInventoryOptionalProgress(
		ProgressBar_CharacterMana, static_cast<float>(Summary.Resources.CurrentMana), static_cast<float>(Summary.DerivedStats.MaxMana));
	SetInventoryOptionalText(Text_CharacterInventorySlots, FormatInventorySlotUsage(Summary.UsedInventorySlots, Summary.MaxInventorySlots));

	const FText PhysicalArmorText = FormatIntWithBonus(Summary.Resources.CurrentPhysicalArmor, Summary.EquipmentStatBonus.ArmorBonus);
	SetInventoryOptionalText(Text_CharacterPhysicalArmor, PhysicalArmorText);
	SetInventoryOptionalText(Text_CharacterMagicalArmor, FText::AsNumber(Summary.Resources.CurrentMagicalArmor));
	SetInventoryOptionalText(Text_CharacterInitiative, FText::AsNumber(Summary.DerivedStats.Initiative));
	SetInventoryOptionalText(Text_CharacterAccuracy, FText::AsNumber(Summary.DerivedStats.Accuracy));
	SetInventoryOptionalText(Text_CharacterEvasion, FText::AsNumber(Summary.DerivedStats.Evasion));
	SetInventoryOptionalText(Text_ResistancePhysical, FText::AsNumber(Summary.FinalResistances.PhysicalResistance));
	SetInventoryOptionalText(Text_ResistanceFire, FText::AsNumber(Summary.FinalResistances.FireResistance));
	SetInventoryOptionalText(Text_ResistanceIce, FText::AsNumber(Summary.FinalResistances.IceResistance));
	SetInventoryOptionalText(Text_ResistanceLightning, FText::AsNumber(Summary.FinalResistances.LightningResistance));
	SetInventoryOptionalText(Text_ResistancePoison, FText::AsNumber(Summary.FinalResistances.PoisonResistance));
	SetInventoryOptionalText(Text_ResistanceHoly, FText::AsNumber(Summary.FinalResistances.HolyResistance));
	SetInventoryOptionalText(Text_ResistanceNecrotic, FText::AsNumber(Summary.FinalResistances.NecroticResistance));
	SetInventoryOptionalText(Text_ResistanceArcane, FText::AsNumber(Summary.FinalResistances.ArcaneResistance));

	if (Image_CharacterPortrait)
	{
		if (Summary.Portrait.IsNull())
		{
			Image_CharacterPortrait->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_CharacterPortrait->SetBrushFromSoftTexture(Summary.Portrait, false);
			Image_CharacterPortrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UGridInventoryWidget::RegisterInventorySlotWidget(UGridInventorySlotWidget* SlotWidget, EGridInventoryUiSlotType SlotType, int32 SlotIndex)
{
	if (!SlotWidget)
	{
		return;
	}

	SlotWidget->InitializeInventorySlot(SlotType, SlotIndex);
	SlotWidget->SetOwnerInventoryWidget(this);
	SlotWidget->OnSlotClicked.RemoveDynamic(this, &UGridInventoryWidget::HandleRegisteredSlotClicked);
	SlotWidget->OnSlotClicked.AddDynamic(this, &UGridInventoryWidget::HandleRegisteredSlotClicked);

	switch (SlotType)
	{
		case EGridInventoryUiSlotType::Inventory:
			RegisteredInventorySlots.AddUnique(SlotWidget);
			break;
		case EGridInventoryUiSlotType::Equipment:
		{
			const EGridEquipmentSlot EquipmentSlot = static_cast<EGridEquipmentSlot>(SlotIndex);
			SlotWidget->InitializeEquipmentSlot(EquipmentSlot);
			if (EquipmentSlot != EGridEquipmentSlot::None)
			{
				RegisteredEquipmentSlotWidgets.FindOrAdd(EquipmentSlot) = SlotWidget;
			}
			if (EquipmentSlot == EGridEquipmentSlot::MainHand)
			{
				MainHandSlotWidget = SlotWidget;
			}
			else if (EquipmentSlot == EGridEquipmentSlot::OffHand)
			{
				OffHandSlotWidget = SlotWidget;
			}
			break;
		}
		case EGridInventoryUiSlotType::MainHand:
			MainHandSlotWidget = SlotWidget;
			break;
		case EGridInventoryUiSlotType::OffHand:
			OffHandSlotWidget = SlotWidget;
			break;
		case EGridInventoryUiSlotType::Cursor:
			CursorSlotWidget = SlotWidget;
			break;
		default:
			break;
	}

	RefreshRegisteredSlotWidget(SlotWidget);
}

void UGridInventoryWidget::RegisterEquipmentSlotWidget(UGridInventorySlotWidget* SlotWidget, EGridEquipmentSlot EquipmentSlot)
{
	if (!SlotWidget || EquipmentSlot == EGridEquipmentSlot::None)
	{
		return;
	}

	SlotWidget->SetOwnerInventoryWidget(this);
	SlotWidget->InitializeEquipmentSlot(EquipmentSlot);
	SlotWidget->OnSlotClicked.RemoveDynamic(this, &UGridInventoryWidget::HandleRegisteredSlotClicked);
	SlotWidget->OnSlotClicked.AddDynamic(this, &UGridInventoryWidget::HandleRegisteredSlotClicked);
	RegisteredEquipmentSlotWidgets.FindOrAdd(EquipmentSlot) = SlotWidget;

	if (EquipmentSlot == EGridEquipmentSlot::MainHand)
	{
		MainHandSlotWidget = SlotWidget;
	}
	else if (EquipmentSlot == EGridEquipmentSlot::OffHand)
	{
		OffHandSlotWidget = SlotWidget;
	}

	RefreshRegisteredSlotWidget(SlotWidget);
}

void UGridInventoryWidget::RegisterPaperDollEquipmentSlotWidget(UGridInventorySlotWidget* SlotWidget, EGridEquipmentSlot EquipmentSlot, const TCHAR* WidgetName)
{
	if (!SlotWidget)
	{
		UE_LOG(
			LogTemp, Warning, TEXT("GridInventory PaperDoll SlotMissing Widget=%s EquipmentSlot=%s"), WidgetName, GetPaperDollEquipmentSlotName(EquipmentSlot));
		return;
	}

	RegisterEquipmentSlotWidget(SlotWidget, EquipmentSlot);

	bool bValidRegistration = true;
	if (SlotWidget->SlotType != EGridInventoryUiSlotType::Equipment)
	{
		bValidRegistration = false;
	}
	if (SlotWidget->EquipmentSlot != EquipmentSlot)
	{
		bValidRegistration = false;
	}
	if (SlotWidget->InventorySlotIndex != static_cast<int32>(EquipmentSlot))
	{
		bValidRegistration = false;
	}

	if (!bValidRegistration)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("GridInventory PaperDoll SlotInvalid Widget=%s EquipmentSlot=%s SlotType=%s WidgetEquipmentSlot=%s SlotIndex=%d ExpectedSlotIndex=%d"),
			WidgetName, GetPaperDollEquipmentSlotName(EquipmentSlot), GetGridInventoryUiSlotTypeName(SlotWidget->SlotType),
			GetPaperDollEquipmentSlotName(SlotWidget->EquipmentSlot), SlotWidget->InventorySlotIndex, static_cast<int32>(EquipmentSlot));
	}
}

void UGridInventoryWidget::RegisterPaperDollEquipmentSlotWidgets()
{
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Head, EGridEquipmentSlot::Head, TEXT("SlotWidget_Head"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Amulet, EGridEquipmentSlot::Amulet, TEXT("SlotWidget_Amulet"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Shoulders, EGridEquipmentSlot::Shoulders, TEXT("SlotWidget_Shoulders"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Shirt, EGridEquipmentSlot::Shirt, TEXT("SlotWidget_Shirt"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Chest, EGridEquipmentSlot::Chest, TEXT("SlotWidget_Chest"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Cloak, EGridEquipmentSlot::Cloak, TEXT("SlotWidget_Cloak"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Bracers, EGridEquipmentSlot::Bracers, TEXT("SlotWidget_Bracers"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Gloves, EGridEquipmentSlot::Gloves, TEXT("SlotWidget_Gloves"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Belt, EGridEquipmentSlot::Belt, TEXT("SlotWidget_Belt"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Legs, EGridEquipmentSlot::Legs, TEXT("SlotWidget_Legs"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Feet, EGridEquipmentSlot::Feet, TEXT("SlotWidget_Feet"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Ring1, EGridEquipmentSlot::Ring1, TEXT("SlotWidget_Ring1"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_Ring2, EGridEquipmentSlot::Ring2, TEXT("SlotWidget_Ring2"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_MainHand, EGridEquipmentSlot::MainHand, TEXT("SlotWidget_MainHand"));
	RegisterPaperDollEquipmentSlotWidget(SlotWidget_OffHand, EGridEquipmentSlot::OffHand, TEXT("SlotWidget_OffHand"));
}

bool UGridInventoryWidget::ValidatePaperDollEquipmentRegistration() const
{
	bool bIsValid = true;
	int32 RegisteredPaperDollSlotCount = 0;

	for (const EGridEquipmentSlot EquipmentSlot : PaperDollEquipmentSlots)
	{
		const TObjectPtr<UGridInventorySlotWidget>* SlotWidgetPtr = RegisteredEquipmentSlotWidgets.Find(EquipmentSlot);
		if (!SlotWidgetPtr)
		{
			bIsValid = false;
			UE_LOG(LogTemp, Warning, TEXT("GridInventory PaperDoll Validation Missing EquipmentSlot=%s"), GetPaperDollEquipmentSlotName(EquipmentSlot));
			continue;
		}

		if (!SlotWidgetPtr->Get())
		{
			bIsValid = false;
			UE_LOG(LogTemp, Warning, TEXT("GridInventory PaperDoll Validation NullWidget EquipmentSlot=%s"), GetPaperDollEquipmentSlotName(EquipmentSlot));
			continue;
		}

		++RegisteredPaperDollSlotCount;
	}

	for (const EGridEquipmentSlot EquipmentSlot : ForbiddenPaperDollEquipmentSlots)
	{
		if (RegisteredEquipmentSlotWidgets.Contains(EquipmentSlot))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory PaperDoll Validation Forbidden EquipmentSlot=%s"), GetPaperDollEquipmentSlotName(EquipmentSlot));
		}
	}

	if (bIsValid)
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory PaperDoll Validation OK Registered=%d"), RegisteredPaperDollSlotCount);
	}

	return bIsValid;
}

void UGridInventoryWidget::SetInventoryFilterCategory(EGridInventoryFilterCategory InFilterCategory)
{
	if (InventoryFilterCategory == InFilterCategory)
	{
		return;
	}

	InventoryFilterCategory = InFilterCategory;
	if (InventorySlotsGridPanel && InventorySlotWidgetClass)
	{
		RebuildInventorySlotWidgets();
	}
	RefreshSelectedInventoryBagPresentation();
	HandleInventoryFilterCategoryChanged();
}

void UGridInventoryWidget::HandleInventoryFilterCategoryChanged()
{
}

void UGridInventoryWidget::SetInventorySortMode(EGridInventorySortMode InSortMode)
{
	if (InventorySortMode == InSortMode)
	{
		return;
	}

	InventorySortMode = InSortMode;
	if (InventorySlotsGridPanel && InventorySlotWidgetClass)
	{
		RebuildInventorySlotWidgets();
	}
	RefreshSelectedInventoryBagPresentation();
	HandleInventorySortModeChanged();
}


void UGridInventoryWidget::HandleInventorySortModeChanged()
{
}

int32 UGridInventoryWidget::ResolveInventorySourceSlotCapacity() const
{
	return GetInventorySlotCount();
}

void UGridInventoryWidget::BuildInventoryProjectionSourceSlotIndices(TArray<int32>& OutSourceSlotIndices) const
{
	OutSourceSlotIndices.Reset();
	const int32 SourceSlotCapacity = FMath::Max(0, ResolveInventorySourceSlotCapacity());

	TArray<FGridInventoryProjectionEntry> Entries;
	Entries.Reserve(SourceSlotCapacity);

	if (InventoryComponent)
	{
		const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
		const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
		if (State.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			const FGridCharacterInventoryState& Character = State.ActiveCharacters[CharacterIndex];
			const int32 SlotLimit = FMath::Min(SourceSlotCapacity, Character.InventorySlots.Num());
			for (int32 SlotIndex = 0; SlotIndex < SlotLimit; ++SlotIndex)
			{
				const FGridInventorySlot& InventorySlot = Character.InventorySlots[SlotIndex];
				if (InventorySlot.IsEmpty())
				{
					continue;
				}

				const FGridItemInstance& Item = InventorySlot.Item;
				const UGridItemDefinitionAsset* Definition = InventoryComponent->FindItemDefinition(Item.ItemDefinitionId);
				const EGridItemType ItemType = Definition ? Definition->ItemType : EGridItemType::None;
				if (!DoesGridItemTypeMatchInventoryFilter(ItemType, InventoryFilterCategory))
				{
					continue;
				}

				FGridInventoryProjectionEntry& Entry = Entries.AddDefaulted_GetRef();
				Entry.SourceSlotIndex = SlotIndex;
				Entry.DisplayName = Definition && !Definition->DisplayName.IsEmpty()
					? Definition->DisplayName.ToString()
					: (!Item.DisplayName.IsEmpty() ? Item.DisplayName.ToString() : Item.ItemDefinitionId.ToString());
				Entry.ItemType = ItemType;
				const float UnitWeight = Definition ? Definition->Weight : Item.Weight;
				Entry.TotalWeight = FMath::Max(0.0f, UnitWeight) * static_cast<float>(FMath::Max(1, Item.Quantity));
			}
		}
	}

	Entries.Sort(
		[this](const FGridInventoryProjectionEntry& Left, const FGridInventoryProjectionEntry& Right)
		{
			const int32 NameComparison = Left.DisplayName.Compare(Right.DisplayName, ESearchCase::IgnoreCase);

			switch (InventorySortMode)
			{
				case EGridInventorySortMode::NameDescending:
					return NameComparison != 0 ? NameComparison > 0 : Left.SourceSlotIndex < Right.SourceSlotIndex;

				case EGridInventorySortMode::TypeAscending:
					if (Left.ItemType != Right.ItemType)
					{
						return static_cast<uint8>(Left.ItemType) < static_cast<uint8>(Right.ItemType);
					}
					return NameComparison != 0 ? NameComparison < 0 : Left.SourceSlotIndex < Right.SourceSlotIndex;

				case EGridInventorySortMode::TypeDescending:
					if (Left.ItemType != Right.ItemType)
					{
						return static_cast<uint8>(Left.ItemType) > static_cast<uint8>(Right.ItemType);
					}
					return NameComparison != 0 ? NameComparison < 0 : Left.SourceSlotIndex < Right.SourceSlotIndex;

				case EGridInventorySortMode::WeightAscending:
					if (!FMath::IsNearlyEqual(Left.TotalWeight, Right.TotalWeight))
					{
						return Left.TotalWeight < Right.TotalWeight;
					}
					return NameComparison != 0 ? NameComparison < 0 : Left.SourceSlotIndex < Right.SourceSlotIndex;

				case EGridInventorySortMode::WeightDescending:
					if (!FMath::IsNearlyEqual(Left.TotalWeight, Right.TotalWeight))
					{
						return Left.TotalWeight > Right.TotalWeight;
					}
					return NameComparison != 0 ? NameComparison < 0 : Left.SourceSlotIndex < Right.SourceSlotIndex;

				case EGridInventorySortMode::NameAscending:
				default:
					return NameComparison != 0 ? NameComparison < 0 : Left.SourceSlotIndex < Right.SourceSlotIndex;
			}
		});

	OutSourceSlotIndices.Reserve(SourceSlotCapacity);
	for (const FGridInventoryProjectionEntry& Entry : Entries)
	{
		OutSourceSlotIndices.Add(Entry.SourceSlotIndex);
	}
	OutSourceSlotIndices.SetNum(SourceSlotCapacity);
	for (int32 Index = Entries.Num(); Index < SourceSlotCapacity; ++Index)
	{
		OutSourceSlotIndices[Index] = INDEX_NONE;
	}
}

void UGridInventoryWidget::GetInventoryProjectionSourceSlotIndices(TArray<int32>& OutSourceSlotIndices) const
{
	BuildInventoryProjectionSourceSlotIndices(OutSourceSlotIndices);
}

int32 UGridInventoryWidget::GetVisibleInventoryItemCount() const
{
	if (!InventoryComponent)
	{
		return 0;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
	if (!State.ActiveCharacters.IsValidIndex(CharacterIndex))
	{
		return 0;
	}

	int32 VisibleItems = 0;
	for (const FGridInventorySlot& InventorySlot : State.ActiveCharacters[CharacterIndex].InventorySlots)
	{
		if (InventorySlot.IsEmpty())
		{
			continue;
		}

		const UGridItemDefinitionAsset* Definition = InventoryComponent->FindItemDefinition(InventorySlot.Item.ItemDefinitionId);
		const EGridItemType ItemType = Definition ? Definition->ItemType : EGridItemType::None;
		if (DoesGridItemTypeMatchInventoryFilter(ItemType, InventoryFilterCategory))
		{
			++VisibleItems;
		}
	}
	return VisibleItems;
}

void UGridInventoryWidget::RebuildInventorySlotWidgets()
{
	if (!InventorySlotsGridPanel)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI RebuildSlots Failed Reason=NoGridPanel"));
		return;
	}

	if (!InventorySlotWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI RebuildSlots Failed Reason=NoSlotWidgetClass"));
		return;
	}

	TArray<int32> SourceSlotIndices;
	BuildInventoryProjectionSourceSlotIndices(SourceSlotIndices);
	const int32 SlotCount = SourceSlotIndices.Num();
	const int32 ColumnCount = FMath::Max(1, InventoryComponent ? InventoryComponent->InventoryColumnCount : 1);

	const bool bTopologyMatches = bInventorySlotsBuilt && LastBuiltSlotCount == SlotCount && LastBuiltColumnCount == ColumnCount &&
		LastBuiltSlotWidgetClass == InventorySlotWidgetClass && LastBuiltGridPanel == InventorySlotsGridPanel &&
		GeneratedInventorySlotWidgets.Num() == SlotCount;

	if (bTopologyMatches)
	{
		ApplyInventoryProjectionToGeneratedSlots(SourceSlotIndices);
		UE_LOG(LogTemp, Verbose, TEXT("GridInventory UI Projection UpdatedInPlace Count=%d Columns=%d"), SlotCount, ColumnCount);
		return;
	}

	ClearGeneratedInventorySlotWidgets();

	for (int32 DisplayIndex = 0; DisplayIndex < SlotCount; ++DisplayIndex)
	{
		const int32 SourceSlotIndex = SourceSlotIndices[DisplayIndex];
		UGridInventorySlotWidget* NewSlot = CreateWidget<UGridInventorySlotWidget>(this, InventorySlotWidgetClass);
		if (!NewSlot)
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory UI RebuildSlots Failed Reason=CreateWidgetFailed DisplayIndex=%d SourceSlot=%d"),
				DisplayIndex, SourceSlotIndex);
			continue;
		}

		NewSlot->SetOwnerInventoryWidget(this);
		RegisterInventorySlotWidget(NewSlot, EGridInventoryUiSlotType::Inventory, SourceSlotIndex);

		const int32 Row = DisplayIndex / ColumnCount;
		const int32 Column = DisplayIndex % ColumnCount;
		if (UUniformGridSlot* GridSlot = InventorySlotsGridPanel->AddChildToUniformGrid(NewSlot, Row, Column))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Left);
			GridSlot->SetVerticalAlignment(VAlign_Top);
		}

		GeneratedInventorySlotWidgets.Add(NewSlot);
	}

	bInventorySlotsBuilt = true;
	LastBuiltSlotCount = SlotCount;
	LastBuiltColumnCount = ColumnCount;
	LastBuiltSlotWidgetClass = InventorySlotWidgetClass;
	LastBuiltGridPanel = InventorySlotsGridPanel;
	UE_LOG(LogTemp, Log, TEXT("GridInventory UI RebuildSlots Count=%d Columns=%d"), SlotCount, ColumnCount);
}

void UGridInventoryWidget::ApplyInventoryProjectionToGeneratedSlots(const TArray<int32>& SourceSlotIndices)
{
	const int32 SlotCount = FMath::Min(SourceSlotIndices.Num(), GeneratedInventorySlotWidgets.Num());
	for (int32 DisplayIndex = 0; DisplayIndex < SlotCount; ++DisplayIndex)
	{
		UGridInventorySlotWidget* SlotWidget = GeneratedInventorySlotWidgets[DisplayIndex];
		if (!SlotWidget)
		{
			continue;
		}

		const int32 SourceSlotIndex = SourceSlotIndices[DisplayIndex];
		if (SlotWidget->SlotType == EGridInventoryUiSlotType::Inventory && SlotWidget->InventorySlotIndex == SourceSlotIndex)
		{
			continue;
		}

		SlotWidget->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, SourceSlotIndex);
		RefreshRegisteredSlotWidget(SlotWidget);
	}
}


void UGridInventoryWidget::ClearGeneratedInventorySlotWidgets()
{
	RemoveGeneratedInventorySlotsFromRegistry();

	for (UGridInventorySlotWidget* SlotWidget : GeneratedInventorySlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->RemoveFromParent();
		}
	}

	if (InventorySlotsGridPanel)
	{
		InventorySlotsGridPanel->ClearChildren();
	}

	GeneratedInventorySlotWidgets.Empty();
	bInventorySlotsBuilt = false;
	LastBuiltSlotCount = 0;
	LastBuiltColumnCount = 0;
	LastBuiltSlotWidgetClass = nullptr;
	LastBuiltGridPanel = nullptr;
}

int32 UGridInventoryWidget::ResolveInventorySlotWidgetCount() const
{
	return FMath::Max(0, ResolveInventorySourceSlotCapacity());
}

void UGridInventoryWidget::SetInventorySlotWidgetClass(TSubclassOf<UGridInventorySlotWidget> InClass)
{
	InventorySlotWidgetClass = InClass;
	bInventorySlotsBuilt = false;
}

void UGridInventoryWidget::SetInventorySlotsGridPanel(UUniformGridPanel* InGridPanel)
{
	InventorySlotsGridPanel = InGridPanel;
	bInventorySlotsBuilt = false;
}

void UGridInventoryWidget::RemoveGeneratedInventorySlotsFromRegistry()
{
	RegisteredInventorySlots.RemoveAll(
		[this](const TObjectPtr<UGridInventorySlotWidget>& SlotWidget)
		{
			if (!SlotWidget || SlotWidget->SlotType != EGridInventoryUiSlotType::Inventory)
			{
				return false;
			}

			return GeneratedInventorySlotWidgets.Contains(SlotWidget) || (InventorySlotsGridPanel && SlotWidget->GetParent() == InventorySlotsGridPanel);
		});
}

void UGridInventoryWidget::RefreshRegisteredSlotWidget(UGridInventorySlotWidget* SlotWidget)
{
	if (!SlotWidget)
	{
		return;
	}

	FGridItemInstance Item;
	switch (SlotWidget->SlotType)
	{
		case EGridInventoryUiSlotType::Inventory:
			if (GetInventoryItemAtSlot(SlotWidget->InventorySlotIndex, Item))
			{
				SlotWidget->SetItem(Item);
			}
			else
			{
				SlotWidget->ClearItem();
			}
			break;

		case EGridInventoryUiSlotType::Equipment:
			if (GetEquipmentItem(SlotWidget->EquipmentSlot, Item))
			{
				SlotWidget->SetItem(Item);
			}
			else
			{
				SlotWidget->ClearItem();
			}
			break;

		case EGridInventoryUiSlotType::MainHand:
			if (GetMainHandItem(Item))
			{
				SlotWidget->SetItem(Item);
			}
			else
			{
				SlotWidget->ClearItem();
			}
			break;

		case EGridInventoryUiSlotType::OffHand:
			if (GetOffHandItem(Item))
			{
				SlotWidget->SetItem(Item);
			}
			else
			{
				SlotWidget->ClearItem();
			}
			break;

		case EGridInventoryUiSlotType::Cursor:
			if (GetCursorItem(Item))
			{
				SlotWidget->SetItem(Item);
			}
			else
			{
				SlotWidget->ClearItem();
			}
			break;

		default:
			break;
	}
}

void UGridInventoryWidget::RefreshRegisteredSlotWidgets()
{
	for (UGridInventorySlotWidget* SlotWidget : RegisteredInventorySlots)
	{
		RefreshRegisteredSlotWidget(SlotWidget);
	}

	for (const TPair<EGridEquipmentSlot, TObjectPtr<UGridInventorySlotWidget>>& RegisteredEquipmentSlot : RegisteredEquipmentSlotWidgets)
	{
		RefreshRegisteredSlotWidget(RegisteredEquipmentSlot.Value);
	}

	if (MainHandSlotWidget && !RegisteredEquipmentSlotWidgets.Contains(EGridEquipmentSlot::MainHand))
	{
		RefreshRegisteredSlotWidget(MainHandSlotWidget);
	}
	if (OffHandSlotWidget && !RegisteredEquipmentSlotWidgets.Contains(EGridEquipmentSlot::OffHand))
	{
		RefreshRegisteredSlotWidget(OffHandSlotWidget);
	}

	RefreshRegisteredSlotWidget(CursorSlotWidget);
}

void UGridInventoryWidget::HandleRegisteredSlotClicked(EGridInventoryUiSlotType SlotType, int32 SlotIndex)
{
	switch (SlotType)
	{
		case EGridInventoryUiSlotType::Inventory:
			HandleInventorySlotClicked(SlotIndex);
			break;
		case EGridInventoryUiSlotType::Equipment:
			HandleEquipmentSlotClicked(static_cast<EGridEquipmentSlot>(SlotIndex));
			break;
		case EGridInventoryUiSlotType::MainHand:
			HandleMainHandClicked();
			break;
		case EGridInventoryUiSlotType::OffHand:
			HandleOffHandClicked();
			break;
		case EGridInventoryUiSlotType::Cursor:
			HandleCursorReturnToInventoryClicked();
			break;
		default:
			break;
	}
}

bool UGridInventoryWidget::HandleItemSlotRightClicked(EGridInventoryUiSlotType SlotType, int32 SlotIndex)
{
	LastContextItem = FGridItemInstance();
	LastFacingTargetContext = FGridFacingTargetContext();
	LastContextActions.Reset();

	const bool bBuilt = BuildContextActionsForSlot(SlotType, SlotIndex, LastFacingTargetContext, LastContextActions);

	UE_LOG(LogTemp, Log, TEXT("GridInventory RightClick Slot=%d Item=%s Actions=%d Result=%s"), SlotIndex,
		LastContextItem.ItemDefinitionId.IsNone() ? TEXT("None") : *LastContextItem.ItemDefinitionId.ToString(), LastContextActions.Num(),
		bBuilt ? TEXT("true") : TEXT("false"));

	if (bBuilt)
	{
		bItemActionMenuCloseRequested = false;
		const bool bHasContextMenuPresenter = OnContextActionsRequested.IsBound();
		OnContextActionsRequested.Broadcast(SlotType, SlotIndex);
		if (!bHasContextMenuPresenter)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("GridInventory RightClick PresentationMissing Widget=%s Reason=OnContextActionsRequestedUnbound Slot=%s:%d"),
				*GetNameSafe(this), GetGridInventoryUiSlotTypeName(SlotType), SlotIndex);
		}
	}
	return bBuilt;
}

bool UGridInventoryWidget::BuildContextActionsForSlot(
	EGridInventoryUiSlotType SlotType, int32 SlotIndex, FGridFacingTargetContext& OutFacingTarget, TArray<FGridItemContextAction>& OutActions)
{
	OutFacingTarget = FGridFacingTargetContext();
	OutActions.Reset();
	LastContextItem = FGridItemInstance();
	if (!OwningPartyPawn || !InventoryComponent)
	{
		return false;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	if (SlotType == EGridInventoryUiSlotType::Inventory)
	{
		if (!GetInventoryItemAtSlot(SlotIndex, LastContextItem))
		{
			return false;
		}

		return UGridItemContextActionLibrary::BuildInventorySlotContextActions(OwningPartyPawn, CharacterIndex, SlotIndex, OutFacingTarget, OutActions);
	}

	FGridItemActionContext ItemContext;
	ItemContext.PartyPawn = OwningPartyPawn;
	ItemContext.CharacterIndex = CharacterIndex;
	ItemContext.InventorySlotIndex = INDEX_NONE;

	switch (SlotType)
	{
		case EGridInventoryUiSlotType::Equipment:
		{
			const EGridEquipmentSlot EquipmentSlot = ResolveUiEquipmentSlot(SlotType, SlotIndex);
			if (!GetEquipmentItem(EquipmentSlot, LastContextItem))
			{
				return false;
			}
			ItemContext.EquipmentSlot = EquipmentSlot;
			break;
		}
		case EGridInventoryUiSlotType::MainHand:
			if (!GetMainHandItem(LastContextItem))
			{
				return false;
			}
			ItemContext.EquipmentSlot = EGridEquipmentSlot::MainHand;
			break;
		case EGridInventoryUiSlotType::OffHand:
			if (!GetOffHandItem(LastContextItem))
			{
				return false;
			}
			ItemContext.EquipmentSlot = EGridEquipmentSlot::OffHand;
			break;
		case EGridInventoryUiSlotType::Cursor:
			if (!GetCursorItem(LastContextItem))
			{
				return false;
			}
			break;
		default:
			return false;
	}

	ItemContext.Item = LastContextItem;
	ItemContext.ItemDefinition = InventoryComponent->FindItemDefinition(LastContextItem.ItemDefinitionId);
	return UGridItemContextActionLibrary::BuildItemContextActions(ItemContext, OutFacingTarget, OutActions);
}

bool UGridInventoryWidget::ExecuteInventoryContextAction(EGridItemActionType ActionType, EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex)
{
	FGridFacingTargetContext FacingTarget;
	TArray<FGridItemContextAction> AvailableActions;
	if (!BuildContextActionsForSlot(SourceSlotType, SourceSlotIndex, FacingTarget, AvailableActions))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute Failed Action=%s Reason=InvalidSource"), GetContextActionName(ActionType));
		return false;
	}

	const int32 MatchingActionCount = AvailableActions
										  .FilterByPredicate(
											  [ActionType](const FGridItemContextAction& Action)
											  {
												  return Action.ActionType == ActionType;
											  })
										  .Num();
	if (MatchingActionCount > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute Failed Action=%s Reason=AmbiguousActionType"), GetContextActionName(ActionType));
		return false;
	}

	const FGridItemContextAction* SelectedAction = AvailableActions.FindByPredicate(
		[ActionType](const FGridItemContextAction& Action)
		{
			return Action.ActionType == ActionType;
		});
	if (!SelectedAction || !SelectedAction->bEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute Failed Action=%s Item=%s Reason=%s"), GetContextActionName(ActionType),
			*LastContextItem.ItemDefinitionId.ToString(), SelectedAction ? TEXT("ActionDisabled") : TEXT("ActionUnavailable"));
		return false;
	}

	return ExecuteResolvedInventoryContextAction(*SelectedAction, FacingTarget, SourceSlotType, SourceSlotIndex);
}

bool UGridInventoryWidget::ExecuteInventoryContextActionByIndex(EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex, int32 ActionIndex)
{
	FGridFacingTargetContext FacingTarget;
	TArray<FGridItemContextAction> AvailableActions;
	if (!BuildContextActionsForSlot(SourceSlotType, SourceSlotIndex, FacingTarget, AvailableActions))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions ExecuteByIndex Failed Reason=InvalidSource Slot=%s:%d ActionIndex=%d"),
			GetGridInventoryUiSlotTypeName(SourceSlotType), SourceSlotIndex, ActionIndex);
		return false;
	}

	if (!AvailableActions.IsValidIndex(ActionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions ExecuteByIndex Failed Reason=InvalidActionIndex Slot=%s:%d ActionIndex=%d ActionCount=%d"),
			GetGridInventoryUiSlotTypeName(SourceSlotType), SourceSlotIndex, ActionIndex, AvailableActions.Num());
		return false;
	}

	const FGridItemContextAction& SelectedAction = AvailableActions[ActionIndex];
	UE_LOG(LogTemp, Log, TEXT("GridItemActions ExecuteByIndex Slot=%s:%d ActionIndex=%d Action=%s Label=\"%s\" EquipmentSlot=%s TargetType=%s"),
		GetGridInventoryUiSlotTypeName(SourceSlotType), SourceSlotIndex, ActionIndex, GetContextActionName(SelectedAction.ActionType),
		*SelectedAction.Label.ToString(), GetContextEquipmentSlotName(SelectedAction.EquipmentSlot), GetContextTargetTypeName(FacingTarget.TargetType));

	if (!SelectedAction.bEnabled)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions ExecuteByIndex Failed Reason=ActionDisabled Slot=%s:%d ActionIndex=%d Action=%s"),
			GetGridInventoryUiSlotTypeName(SourceSlotType), SourceSlotIndex, ActionIndex, GetContextActionName(SelectedAction.ActionType));
		return false;
	}

	return ExecuteResolvedInventoryContextAction(SelectedAction, FacingTarget, SourceSlotType, SourceSlotIndex);
}

void UGridInventoryWidget::CloseItemActionMenu(FName Reason)
{
	const FString ReasonString = Reason.IsNone() ? TEXT("Unspecified") : Reason.ToString();

	UE_LOG(LogTemp, Log, TEXT("GridItemActionMenu Close Requested Reason=%s"), *ReasonString);

	if (bItemActionMenuCloseRequested)
	{
		UE_LOG(LogTemp, Verbose, TEXT("GridItemActionMenu Close Skipped Reason=AlreadyDetached RequestedReason=%s"), *ReasonString);
		return;
	}

	bItemActionMenuCloseRequested = true;
	if (UUserWidget* CurrentItemActionMenu = GetCurrentItemActionMenuWidget(this); IsItemActionMenuDetached(CurrentItemActionMenu))
	{
		UE_LOG(LogTemp, Verbose, TEXT("GridItemActionMenu Close Skipped Reason=AlreadyDetached RequestedReason=%s"), *ReasonString);
		ClearCurrentItemActionMenuWidget(this);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("GridItemActionMenu Closed Reason=%s"), *ReasonString);
	OnItemActionMenuCloseRequested(Reason);

	if (UUserWidget* CurrentItemActionMenu = GetCurrentItemActionMenuWidget(this); IsItemActionMenuDetached(CurrentItemActionMenu))
	{
		ClearCurrentItemActionMenuWidget(this);
	}
}

bool UGridInventoryWidget::IsItemActionMenuOpen() const
{
	const UUserWidget* CurrentItemActionMenu = GetCurrentItemActionMenuWidget(this);
	return CurrentItemActionMenu && !IsItemActionMenuDetached(CurrentItemActionMenu);
}

void UGridInventoryWidget::CloseItemReadPanel(FName Reason)
{
	UE_LOG(LogTemp, Log, TEXT("GridItemReadPanel Closed Reason=%s"), Reason.IsNone() ? TEXT("Unspecified") : *Reason.ToString());
	OnItemReadPanelCloseRequested(Reason);
}

bool UGridInventoryWidget::ExecuteResolvedInventoryContextAction(
	const FGridItemContextAction& Action, const FGridFacingTargetContext& FacingTarget, EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex)
{
	const int32 CharacterIndex = InventoryComponent ? InventoryComponent->GetSelectedCharacterIndex() : INDEX_NONE;
	bool bExecuted = false;
	switch (Action.ActionType)
	{
		case EGridItemActionType::Examine:
		{
			UGridInventorySlotWidget* SourceWidget = FindRegisteredSlotWidget(SourceSlotType, SourceSlotIndex);
			const FText TooltipText = SourceWidget ? SourceWidget->GetTooltipText() : LastContextItem.DisplayName;
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute Examine Item=%s"), *LastContextItem.ItemDefinitionId.ToString());
			PresentItemExamination(LastContextItem, TooltipText);
			bExecuted = true;
			break;
		}

		case EGridItemActionType::Read:
		{
			const UGridItemDefinitionAsset* ItemDefinition =
				InventoryComponent ? InventoryComponent->FindItemDefinition(LastContextItem.ItemDefinitionId) : nullptr;
			FText Title = LastContextItem.ReadTitleOverride;
			if (Title.IsEmpty() && LastContextItem.ReadableContentAsset)
			{
				Title = LastContextItem.ReadableContentAsset->Title;
			}
			if (Title.IsEmpty())
			{
				Title = LastContextItem.DisplayName;
			}
			if (Title.IsEmpty() && ItemDefinition)
			{
				Title = ItemDefinition->DisplayName;
			}
			if (Title.IsEmpty())
			{
				Title = FText::FromName(LastContextItem.ItemDefinitionId);
			}

			FText ReadText = LastContextItem.ReadTextOverride;
			const TCHAR* ReadSource = TEXT("InstanceOverride");
			FName ResolvedContentId = LastContextItem.ReadableContentId;
			if (ReadText.IsEmpty() && LastContextItem.ReadableContentAsset)
			{
				ReadText = LastContextItem.ReadableContentAsset->BodyText;
				ReadSource = TEXT("ReadableContentAsset");
				if (ResolvedContentId.IsNone())
				{
					ResolvedContentId = LastContextItem.ReadableContentAsset->ReadableContentId;
				}
			}
			if (ReadText.IsEmpty() && ItemDefinition && !ItemDefinition->ReadText.IsEmpty())
			{
				ReadText = ItemDefinition->ReadText;
				ReadSource = TEXT("DefinitionFallback");
			}
			if (ReadText.IsEmpty())
			{
				ReadText = NSLOCTEXT("GridItemActions", "EmptyReadText", "Rien n'est écrit.");
				ReadSource = TEXT("EmptyFallback");
			}

			UE_LOG(LogTemp, Log, TEXT("GridItemReading Resolve Item=%s Source=%s Content=%s"), *LastContextItem.ItemDefinitionId.ToString(), ReadSource,
				ResolvedContentId.IsNone() ? TEXT("None") : *ResolvedContentId.ToString());
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute Read Item=%s"), *LastContextItem.ItemDefinitionId.ToString());
			PresentItemReading(LastContextItem, Title, ReadText);
			bExecuted = true;
			break;
		}

		case EGridItemActionType::Equip:
		{
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute Equip Item=%s EquipmentSlot=%s"), *LastContextItem.ItemDefinitionId.ToString(),
				GetContextEquipmentSlotName(Action.EquipmentSlot));
			if (SourceSlotType == EGridInventoryUiSlotType::Inventory && InventoryComponent && OwningPartyPawn &&
				Action.EquipmentSlot != EGridEquipmentSlot::None)
			{
				const UGridItemDefinitionAsset* ItemDefinition = InventoryComponent->FindItemDefinition(LastContextItem.ItemDefinitionId);
				if (!ItemDefinition || !ItemDefinition->CanEquipToSlot(Action.EquipmentSlot))
				{
					UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute Equip Failed Item=%s EquipmentSlot=%s Reason=IncompatibleSlot"),
						*LastContextItem.ItemDefinitionId.ToString(), GetContextEquipmentSlotName(Action.EquipmentSlot));
					break;
				}

				if (!InventoryComponent->CanEquipItemToSlot(CharacterIndex, LastContextItem, Action.EquipmentSlot))
				{
					UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute Equip Failed Item=%s EquipmentSlot=%s Reason=IncompatibleSlot"),
						*LastContextItem.ItemDefinitionId.ToString(), GetContextEquipmentSlotName(Action.EquipmentSlot));
					break;
				}

				bExecuted = InventoryComponent->EquipItemFromInventorySlot(CharacterIndex, SourceSlotIndex, Action.EquipmentSlot);
				if (bExecuted)
				{
					OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();
				}
			}
			break;
		}

		case EGridItemActionType::Unequip:
		{
			const EGridEquipmentSlot SourceEquipmentSlot = ResolveSourceEquipmentSlot(Action, SourceSlotType);
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute Unequip Item=%s EquipmentSlot=%s"), *LastContextItem.ItemDefinitionId.ToString(),
				GetContextEquipmentSlotName(SourceEquipmentSlot));
			if (InventoryComponent && OwningPartyPawn && SourceEquipmentSlot != EGridEquipmentSlot::None)
			{
				if (!InventoryComponent->CanAddItemToCharacterInventory(CharacterIndex, LastContextItem))
				{
					UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute Unequip Failed Item=%s Reason=NoFreeInventorySlot"),
						*LastContextItem.ItemDefinitionId.ToString());
					break;
				}

				bExecuted = InventoryComponent->UnequipItemToInventory(CharacterIndex, SourceEquipmentSlot);
				if (bExecuted)
				{
					OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute Unequip Failed Item=%s Reason=NoFreeInventorySlot"),
						*LastContextItem.ItemDefinitionId.ToString());
				}
			}
			break;
		}

		case EGridItemActionType::InsertIntoTarget:
		{
			AGridWallLockActor* WallLock = Cast<AGridWallLockActor>(FacingTarget.TargetActor);
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute InsertIntoTarget Item=%s Target=%s"), *LastContextItem.ItemDefinitionId.ToString(),
				WallLock ? TEXT("WallLock") : TEXT("None"));
			if (SourceSlotType == EGridInventoryUiSlotType::Inventory && WallLock && InventoryComponent && OwningPartyPawn)
			{
				bExecuted =
					UGridItemTransferService::TransferInventorySlotToWallLock(InventoryComponent, CharacterIndex, SourceSlotIndex, WallLock, OwningPartyPawn)
						.bSuccess;
			}
			break;
		}

		case EGridItemActionType::PlaceOnTarget:
		{
			AGridReceptacleActor* Receptacle = Cast<AGridReceptacleActor>(FacingTarget.TargetActor);
			const bool bIsTorchHolder = FacingTarget.TargetType == EGridFacingTargetType::TorchHolder;
			const bool bIsReceptacleTarget =
				FacingTarget.TargetType == EGridFacingTargetType::Receptacle || FacingTarget.TargetType == EGridFacingTargetType::TorchHolder;
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute PlaceOnTarget Item=%s Source=%s Target=%s"), *LastContextItem.ItemDefinitionId.ToString(),
				GetGridInventoryUiSlotTypeName(SourceSlotType),
				bIsTorchHolder ? TEXT("TorchHolder") : (bIsReceptacleTarget ? TEXT("Receptacle") : TEXT("None")));
			if (!Receptacle || !InventoryComponent || !bIsReceptacleTarget)
			{
				UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute PlaceOnTarget Failed Item=%s Source=%s Reason=TargetRejected"),
					*LastContextItem.ItemDefinitionId.ToString(), GetGridInventoryUiSlotTypeName(SourceSlotType));
				break;
			}

			FGridItemTransferResult TransferResult;
			if (SourceSlotType == EGridInventoryUiSlotType::Inventory)
			{
				TransferResult = UGridItemTransferService::TransferInventorySlotToReceptacle(InventoryComponent, CharacterIndex, SourceSlotIndex, Receptacle);
			}
			else if (SourceSlotType == EGridInventoryUiSlotType::Equipment || SourceSlotType == EGridInventoryUiSlotType::MainHand ||
				SourceSlotType == EGridInventoryUiSlotType::OffHand)
			{
				const EGridEquipmentSlot SourceEquipmentSlot = ResolveSourceEquipmentSlot(Action, SourceSlotType);
				TransferResult =
					UGridItemTransferService::TransferEquipmentSlotToReceptacle(InventoryComponent, CharacterIndex, SourceEquipmentSlot, Receptacle);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute PlaceOnTarget Failed Item=%s Source=%s Reason=InvalidSource"),
					*LastContextItem.ItemDefinitionId.ToString(), GetGridInventoryUiSlotTypeName(SourceSlotType));
				break;
			}

			bExecuted = TransferResult.bSuccess;
			if (!bExecuted)
			{
				const TCHAR* Reason = TransferResult.Result == EGridItemTransferResult::InvalidSource ? TEXT("InvalidSource") : TEXT("TargetRejected");
				UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute PlaceOnTarget Failed Item=%s Source=%s Reason=%s"),
					*LastContextItem.ItemDefinitionId.ToString(), GetGridInventoryUiSlotTypeName(SourceSlotType), Reason);
				break;
			}

			if (bIsTorchHolder)
			{
				const int32 InsertedItemIndex = Receptacle->GetContainedItemCount() - 1;
				const bool bTorchLightEnabled = Receptacle->SetContainedItemLightsEnabled(InsertedItemIndex, true);
				UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute PlaceOnTarget TorchLightEnabled=%s"), bTorchLightEnabled ? TEXT("true") : TEXT("false"));
			}
			break;
		}

		case EGridItemActionType::SplitStack:
		{
			bExecuted = SourceSlotType == EGridInventoryUiSlotType::Inventory && InventoryComponent &&
				InventoryComponent->TrySplitInventoryStackToFirstFreeSlot(CharacterIndex, SourceSlotIndex);
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute SplitStack Item=%s Slot=%d Result=%s"),
				*LastContextItem.ItemDefinitionId.ToString(), SourceSlotIndex, bExecuted ? TEXT("true") : TEXT("false"));
			if (bExecuted)
			{
				CloseItemActionMenu(TEXT("SplitStack"));
				return true;
			}
			break;
		}

		case EGridItemActionType::AddToHotbar:
		{
			if (SourceSlotType != EGridInventoryUiSlotType::Inventory || !InventoryComponent || LastContextItem.ItemDefinitionId.IsNone())
			{
				UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute AddToHotbar Failed Item=%s Reason=InvalidSource"),
					*LastContextItem.ItemDefinitionId.ToString());
				break;
			}

			int32 TargetHotbarSlotIndex = INDEX_NONE;
			bool bAlreadyBound = false;
			for (int32 HotbarSlotIndex = 0; HotbarSlotIndex < FGridCombatHotbarBinding::SlotCount; ++HotbarSlotIndex)
			{
				if (HotbarSlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex)
				{
					continue;
				}

				FGridCombatHotbarBinding Binding;
				if (!InventoryComponent->GetCharacterCombatHotbarBinding(CharacterIndex, HotbarSlotIndex, Binding))
				{
					continue;
				}

				if (Binding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem && Binding.SourceDefinitionId == LastContextItem.ItemDefinitionId)
				{
					bAlreadyBound = true;
					break;
				}

				if (TargetHotbarSlotIndex == INDEX_NONE && Binding.IsEmpty())
				{
					TargetHotbarSlotIndex = HotbarSlotIndex;
				}
			}

			if (bAlreadyBound)
			{
				UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute AddToHotbar Skipped Item=%s Reason=AlreadyBound"),
					*LastContextItem.ItemDefinitionId.ToString());
				break;
			}
			if (TargetHotbarSlotIndex == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute AddToHotbar Failed Item=%s Reason=HotbarFull"),
					*LastContextItem.ItemDefinitionId.ToString());
				break;
			}

			bExecuted = InventoryComponent->SetCharacterCombatHotbarBindingFromItem(
				CharacterIndex, TargetHotbarSlotIndex, LastContextItem, EGridEquipmentSlot::None);
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute AddToHotbar Item=%s Slot=%d Result=%s"),
				*LastContextItem.ItemDefinitionId.ToString(), TargetHotbarSlotIndex, bExecuted ? TEXT("true") : TEXT("false"));
			break;
		}

		case EGridItemActionType::Throw:
		{
			const EGridEquipmentSlot SourceEquipmentSlot = ResolveSourceEquipmentSlot(Action, SourceSlotType);
			if (OwningPartyPawn && SourceEquipmentSlot == EGridEquipmentSlot::MainHand)
			{
				bExecuted = OwningPartyPawn->BeginSelectedCharacterMainHandThrowAiming();
			}
			break;
		}

		case EGridItemActionType::DropToGround:
			bExecuted = DropContextItemToGround(Action, SourceSlotType, SourceSlotIndex);
			break;

		default:
			UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute NotImplemented Action=%s Item=%s"), GetContextActionName(Action.ActionType),
				*LastContextItem.ItemDefinitionId.ToString());
			break;
	}

	if (bExecuted)
	{
		RefreshInventory();
	}
	return bExecuted;
}

EGridEquipmentSlot UGridInventoryWidget::ResolveSourceEquipmentSlot(const FGridItemContextAction& Action, EGridInventoryUiSlotType SourceSlotType) const
{
	if (Action.EquipmentSlot != EGridEquipmentSlot::None)
	{
		return Action.EquipmentSlot;
	}

	switch (SourceSlotType)
	{
		case EGridInventoryUiSlotType::MainHand:
			return EGridEquipmentSlot::MainHand;
		case EGridInventoryUiSlotType::OffHand:
			return EGridEquipmentSlot::OffHand;
		default:
			return EGridEquipmentSlot::None;
	}
}

bool UGridInventoryWidget::DropContextItemToGround(const FGridItemContextAction& Action, EGridInventoryUiSlotType SourceSlotType, int32 SourceSlotIndex)
{
	if (!InventoryComponent || !OwningPartyPawn || !OwningPartyPawn->LevelRuntimeActor)
	{
		UE_LOG(
			LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=MissingRuntime"), *LastContextItem.ItemDefinitionId.ToString());
		return false;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	if (SourceSlotType == EGridInventoryUiSlotType::Inventory)
	{
		const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
		if (!State.ActiveCharacters.IsValidIndex(CharacterIndex) || !State.ActiveCharacters[CharacterIndex].InventorySlots.IsValidIndex(SourceSlotIndex) ||
			State.ActiveCharacters[CharacterIndex].InventorySlots[SourceSlotIndex].IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidInventorySlot"),
				*LastContextItem.ItemDefinitionId.ToString());
			return false;
		}
	}
	else if (SourceSlotType == EGridInventoryUiSlotType::Equipment || SourceSlotType == EGridInventoryUiSlotType::MainHand ||
		SourceSlotType == EGridInventoryUiSlotType::OffHand)
	{
		const EGridEquipmentSlot SourceEquipmentSlot = ResolveSourceEquipmentSlot(Action, SourceSlotType);
		if (!InventoryComponent->PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentState"),
				*LastContextItem.ItemDefinitionId.ToString());
			return false;
		}

		const FGridCharacterEquipmentState& EquipmentState = InventoryComponent->PartyInventoryState.ActiveEquipment[CharacterIndex];
		const FGridItemInstance* EquippedItem = EquipmentState.GetSlot(SourceEquipmentSlot);
		if (!EquippedItem || !EquippedItem->IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentSlot"),
				*LastContextItem.ItemDefinitionId.ToString());
			return false;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=UnsupportedSource"),
			*LastContextItem.ItemDefinitionId.ToString());
		return false;
	}

	FGridItemInstance ItemToDrop = LastContextItem;
	ItemToDrop.OwnerType = EGridItemOwnerType::World;
	ItemToDrop.OwnerGuid = FGuid();
	ItemToDrop.OwnerCharacterIndex = INDEX_NONE;
	ItemToDrop.EquipmentSlot = EGridEquipmentSlot::None;

	if (!OwningPartyPawn->LevelRuntimeActor->TryDropItemInstanceAtCell(
			ItemToDrop, OwningPartyPawn->CurrentCellX, OwningPartyPawn->CurrentCellY, EGridEdge::None, FVector::ZeroVector))
	{
		UE_LOG(
			LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=WorldDropFailed"), *LastContextItem.ItemDefinitionId.ToString());
		return false;
	}

	if (SourceSlotType == EGridInventoryUiSlotType::Inventory)
	{
		UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute DropToGround Item=%s Source=Inventory Slot=%d"), *LastContextItem.ItemDefinitionId.ToString(),
			SourceSlotIndex);
		FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
		if (!State.ActiveCharacters.IsValidIndex(CharacterIndex) || !State.ActiveCharacters[CharacterIndex].InventorySlots.IsValidIndex(SourceSlotIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidInventorySlotAfterDrop"),
				*LastContextItem.ItemDefinitionId.ToString());
			return false;
		}

		State.ActiveCharacters[CharacterIndex].InventorySlots[SourceSlotIndex] = FGridInventorySlot();
		InventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
		return true;
	}

	const EGridEquipmentSlot SourceEquipmentSlot = ResolveSourceEquipmentSlot(Action, SourceSlotType);
	UE_LOG(LogTemp, Log, TEXT("GridItemActions Execute DropToGround Item=%s Source=%s"), *LastContextItem.ItemDefinitionId.ToString(),
		GetContextEquipmentSlotName(SourceEquipmentSlot));
	if (!InventoryComponent->PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentStateAfterDrop"),
			*LastContextItem.ItemDefinitionId.ToString());
		return false;
	}

	FGridCharacterEquipmentState& EquipmentState = InventoryComponent->PartyInventoryState.ActiveEquipment[CharacterIndex];
	if (FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot(SourceEquipmentSlot))
	{
		*EquippedItem = FGridItemInstance();
		InventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
		OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("GridItemActions Execute DropToGround Failed Item=%s Reason=InvalidEquipmentSlot"),
		*LastContextItem.ItemDefinitionId.ToString());
	return false;
}

bool UGridInventoryWidget::HandleSlotDrop(
	EGridInventoryUiSlotType SourceType, int32 SourceIndex, EGridInventoryUiSlotType TargetType, int32 TargetIndex)
{
	UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop Source=%s SourceIndex=%d Target=%s TargetIndex=%d"), GetGridInventoryUiSlotTypeName(SourceType),
		SourceIndex, GetGridInventoryUiSlotTypeName(TargetType), TargetIndex);

	if (!InventoryComponent || !OwningPartyPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Drop Failed Reason=MissingPawnOrInventoryComponent"));
		RefreshInventory();
		return false;
	}

	if (SourceType == TargetType && SourceIndex == TargetIndex)
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop Result=true Reason=SameSlot"));
		RefreshInventory();
		return true;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	auto ValidateOwnership = [&]()
	{
		FString OwnershipError;
		if (!InventoryComponent->ValidateInventoryOwnership(OwnershipError))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Drop Ownership Failed Error=%s"), *OwnershipError);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop Ownership OK"));
		}
	};

	auto ResolveEquipmentSlot = [](EGridInventoryUiSlotType SlotType, int32 SlotIndex) -> EGridEquipmentSlot
	{
		switch (SlotType)
		{
			case EGridInventoryUiSlotType::Equipment:
				return static_cast<EGridEquipmentSlot>(SlotIndex);
			case EGridInventoryUiSlotType::MainHand:
				return EGridEquipmentSlot::MainHand;
			case EGridInventoryUiSlotType::OffHand:
				return EGridEquipmentSlot::OffHand;
			default:
				return EGridEquipmentSlot::None;
		}
	};

	auto SyncHeldVisualIfHandEquipmentSlot = [&](EGridEquipmentSlot EquipmentSlot, const TCHAR* Reason)
	{
		if (OwningPartyPawn && IsGridInventoryHandEquipmentSlot(EquipmentSlot))
		{
			OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();
			UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop SyncHeldVisual Slot=%s Reason=%s"), GetContextEquipmentSlotName(EquipmentSlot), Reason);
		}
	};

	auto GetMutableSlotItem = [&](EGridInventoryUiSlotType SlotType, int32 SlotIndex) -> FGridItemInstance*
	{
		if (!InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex) ||
			!InventoryComponent->PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
		{
			return nullptr;
		}

		if (SlotType == EGridInventoryUiSlotType::Inventory)
		{
			FGridCharacterInventoryState& CharacterState = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
			if (!CharacterState.InventorySlots.IsValidIndex(SlotIndex) || CharacterState.InventorySlots[SlotIndex].IsEmpty())
			{
				return nullptr;
			}
			return &CharacterState.InventorySlots[SlotIndex].Item;
		}

		if (const EGridEquipmentSlot EquipmentSlot = ResolveEquipmentSlot(SlotType, SlotIndex); EquipmentSlot != EGridEquipmentSlot::None)
		{
			FGridCharacterEquipmentState& EquipmentState = InventoryComponent->PartyInventoryState.ActiveEquipment[CharacterIndex];
			FGridItemInstance* Item = EquipmentState.GetMutableSlot(EquipmentSlot);
			return Item && Item->IsValid() ? Item : nullptr;
		}

		return nullptr;
	};

	auto CanPlaceItemInSlot = [&](const FGridItemInstance& Item, EGridInventoryUiSlotType SlotType, int32 SlotIndex) -> bool
	{
		if (SlotType == EGridInventoryUiSlotType::Inventory)
		{
			return Item.IsValid();
		}

		const EGridEquipmentSlot EquipmentSlot = ResolveEquipmentSlot(SlotType, SlotIndex);
		return EquipmentSlot != EGridEquipmentSlot::None && InventoryComponent->CanEquipItemToSlot(CharacterIndex, Item, EquipmentSlot);
	};

	auto PrepareItemForSlot = [&](FGridItemInstance& Item, EGridInventoryUiSlotType SlotType, int32 SlotIndex)
	{
		if (SlotType == EGridInventoryUiSlotType::Inventory)
		{
			Item.OwnerType = EGridItemOwnerType::CharacterInventory;
			Item.OwnerGuid = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
			Item.OwnerCharacterIndex = CharacterIndex;
			Item.EquipmentSlot = EGridEquipmentSlot::None;
			return;
		}

		const EGridEquipmentSlot EquipmentSlot = ResolveEquipmentSlot(SlotType, SlotIndex);
		Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
		Item.OwnerGuid = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
		Item.OwnerCharacterIndex = CharacterIndex;
		Item.EquipmentSlot = EquipmentSlot;
	};

	bool bSwapOccupiedSlotsAttempted = false;
	auto TrySwapOccupiedSlots = [&]() -> bool
	{
		if (SourceType == EGridInventoryUiSlotType::Cursor || TargetType == EGridInventoryUiSlotType::Cursor ||
			(SourceType == EGridInventoryUiSlotType::Inventory && TargetType == EGridInventoryUiSlotType::Inventory))
		{
			return false;
		}

		FGridItemInstance* SourceItemPtr = GetMutableSlotItem(SourceType, SourceIndex);
		FGridItemInstance* TargetItemPtr = GetMutableSlotItem(TargetType, TargetIndex);
		if (!SourceItemPtr || !TargetItemPtr)
		{
			return false;
		}
		bSwapOccupiedSlotsAttempted = true;

		UE_LOG(LogTemp, Log, TEXT("GridInventory SwapSlots Source=%s SourceIndex=%d Target=%s TargetIndex=%d"), GetGridInventoryUiSlotTypeName(SourceType),
			SourceIndex, GetGridInventoryUiSlotTypeName(TargetType), TargetIndex);

		FGridItemInstance SourceItem = *SourceItemPtr;
		FGridItemInstance TargetItem = *TargetItemPtr;
		if (!CanPlaceItemInSlot(SourceItem, TargetType, TargetIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory SwapSlots Failed Reason=IncompatibleSourceToTarget Item=%s"), *SourceItem.ItemDefinitionId.ToString());
			return false;
		}

		if (!CanPlaceItemInSlot(TargetItem, SourceType, SourceIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory SwapSlots Failed Reason=IncompatibleTargetToSource Item=%s"), *TargetItem.ItemDefinitionId.ToString());
			return false;
		}

		PrepareItemForSlot(SourceItem, TargetType, TargetIndex);
		PrepareItemForSlot(TargetItem, SourceType, SourceIndex);
		*SourceItemPtr = TargetItem;
		*TargetItemPtr = SourceItem;
		InventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
		if (SourceType == EGridInventoryUiSlotType::Equipment || SourceType == EGridInventoryUiSlotType::MainHand ||
			SourceType == EGridInventoryUiSlotType::OffHand || TargetType == EGridInventoryUiSlotType::Equipment ||
			TargetType == EGridInventoryUiSlotType::MainHand || TargetType == EGridInventoryUiSlotType::OffHand)
		{
			OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();
		}

		UE_LOG(LogTemp, Log, TEXT("GridInventory SwapSlots Success ItemA=%s ItemB=%s"), *SourceItem.ItemDefinitionId.ToString(),
			*TargetItem.ItemDefinitionId.ToString());
		return true;
	};

	if (TrySwapOccupiedSlots())
	{
		ValidateOwnership();
		RefreshInventory();
		return true;
	}
	if (bSwapOccupiedSlotsAttempted)
	{
		ValidateOwnership();
		RefreshInventory();
		return false;
	}

	if (TargetType == EGridInventoryUiSlotType::Inventory && TargetIndex != INDEX_NONE)
	{
		if (TargetIndex < 0 || TargetIndex >= GetInventorySlotCount())
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Drop Failed Reason=InvalidTargetIndex Target=%d"), TargetIndex);
			RefreshInventory();
			return false;
		}

		bool bInventoryTargetResult = false;
		switch (SourceType)
		{
			case EGridInventoryUiSlotType::Inventory:
				bInventoryTargetResult = InventoryComponent->TryMoveCharacterInventorySlot(CharacterIndex, SourceIndex, TargetIndex);
				UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop InventoryToInventory Source=%d Target=%d Result=%s"), SourceIndex, TargetIndex,
					bInventoryTargetResult ? TEXT("true") : TEXT("false"));
				break;

			case EGridInventoryUiSlotType::Cursor:
				bInventoryTargetResult = InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot(CharacterIndex, TargetIndex);
				UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop CursorToInventory Target=%d Result=%s"), TargetIndex,
					bInventoryTargetResult ? TEXT("true") : TEXT("false"));
				break;

			case EGridInventoryUiSlotType::MainHand:
				bInventoryTargetResult = OwningPartyPawn->TryTakeSelectedCharacterMainHandToCursor() &&
					InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot(CharacterIndex, TargetIndex);
				UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop MainHandToInventory Target=%d Result=%s"), TargetIndex,
					bInventoryTargetResult ? TEXT("true") : TEXT("false"));
				break;

			case EGridInventoryUiSlotType::OffHand:
				bInventoryTargetResult = OwningPartyPawn->TryTakeSelectedCharacterOffHandToCursor() &&
					InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot(CharacterIndex, TargetIndex);
				UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop OffHandToInventory Target=%d Result=%s"), TargetIndex,
					bInventoryTargetResult ? TEXT("true") : TEXT("false"));
				break;

			case EGridInventoryUiSlotType::Equipment:
			{
				const EGridEquipmentSlot SourceEquipmentSlot = ResolveEquipmentSlot(SourceType, SourceIndex);
				bInventoryTargetResult = SourceEquipmentSlot != EGridEquipmentSlot::None &&
					InventoryComponent->TryTakeEquipmentSlotToCursor(CharacterIndex, SourceEquipmentSlot) &&
					InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot(CharacterIndex, TargetIndex);
				UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop EquipmentToInventory Slot=%s Target=%d Result=%s"),
					GetContextEquipmentSlotName(SourceEquipmentSlot), TargetIndex, bInventoryTargetResult ? TEXT("true") : TEXT("false"));
				if (bInventoryTargetResult)
				{
					SyncHeldVisualIfHandEquipmentSlot(SourceEquipmentSlot, TEXT("EquipmentToInventory"));
				}
				break;
			}

			default:
				break;
		}

		ValidateOwnership();
		RefreshInventory();
		return bInventoryTargetResult;
	}

	if (TargetType == EGridInventoryUiSlotType::Inventory && TargetIndex == INDEX_NONE &&
		SourceType == EGridInventoryUiSlotType::Inventory)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("GridInventory UI Drop Ignored Reason=VirtualProjectionTargetHasNoPhysicalSlot Source=%d"), SourceIndex);
		RefreshInventory();
		return false;
	}

	auto HasCurrentSourceItem = [&]() -> bool
	{
		FGridItemInstance Item;
		switch (SourceType)
		{
			case EGridInventoryUiSlotType::Inventory:
				return GetInventoryItemAtSlot(SourceIndex, Item);
			case EGridInventoryUiSlotType::MainHand:
				return GetMainHandItem(Item);
			case EGridInventoryUiSlotType::OffHand:
				return GetOffHandItem(Item);
			case EGridInventoryUiSlotType::Equipment:
				return GetEquipmentItem(ResolveEquipmentSlot(SourceType, SourceIndex), Item);
			case EGridInventoryUiSlotType::Cursor:
				return GetCursorItem(Item);
			default:
				return false;
		}
	};

	auto TakeSourceToCursor = [&]() -> bool
	{
		switch (SourceType)
		{
			case EGridInventoryUiSlotType::Inventory:
				return InventoryComponent->TryTakeInventorySlotToCursor(CharacterIndex, SourceIndex);
			case EGridInventoryUiSlotType::MainHand:
				return OwningPartyPawn->TryTakeSelectedCharacterMainHandToCursor();
			case EGridInventoryUiSlotType::OffHand:
				return OwningPartyPawn->TryTakeSelectedCharacterOffHandToCursor();
			case EGridInventoryUiSlotType::Equipment:
			{
				const EGridEquipmentSlot SourceEquipmentSlot = ResolveEquipmentSlot(SourceType, SourceIndex);
				const bool bTaken = InventoryComponent->TryTakeEquipmentSlotToCursor(CharacterIndex, SourceEquipmentSlot);
				if (bTaken)
				{
					SyncHeldVisualIfHandEquipmentSlot(SourceEquipmentSlot, TEXT("EquipmentToCursor"));
				}
				return bTaken;
			}
			case EGridInventoryUiSlotType::Cursor:
				return InventoryComponent->HasCursorItem();
			default:
				return false;
		}
	};

	auto PlaceCursorToTarget = [&]() -> bool
	{
		switch (TargetType)
		{
			case EGridInventoryUiSlotType::Inventory:
				UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop InventoryTargetIndex Informative TargetIndex=%d"), TargetIndex);
				return InventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory();
			case EGridInventoryUiSlotType::MainHand:
				return OwningPartyPawn->TryEquipCursorItemToSelectedCharacterMainHand();
			case EGridInventoryUiSlotType::OffHand:
				return OwningPartyPawn->TryEquipCursorItemToSelectedCharacterOffHand();
			case EGridInventoryUiSlotType::Equipment:
			{
				const EGridEquipmentSlot TargetEquipmentSlot = ResolveEquipmentSlot(TargetType, TargetIndex);
				const bool bEquipped = InventoryComponent->TryEquipCursorItemToCharacterSlot(CharacterIndex, TargetEquipmentSlot);
				if (bEquipped)
				{
					SyncHeldVisualIfHandEquipmentSlot(TargetEquipmentSlot, TEXT("CursorToEquipment"));
				}
				return bEquipped;
			}
			case EGridInventoryUiSlotType::Cursor:
				return InventoryComponent->HasCursorItem();
			default:
				return false;
		}
	};

	if (!HasCurrentSourceItem())
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Drop Failed Reason=SourceEmpty"));
		RefreshInventory();
		return false;
	}

	bool bResult = false;
	bool bTookSourceToCursor = false;

	if (SourceType == EGridInventoryUiSlotType::Cursor)
	{
		bResult = PlaceCursorToTarget();
	}
	else
	{
		if (InventoryComponent->HasCursorItem())
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Drop Failed Reason=CursorOccupied"));
			RefreshInventory();
			return false;
		}

		bTookSourceToCursor = TakeSourceToCursor();
		if (!bTookSourceToCursor)
		{
			UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Drop Failed Reason=TakeSourceFailed"));
			RefreshInventory();
			return false;
		}

		bResult = TargetType == EGridInventoryUiSlotType::Cursor ? true : PlaceCursorToTarget();
	}

	if (!bResult && bTookSourceToCursor && InventoryComponent->HasCursorItem())
	{
		const bool bRecoveryResult = InventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory();
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI Drop Recovery Result=%s"), bRecoveryResult ? TEXT("true") : TEXT("false"));
	}

	UE_LOG(LogTemp, Log, TEXT("GridInventory UI Drop Result=%s"), bResult ? TEXT("true") : TEXT("false"));
	ValidateOwnership();
	RefreshInventory();
	return bResult;
}

UGridInventorySlotWidget* UGridInventoryWidget::FindRegisteredSlotWidget(EGridInventoryUiSlotType SlotType, int32 SlotIndex) const
{
	switch (SlotType)
	{
		case EGridInventoryUiSlotType::Inventory:
			for (UGridInventorySlotWidget* SlotWidget : RegisteredInventorySlots)
			{
				if (SlotWidget && SlotWidget->InventorySlotIndex == SlotIndex)
				{
					return SlotWidget;
				}
			}
			return nullptr;
		case EGridInventoryUiSlotType::Equipment:
			if (const TObjectPtr<UGridInventorySlotWidget>* SlotWidget = RegisteredEquipmentSlotWidgets.Find(static_cast<EGridEquipmentSlot>(SlotIndex)))
			{
				return SlotWidget->Get();
			}
			return nullptr;
		case EGridInventoryUiSlotType::MainHand:
			return MainHandSlotWidget;
		case EGridInventoryUiSlotType::OffHand:
			return OffHandSlotWidget;
		case EGridInventoryUiSlotType::Cursor:
			return CursorSlotWidget;
		default:
			return nullptr;
	}
}

bool UGridInventoryWidget::HandleInventorySlotClicked(int32 SlotIndex)
{
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI SlotClicked Slot=%d CursorBefore=false Result=false Reason=NoInventoryComponent"), SlotIndex);
		return false;
	}

	const bool bCursorBefore = InventoryComponent->HasCursorItem();
	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	const bool bResult = bCursorBefore
		? (SlotIndex == INDEX_NONE ? InventoryComponent->TryPlaceCursorItemInCharacterInventory(CharacterIndex)
								 : InventoryComponent->TryPlaceCursorItemInCharacterInventorySlot(CharacterIndex, SlotIndex))
		: InventoryComponent->TryTakeInventorySlotToCursor(CharacterIndex, SlotIndex);

	UE_LOG(LogTemp, Log, TEXT("GridInventory UI SlotClicked Slot=%d CursorBefore=%s Result=%s"), SlotIndex, bCursorBefore ? TEXT("true") : TEXT("false"),
		bResult ? TEXT("true") : TEXT("false"));
	return bResult;
}

bool UGridInventoryWidget::HandleMainHandClicked()
{
	return HandleEquipmentSlotClicked(EGridEquipmentSlot::MainHand);
}

bool UGridInventoryWidget::HandleOffHandClicked()
{
	return HandleEquipmentSlotClicked(EGridEquipmentSlot::OffHand);
}

bool UGridInventoryWidget::HandleEquipmentSlotClicked(EGridEquipmentSlot EquipmentSlot)
{
	if (!InventoryComponent || EquipmentSlot == EGridEquipmentSlot::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI EquipmentClicked Slot=%s CursorBefore=false Result=false Reason=MissingInventoryOrInvalidSlot"),
			GetContextEquipmentSlotName(EquipmentSlot));
		RefreshInventory();
		return false;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	const bool bCursorBefore = InventoryComponent->HasCursorItem();
	const bool bResult = bCursorBefore ? InventoryComponent->TryEquipCursorItemToCharacterSlot(CharacterIndex, EquipmentSlot)
									   : InventoryComponent->TryTakeEquipmentSlotToCursor(CharacterIndex, EquipmentSlot);

	UE_LOG(LogTemp, Log, TEXT("GridInventory UI EquipmentClicked Slot=%s CursorBefore=%s Result=%s"), GetContextEquipmentSlotName(EquipmentSlot),
		bCursorBefore ? TEXT("true") : TEXT("false"), bResult ? TEXT("true") : TEXT("false"));

	if (bResult)
	{
		if (OwningPartyPawn && IsGridInventoryHandEquipmentSlot(EquipmentSlot))
		{
			OwningPartyPawn->SyncHeldVisualFromSelectedCharacterEquipment();
			UE_LOG(LogTemp, Log, TEXT("GridInventory UI EquipmentClicked SyncHeldVisual Slot=%s"), GetContextEquipmentSlotName(EquipmentSlot));
		}
		RefreshInventory();
	}
	return bResult;
}

bool UGridInventoryWidget::HandleCursorReturnToInventoryClicked()
{
	if (!OwningPartyPawn || !InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("GridInventory UI CursorReturnToInventory CursorBefore=false Result=false Reason=MissingPawnOrInventoryComponent"));
		RefreshInventory();
		return false;
	}

	const bool bCursorBefore = InventoryComponent->HasCursorItem();
	if (!bCursorBefore)
	{
		UE_LOG(LogTemp, Log, TEXT("GridInventory UI CursorReturnToInventory CursorBefore=false Result=false"));
		RefreshInventory();
		return false;
	}

	const bool bResult = OwningPartyPawn->DebugPlaceCursorItemInSelectedInventory();
	UE_LOG(LogTemp, Log, TEXT("GridInventory UI CursorReturnToInventory CursorBefore=true Result=%s"), bResult ? TEXT("true") : TEXT("false"));

	RefreshInventory();
	return bResult;
}
