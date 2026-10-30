#include "UI/GridCombatHudWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "InputCoreTypes.h"
#include "Magic/GridPartySpellbookComponent.h"
#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridCombatActionPanelWidget.h"
#include "UI/GridCombatHotbarDragDropOperation.h"
#include "UI/GridInventoryDragDropOperation.h"
#include "UI/GridWeaponVisualUiResolver.h"
#include "UI/GridPersistentHudWidget.h"
#include "UI/UILocalizationText.h"

namespace
{
	constexpr float InitiativeRoundSeparatorWidth = 24.0f;
	constexpr float InitiativeRoundSeparatorHeight = 64.0f;
	constexpr float InitiativeRoundSeparatorTextAngle = -90.0f;

	FText GetHotbarShortcutText(int32 SlotIndex)
	{
		if (SlotIndex >= 0 && SlotIndex <= 8)
		{
			return FText::AsNumber(SlotIndex + 1);
		}
		switch (SlotIndex)
		{
			case 9:
				return FText::FromString(TEXT("0"));
			case 10:
				return FText::FromString(TEXT("'"));
			case 11:
				return FText::FromString(TEXT("^"));
			default:
				return FText::GetEmpty();
		}
	}

	bool DoesHotbarBindingMatchAction(const FGridCombatHotbarBinding& Binding, const FGridAvailableCombatAction& Action)
	{
		if (Binding.IsEmpty() || Binding.ActionId != Action.Definition.ActionId || Binding.SourcePolicy != Action.Definition.SourcePolicy ||
			Binding.SourceDefinitionId != Action.SourceDefinitionId)
		{
			return false;
		}

		if (Binding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment)
		{
			return Binding.PreferredSourceRuntimeId.IsValid() && Binding.PreferredSourceRuntimeId == Action.SourceRuntimeId;
		}
		if (Binding.PreferredSourceRuntimeId.IsValid())
		{
			return Binding.PreferredSourceRuntimeId == Action.SourceRuntimeId;
		}
		return true;
	}

	bool HaveSameHotbarIdentity(const FGridCombatHotbarBinding& Left, const FGridCombatHotbarBinding& Right)
	{
		return Left.ActionId == Right.ActionId && Left.SourcePolicy == Right.SourcePolicy && Left.SourceDefinitionId == Right.SourceDefinitionId &&
			Left.PreferredSourceRuntimeId == Right.PreferredSourceRuntimeId && Left.PreferredEquipmentSlot == Right.PreferredEquipmentSlot;
	}

	bool IsPartyStatusFeedbackType(EGridCombatLogEntryType Type)
	{
		return Type == EGridCombatLogEntryType::StatusApplied || Type == EGridCombatLogEntryType::StatusRefreshed ||
			Type == EGridCombatLogEntryType::StatusTicked || Type == EGridCombatLogEntryType::StatusExpired;
	}

	bool IsDirectHotbarActionSource(EGridCombatActionSourcePolicy SourcePolicy)
	{
		return SourcePolicy == EGridCombatActionSourcePolicy::Universal || SourcePolicy == EGridCombatActionSourcePolicy::Ability ||
			SourcePolicy == EGridCombatActionSourcePolicy::Spell;
	}

	FGridAvailableCombatAction BuildPhysicalThrowInventoryAction(const AGrimrockPartyPawn* PartyPawn,
		const UGridPartyInventoryComponent* InventoryComponent, int32 CharacterIndex, const FGridCombatHotbarBinding& Binding)
	{
		FGridAvailableCombatAction Action;
		Action.Definition.ActionId = Binding.ActionId;
		Action.Definition.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
		Action.Definition.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.Definition.ResolutionProfile = EGridCombatActionResolutionProfile::Interaction;
		Action.Definition.ActionPointCost = 0;
		Action.SourceDefinitionId = Binding.SourceDefinitionId;
		Action.CharacterIndex = CharacterIndex;
		Action.CurrentActionPointCost = 0;
		Action.CurrentSourceItemQuantityCost = 1;

		if (!IsValid(InventoryComponent) || !InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			Action.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.Character"));
			return Action;
		}
		Action.CharacterId = InventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
		UGridItemDefinitionAsset* Definition = InventoryComponent->FindItemDefinition(Binding.SourceDefinitionId);
		if (!IsValid(Definition) && IsValid(PartyPawn) && IsValid(PartyPawn->LevelRuntimeActor))
		{
			Definition = PartyPawn->LevelRuntimeActor->ResolveRuntimeItemDefinition(Binding.SourceDefinitionId);
		}
		if (!IsValid(Definition))
		{
			Action.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.ItemDefinition"));
			return Action;
		}
		Action.Definition.DisplayName = FText::FormatNamed(
			FUILocalizationText::Get(TEXT("Combat.Action.ThrowName")),
			TEXT("ItemName"), Definition->DisplayName);
		Action.Definition.Description = Definition->Description;
		Action.Definition.Icon = Definition->Icon;
		Action.CurrentSourceItemQuantity = InventoryComponent->CountItemDefinitionInCharacterInventory(CharacterIndex, Binding.SourceDefinitionId);
		if (Action.CurrentSourceItemQuantity <= 0)
		{
			Action.AvailabilityReason = EGridCombatActionAvailabilityReason::InsufficientSourceItems;
			Action.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.NoInventoryCopies"));
			return Action;
		}
		FGridInventoryCharacterSummary Summary;
		if (!InventoryComponent->GetCharacterSummary(CharacterIndex, Summary) || !Definition->CanBeThrownByStrength(Summary.Attributes.Strength))
		{
			Action.AvailabilityReason = EGridCombatActionAvailabilityReason::MissingRequirement;
			Action.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.ThrowRequirement"));
			return Action;
		}
		Action.bEnabled = true;
		Action.AvailabilityReason = EGridCombatActionAvailabilityReason::None;
		return Action;
	}


    /** ARMEMENT01.15.2: combat catalogue intentionally disables every action
     * during exploration. Project only supported self restorative QuickItems
     * into the persistent hotbar without opening a synthetic combat turn.
     */
    bool BuildExplorationSelfRestoration(
        const UGridPartyInventoryComponent* Inventory, int32 CharacterIndex,
        const FGridCombatHotbarBinding& Binding, FGridAvailableCombatAction& OutAction)
    {
        if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex) ||
            Binding.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem ||
            Binding.IsPhysicalThrowItemBinding())
        {
            return false;
        }

        const UGridItemDefinitionAsset* Definition = Inventory->FindItemDefinition(Binding.SourceDefinitionId);
        FGridCombatActionDefinition QuickItem;
        if (!IsValid(Definition) ||
            (Definition->ItemType != EGridItemType::Potion && Definition->ItemType != EGridItemType::Food) ||
            !Definition->BuildQuickItemCombatActionDefinition(QuickItem) ||
            QuickItem.ActionId != Binding.ActionId ||
            QuickItem.TargetingPolicy != EGridCombatTargetingPolicy::Self ||
            QuickItem.ResolutionProfile != EGridCombatActionResolutionProfile::Effect ||
            QuickItem.ResourceCosts.SourceItemQuantityCost != 1 ||
            QuickItem.ResourceCosts.ManaCost != 0 ||
            !QuickItem.EffectProfile.IsValid() ||
            !QuickItem.StatusApplications.IsEmpty() || !QuickItem.StatusRemovals.IsEmpty() ||
            !QuickItem.ArmorEffects.IsEmpty() || !QuickItem.MovementEffects.IsEmpty() ||
            !QuickItem.SurfaceEffects.IsEmpty() || !QuickItem.SurfaceConversions.IsEmpty() ||
            QuickItem.TrapEffect.bPlaceTrap)
        {
            return false;
        }

        const FGridCharacterInventoryState& Character =
            Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
        OutAction = FGridAvailableCombatAction();
        OutAction.Definition = QuickItem;
        OutAction.CharacterIndex = CharacterIndex;
        OutAction.CharacterId = Character.CharacterId;
        OutAction.SourceDefinitionId = Binding.SourceDefinitionId;
        OutAction.SourceEquipmentSlot = EGridEquipmentSlot::None;
        OutAction.CurrentActionPointCost = 0; // Exploration does not spend combat AP.
        OutAction.CurrentSourceItemQuantityCost = 1;
        OutAction.CurrentSourceItemQuantity = Inventory->CountItemDefinitionInCharacterInventory(
            CharacterIndex, Binding.SourceDefinitionId);

        if (OutAction.CurrentSourceItemQuantity == 0)
        {
            OutAction.AvailabilityReason = EGridCombatActionAvailabilityReason::InsufficientSourceItems;
            OutAction.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.NoInventoryCopies"));
            return true;
        }

        FGridInventoryCharacterSummary Summary;
        if (!Inventory->GetCharacterSummary(CharacterIndex, Summary) ||
            Summary.Resources.CurrentHealth <= 0)
        {
            OutAction.AvailabilityReason = EGridCombatActionAvailabilityReason::CharacterDefeated;
            OutAction.DisabledReason = NSLOCTEXT("GridHotbar", "SelfPotionCharacterUnavailable",
                "Ce personnage ne peut pas utiliser cet objet.");
            return true;
        }

        const bool bUsefulHealth = QuickItem.EffectProfile.ResolveHealthRestore(
            Summary.DerivedStats.MaxHealth) > 0 &&
            Summary.Resources.CurrentHealth < Summary.DerivedStats.MaxHealth;
        const bool bUsefulMana = QuickItem.EffectProfile.ResolveManaRestore(
            Summary.DerivedStats.MaxMana) > 0 &&
            Summary.Resources.CurrentMana < Summary.DerivedStats.MaxMana;
        if (!bUsefulHealth && !bUsefulMana)
        {
            OutAction.AvailabilityReason = EGridCombatActionAvailabilityReason::NoApplicableEffect;
            OutAction.DisabledReason = NSLOCTEXT("GridHotbar", "SelfPotionNoEffect",
                "Les ressources du personnage sont déjà au maximum.");
            return true;
        }

        OutAction.bEnabled = true;
        OutAction.AvailabilityReason = EGridCombatActionAvailabilityReason::None;
        return true;
    }

	void ApplyPrimaryAttackProjection(const TArray<FGridAvailableCombatAction>& AvailableActions, TArray<FGridCombatHudActionView>& ActionViews)
	{
		if (!ActionViews.IsValidIndex(FGridCombatHotbarBinding::PrimaryAttackSlotIndex))
		{
			return;
		}
		FGridCombatHudActionView& PrimaryView = ActionViews[FGridCombatHotbarBinding::PrimaryAttackSlotIndex];
		if (!PrimaryView.Binding.IsPrimaryAttackBinding())
		{
			return;
		}
		const FGridAvailableCombatAction* ResolvedPrimary = AvailableActions.FindByPredicate(
			[](const FGridAvailableCombatAction& Candidate)
			{
				return Candidate.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
					Candidate.SourceEquipmentSlot == EGridEquipmentSlot::MainHand &&
					Candidate.Definition.ResolutionProfile == EGridCombatActionResolutionProfile::Attack;
			});
		if (!ResolvedPrimary)
		{
			ResolvedPrimary = AvailableActions.FindByPredicate(
				[](const FGridAvailableCombatAction& Candidate)
				{
					return Candidate.Definition.ActionId == FName(TEXT("Attack_Unarmed")) &&
						Candidate.Definition.SourcePolicy == EGridCombatActionSourcePolicy::Universal;
				});
		}
		if (!ResolvedPrimary)
		{
			return;
		}
		PrimaryView.Action = *ResolvedPrimary;
		PrimaryView.bHasBinding = true;
		PrimaryView.bResolved = true;
		PrimaryView.DisabledReason = ResolvedPrimary->bEnabled ? FText::GetEmpty() : ResolvedPrimary->DisabledReason;
	}

	EGridEquipmentSlot ResolveDraggedEquipmentSlot(EGridInventoryUiSlotType SlotType, int32 SlotIndex)
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

	FText FormatActionCost(const FGridAvailableCombatAction& Action)
	{
		if (Action.CurrentManaCost > 0)
		{
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Combat.Cost.ActionPointsMana")),
				TEXT("ActionPoints"), Action.CurrentActionPointCost,
				TEXT("Mana"), Action.CurrentManaCost);
		}
		return FText::FormatNamed(
			FUILocalizationText::Get(TEXT("Combat.Cost.ActionPoints")),
			TEXT("ActionPoints"), Action.CurrentActionPointCost);
	}

	FText FormatActionCostAndQuantity(const FGridAvailableCombatAction& Action)
	{
		const FText Cost = FormatActionCost(Action);
		if (Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem && Action.CurrentSourceItemQuantityCost > 0)
		{
			if (Action.CurrentSourceItemQuantityCost > 1)
			{
				return FText::FormatNamed(
					FUILocalizationText::Get(TEXT("Combat.Cost.WithQuantityConsumed")),
					TEXT("Cost"), Cost,
					TEXT("Quantity"), Action.CurrentSourceItemQuantity,
					TEXT("Consumed"), Action.CurrentSourceItemQuantityCost);
			}
			return FText::FormatNamed(
				FUILocalizationText::Get(TEXT("Combat.Cost.WithQuantity")),
				TEXT("Cost"), Cost,
				TEXT("Quantity"), Action.CurrentSourceItemQuantity);
		}
		return Cost;
	}

	FText FormatResourceCurrentAndMaximum(const TCHAR* Key, int32 Current, int32 Maximum)
	{
		return FText::FormatNamed(
			FUILocalizationText::Get(Key),
			TEXT("Current"), FMath::Max(0, Current),
			TEXT("Maximum"), FMath::Max(0, Maximum));
	}

	FText BuildAssignedHotbarToolTip(const FGridCombatHudActionView& ActionView)
	{
		const FGridCombatActionDefinition& Definition = ActionView.Action.Definition;
		TArray<FText> Sections;

		FText DisplayName = Definition.DisplayName;
		if (DisplayName.IsEmpty())
		{
			DisplayName = FText::FromName(!Definition.ActionId.IsNone() ? Definition.ActionId : ActionView.Binding.ActionId);
		}
		Sections.Add(DisplayName);

		if (ActionView.bResolved)
		{
			Sections.Add(
				FText::FormatNamed(
					FUILocalizationText::Get(TEXT("Combat.Hotbar.Tooltip.Cost")),
					TEXT("Cost"), FormatActionCostAndQuantity(ActionView.Action)));
		}
		if (!Definition.Description.IsEmpty())
		{
			Sections.Add(Definition.Description);
		}
		if (!ActionView.bResolved || !ActionView.Action.bEnabled)
		{
			const FText Reason = ActionView.DisabledReason.IsEmpty()
				? FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.Stale"))
				: ActionView.DisabledReason;
			Sections.Add(
				FText::FormatNamed(
					FUILocalizationText::Get(TEXT("Combat.Hotbar.Tooltip.Unavailable")),
					TEXT("Reason"), Reason));
		}

		TArray<FText> ShortcutInstructions;
		ShortcutInstructions.Add(FUILocalizationText::Get(TEXT("Combat.Hotbar.Tooltip.RemoveShortcut")));
		ShortcutInstructions.Add(FUILocalizationText::Get(TEXT("Combat.Hotbar.Tooltip.KeepsItem")));
		Sections.Add(FText::Join(FText::FromString(TEXT("\n")), ShortcutInstructions));
		return FText::Join(FText::FromString(TEXT("\n\n")), Sections);
	}

}

void FGridCombatHudViewModelBuilder::BuildPartyMembers(
	int32 ActiveCharacterCount, const TArray<FGridPlayerCharacterTurnState>& TurnStates, TArray<FGridCombatHudPartyMemberView>& OutMembers)
{
	OutMembers.Reset(PartyPanelCount);
	for (int32 CharacterIndex = 0; CharacterIndex < PartyPanelCount; ++CharacterIndex)
	{
		FGridCombatHudPartyMemberView& Member = OutMembers.AddDefaulted_GetRef();
		Member.CharacterIndex = CharacterIndex;
		Member.bPresent = CharacterIndex < ActiveCharacterCount;

		const FGridPlayerCharacterTurnState* TurnState = TurnStates.FindByPredicate(
			[CharacterIndex](const FGridPlayerCharacterTurnState& Candidate)
			{
				return Candidate.CharacterIndex == CharacterIndex;
			});
		if (TurnState)
		{
			Member.TurnState = TurnState->State;
			Member.RemainingActionPoints = TurnState->RemainingActionPoints;
			Member.MaximumActionPoints = TurnState->MaximumActionPoints;
			Member.bActive = TurnState->State == EGridCombatantTurnState::Active;
		}
	}
}

void FGridCombatHudViewModelBuilder::BuildHotbarActions(
	const TArray<FGridCombatHotbarBinding>& Bindings, const TArray<FGridAvailableCombatAction>& AvailableActions, TArray<FGridCombatHudActionView>& OutActions)
{
	OutActions.Reset(Bindings.Num());
	for (int32 SlotIndex = 0; SlotIndex < Bindings.Num(); ++SlotIndex)
	{
		FGridCombatHudActionView& ActionView = OutActions.AddDefaulted_GetRef();
		ActionView.HotbarSlotIndex = SlotIndex;
		ActionView.ShortcutText = GetHotbarShortcutText(SlotIndex);
		ActionView.Binding.Reset(SlotIndex);
		if (!Bindings.IsValidIndex(SlotIndex) || !Bindings[SlotIndex].IsValid() || Bindings[SlotIndex].IsEmpty())
		{
			continue;
		}

		ActionView.Binding = Bindings[SlotIndex];
		ActionView.Binding.SlotIndex = SlotIndex;
		ActionView.bHasBinding = true;
		const FGridAvailableCombatAction* ResolvedAction = AvailableActions.FindByPredicate(
			[&ActionView](const FGridAvailableCombatAction& Candidate)
			{
				return DoesHotbarBindingMatchAction(ActionView.Binding, Candidate);
			});
		if (ResolvedAction)
		{
			ActionView.Action = *ResolvedAction;
			ActionView.bResolved = true;
			ActionView.DisabledReason = ResolvedAction->bEnabled ? FText::GetEmpty() : ResolvedAction->DisabledReason;
			continue;
		}

		ActionView.Action.Definition.ActionId = ActionView.Binding.ActionId;
		ActionView.Action.Definition.SourcePolicy = ActionView.Binding.SourcePolicy;
		ActionView.Action.SourceDefinitionId = ActionView.Binding.SourceDefinitionId;
		ActionView.Action.SourceRuntimeId = ActionView.Binding.PreferredSourceRuntimeId;
		ActionView.Action.SourceEquipmentSlot = ActionView.Binding.PreferredEquipmentSlot;
	}
}

FGridCombatHudMobilityView FGridCombatHudViewModelBuilder::BuildMobility(const FGridPartyMobilityState& MobilityState)
{
	FGridCombatHudMobilityView View;
	View.RemainingMobilityActionPoints = MobilityState.RemainingMobilityActionPoints;
	View.MaximumMobilityActionPoints = MobilityState.MaximumMobilityActionPoints;
	View.DisplayText = FormatResourceCurrentAndMaximum(TEXT("Combat.Resource.MobilityActionPoints"), View.RemainingMobilityActionPoints, View.MaximumMobilityActionPoints);
	return View;
}

void FGridCombatHudViewModelBuilder::BuildInitiative(
	const TArray<FGridInitiativePreviewEntry>& InitiativePreview, TArray<FGridCombatHudInitiativeView>& OutInitiative, int32 MaximumVisibleEntries)
{
	const int32 VisibleCount = FMath::Min(InitiativePreview.Num(), FMath::Max(0, MaximumVisibleEntries));
	OutInitiative.Reset(VisibleCount);
	for (int32 Index = 0; Index < VisibleCount; ++Index)
	{
		FGridCombatHudInitiativeView& Entry = OutInitiative.AddDefaulted_GetRef();
		const FGridInitiativePreviewEntry& Preview = InitiativePreview[Index];
		Entry.Combatant = Preview.Combatant;
		Entry.bActive = Preview.bIsActive;
		Entry.RoundNumber = Preview.RoundNumber;
		Entry.ActivationIndex = Preview.ActivationIndex;
		Entry.bStartsNewRound = Preview.bStartsNewRound;
		Entry.HealthPercent = CalculateHealthPercent(Entry.Combatant.CurrentHealth, Entry.Combatant.MaximumHealth);
	}
}

float FGridCombatHudViewModelBuilder::CalculateHealthPercent(int32 CurrentHealth, int32 MaximumHealth)
{
	if (MaximumHealth <= 0)
	{
		return 0.0f;
	}

	return FMath::Clamp(static_cast<float>(CurrentHealth) / static_cast<float>(MaximumHealth), 0.0f, 1.0f);
}

void UGridCombatHudActionWidget::InitializeAction(UGridCombatHudWidget* InOwnerHud, const FGridCombatHudActionView& InView)
{
	OwnerHud = InOwnerHud;
	View = InView;
	EnsureQuantityBadge();
	RefreshWidgets();
}

void UGridCombatHudActionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureQuantityBadge();
	RefreshWidgets();
}

void UGridCombatHudActionWidget::NativeDestruct()
{
	bLeftMousePressed = false;
	bDragDetected = false;
	OwnerHud = nullptr;
	Super::NativeDestruct();
}

bool UGridCombatHudActionWidget::TryExecuteAction()
{
	if (!IsValid(OwnerHud) || !View.bHasBinding)
	{
		return false;
	}

	FGridCombatActionRequestResult Result;
	return OwnerHud->RequestHotbarSlot(View.HotbarSlotIndex, Result);
}

void UGridCombatHudActionWidget::EnsureQuantityBadge()
{
	if (Text_Quantity || !WidgetTree)
	{
		return;
	}

	UOverlay* ContentOverlay = Button_Action ? Cast<UOverlay>(Button_Action->GetContent()) : nullptr;
	if (!ContentOverlay)
	{
		ContentOverlay = Cast<UOverlay>(WidgetTree->FindWidget(TEXT("Overlay_ActionContent")));
	}
	if (!ContentOverlay && Text_ShortcutNumber)
	{
		ContentOverlay = Cast<UOverlay>(Text_ShortcutNumber->GetParent());
	}
	if (!ContentOverlay)
	{
		return;
	}

	UBorder* QuantityBackground = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), MakeUniqueObjectName(WidgetTree, UBorder::StaticClass(), TEXT("Border_Quantity_Runtime")));
	UTextBlock* QuantityText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), TEXT("Text_Quantity_Runtime")));
	if (!QuantityBackground || !QuantityText)
	{
		return;
	}

	QuantityBackground->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.82f));
	QuantityBackground->SetPadding(FMargin(4.0f, 1.0f, 4.0f, 1.0f));
	QuantityBackground->SetHorizontalAlignment(HAlign_Center);
	QuantityBackground->SetVerticalAlignment(VAlign_Center);
	QuantityBackground->SetVisibility(ESlateVisibility::Collapsed);

	FSlateFontInfo QuantityFont = QuantityText->GetFont();
	QuantityFont.Size = 15;
	QuantityText->SetFont(QuantityFont);
	QuantityText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	QuantityText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	QuantityText->SetShadowColorAndOpacity(FLinearColor::Black);
	QuantityText->SetJustification(ETextJustify::Center);
	QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
	QuantityBackground->AddChild(QuantityText);

	UOverlaySlot* QuantitySlot = ContentOverlay->AddChildToOverlay(QuantityBackground);
	if (QuantitySlot)
	{
		QuantitySlot->SetHorizontalAlignment(HAlign_Right);
		QuantitySlot->SetVerticalAlignment(VAlign_Top);
		QuantitySlot->SetPadding(FMargin(0.0f, 5.0f, 5.0f, 0.0f));
	}

	Text_Quantity = QuantityText;
	QuantityBadgeContainer = QuantityBackground;
}

void UGridCombatHudActionWidget::RefreshWidgets()
{
	if (Button_Action)
	{
		Button_Action->SetIsEnabled(true);
		if (!View.bHasBinding)
		{
			Button_Action->SetToolTipText(
				FUILocalizationText::Get(TEXT("Combat.Hotbar.EmptyTooltip")));
		}
		else
		{
			Button_Action->SetToolTipText(BuildAssignedHotbarToolTip(View));
		}
	}
	if (Image_ActionIcon)
	{
		if (!View.bHasBinding || View.Action.Definition.Icon.IsNull())
		{
			Image_ActionIcon->SetBrushFromTexture(nullptr);
			Image_ActionIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Image_ActionIcon->SetBrushFromSoftTexture(View.Action.Definition.Icon, false);
			Image_ActionIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
	if (Text_ShortcutNumber)
	{
		Text_ShortcutNumber->SetText(View.ShortcutText);
		Text_ShortcutNumber->SetVisibility(View.ShortcutText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* ShortcutSlot = Cast<UOverlaySlot>(Text_ShortcutNumber->Slot))
		{
			ShortcutSlot->SetHorizontalAlignment(HAlign_Right);
			ShortcutSlot->SetVerticalAlignment(VAlign_Bottom);
			ShortcutSlot->SetPadding(FMargin(0.0f, 0.0f, 5.0f, 5.0f));
		}
	}
	if (Text_Quantity)
	{
		const bool bShowQuantity = View.bHasBinding && View.bResolved &&
			View.Action.Definition.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem && View.Action.CurrentSourceItemQuantityCost > 0 &&
			View.Action.CurrentSourceItemQuantity > 0;
		Text_Quantity->SetText(bShowQuantity
				? FText::FromString(FString::Printf(TEXT("x%d"), View.Action.CurrentSourceItemQuantity))
				: FText::GetEmpty());
		Text_Quantity->SetVisibility(bShowQuantity ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (QuantityBadgeContainer)
		{
			QuantityBadgeContainer->SetVisibility(bShowQuantity ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	SetRenderOpacity(!View.bHasBinding               ? FMath::Clamp(EmptySlotOpacity, 0.0f, 1.0f)
			: View.bResolved && View.Action.bEnabled ? 1.0f
													 : FMath::Clamp(UnavailableSlotOpacity, 0.0f, 1.0f));
}

FReply UGridCombatHudActionWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IsValid(OwnerHud))
	{
		return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && View.bHasBinding)
	{
		bLeftMousePressed = false;
		bDragDetected = false;
		return OwnerHud->ClearHotbarSlot(View.HotbarSlotIndex) ? FReply::Handled() : FReply::Unhandled();
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && View.bHasBinding)
	{
		bLeftMousePressed = true;
		bDragDetected = false;
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGridCombatHudActionWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bLeftMousePressed)
	{
		const bool bShouldExecute = !bDragDetected;
		bLeftMousePressed = false;
		bDragDetected = false;
		if (bShouldExecute)
		{
			TryExecuteAction();
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UGridCombatHudActionWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	bDragDetected = true;
	bLeftMousePressed = false;
	if (!IsValid(OwnerHud) || !View.bHasBinding)
	{
		return;
	}

	UGridCombatHotbarDragDropOperation* Operation = NewObject<UGridCombatHotbarDragDropOperation>(this);
	if (!Operation)
	{
		return;
	}
	Operation->InitializeFromHotbarSlot(View.Action.CharacterIndex, View.HotbarSlotIndex, View.Binding);
	Operation->DefaultDragVisual = nullptr;
	Operation->Pivot = EDragPivot::MouseDown;
	OutOperation = Operation;
}

bool UGridCombatHudActionWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (IsValid(OwnerHud) && OwnerHud->HandleHotbarDrop(View.HotbarSlotIndex, InOperation))
	{
		return true;
	}
	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UGridCombatHudInitiativeSlotWidget::InitializeInitiativeSlot(const FGridCombatHudInitiativeView& InView)
{
	View = InView;
	EnsureHealthProgressBar();
	RefreshWidgets();
}

void UGridCombatHudInitiativeSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureHealthProgressBar();
	RefreshWidgets();
}

void UGridCombatHudInitiativeSlotWidget::EnsureHealthProgressBar()
{
	if (ProgressBar_Health || !Text_Health || !WidgetTree)
	{
		return;
	}

	UVerticalBox* TextContainer = Cast<UVerticalBox>(Text_Health->GetParent());
	if (!TextContainer)
	{
		return;
	}

	USizeBox* HealthBarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox_HealthBar_Runtime"));
	UProgressBar* HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ProgressBar_Health_Runtime"));
	if (!HealthBarSizeBox || !HealthBar)
	{
		return;
	}

	HealthBarSizeBox->SetHeightOverride(6.0f);
	HealthBarSizeBox->AddChild(HealthBar);
	UVerticalBoxSlot* HealthBarSlot = TextContainer->AddChildToVerticalBox(HealthBarSizeBox);
	if (HealthBarSlot)
	{
		HealthBarSlot->SetPadding(FMargin(2.0f, 1.0f, 2.0f, 0.0f));
		HealthBarSlot->SetHorizontalAlignment(HAlign_Fill);
	}
	ProgressBar_Health = HealthBar;
}

void UGridCombatHudInitiativeSlotWidget::RefreshWidgets()
{
	const bool bPartyPortrait = View.Combatant.IsPartyMember();
	if (Image_LevelUpAlert)
	{
		Image_LevelUpAlert->SetVisibility(
			bPartyPortrait && View.bHasUnspentProgressionPoints ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Image_WeightAlert)
	{
		Image_WeightAlert->SetVisibility(
			bPartyPortrait && View.bOverloaded ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (Image_Portrait)
	{
		if (View.Combatant.Portrait.IsNull())
		{
			Image_Portrait->SetBrushFromTexture(nullptr);
		}
		else
		{
			Image_Portrait->SetBrushFromSoftTexture(View.Combatant.Portrait, false);
		}
	}
	if (Text_Name)
	{
		Text_Name->SetText(View.Combatant.DisplayName);
	}
	if (Text_Health)
	{
		Text_Health->SetText(FormatResourceCurrentAndMaximum(TEXT("Combat.Resource.Health"), View.Combatant.CurrentHealth, View.Combatant.MaximumHealth));
	}
	if (ProgressBar_Health)
	{
		ProgressBar_Health->SetPercent(View.HealthPercent);
		ProgressBar_Health->SetFillColorAndOpacity(HealthBarFillColor);
		ProgressBar_Health->SetVisibility(ESlateVisibility::HitTestInvisible);
		ProgressBar_Health->SetToolTipText(FormatResourceCurrentAndMaximum(TEXT("Combat.Resource.Health"), View.Combatant.CurrentHealth, View.Combatant.MaximumHealth));
	}
	if (Text_State)
	{
		Text_State->SetText(View.bActive ? FUILocalizationText::Get(TEXT("Combat.State.Active")) : FText::GetEmpty());
		Text_State->SetVisibility(View.bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Border_Active)
	{
		Border_Active->SetVisibility(View.bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	SetRenderScale(FVector2D(View.bActive ? FMath::Max(1.0f, ActiveScale) : 1.0f));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

void UGridCombatHudWidget::InitializeCombatHud(AGrimrockPartyPawn* InPartyPawn, UGridTurnManagerComponent* InTurnManager)
{
	UnbindFromSources();
	bCombatActionTargetingActive = false;
	PendingTargetingActionView = FGridCombatHudActionView();
	TargetingPreview = FGridCombatActionTargetingPreview();
	PartyPawn = InPartyPawn;
	InventoryComponent = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent : nullptr;
	TurnManagerComponent = InTurnManager;
	if (!IsValid(TurnManagerComponent) && IsValid(PartyPawn) && IsValid(PartyPawn->LevelRuntimeActor))
	{
		TurnManagerComponent = PartyPawn->LevelRuntimeActor->FindComponentByClass<UGridTurnManagerComponent>();
	}
	BindToSources();
	EnsurePartyMemberPanels();
	RefreshFromSources();
}

void UGridCombatHudWidget::RefreshFromSources()
{
	View = FGridCombatHudView();
	View.bCombatActive = IsValid(TurnManagerComponent) && TurnManagerComponent->bCombatActive;

	TArray<FGridPlayerCharacterTurnState> TurnStates;
	const int32 ActiveCharacterCount = IsValid(InventoryComponent) ? InventoryComponent->GetActiveCharacterCount() : 0;
	for (int32 CharacterIndex = 0; CharacterIndex < FGridCombatHudViewModelBuilder::PartyPanelCount; ++CharacterIndex)
	{
		FGridPlayerCharacterTurnState TurnState;
		if (IsValid(TurnManagerComponent) && TurnManagerComponent->GetPlayerCharacterTurnState(CharacterIndex, TurnState))
		{
			TurnStates.Add(TurnState);
		}
	}
	FGridCombatHudViewModelBuilder::BuildPartyMembers(ActiveCharacterCount, TurnStates, View.PartyMembers);

	UGridStatusEffectLifecycleSubsystem* StatusLifecycle = nullptr;
	if (IsValid(PartyPawn))
	{
		if (UWorld* World = PartyPawn->GetWorld())
		{
			StatusLifecycle = World->GetSubsystem<UGridStatusEffectLifecycleSubsystem>();
		}
	}

	for (FGridCombatHudPartyMemberView& Member : View.PartyMembers)
	{
		if (!Member.bPresent)
		{
			continue;
		}

		Member.bCanAct = IsValid(TurnManagerComponent) && TurnManagerComponent->CanCharacterAct(Member.CharacterIndex);

		FGridInventoryCharacterSummary Summary;
		if (!IsValid(InventoryComponent) || !InventoryComponent->GetCharacterSummary(Member.CharacterIndex, Summary))
		{
			continue;
		}

		Member.DisplayName = Summary.DisplayName.IsEmpty()
			? FText::FromString(FString::Printf(TEXT("Hero_%02d"), Member.CharacterIndex + 1))
			: Summary.DisplayName;
		Member.Portrait = Summary.Portrait;
		Member.bHasUnspentProgressionPoints = Summary.bHasUnspentProgressionPoints;
		Member.bOverloaded = Summary.bOverloaded;
		Member.CurrentHealth = Summary.Resources.CurrentHealth;
		Member.MaximumHealth = Summary.DerivedStats.MaxHealth;
		Member.CurrentMana = Summary.Resources.CurrentMana;
		Member.MaximumMana = Summary.DerivedStats.MaxMana;

		if (InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(Member.CharacterIndex))
		{
			const FGridCharacterInventoryState& Character = InventoryComponent->PartyInventoryState.ActiveCharacters[Member.CharacterIndex];
			FGridStatusEffectPresentationBuilder::Build(Character.StatusEffects, Member.StatusEffects);
			Member.StatusSummary = FGridStatusEffectPresentationBuilder::BuildSummary(Member.StatusEffects);
		}

		if (IsValid(StatusLifecycle))
		{
			const FGridCombatLogEntry& Feedback = StatusLifecycle->LastStatusEffectFeedback;
			if (Feedback.TargetCharacterIndex == Member.CharacterIndex && IsPartyStatusFeedbackType(Feedback.Type))
			{
				Member.LatestStatusFeedback = Feedback.Message;
			}
		}
	}

	FGridCombatantInitiativeEntry ActiveCombatant;
	TArray<FGridAvailableCombatAction> AvailableActions;
	TArray<FGridCombatHotbarBinding> HotbarBindings;
	if (IsValid(InventoryComponent) && InventoryComponent->IsValidCharacterIndex(InventoryComponent->GetSelectedCharacterIndex()))
	{
		View.ActiveCharacterIndex = InventoryComponent->GetSelectedCharacterIndex();
	}
	if (View.bCombatActive && IsValid(TurnManagerComponent) && TurnManagerComponent->GetActiveCombatant(ActiveCombatant) &&
		ActiveCombatant.Side == EGridCombatantSide::Party)
	{
		View.ActiveCharacterIndex = ActiveCombatant.CharacterIndex;
		if (View.PartyMembers.IsValidIndex(View.ActiveCharacterIndex))
		{
			View.PartyMembers[View.ActiveCharacterIndex].bActive = true;
		}
	}
	if (View.ActiveCharacterIndex != INDEX_NONE)
	{
		if (IsValid(TurnManagerComponent))
		{
			TurnManagerComponent->GetAvailableCombatActions(View.ActiveCharacterIndex, AvailableActions);
		}
		if (IsValid(InventoryComponent))
		{
			const int32 HotbarSlotCount = InventoryComponent->GetCharacterCombatHotbarSlotCount(View.ActiveCharacterIndex);
			HotbarBindings.SetNum(HotbarSlotCount);
			for (int32 SlotIndex = 0; SlotIndex < HotbarSlotCount; ++SlotIndex)
			{
				HotbarBindings[SlotIndex].Reset(SlotIndex);
				InventoryComponent->GetCharacterCombatHotbarBinding(View.ActiveCharacterIndex, SlotIndex, HotbarBindings[SlotIndex]);
			}
		}
	}
	for (const FGridCombatHotbarBinding& Binding : HotbarBindings)
	{
		if (Binding.IsPhysicalThrowItemBinding())
		{
			AvailableActions.Add(BuildPhysicalThrowInventoryAction(PartyPawn, InventoryComponent, View.ActiveCharacterIndex, Binding));
		}
	}
	// In exploration, overlay only the supported self restorative actions.
	// Preserve the combat catalogue, other action types, and physical throws.
	if (!View.bCombatActive && IsValid(InventoryComponent) && View.ActiveCharacterIndex != INDEX_NONE)
	{
		for (const FGridCombatHotbarBinding& Binding : HotbarBindings)
		{
			FGridAvailableCombatAction Exploration;
			if (!BuildExplorationSelfRestoration(InventoryComponent, View.ActiveCharacterIndex, Binding, Exploration))
			{
				continue;
			}
			AvailableActions.RemoveAll([&Exploration](const FGridAvailableCombatAction& Candidate)
			{
				return Candidate.MatchesSource(
					Exploration.Definition.ActionId, Exploration.Definition.SourcePolicy,
					Exploration.SourceDefinitionId, Exploration.SourceEquipmentSlot);
			});
			AvailableActions.Add(MoveTemp(Exploration));
		}
	}
	FGridCombatHudViewModelBuilder::BuildHotbarActions(HotbarBindings, AvailableActions, View.Actions);
	ApplyPrimaryAttackProjection(AvailableActions, View.Actions);
	for (FGridCombatHudActionView& ActionView : View.Actions)
	{
		ActionView.Action.CharacterIndex = View.ActiveCharacterIndex;
	}
	EnrichHotbarActionPresentation();
	ValidateCombatActionTargetingState();

	View.Mobility = FGridCombatHudViewModelBuilder::BuildMobility(
		IsValid(TurnManagerComponent) ? TurnManagerComponent->GetPartyMobilityState() : FGridPartyMobilityState());

	TArray<FGridInitiativePreviewEntry> InitiativePreview;
	if (IsValid(TurnManagerComponent))
	{
		TurnManagerComponent->GetInitiativePreview(InitiativePreview, FMath::Clamp(VisibleInitiativeSlotCount, 7, 10));
	}
	FGridCombatHudViewModelBuilder::BuildInitiative(InitiativePreview, View.Initiative, FMath::Clamp(VisibleInitiativeSlotCount, 7, 10));

	for (FGridCombatHudInitiativeView& InitiativeEntry : View.Initiative)
	{
		if (!InitiativeEntry.Combatant.IsPartyMember() || !IsValid(InventoryComponent))
		{
			continue;
		}
		FGridInventoryCharacterSummary Summary;
		if (InventoryComponent->GetCharacterSummary(InitiativeEntry.Combatant.CharacterIndex, Summary))
		{
			InitiativeEntry.bHasUnspentProgressionPoints = Summary.bHasUnspentProgressionPoints;
			InitiativeEntry.bOverloaded = Summary.bOverloaded;
		}
	}

	View.bCanEndTurn = IsValid(TurnManagerComponent) && TurnManagerComponent->CanEndActivePlayerTurn();
	if (!View.bCanEndTurn)
	{
		if (!View.bCombatActive)
		{
			View.EndTurnDisabledReason = FUILocalizationText::Get(TEXT("Combat.EndTurn.InactiveCombat"));
		}
		else if (!IsValid(TurnManagerComponent) || !TurnManagerComponent->GetActiveCombatant(ActiveCombatant) ||
			ActiveCombatant.Side != EGridCombatantSide::Party)
		{
			View.EndTurnDisabledReason = FUILocalizationText::Get(TEXT("Combat.EndTurn.EnemyTurn"));
		}
		else if (TurnManagerComponent->IsPartyMotionInProgress())
		{
			View.EndTurnDisabledReason = FUILocalizationText::Get(TEXT("Combat.EndTurn.PartyMotion"));
		}
		else
		{
			View.EndTurnDisabledReason = FUILocalizationText::Get(TEXT("Combat.EndTurn.Resolution"));
		}
	}

	for (int32 Index = 0; Index < PartyMemberPanels.Num(); ++Index)
	{
		if (IsValid(PartyMemberPanels[Index]) && View.PartyMembers.IsValidIndex(Index))
		{
			PartyMemberPanels[Index]->SetView(View.PartyMembers[Index]);
		}
	}
	EnsureInitiativeWidgets();
	RefreshInitiativeWidgets();
	RefreshBoundWidgets();
	if (IsValid(PartyPawn))
	{
		PartyPawn->RefreshPersistentHudWidget();
	}
}

bool UGridCombatHudWidget::RequestCombatAction(const FGridCombatHudActionView& ActionView, FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	if (!IsValid(TurnManagerComponent) || !ActionView.bHasBinding || !ActionView.bResolved)
	{
		return false;
	}

	const FGridAvailableCombatAction& Action = ActionView.Action;
	const bool bAccepted = TurnManagerComponent->RequestCharacterCombatAction(
		Action.CharacterIndex, Action.Definition.ActionId, Action.Definition.SourcePolicy, Action.SourceDefinitionId, Action.SourceEquipmentSlot, OutResult);
	RefreshFromSources();
	return bAccepted;
}

bool UGridCombatHudWidget::RequestHotbarSlot(int32 SlotIndex, FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	if ((IsValid(PartyPawn) && PartyPawn->IsCombatHotbarExecutionBlocked()) || SlotIndex < 0)
	{
		return false;
	}

	RefreshFromSources();
	if (!View.Actions.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FGridCombatHudActionView ActionView = View.Actions[SlotIndex];
	if (!View.bCombatActive && ActionView.bHasBinding &&
		ActionView.Binding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
		!ActionView.Binding.IsPhysicalThrowItemBinding())
	{
		// Use the already validated inventory authority from ARMEMENT01.15.1.
		// Never enter the combat action catalogue or spend turn AP while exploring.
		if (!ActionView.bResolved || !ActionView.Action.bEnabled ||
			!IsValid(InventoryComponent) ||
			!InventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex(View.ActiveCharacterIndex))
		{
			return false;
		}
		const FGridCharacterInventoryState& Character =
			InventoryComponent->PartyInventoryState.ActiveCharacters[View.ActiveCharacterIndex];
		const FGridInventorySlot* Source = Character.InventorySlots.FindByPredicate(
			[&ActionView](const FGridInventorySlot& Slot)
			{
				return !Slot.IsEmpty() &&
					Slot.Item.ItemDefinitionId == ActionView.Binding.SourceDefinitionId;
			});
		if (!Source)
		{
			return false;
		}
		FText Error;
		const bool bAccepted = InventoryComponent->TryConsumeSelfRestorativeQuickItem(
			View.ActiveCharacterIndex, Source->Item.RuntimeObjectId, Error);
		OutResult.Action = ActionView.Action;
		OutResult.bAccepted = bAccepted;
		OutResult.RejectReason = bAccepted ? EGridCombatActionRequestRejectReason::None :
			EGridCombatActionRequestRejectReason::QuickItemRejected;
		UE_LOG(LogTemp, Log, TEXT("[ARMEMENT01.15.2] HotbarSelfConsume Accepted=%s Character=%d Item=%s Reason=%s"),
			bAccepted ? TEXT("true") : TEXT("false"), View.ActiveCharacterIndex,
			*ActionView.Binding.SourceDefinitionId.ToString(), *Error.ToString());
		RefreshFromSources();
		return bAccepted;
	}
	if (ActionView.Binding.IsPhysicalThrowItemBinding())
	{
		OutResult.Action = ActionView.Action;
		if (!ActionView.bResolved || !ActionView.Action.bEnabled || !IsValid(PartyPawn))
		{
			return false;
		}
		CancelCombatActionTargeting();
		return PartyPawn->BeginSelectedCharacterInventoryItemThrowAiming(ActionView.Binding.SourceDefinitionId);
	}
	if (ActionView.bHasBinding && ActionView.bResolved && ActionView.Action.bEnabled &&
		(ActionView.Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Cell ||
			ActionView.Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Area))
	{
		OutResult.Action = ActionView.Action;
		return BeginCombatActionTargeting(ActionView);
	}
	if (ActionView.bHasBinding && ActionView.bResolved && ActionView.Action.bEnabled)
	{
		CancelCombatActionTargeting();
	}
	return RequestCombatAction(ActionView, OutResult);
}

bool UGridCombatHudWidget::BeginCombatActionTargeting(const FGridCombatHudActionView& ActionView)
{
	if (!IsValid(TurnManagerComponent) || !ActionView.bHasBinding || !ActionView.bResolved || !ActionView.Action.bEnabled ||
		(ActionView.Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Cell &&
			ActionView.Action.Definition.TargetingPolicy != EGridCombatTargetingPolicy::Area))
	{
		return false;
	}

	bCombatActionTargetingActive = true;
	PendingTargetingActionView = ActionView;
	TargetingPreview = FGridCombatActionTargetingPreview();
	TargetingPreview.Action = ActionView.Action;
	return true;
}

bool UGridCombatHudWidget::UpdateCombatActionTargetingPreview(FIntPoint TargetCell)
{
	if (!bCombatActionTargetingActive || !IsValid(TurnManagerComponent))
	{
		return false;
	}

	const FGridAvailableCombatAction& Action = PendingTargetingActionView.Action;
	const bool bValid = TurnManagerComponent->BuildCombatActionTargetingPreview(Action.CharacterIndex, Action.Definition.ActionId,
		Action.Definition.SourcePolicy, Action.SourceDefinitionId, Action.SourceEquipmentSlot, TargetCell, TargetingPreview);
	return bValid;
}

bool UGridCombatHudWidget::ConfirmCombatActionTarget(FIntPoint TargetCell, FGridCombatActionRequestResult& OutResult)
{
	OutResult = FGridCombatActionRequestResult();
	if (!UpdateCombatActionTargetingPreview(TargetCell) || !TargetingPreview.bValid || !IsValid(TurnManagerComponent))
	{
		OutResult.Action = TargetingPreview.Action;
		OutResult.RejectReason = EGridCombatActionRequestRejectReason::InvalidTarget;
		return false;
	}

	const FGridAvailableCombatAction Action = TargetingPreview.Action;
	const bool bAccepted = TurnManagerComponent->RequestCharacterCombatActionAtCell(Action.CharacterIndex, Action.Definition.ActionId,
		Action.Definition.SourcePolicy, Action.SourceDefinitionId, Action.SourceEquipmentSlot, TargetCell, OutResult);
	if (bAccepted)
	{
		bCombatActionTargetingActive = false;
		PendingTargetingActionView = FGridCombatHudActionView();
		TargetingPreview = FGridCombatActionTargetingPreview();
	}
	RefreshFromSources();
	return bAccepted;
}

void UGridCombatHudWidget::ClearCombatActionTargetingPreview()
{
	if (!bCombatActionTargetingActive)
	{
		return;
	}
	TargetingPreview = FGridCombatActionTargetingPreview();
	TargetingPreview.Action = PendingTargetingActionView.Action;
}

void UGridCombatHudWidget::CancelCombatActionTargeting()
{
	bCombatActionTargetingActive = false;
	PendingTargetingActionView = FGridCombatHudActionView();
	TargetingPreview = FGridCombatActionTargetingPreview();
}

bool UGridCombatHudWidget::RequestEndTurn()
{
	if (!IsValid(TurnManagerComponent))
	{
		return false;
	}
	CancelCombatActionTargeting();
	const bool bEnded = TurnManagerComponent->EndActivePlayerTurn();
	RefreshFromSources();
	return bEnded;
}

bool UGridCombatHudWidget::HandleHotbarDrop(int32 TargetSlotIndex, UDragDropOperation* DragOperation)
{
	if (!IsValid(InventoryComponent) || View.ActiveCharacterIndex == INDEX_NONE || TargetSlotIndex < 0 ||
		TargetSlotIndex >= InventoryComponent->GetCharacterCombatHotbarSlotCount(View.ActiveCharacterIndex) || !IsValid(DragOperation))
	{
		return false;
	}

	if (const UGridCombatHotbarDragDropOperation* HotbarOperation = Cast<UGridCombatHotbarDragDropOperation>(DragOperation))
	{
		if (HotbarOperation->bFromSpellbook)
		{
			if (HotbarOperation->CharacterIndex != View.ActiveCharacterIndex || !IsValid(PartyPawn) ||
				!InventoryComponent->IsValidCharacterIndex(View.ActiveCharacterIndex))
			{
				return false;
			}

			UGridPartySpellbookComponent* SpellbookComponent = PartyPawn->FindComponentByClass<UGridPartySpellbookComponent>();
			if (!IsValid(SpellbookComponent))
			{
				return false;
			}

			const FGuid CharacterId = InventoryComponent->PartyInventoryState.ActiveCharacters[View.ActiveCharacterIndex].CharacterId;
			if (!CharacterId.IsValid())
			{
				return false;
			}

			FGridCharacterSpellbookState CharacterSpellbook;
			if (!SpellbookComponent->GetCharacterSpellbookState(CharacterId, CharacterSpellbook))
			{
				return false;
			}

			return HotbarOperation->CommitSpellbookDrop(InventoryComponent, CharacterSpellbook, TargetSlotIndex) == EGridSpellHotbarAssignmentResult::Success;
		}

		FGridCombatHotbarBinding CurrentBinding;
		return HotbarOperation->CharacterIndex == View.ActiveCharacterIndex &&
			InventoryComponent->GetCharacterCombatHotbarBinding(View.ActiveCharacterIndex, HotbarOperation->SourceSlotIndex, CurrentBinding) &&
			HaveSameHotbarIdentity(CurrentBinding, HotbarOperation->Binding) &&
			InventoryComponent->MoveOrSwapCharacterCombatHotbarBinding(View.ActiveCharacterIndex, HotbarOperation->SourceSlotIndex, TargetSlotIndex);
	}

	const UGridInventoryDragDropOperation* InventoryOperation = Cast<UGridInventoryDragDropOperation>(DragOperation);
	if (!InventoryOperation || !InventoryOperation->bHasItem || InventoryOperation->SourceSlotType == EGridInventoryUiSlotType::Cursor)
	{
		return false;
	}

	const EGridEquipmentSlot EquipmentSlot = ResolveDraggedEquipmentSlot(InventoryOperation->SourceSlotType, InventoryOperation->SourceSlotIndex);
	return InventoryComponent->SetCharacterCombatHotbarBindingFromItem(
		View.ActiveCharacterIndex, TargetSlotIndex, InventoryOperation->SourceItem, EquipmentSlot);
}

bool UGridCombatHudWidget::AssignCombatActionToHotbarSlot(int32 TargetSlotIndex, const FGridAvailableCombatAction& Action)
{
	if (!IsValid(InventoryComponent) || View.ActiveCharacterIndex == INDEX_NONE || Action.CharacterIndex != View.ActiveCharacterIndex || TargetSlotIndex < 0 ||
		TargetSlotIndex >= InventoryComponent->GetCharacterCombatHotbarSlotCount(View.ActiveCharacterIndex) ||
		!IsDirectHotbarActionSource(Action.Definition.SourcePolicy))
	{
		return false;
	}

	if (!IsValid(TurnManagerComponent))
	{
		return false;
	}

	TArray<FGridAvailableCombatAction> CurrentActions;
	TurnManagerComponent->GetAvailableCombatActions(View.ActiveCharacterIndex, CurrentActions);
	const FGridAvailableCombatAction* CurrentAction = CurrentActions.FindByPredicate(
		[&Action](const FGridAvailableCombatAction& Candidate)
		{
			return Candidate.MatchesSource(Action.Definition.ActionId, Action.Definition.SourcePolicy, Action.SourceDefinitionId, Action.SourceEquipmentSlot);
		});
	if (!CurrentAction || !IsDirectHotbarActionSource(CurrentAction->Definition.SourcePolicy))
	{
		return false;
	}

	FGridCombatHotbarBinding Binding;
	Binding.SlotIndex = TargetSlotIndex;
	Binding.ActionId = CurrentAction->Definition.ActionId;
	Binding.SourcePolicy = CurrentAction->Definition.SourcePolicy;
	Binding.SourceDefinitionId = CurrentAction->SourceDefinitionId;
	return Binding.IsValid() && InventoryComponent->SetCharacterCombatHotbarBinding(View.ActiveCharacterIndex, TargetSlotIndex, Binding);
}

bool UGridCombatHudWidget::ClearHotbarSlot(int32 SlotIndex)
{
	if (SlotIndex == FGridCombatHotbarBinding::PrimaryAttackSlotIndex)
	{
		return false;
	}
	return IsValid(InventoryComponent) && View.ActiveCharacterIndex != INDEX_NONE &&
		InventoryComponent->ClearCharacterCombatHotbarBinding(View.ActiveCharacterIndex, SlotIndex);
}

void UGridCombatHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button_EndTurn)
	{
		Button_EndTurn->OnClicked.AddUniqueDynamic(this, &UGridCombatHudWidget::HandleEndTurnClicked);
	}
	BindToSources();
	EnsurePartyMemberPanels();
	RefreshFromSources();
}

void UGridCombatHudWidget::NativeDestruct()
{
	CancelCombatActionTargeting();
	if (Button_EndTurn)
	{
		Button_EndTurn->OnClicked.RemoveDynamic(this, &UGridCombatHudWidget::HandleEndTurnClicked);
	}
	UnbindFromSources();
	PartyMemberPanels.Reset();
	InitiativeSlotWidgets.Reset();
	InitiativeRoundSeparatorWidgets.Reset();
	InitiativeRoundSeparatorTexts.Reset();
	Super::NativeDestruct();
}

void UGridCombatHudWidget::BindToSources()
{
	if (bSourcesBound)
	{
		return;
	}
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnPartyInventoryChanged.AddUniqueDynamic(this, &UGridCombatHudWidget::HandleInventoryChanged);
	}
	if (IsValid(TurnManagerComponent))
	{
		TurnManagerComponent->OnPhaseChanged.AddUniqueDynamic(this, &UGridCombatHudWidget::HandlePhaseChanged);
		TurnManagerComponent->OnRoundStarted.AddUniqueDynamic(this, &UGridCombatHudWidget::HandleRoundStarted);
		TurnManagerComponent->OnPlayerCharacterTurnStateChanged.AddUniqueDynamic(this, &UGridCombatHudWidget::HandlePlayerTurnStateChanged);
		TurnManagerComponent->OnPartyMobilityStateChanged.AddUniqueDynamic(this, &UGridCombatHudWidget::HandlePartyMobilityChanged);
		TurnManagerComponent->OnTurnOrderChanged.AddUniqueDynamic(this, &UGridCombatHudWidget::HandleTurnOrderChanged);
		TurnManagerComponent->OnActiveCombatantChanged.AddUniqueDynamic(this, &UGridCombatHudWidget::HandleActiveCombatantChanged);
		TurnManagerComponent->OnCombatantStateChanged.AddUniqueDynamic(this, &UGridCombatHudWidget::HandleCombatantStateChanged);
		TurnManagerComponent->OnCombatEnded.AddUniqueDynamic(this, &UGridCombatHudWidget::HandleCombatEnded);
	}
	bSourcesBound = IsValid(InventoryComponent) || IsValid(TurnManagerComponent);
}

void UGridCombatHudWidget::UnbindFromSources()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridCombatHudWidget::HandleInventoryChanged);
	}
	if (IsValid(TurnManagerComponent))
	{
		TurnManagerComponent->OnPhaseChanged.RemoveDynamic(this, &UGridCombatHudWidget::HandlePhaseChanged);
		TurnManagerComponent->OnRoundStarted.RemoveDynamic(this, &UGridCombatHudWidget::HandleRoundStarted);
		TurnManagerComponent->OnPlayerCharacterTurnStateChanged.RemoveDynamic(this, &UGridCombatHudWidget::HandlePlayerTurnStateChanged);
		TurnManagerComponent->OnPartyMobilityStateChanged.RemoveDynamic(this, &UGridCombatHudWidget::HandlePartyMobilityChanged);
		TurnManagerComponent->OnTurnOrderChanged.RemoveDynamic(this, &UGridCombatHudWidget::HandleTurnOrderChanged);
		TurnManagerComponent->OnActiveCombatantChanged.RemoveDynamic(this, &UGridCombatHudWidget::HandleActiveCombatantChanged);
		TurnManagerComponent->OnCombatantStateChanged.RemoveDynamic(this, &UGridCombatHudWidget::HandleCombatantStateChanged);
		TurnManagerComponent->OnCombatEnded.RemoveDynamic(this, &UGridCombatHudWidget::HandleCombatEnded);
	}
	bSourcesBound = false;
}

void UGridCombatHudWidget::EnsurePartyMemberPanels()
{
	if (!Panel_PartyMembers || !PartyMemberPanelWidgetClass)
	{
		return;
	}
	if (PartyMemberPanels.Num() == FGridCombatHudViewModelBuilder::PartyPanelCount &&
		Panel_PartyMembers->GetChildrenCount() == FGridCombatHudViewModelBuilder::PartyPanelCount)
	{
		return;
	}

	Panel_PartyMembers->ClearChildren();
	PartyMemberPanels.Reset(FGridCombatHudViewModelBuilder::PartyPanelCount);
	for (int32 CharacterIndex = 0; CharacterIndex < FGridCombatHudViewModelBuilder::PartyPanelCount; ++CharacterIndex)
	{
		UGridCombatActionPanelWidget* Panel = CreateWidget<UGridCombatActionPanelWidget>(this, PartyMemberPanelWidgetClass);
		if (!Panel)
		{
			continue;
		}
		if (UHorizontalBoxSlot* PartySlot = Cast<UHorizontalBoxSlot>(Panel_PartyMembers->AddChild(Panel)))
		{
			const float RightPadding = CharacterIndex < FGridCombatHudViewModelBuilder::PartyPanelCount - 1
				? FMath::Max(0.0f, PartyMemberPanelSpacing)
				: 0.0f;
			PartySlot->SetPadding(FMargin(0.0f, 0.0f, RightPadding, 0.0f));
		}
		PartyMemberPanels.Add(Panel);
	}
}

void UGridCombatHudWidget::EnrichHotbarActionPresentation()
{
	for (FGridCombatHudActionView& ActionView : View.Actions)
	{
		if (!ActionView.bHasBinding)
		{
			continue;
		}

		// Slot 1 is the PrimaryAttack alias: its binding has no item definition.
		// When armed, the resolved action carries the actual equipment source.
		const FName SourceDefinitionId = ActionView.bResolved && !ActionView.Action.SourceDefinitionId.IsNone()
			? ActionView.Action.SourceDefinitionId : ActionView.Binding.SourceDefinitionId;
		const UGridItemDefinitionAsset* ItemDefinition = IsValid(InventoryComponent) && !SourceDefinitionId.IsNone()
			? InventoryComponent->FindItemDefinition(SourceDefinitionId)
			: nullptr;
		if (IsValid(ItemDefinition))
		{
			if (ActionView.Action.Definition.DisplayName.IsEmpty())
			{
				ActionView.Action.Definition.DisplayName = ItemDefinition->DisplayName;
			}
			if (ActionView.Action.Definition.Description.IsEmpty())
			{
				ActionView.Action.Definition.Description = ItemDefinition->Description;
			}
			if (ActionView.Action.Definition.Icon.IsNull())
			{
				ActionView.Action.Definition.Icon = ItemDefinition->Icon;
			}

			const EGridCombatActionSourcePolicy SourcePolicy = ActionView.bResolved
				? ActionView.Action.Definition.SourcePolicy : ActionView.Binding.SourcePolicy;
			if (SourcePolicy == EGridCombatActionSourcePolicy::Equipment ||
				SourcePolicy == EGridCombatActionSourcePolicy::QuickItem)
			{
				FGridItemInstance EquippedItem;
				const FGridItemInstance* SourceItem = nullptr;
				if (SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
					IsValid(InventoryComponent) && View.ActiveCharacterIndex != INDEX_NONE &&
					ActionView.Action.SourceEquipmentSlot != EGridEquipmentSlot::None &&
					InventoryComponent->GetEquippedItem(View.ActiveCharacterIndex,
						ActionView.Action.SourceEquipmentSlot, EquippedItem) &&
					EquippedItem.IsValid() && EquippedItem.ItemDefinitionId == SourceDefinitionId &&
					(!ActionView.Action.SourceRuntimeId.IsValid() ||
						EquippedItem.RuntimeObjectId == ActionView.Action.SourceRuntimeId))
				{
					SourceItem = &EquippedItem;
				}
				FGridWeaponVisualUiResolver::ApplyCatalogueIcon(
					ItemDefinition, SourceItem, ActionView.Action.Definition.Icon);
			}
		}

		if (ActionView.bResolved)
		{
			continue;
		}

		switch (ActionView.Binding.SourcePolicy)
		{
			case EGridCombatActionSourcePolicy::QuickItem:
				ActionView.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.QuickItemMissingAction"));
				break;
			case EGridCombatActionSourcePolicy::Equipment:
				ActionView.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.EquipmentMissing"));
				break;
			default:
				ActionView.DisabledReason = FUILocalizationText::Get(TEXT("Combat.Action.Unavailable.Generic"));
				break;
		}
	}
}

void UGridCombatHudWidget::EnsureInitiativeWidgets()
{
	if (!Panel_Initiative || !InitiativeSlotWidgetClass)
	{
		return;
	}

	const int32 SlotCount = FMath::Clamp(VisibleInitiativeSlotCount, 7, 10);
	bool bPoolValid = InitiativeSlotWidgets.Num() == SlotCount && InitiativeRoundSeparatorWidgets.Num() == SlotCount - 1 &&
		InitiativeRoundSeparatorTexts.Num() == SlotCount - 1;
	for (const UGridCombatHudInitiativeSlotWidget* SlotWidget : InitiativeSlotWidgets)
	{
		bPoolValid = bPoolValid && IsValid(SlotWidget);
	}
	for (int32 Index = 0; Index < InitiativeRoundSeparatorWidgets.Num(); ++Index)
	{
		bPoolValid = bPoolValid && IsValid(InitiativeRoundSeparatorWidgets[Index]) && InitiativeRoundSeparatorTexts.IsValidIndex(Index) &&
			IsValid(InitiativeRoundSeparatorTexts[Index]);
	}
	if (bPoolValid)
	{
		return;
	}

	Panel_Initiative->ClearChildren();
	InitiativeSlotWidgets.Reset(SlotCount);
	InitiativeRoundSeparatorWidgets.Reset(SlotCount - 1);
	InitiativeRoundSeparatorTexts.Reset(SlotCount - 1);
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UGridCombatHudInitiativeSlotWidget* InitiativeWidget = CreateWidget<UGridCombatHudInitiativeSlotWidget>(this, InitiativeSlotWidgetClass);
		if (InitiativeWidget)
		{
			InitiativeWidget->SetVisibility(ESlateVisibility::Collapsed);
			Panel_Initiative->AddChild(InitiativeWidget);
			InitiativeSlotWidgets.Add(InitiativeWidget);
		}
	}

	for (int32 Index = 0; Index < SlotCount - 1; ++Index)
	{
		UBorder* Separator =
			NewObject<UBorder>(this, UBorder::StaticClass(), MakeUniqueObjectName(this, UBorder::StaticClass(), TEXT("Border_InitiativeRoundSeparator")));
		UTextBlock* SeparatorText =
			NewObject<UTextBlock>(this, UTextBlock::StaticClass(), MakeUniqueObjectName(this, UTextBlock::StaticClass(), TEXT("Text_InitiativeRoundNumber")));
		USizeBox* SeparatorContent =
			NewObject<USizeBox>(this, USizeBox::StaticClass(), MakeUniqueObjectName(this, USizeBox::StaticClass(), TEXT("SizeBox_InitiativeRoundSeparator")));
		UCanvasPanel* SeparatorCanvas = NewObject<UCanvasPanel>(
			this, UCanvasPanel::StaticClass(), MakeUniqueObjectName(this, UCanvasPanel::StaticClass(), TEXT("Canvas_InitiativeRoundSeparator")));
		if (!Separator || !SeparatorText || !SeparatorContent || !SeparatorCanvas)
		{
			continue;
		}

		Separator->SetBrushColor(FLinearColor(0.28f, 0.18f, 0.06f, 0.92f));
		Separator->SetPadding(FMargin(2.0f, 6.0f));
		Separator->SetHorizontalAlignment(HAlign_Center);
		Separator->SetVerticalAlignment(VAlign_Center);
		Separator->SetVisibility(ESlateVisibility::Collapsed);
		SeparatorContent->SetWidthOverride(InitiativeRoundSeparatorWidth);
		SeparatorContent->SetHeightOverride(InitiativeRoundSeparatorHeight);
		SeparatorText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.78f, 0.45f, 1.0f)));
		SeparatorText->SetJustification(ETextJustify::Center);
		SeparatorText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SeparatorText->SetRenderTransformAngle(InitiativeRoundSeparatorTextAngle);
		FSlateFontInfo Font = SeparatorText->GetFont();
		Font.Size = 10;
		SeparatorText->SetFont(Font);
		UCanvasPanelSlot* SeparatorTextSlot = SeparatorCanvas->AddChildToCanvas(SeparatorText);
		if (!SeparatorTextSlot)
		{
			continue;
		}
		SeparatorTextSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		SeparatorTextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		SeparatorTextSlot->SetPosition(FVector2D::ZeroVector);
		SeparatorTextSlot->SetAutoSize(true);
		SeparatorContent->AddChild(SeparatorCanvas);
		Separator->AddChild(SeparatorContent);
		InitiativeRoundSeparatorWidgets.Add(Separator);
		InitiativeRoundSeparatorTexts.Add(SeparatorText);
	}
}

void UGridCombatHudWidget::RefreshInitiativeWidgets()
{
	if (!Panel_Initiative)
	{
		return;
	}

	Panel_Initiative->ClearChildren();
	int32 UsedSeparatorCount = 0;
	for (int32 Index = 0; Index < InitiativeSlotWidgets.Num(); ++Index)
	{
		UGridCombatHudInitiativeSlotWidget* InitiativeWidget = InitiativeSlotWidgets[Index];
		if (!IsValid(InitiativeWidget))
		{
			continue;
		}
		if (!View.Initiative.IsValidIndex(Index))
		{
			InitiativeWidget->SetVisibility(ESlateVisibility::Collapsed);
			Panel_Initiative->AddChild(InitiativeWidget);
			continue;
		}

		const FGridCombatHudInitiativeView& InitiativeView = View.Initiative[Index];
		if (InitiativeView.bStartsNewRound && InitiativeRoundSeparatorWidgets.IsValidIndex(UsedSeparatorCount) &&
			InitiativeRoundSeparatorTexts.IsValidIndex(UsedSeparatorCount))
		{
			UBorder* Separator = InitiativeRoundSeparatorWidgets[UsedSeparatorCount];
			UTextBlock* SeparatorText = InitiativeRoundSeparatorTexts[UsedSeparatorCount];
			SeparatorText->SetText(
				FText::FormatNamed(
					FUILocalizationText::Get(TEXT("Combat.Initiative.Round")),
					TEXT("Round"), InitiativeView.RoundNumber));
			Separator->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Panel_Initiative->AddChild(Separator);
			++UsedSeparatorCount;
		}

		InitiativeWidget->InitializeInitiativeSlot(InitiativeView);
		InitiativeWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Panel_Initiative->AddChild(InitiativeWidget);
	}

	for (int32 Index = UsedSeparatorCount; Index < InitiativeRoundSeparatorWidgets.Num(); ++Index)
	{
		if (IsValid(InitiativeRoundSeparatorWidgets[Index]))
		{
			InitiativeRoundSeparatorWidgets[Index]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UGridCombatHudWidget::RefreshBoundWidgets()
{
	const bool bConfiguringHotbar = IsValid(PartyPawn) && PartyPawn->bInventoryWidgetVisible;
	const bool bShowCombatOnly = View.bCombatActive && !bConfiguringHotbar;
	if (Panel_CombatHud)
	{
		Panel_CombatHud->SetVisibility(bShowCombatOnly ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Panel_Initiative)
	{
		Panel_Initiative->SetVisibility(bShowCombatOnly ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Text_MobilityActionPoints)
	{
		Text_MobilityActionPoints->SetText(View.Mobility.DisplayText);
		Text_MobilityActionPoints->SetVisibility(bShowCombatOnly ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Button_EndTurn)
	{
		Button_EndTurn->SetIsEnabled(View.bCanEndTurn);
		Button_EndTurn->SetVisibility(bShowCombatOnly ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_EndTurn->SetToolTipText(View.bCanEndTurn ? FUILocalizationText::Get(TEXT("Combat.EndTurn.Tooltip")) : View.EndTurnDisabledReason);
	}
	if (Text_EndTurnDisabledReason)
	{
		Text_EndTurnDisabledReason->SetText(View.EndTurnDisabledReason);
		Text_EndTurnDisabledReason->SetVisibility(!bShowCombatOnly || View.bCanEndTurn ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	for (int32 Index = 0; Index < PartyMemberPanels.Num(); ++Index)
	{
		if (IsValid(PartyMemberPanels[Index]))
		{
			const bool bPresent = View.PartyMembers.IsValidIndex(Index) && View.PartyMembers[Index].bPresent;
			PartyMemberPanels[Index]->SetVisibility(bShowCombatOnly && bPresent ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void UGridCombatHudWidget::ValidateCombatActionTargetingState()
{
	if (!bCombatActionTargetingActive)
	{
		return;
	}

	const FGridCombatHudActionView* CurrentActionView = View.Actions.FindByPredicate(
		[this](const FGridCombatHudActionView& Candidate)
		{
			return Candidate.bHasBinding && Candidate.bResolved && Candidate.Action.bEnabled &&
				HaveSameHotbarIdentity(Candidate.Binding, PendingTargetingActionView.Binding) &&
				(Candidate.Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Cell ||
					Candidate.Action.Definition.TargetingPolicy == EGridCombatTargetingPolicy::Area);
		});
	if (!CurrentActionView || CurrentActionView->Action.CharacterIndex != View.ActiveCharacterIndex)
	{
		bCombatActionTargetingActive = false;
		PendingTargetingActionView = FGridCombatHudActionView();
		TargetingPreview = FGridCombatActionTargetingPreview();
		return;
	}

	PendingTargetingActionView = *CurrentActionView;
	TargetingPreview.Action = CurrentActionView->Action;
}

void UGridCombatHudWidget::HandleEndTurnClicked()
{
	RequestEndTurn();
}

void UGridCombatHudWidget::HandleInventoryChanged(int32 CharacterIndex)
{
	(void)CharacterIndex;
	RefreshFromSources();
}

void UGridCombatHudWidget::HandlePhaseChanged(EGridCombatPhase NewPhase)
{
	(void)NewPhase;
	RefreshFromSources();
}

void UGridCombatHudWidget::HandleRoundStarted(int32 RoundNumber)
{
	(void)RoundNumber;
	RefreshFromSources();
}

void UGridCombatHudWidget::HandlePlayerTurnStateChanged(FGridPlayerCharacterTurnState TurnState)
{
	(void)TurnState;
	RefreshFromSources();
}

void UGridCombatHudWidget::HandlePartyMobilityChanged(FGridPartyMobilityState MobilityState)
{
	(void)MobilityState;
	RefreshFromSources();
}

void UGridCombatHudWidget::HandleTurnOrderChanged()
{
	RefreshFromSources();
}

void UGridCombatHudWidget::HandleActiveCombatantChanged(FGridCombatantInitiativeEntry Combatant)
{
	(void)Combatant;
	RefreshFromSources();
}

void UGridCombatHudWidget::HandleCombatantStateChanged(FGridCombatantInitiativeEntry Combatant)
{
	(void)Combatant;
	RefreshFromSources();
}

void UGridCombatHudWidget::HandleCombatEnded(EGridCombatPhase ResultPhase)
{
	(void)ResultPhase;
	RefreshFromSources();
}
