#include "UI/GridInventorySlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InputCoreTypes.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridInventoryWidget.h"

namespace
{
	FText GetEquipmentSlotDisplayName(EGridEquipmentSlot Slot)
	{
		switch (Slot)
		{
			case EGridEquipmentSlot::MainHand: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotMainHand", "Main directrice");
			case EGridEquipmentSlot::OffHand: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotOffHand", "Main secondaire");
			case EGridEquipmentSlot::Head: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotHead", "Tête");
			case EGridEquipmentSlot::Chest: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotChest", "Torse");
			case EGridEquipmentSlot::Legs: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotLegs", "Jambes");
			case EGridEquipmentSlot::Feet: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotFeet", "Pieds");
			case EGridEquipmentSlot::Amulet: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotAmulet", "Amulette");
			case EGridEquipmentSlot::Ring1: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotRing1", "Anneau I");
			case EGridEquipmentSlot::Ring2: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotRing2", "Anneau II");
			case EGridEquipmentSlot::Shoulders: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotShoulders", "Épaules");
			case EGridEquipmentSlot::Gloves: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotGloves", "Gants");
			case EGridEquipmentSlot::Belt: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotBelt", "Ceinture");
			case EGridEquipmentSlot::Cloak: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotCloak", "Cape");
			case EGridEquipmentSlot::Talisman: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotTalisman", "Talisman");
			case EGridEquipmentSlot::QuickSlot1: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotQuickSlot1", "Raccourci I");
			case EGridEquipmentSlot::QuickSlot2: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotQuickSlot2", "Raccourci II");
			case EGridEquipmentSlot::Shirt: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotShirt", "Chemise");
			case EGridEquipmentSlot::Bracers: return NSLOCTEXT("GridInventoryTooltip", "EquipmentSlotBracers", "Brassards");
			case EGridEquipmentSlot::None:
			default: return FText::GetEmpty();
		}
	}

	FText GetItemTypeDisplayName(EGridItemType ItemType)
	{
		switch (ItemType)
		{
			case EGridItemType::Torch: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeTorch", "Source de lumière");
			case EGridItemType::Weapon: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeWeapon", "Arme");
			case EGridItemType::Shield: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeShield", "Bouclier");
			case EGridItemType::Armor: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeArmor", "Armure");
			case EGridItemType::Jewelry: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeJewelry", "Bijou");
			case EGridItemType::Key: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeKey", "Clé");
			case EGridItemType::Gem: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeGem", "Gemme");
			case EGridItemType::Potion: return NSLOCTEXT("GridInventoryTooltip", "ItemTypePotion", "Potion");
			case EGridItemType::Scroll: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeScroll", "Parchemin");
			case EGridItemType::Book: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeBook", "Livre");
			case EGridItemType::Food: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeFood", "Nourriture");
			case EGridItemType::Component: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeComponent", "Composant");
			case EGridItemType::Quest: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeQuest", "Objet de quête");
			case EGridItemType::None:
			case EGridItemType::Misc:
			default: return NSLOCTEXT("GridInventoryTooltip", "ItemTypeGeneric", "Objet");
		}
	}

	FText GetItemDisplayName(const FGridItemInstance& Item, const UGridItemDefinitionAsset* Definition)
	{
		if (Definition && !Definition->DisplayName.IsEmpty())
		{
			return Definition->DisplayName;
		}
		if (!Item.DisplayName.IsEmpty())
		{
			return Item.DisplayName;
		}
		return Item.ItemDefinitionId.IsNone() ? FText::GetEmpty() : FText::FromName(Item.ItemDefinitionId);
	}

	FText BuildCompatibleEquipmentSlotsText(const UGridItemDefinitionAsset* Definition)
	{
		if (!Definition)
		{
			return FText::GetEmpty();
		}

		TArray<FString> SlotLabels;
		for (const EGridEquipmentSlot CompatibleSlot : Definition->CompatibleEquipmentSlots)
		{
			const FText SlotLabel = GetEquipmentSlotDisplayName(CompatibleSlot);
			if (!SlotLabel.IsEmpty())
			{
				SlotLabels.Add(SlotLabel.ToString());
			}
		}

		return SlotLabels.Num() > 0 ? FText::FromString(FString::Join(SlotLabels, TEXT(", "))) : FText::GetEmpty();
	}

	FText BuildUsageSummary(const FGridItemTooltipView& View)
	{
		TArray<FString> UsageParts;
		if (View.bEquippable)
		{
			UsageParts.Add(TEXT("Équipable"));
		}
		if (View.bReadable)
		{
			UsageParts.Add(TEXT("Lisible"));
		}
		if (View.bCanAssignToHotbar)
		{
			UsageParts.Add(TEXT("Barre d'action"));
		}
		if (View.bProvidesLight)
		{
			UsageParts.Add(View.bLightEnabled ? TEXT("Lumière allumée") : TEXT("Lumière éteinte"));
		}
		return UsageParts.Num() > 0 ? FText::FromString(FString::Join(UsageParts, TEXT(" • "))) : FText::GetEmpty();
	}

	FString FormatTooltipNumber(float Value, bool bInteger)
	{
		if (bInteger)
		{
			return FString::Printf(TEXT("%+.0f"), Value);
		}
		return FString::Printf(TEXT("%+.1f"), Value);
	}

	void AddTooltipStatLine(TArray<FGridItemTooltipStatLine>& Lines, FName StatId, const FText& Label, float ItemValue, float EquippedValue,
		bool bInteger, bool bHasComparison)
	{
		if (FMath::IsNearlyZero(ItemValue) && (!bHasComparison || FMath::IsNearlyZero(EquippedValue)))
		{
			return;
		}

		FGridItemTooltipStatLine& Line = Lines.AddDefaulted_GetRef();
		Line.StatId = StatId;
		Line.Label = Label;
		Line.ItemValue = ItemValue;
		Line.EquippedValue = EquippedValue;
		Line.Delta = bHasComparison ? ItemValue - EquippedValue : 0.0f;
		Line.bHasComparison = bHasComparison;
		if (bHasComparison && !FMath::IsNearlyZero(Line.Delta))
		{
			Line.DeltaState = Line.Delta > 0.0f ? EGridItemTooltipDeltaState::Positive : EGridItemTooltipDeltaState::Negative;
		}
		Line.ValueText = FText::FromString(FormatTooltipNumber(ItemValue, bInteger));
		if (bHasComparison)
		{
			Line.DeltaText = FMath::IsNearlyZero(Line.Delta)
				? FText::FromString(TEXT("±0"))
				: FText::FromString(FormatTooltipNumber(Line.Delta, bInteger));
		}
	}

	void AppendDefinitionStats(
		const UGridItemDefinitionAsset* Candidate, const UGridItemDefinitionAsset* Equipped, bool bComparison, TArray<FGridItemTooltipStatLine>& OutLines)
	{
		if (!Candidate)
		{
			return;
		}

		const FGridEquipmentStatBonus EmptyStats;
		const FGridDamageResistanceSet EmptyResistances;
		const FGridEquipmentStatBonus& CandidateStats = Candidate->EquipmentStatBonus;
		const FGridEquipmentStatBonus& EquippedStats = Equipped ? Equipped->EquipmentStatBonus : EmptyStats;
		const FGridDamageResistanceSet& CandidateRes = Candidate->EquipmentResistanceBonus;
		const FGridDamageResistanceSet& EquippedRes = Equipped ? Equipped->EquipmentResistanceBonus : EmptyResistances;

		AddTooltipStatLine(OutLines, TEXT("Strength"), NSLOCTEXT("GridInventoryTooltip", "StatStrength", "Force"),
			CandidateStats.StrengthBonus, EquippedStats.StrengthBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("Dexterity"), NSLOCTEXT("GridInventoryTooltip", "StatDexterity", "Dextérité"),
			CandidateStats.DexterityBonus, EquippedStats.DexterityBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("Constitution"), NSLOCTEXT("GridInventoryTooltip", "StatConstitution", "Constitution"),
			CandidateStats.ConstitutionBonus, EquippedStats.ConstitutionBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("Intelligence"), NSLOCTEXT("GridInventoryTooltip", "StatIntelligence", "Intelligence"),
			CandidateStats.IntelligenceBonus, EquippedStats.IntelligenceBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("Wisdom"), NSLOCTEXT("GridInventoryTooltip", "StatWisdom", "Sagesse"),
			CandidateStats.WisdomBonus, EquippedStats.WisdomBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("Charisma"), NSLOCTEXT("GridInventoryTooltip", "StatCharisma", "Charisme"),
			CandidateStats.CharismaBonus, EquippedStats.CharismaBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("MaxHealth"), NSLOCTEXT("GridInventoryTooltip", "StatHealth", "PV max"),
			CandidateStats.MaxHealthBonus, EquippedStats.MaxHealthBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("MaxMana"), NSLOCTEXT("GridInventoryTooltip", "StatMana", "Mana max"),
			CandidateStats.MaxManaBonus, EquippedStats.MaxManaBonus, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("CarryWeight"), NSLOCTEXT("GridInventoryTooltip", "StatCarry", "Charge max"),
			CandidateStats.CarryWeightBonus, EquippedStats.CarryWeightBonus, false, bComparison);
		AddTooltipStatLine(OutLines, TEXT("Armor"), NSLOCTEXT("GridInventoryTooltip", "StatArmor", "Armure physique"),
			CandidateStats.ArmorBonus, EquippedStats.ArmorBonus, true, bComparison);

		AddTooltipStatLine(OutLines, TEXT("PhysicalResistance"), NSLOCTEXT("GridInventoryTooltip", "ResPhysical", "Résistance physique"),
			CandidateRes.PhysicalResistance, EquippedRes.PhysicalResistance, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("FireResistance"), NSLOCTEXT("GridInventoryTooltip", "ResFire", "Résistance feu"),
			CandidateRes.FireResistance, EquippedRes.FireResistance, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("IceResistance"), NSLOCTEXT("GridInventoryTooltip", "ResIce", "Résistance glace"),
			CandidateRes.IceResistance, EquippedRes.IceResistance, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("LightningResistance"), NSLOCTEXT("GridInventoryTooltip", "ResLightning", "Résistance foudre"),
			CandidateRes.LightningResistance, EquippedRes.LightningResistance, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("PoisonResistance"), NSLOCTEXT("GridInventoryTooltip", "ResPoison", "Résistance poison"),
			CandidateRes.PoisonResistance, EquippedRes.PoisonResistance, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("HolyResistance"), NSLOCTEXT("GridInventoryTooltip", "ResHoly", "Résistance sacrée"),
			CandidateRes.HolyResistance, EquippedRes.HolyResistance, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("NecroticResistance"), NSLOCTEXT("GridInventoryTooltip", "ResNecrotic", "Résistance nécrotique"),
			CandidateRes.NecroticResistance, EquippedRes.NecroticResistance, true, bComparison);
		AddTooltipStatLine(OutLines, TEXT("ArcaneResistance"), NSLOCTEXT("GridInventoryTooltip", "ResArcane", "Résistance arcanique"),
			CandidateRes.ArcaneResistance, EquippedRes.ArcaneResistance, true, bComparison);
	}

	FText BuildStatSummary(const TArray<FGridItemTooltipStatLine>& Lines, bool bIncludeDelta)
	{
		TArray<FString> Parts;
		for (const FGridItemTooltipStatLine& Line : Lines)
		{
			if (bIncludeDelta && Line.bHasComparison)
			{
				Parts.Add(FString::Printf(TEXT("%s %s (Δ %s)"), *Line.Label.ToString(), *Line.ValueText.ToString(), *Line.DeltaText.ToString()));
			}
			else
			{
				Parts.Add(FString::Printf(TEXT("%s %s"), *Line.Label.ToString(), *Line.ValueText.ToString()));
			}
		}
		return Parts.Num() > 0 ? FText::FromString(FString::Join(Parts, TEXT("\n"))) : FText::GetEmpty();
	}

	FText BuildComparisonSummary(const TArray<FGridItemTooltipEquipmentComparison>& Comparisons)
	{
		TArray<FString> Sections;
		for (const FGridItemTooltipEquipmentComparison& Comparison : Comparisons)
		{
			const FString EquippedLabel = Comparison.bHasEquippedItem ? Comparison.EquippedItemName.ToString() : TEXT("vide");
			FString Section = FString::Printf(TEXT("%s — %s"), *Comparison.SlotLabel.ToString(), *EquippedLabel);
			const FText Stats = BuildStatSummary(Comparison.StatLines, true);
			if (!Stats.IsEmpty())
			{
				Section += TEXT("\n");
				Section += Stats.ToString();
			}
			Sections.Add(MoveTemp(Section));
		}
		return Sections.Num() > 0 ? FText::FromString(FString::Join(Sections, TEXT("\n\n"))) : FText::GetEmpty();
	}
}

void UGridInventorySlotWidget::InitializeInventorySlot(EGridInventoryUiSlotType InSlotType, int32 InInventorySlotIndex)
{
	SlotType = InSlotType;
	InventorySlotIndex = InInventorySlotIndex;
	EquipmentSlot = EGridEquipmentSlot::None;
	if (SlotType == EGridInventoryUiSlotType::MainHand)
	{
		EquipmentSlot = EGridEquipmentSlot::MainHand;
	}
	else if (SlotType == EGridInventoryUiSlotType::OffHand)
	{
		EquipmentSlot = EGridEquipmentSlot::OffHand;
	}
}

void UGridInventorySlotWidget::InitializeEquipmentSlot(EGridEquipmentSlot InEquipmentSlot)
{
	SlotType = EGridInventoryUiSlotType::Equipment;
	EquipmentSlot = InEquipmentSlot;
	InventorySlotIndex = static_cast<int32>(InEquipmentSlot);
}

void UGridInventorySlotWidget::SetItem(const FGridItemInstance& InItem)
{
	CachedItem = InItem;
	bHasItem = true;
	CachedIconTexture = nullptr;
	if (OwningInventoryWidget && OwningInventoryWidget->InventoryComponent)
	{
		if (const UGridItemDefinitionAsset* Definition = OwningInventoryWidget->InventoryComponent->FindItemDefinition(CachedItem.ItemDefinitionId))
		{
			CachedIconTexture = Definition->Icon.LoadSynchronous();
		}
	}

	RefreshSlotVisual();
}

void UGridInventorySlotWidget::ClearItem()
{
	CachedItem = FGridItemInstance();
	bHasItem = false;
	CachedIconTexture = nullptr;
	RefreshSlotVisual();
}

bool UGridInventorySlotWidget::HasItem() const
{
	return bHasItem;
}

FGridItemInstance UGridInventorySlotWidget::GetCachedItem() const
{
	return CachedItem;
}

const UGridItemDefinitionAsset* UGridInventorySlotWidget::GetItemDefinition() const
{
	if (!bHasItem || CachedItem.ItemDefinitionId.IsNone())
	{
		return nullptr;
	}

	const UGridInventoryWidget* InventoryWidget = OwningInventoryWidget.Get();
	const UGridPartyInventoryComponent* InventoryComponent = InventoryWidget ? InventoryWidget->InventoryComponent : nullptr;

	return InventoryComponent ? InventoryComponent->FindItemDefinition(CachedItem.ItemDefinitionId) : nullptr;
}

FString UGridInventorySlotWidget::GetDisplayNameText() const
{
	if (!bHasItem || CachedItem.ItemDefinitionId.IsNone())
	{
		return FString();
	}

	const UGridItemDefinitionAsset* Definition = GetItemDefinition();
	if (Definition && !Definition->DisplayName.IsEmpty())
	{
		return Definition->DisplayName.ToString();
	}
	if (!CachedItem.DisplayName.IsEmpty())
	{
		return CachedItem.DisplayName.ToString();
	}

	return CachedItem.ItemDefinitionId.ToString();
}

FString UGridInventorySlotWidget::GetQuantityText() const
{
	if (!bHasItem)
	{
		return FString();
	}

	return FString::Printf(TEXT("%d"), FMath::Max(1, CachedItem.Quantity));
}

FGridItemTooltipView UGridInventorySlotWidget::GetTooltipView() const
{
	FGridItemTooltipView View;
	if (!bHasItem || !CachedItem.IsValid())
	{
		return View;
	}

	const UGridItemDefinitionAsset* Definition = GetItemDefinition();
	View.bValid = true;
	View.ItemDefinitionId = CachedItem.ItemDefinitionId;
	View.DisplayName = GetItemDisplayName(CachedItem, Definition);
	View.Description = Definition ? Definition->Description : FText::GetEmpty();
	View.ItemType = Definition ? GetItemTypeDisplayName(Definition->ItemType) : NSLOCTEXT("GridInventoryTooltip", "ItemTypeUnknown", "Objet");
	View.Quantity = FMath::Max(1, CachedItem.Quantity);
	View.UnitWeight = FMath::Max(0.0f, Definition ? Definition->Weight : CachedItem.Weight);
	View.TotalWeight = View.UnitWeight * static_cast<float>(View.Quantity);
	View.WeightText = View.Quantity > 1
		? FText::FromString(FString::Printf(TEXT("%.1f × %d = %.1f"), View.UnitWeight, View.Quantity, View.TotalWeight))
		: FText::FromString(FString::Printf(TEXT("%.1f"), View.UnitWeight));
	View.CompatibleSlotsText = BuildCompatibleEquipmentSlotsText(Definition);

	if (!Definition)
	{
		return View;
	}

	View.bEquippable = Definition->CompatibleEquipmentSlots.Num() > 0;
	View.bReadable = UGridItemContextActionLibrary::IsItemReadable(CachedItem, Definition);
	View.bProvidesLight = Definition->HasLightEmitter();
	View.bLightEnabled = View.bProvidesLight && CachedItem.bLightsEnabled;

	FGridCombatActionDefinition InventoryAction;
	View.bCanAssignToHotbar =
		SlotType == EGridInventoryUiSlotType::Inventory && (Definition->BuildInventoryCombatActionDefinition(InventoryAction) || Definition->IsPhysicallyThrowable());


	AppendDefinitionStats(Definition, nullptr, false, View.StatLines);

	const UGridInventoryWidget* InventoryWidget = OwningInventoryWidget.Get();
	const UGridPartyInventoryComponent* InventoryComponent = InventoryWidget ? InventoryWidget->InventoryComponent : nullptr;
	if (!InventoryComponent)
	{
		return View;
	}

	const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	TSet<EGridEquipmentSlot> ComparedSlots;
	for (const EGridEquipmentSlot TargetSlot : Definition->CompatibleEquipmentSlots)
	{
		if (TargetSlot == EGridEquipmentSlot::None || ComparedSlots.Contains(TargetSlot))
		{
			continue;
		}
		ComparedSlots.Add(TargetSlot);

		FGridItemInstance EquippedItem;
		const bool bHasEquippedItem = InventoryComponent->GetEquippedItem(CharacterIndex, TargetSlot, EquippedItem);
		if (bHasEquippedItem && EquippedItem.RuntimeObjectId == CachedItem.RuntimeObjectId)
		{
			continue;
		}

		FGridItemTooltipEquipmentComparison& Comparison = View.EquipmentComparisons.AddDefaulted_GetRef();
		Comparison.EquipmentSlot = TargetSlot;
		Comparison.SlotLabel = GetEquipmentSlotDisplayName(TargetSlot);
		Comparison.bHasEquippedItem = bHasEquippedItem;

		const UGridItemDefinitionAsset* EquippedDefinition = nullptr;
		if (bHasEquippedItem)
		{
			EquippedDefinition = InventoryComponent->FindItemDefinition(EquippedItem.ItemDefinitionId);
			Comparison.EquippedItemName = GetItemDisplayName(EquippedItem, EquippedDefinition);
		}
		AppendDefinitionStats(Definition, EquippedDefinition, true, Comparison.StatLines);
	}
	return View;
}

FText UGridInventorySlotWidget::GetTooltipText() const
{
	const FGridItemTooltipView View = GetTooltipView();
	if (!View.bValid)
	{
		return FText::GetEmpty();
	}

	TArray<FString> Sections;
	Sections.Add(View.DisplayName.ToString());
	if (!View.ItemType.IsEmpty())
	{
		Sections.Add(View.ItemType.ToString());
	}
	if (!View.Description.IsEmpty())
	{
		Sections.Add(View.Description.ToString());
	}

	TArray<FString> Facts;
	if (View.Quantity > 1)
	{
		Facts.Add(FString::Printf(TEXT("Quantité : %d"), View.Quantity));
	}
	Facts.Add(FString::Printf(TEXT("Poids : %s"), *View.WeightText.ToString()));
	if (!View.CompatibleSlotsText.IsEmpty())
	{
		Facts.Add(FString::Printf(TEXT("Équipement : %s"), *View.CompatibleSlotsText.ToString()));
	}
	const FText UsageSummary = BuildUsageSummary(View);
	if (!UsageSummary.IsEmpty())
	{
		Facts.Add(UsageSummary.ToString());
	}
	if (Facts.Num() > 0)
	{
		Sections.Add(FString::Join(Facts, TEXT("\n")));
	}
	const FText StatSummary = BuildStatSummary(View.StatLines, false);
	if (!StatSummary.IsEmpty())
	{
		Sections.Add(StatSummary.ToString());
	}
	const FText ComparisonSummary = BuildComparisonSummary(View.EquipmentComparisons);
	if (!ComparisonSummary.IsEmpty())
	{
		Sections.Add(ComparisonSummary.ToString());
	}
	return FText::FromString(FString::Join(Sections, TEXT("\n\n")));
}

UTexture2D* UGridInventorySlotWidget::GetIconTexture() const
{
	return CachedIconTexture.Get();
}

void UGridInventorySlotWidget::HandleClicked()
{
	if (OwningInventoryWidget && SlotType == EGridInventoryUiSlotType::Inventory)
	{
		OwningInventoryWidget->HandleInventorySlotClicked(InventorySlotIndex, bSplitStackRequestedByClick);
		bSplitStackRequestedByClick = false;
		return;
	}

	OnSlotClicked.Broadcast(SlotType, InventorySlotIndex);
}

void UGridInventorySlotWidget::SetOwnerInventoryWidget(UGridInventoryWidget* InOwnerInventoryWidget)
{
	OwningInventoryWidget = InOwnerInventoryWidget;
}

bool UGridInventorySlotWidget::CanStartDrag() const
{
	return bDragEnabled && bHasItem && CachedItem.IsValid();
}

UGridInventoryDragDropOperation* UGridInventorySlotWidget::CreateDragDropOperation() const
{
	if (!CanStartDrag())
	{
		return nullptr;
	}

	UGridInventoryDragDropOperation* Operation = NewObject<UGridInventoryDragDropOperation>();
	if (!Operation)
	{
		return nullptr;
	}

	Operation->InitializeFromSlot(SlotType, InventorySlotIndex, CachedItem);
	Operation->SourceCharacterIndex = OwningInventoryWidget ? OwningInventoryWidget->GetSelectedCharacterIndex() : INDEX_NONE;
	Operation->DefaultDragVisual = nullptr;
	Operation->Pivot = EDragPivot::MouseDown;
	return Operation;
}

void UGridInventorySlotWidget::RefreshSlotVisual_Implementation()
{
}

FReply UGridInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	bSplitStackRequestedByClick = SlotType == EGridInventoryUiSlotType::Inventory && InMouseEvent.IsControlDown();

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bHasItem && OwningInventoryWidget)
	{
		OwningInventoryWidget->HandleItemSlotRightClicked(SlotType, InventorySlotIndex);
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanStartDrag())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UGridInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UGridInventoryDragDropOperation* Operation = CreateDragDropOperation();
	if (!Operation)
	{
		return;
	}

	Operation->bSplitStack = SlotType == EGridInventoryUiSlotType::Inventory && CachedItem.Quantity > 1 && InMouseEvent.IsControlDown();
	Operation->RequestedQuantity = Operation->bSplitStack ? 1 : 0;
	bSplitStackRequestedByClick = false;
	OutOperation = Operation;
	UE_LOG(LogTemp, Verbose, TEXT("GridInventory UI DragStarted Type=%s Slot=%d Item=%s RuntimeId=%s"), GetGridInventoryUiSlotTypeName(SlotType),
		InventorySlotIndex, *CachedItem.ItemDefinitionId.ToString(), *CachedItem.RuntimeObjectId.ToString());
}

bool UGridInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UGridInventoryDragDropOperation* Operation = Cast<UGridInventoryDragDropOperation>(InOperation);
	if (!Operation || !OwningInventoryWidget)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	return OwningInventoryWidget->HandleSlotDrop(
		Operation->SourceSlotType, Operation->SourceSlotIndex, SlotType, InventorySlotIndex, Operation->bSplitStack, Operation->RequestedQuantity);
}
