#include "UI/GridTalentDetailWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "UI/GridTalentNodeWidget.h"

namespace GridTalentDetailWidgetPrivate
{
	FText StateText(EGridTalentNodeState State)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired: return FText::FromString(TEXT("Acquis"));
			case EGridTalentNodeState::Available: return FText::FromString(TEXT("Disponible"));
			case EGridTalentNodeState::LockedLevel: return FText::FromString(TEXT("Niveau requis"));
			case EGridTalentNodeState::LockedPrerequisite: return FText::FromString(TEXT("Prérequis manquant"));
			case EGridTalentNodeState::LockedPoints: return FText::FromString(TEXT("Points insuffisants"));
			case EGridTalentNodeState::LockedExclusive: return FText::FromString(TEXT("Choix exclusif"));
			default: return FText::GetEmpty();
		}
	}

}

void UGridTalentDetailWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindAcquireButtons();
	ApplyAcquisitionPresentation();
}

void UGridTalentDetailWidget::NativeDestruct()
{
	UnbindAcquireButtons();
	Super::NativeDestruct();
}

void UGridTalentDetailWidget::BindAcquireButtons()
{
	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleAcquireClicked);
		Button_AcquireTalent->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleAcquireClicked);
	}
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleChooseVariantClicked);
		Button_ChooseVariant->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleChooseVariantClicked);
	}
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->OnSelectionChanged.RemoveDynamic(this, &UGridTalentDetailWidget::HandleVariantSelectionChanged);
		Combo_VariantChoice->OnSelectionChanged.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleVariantSelectionChanged);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleConfirmAcquireClicked);
		Button_ConfirmAcquire->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleConfirmAcquireClicked);
	}
	if (Button_CancelAcquire)
	{
		Button_CancelAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleCancelAcquireClicked);
		Button_CancelAcquire->OnClicked.AddUniqueDynamic(this, &UGridTalentDetailWidget::HandleCancelAcquireClicked);
	}
}

void UGridTalentDetailWidget::UnbindAcquireButtons()
{
	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleAcquireClicked);
	}
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleChooseVariantClicked);
	}
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->OnSelectionChanged.RemoveDynamic(this, &UGridTalentDetailWidget::HandleVariantSelectionChanged);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleConfirmAcquireClicked);
	}
	if (Button_CancelAcquire)
	{
		Button_CancelAcquire->OnClicked.RemoveDynamic(this, &UGridTalentDetailWidget::HandleCancelAcquireClicked);
	}
}

bool UGridTalentDetailWidget::InitializeTalentDetail(
	const FGridTalentNodeView& InNodeView,
	const FRPGTalentBranchPresentationDefinition& InBranchPresentation)
{
	ClearTalentDetail();

	FText DisplayName;
	FText Description;
	if (!UGridTalentNodeWidget::ResolvePresentationText(
			InNodeView, InBranchPresentation, DisplayName, Description))
	{
		return false;
	}

	NodeView = InNodeView;
	ResolvedDisplayName = DisplayName;
	ResolvedDescription = Description;
	BranchAccentColor = InBranchPresentation.AccentColor;
	bInitialized = true;
	RebuildVariantOptions();
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

void UGridTalentDetailWidget::ClearTalentDetail()
{
	NodeView = FGridTalentNodeView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedDescription = FText::GetEmpty();
	ResolvedVariantDisplayName = FText::GetEmpty();
	ResolvedVariantDescription = FText::GetEmpty();
	ResolvedActionSummary = FText::GetEmpty();
	BranchAccentColor = FLinearColor::White;
	bInitialized = false;
	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	VariantOptionLabels.Reset();
	VariantOptionChoiceIds.Reset();
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearOptions();
		Combo_VariantChoice->ClearSelection();
	}

	if (Text_DetailName) Text_DetailName->SetText(FText::GetEmpty());
	if (Text_DetailDescription) Text_DetailDescription->SetText(FText::GetEmpty());
	if (Text_DetailLevel) Text_DetailLevel->SetText(FText::GetEmpty());
	if (Text_DetailCost) Text_DetailCost->SetText(FText::GetEmpty());
	if (Text_DetailState) Text_DetailState->SetText(FText::GetEmpty());
	if (Text_DetailVariants) Text_DetailVariants->SetText(FText::GetEmpty());
	if (Text_DetailVariantName)
	{
		Text_DetailVariantName->SetText(FText::GetEmpty());
		Text_DetailVariantName->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailVariantDescription)
	{
		Text_DetailVariantDescription->SetText(FText::GetEmpty());
		Text_DetailVariantDescription->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Text_DetailActionSummary)
	{
		Text_DetailActionSummary->SetText(FText::GetEmpty());
		Text_DetailActionSummary->SetVisibility(ESlateVisibility::Collapsed);
	}
	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::CanRequestSimpleAcquisition() const
{
	return bInitialized &&
		NodeView.State == EGridTalentNodeState::Available &&
		NodeView.Variants.Num() == 1 &&
		!NodeView.Variants[0].ChoiceId.IsNone() &&
		!NodeView.Variants[0].bSelected;
}

bool UGridTalentDetailWidget::CanRequestVariantAcquisition() const
{
	if (!bInitialized ||
		NodeView.State != EGridTalentNodeState::Available ||
		NodeView.Variants.Num() <= 1)
	{
		return false;
	}

	return NodeView.Variants.ContainsByPredicate(
		[](const FGridTalentVariantView& Variant)
		{
			return !Variant.ChoiceId.IsNone() && Variant.bAvailable && !Variant.bSelected;
		});
}

bool UGridTalentDetailWidget::BeginAcquireConfirmation()
{
	if (!CanRequestSimpleAcquisition())
	{
		return false;
	}

	bAcquireConfirmationPending = true;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::BeginVariantSelection()
{
	if (!CanRequestVariantAcquisition())
	{
		return false;
	}

	bAcquireConfirmationPending = false;
	bVariantSelectionPending = true;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearSelection();
	}
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::SelectVariantChoice(FName ChoiceId)
{
	if (!bInitialized || NodeView.Variants.Num() <= 1 || ChoiceId.IsNone())
	{
		return false;
	}

	const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});

	if (!Variant)
	{
		return false;
	}

	// Inspection never grants acquisition rights, including for locked variants.
	SelectedVariantChoiceId = ChoiceId;
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::GetVariantDisplayLabel(FName ChoiceId, FText& OutLabel) const
{
	OutLabel = FText::GetEmpty();
	const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});
	if (!Variant)
	{
		return false;
	}

	OutLabel = MakeVariantDisplayLabel(*Variant);
	return !OutLabel.IsEmpty();
}

void UGridTalentDetailWidget::CancelAcquireConfirmation()
{
	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	AcquisitionFeedback = FText::GetEmpty();
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearSelection();
	}
	RefreshVariantDetailPreview();
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::ConfirmAcquire()
{
	if (!bInitialized)
	{
		return false;
	}

	FName ChoiceId = NAME_None;
	if (bAcquireConfirmationPending && NodeView.Variants.Num() == 1)
	{
		ChoiceId = NodeView.Variants[0].ChoiceId;
	}
	else if (bVariantSelectionPending)
	{
		ChoiceId = SelectedVariantChoiceId;
	}

	if (ChoiceId.IsNone())
	{
		return false;
	}

	const FGridTalentVariantView* Variant = NodeView.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Candidate)
		{
			return Candidate.ChoiceId == ChoiceId;
		});
	if (!Variant || !Variant->bAvailable || Variant->bSelected)
	{
		return false;
	}

	bAcquireConfirmationPending = false;
	bVariantSelectionPending = false;
	SelectedVariantChoiceId = NAME_None;
	ApplyAcquisitionPresentation();
	OnAcquireConfirmed.Broadcast(ChoiceId);
	return true;
}

void UGridTalentDetailWidget::SetAcquisitionFeedback(const FText& InFeedback)
{
	AcquisitionFeedback = InFeedback;
	ApplyAcquisitionPresentation();
}

void UGridTalentDetailWidget::HandleAcquireClicked()
{
	BeginAcquireConfirmation();
}

void UGridTalentDetailWidget::HandleChooseVariantClicked()
{
	BeginVariantSelection();
}

void UGridTalentDetailWidget::HandleVariantSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	(void)SelectionType;
	const int32 Index = VariantOptionLabels.IndexOfByKey(SelectedItem);
	if (VariantOptionChoiceIds.IsValidIndex(Index))
	{
		SelectVariantChoice(VariantOptionChoiceIds[Index]);
	}
}

void UGridTalentDetailWidget::HandleConfirmAcquireClicked()
{
	ConfirmAcquire();
}

void UGridTalentDetailWidget::HandleCancelAcquireClicked()
{
	CancelAcquireConfirmation();
}

void UGridTalentDetailWidget::RebuildVariantOptions()
{
	VariantOptionLabels.Reset();
	VariantOptionChoiceIds.Reset();

	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->ClearOptions();
		Combo_VariantChoice->ClearSelection();
	}

	if (!bInitialized || NodeView.Variants.Num() <= 1)
	{
		return;
	}

	for (const FGridTalentVariantView& Variant : NodeView.Variants)
	{
		if (Variant.ChoiceId.IsNone())
		{
			continue;
		}

		FString Label = MakeVariantDisplayLabel(Variant).ToString();
		if (Label.IsEmpty())
		{
			Label = Variant.ChoiceId.ToString();
		}

		FString UniqueLabel = Label;
		if (VariantOptionLabels.Contains(UniqueLabel))
		{
			UniqueLabel = FString::Printf(TEXT("%s [%s]"), *Label, *Variant.ChoiceId.ToString());
		}

		VariantOptionLabels.Add(UniqueLabel);
		VariantOptionChoiceIds.Add(Variant.ChoiceId);
		if (Combo_VariantChoice)
		{
			Combo_VariantChoice->AddOption(UniqueLabel);
		}
	}
}

FText UGridTalentDetailWidget::MakeVariantDisplayLabel(const FGridTalentVariantView& Variant) const
{
	FString Label = Variant.DisplayName.IsEmpty() ? Variant.ChoiceId.ToString() : Variant.DisplayName.ToString();
	const FString Conceptual = ResolvedDisplayName.ToString();

	if (!Conceptual.IsEmpty())
	{
		const TArray<FString> Prefixes = {
			Conceptual + TEXT(" — "),
			Conceptual + TEXT(" – "),
			Conceptual + TEXT(" - "),
			Conceptual + TEXT(": ")
		};
		for (const FString& Prefix : Prefixes)
		{
			if (Label.StartsWith(Prefix, ESearchCase::CaseSensitive))
			{
				Label.RightChopInline(Prefix.Len(), EAllowShrinking::No);
				break;
			}
		}
	}

	Label.TrimStartAndEndInline();
	return FText::FromString(Label);
}

void UGridTalentDetailWidget::RefreshVariantDetailPreview()
{
	ResolvedVariantDisplayName = FText::GetEmpty();
	ResolvedVariantDescription = FText::GetEmpty();
	ResolvedActionSummary = FText::GetEmpty();

	if (!bInitialized || NodeView.Variants.IsEmpty())
	{
		return;
	}

	const FGridTalentVariantView* PreviewVariant = nullptr;
	if (!SelectedVariantChoiceId.IsNone())
	{
		PreviewVariant = NodeView.Variants.FindByPredicate(
			[this](const FGridTalentVariantView& Candidate)
			{
				return Candidate.ChoiceId == SelectedVariantChoiceId;
			});
	}

	if (!PreviewVariant && !NodeView.SelectedChoiceId.IsNone())
	{
		PreviewVariant = NodeView.Variants.FindByPredicate(
			[this](const FGridTalentVariantView& Candidate)
			{
				return Candidate.ChoiceId == NodeView.SelectedChoiceId;
			});
	}

	if (!PreviewVariant && NodeView.Variants.Num() == 1)
	{
		PreviewVariant = &NodeView.Variants[0];
	}

	if (!PreviewVariant)
	{
		return;
	}

	if (NodeView.Variants.Num() > 1)
	{
		ResolvedVariantDisplayName = MakeVariantDisplayLabel(*PreviewVariant);
		ResolvedVariantDescription = PreviewVariant->EffectCategory.IsEmpty()
			? PreviewVariant->Description
			: FText::FromString(PreviewVariant->EffectCategory.ToString() + TEXT("\n") + PreviewVariant->Description.ToString());
	}
	ResolvedActionSummary = BuildActionSummary(*PreviewVariant);
}

FText UGridTalentDetailWidget::BuildActionSummary(const FGridTalentVariantView& Variant) const
{
	if (Variant.UnlockedActions.IsEmpty())
	{
		return FText::GetEmpty();
	}

	TArray<FString> ActionBlocks;
	ActionBlocks.Reserve(Variant.UnlockedActions.Num());
	for (const FGridTalentUnlockedActionView& Action : Variant.UnlockedActions)
	{
		FString Header = Action.DisplayName.IsEmpty() ? Action.ActionId.ToString() : Action.DisplayName.ToString();

		TArray<FString> Costs;
		Costs.Add(FString::Printf(TEXT("%d point%s d'action"), Action.ActionPointCost, Action.ActionPointCost > 1 ? TEXT("s") : TEXT("")));
		if (Action.ManaCost > 0)
		{
			Costs.Add(FString::Printf(TEXT("%d mana"), Action.ManaCost));
		}
		if (Action.RangeCells > 0)
		{
			Costs.Add(FString::Printf(TEXT("portée : %d case%s"), Action.RangeCells, Action.RangeCells > 1 ? TEXT("s") : TEXT("")));
		}
		if (Action.CooldownRounds > 0)
		{
			Costs.Add(FString::Printf(
				TEXT("recharge : %d tour%s"),
				Action.CooldownRounds,
				Action.CooldownRounds > 1 ? TEXT("s") : TEXT("")));
		}

		if (!Costs.IsEmpty())
		{
			Header += TEXT(" — ");
			Header += FString::Join(Costs, TEXT(" — "));
		}

		FString Description = Action.Description.ToString();
		Description.TrimStartAndEndInline();
		Description.ReplaceInline(TEXT(" WD"), TEXT(" des dégâts de l'arme"), ESearchCase::CaseSensitive);
		Description.ReplaceInline(TEXT(" PA"), TEXT(" points d'action"), ESearchCase::CaseSensitive);
		ActionBlocks.Add(Description.IsEmpty()
			? Header
			: FString::Printf(TEXT("%s\nEffet : %s"), *Header, *Description));
	}

	return FText::FromString(TEXT("ACTION DÉBLOQUÉE\n") + FString::Join(ActionBlocks, TEXT("\n\n")));
}

void UGridTalentDetailWidget::ApplyDetailPresentation()
{
	if (!bInitialized)
	{
		return;
	}

	using namespace GridTalentDetailWidgetPrivate;

	if (Border_DetailAccent)
	{
		Border_DetailAccent->SetBrushColor(BranchAccentColor);
	}
	if (Text_DetailName)
	{
		Text_DetailName->SetText(ResolvedDisplayName);
	}
	if (Text_DetailDescription)
	{
		if (NodeView.Variants.Num() == 1 && !NodeView.Variants[0].EffectCategory.IsEmpty())
		{
			Text_DetailDescription->SetText(FText::FromString(
				NodeView.Variants[0].EffectCategory.ToString() + TEXT("\n") +
				ResolvedDescription.ToString().Replace(TEXT(" WD"), TEXT(" des dégâts de l'arme")).Replace(TEXT(" PA"), TEXT(" points d'action"))));
		}
		else
		{
			Text_DetailDescription->SetText(ResolvedDescription);
		}
	}
	if (Text_DetailLevel)
	{
		Text_DetailLevel->SetText(
			FText::FromString(FString::Printf(TEXT("Niveau requis : %d"), NodeView.MinimumLevel)));
	}
	if (Text_DetailCost)
	{
		Text_DetailCost->SetText(
			FText::FromString(FString::Printf(TEXT("Coût : %d point%s"),
				NodeView.PointCost,
				NodeView.PointCost > 1 ? TEXT("s") : TEXT(""))));
	}
	if (Text_DetailState)
	{
		Text_DetailState->SetText(StateText(NodeView.State));
	}
	if (Text_DetailVariants)
	{
		if (NodeView.Variants.Num() <= 1)
		{
			Text_DetailVariants->SetText(FText::GetEmpty());
		}
		else
		{
			TArray<FString> Labels;
			Labels.Reserve(NodeView.Variants.Num());
			for (const FGridTalentVariantView& Variant : NodeView.Variants)
			{
				Labels.Add(MakeVariantDisplayLabel(Variant).ToString());
			}

			FText SelectedLabel;
			if (!NodeView.SelectedChoiceId.IsNone() &&
				GetVariantDisplayLabel(NodeView.SelectedChoiceId, SelectedLabel))
			{
				Text_DetailVariants->SetText(
					FText::Format(
						NSLOCTEXT("GridTalentDetail", "SelectedVariant", "Variante choisie : {0}"),
						SelectedLabel));
			}
			else
			{
				Text_DetailVariants->SetText(
					FText::FromString(FString::Printf(TEXT("Variantes : %s"), *FString::Join(Labels, TEXT(" / ")))));
			}
		}
	}

	if (Text_DetailVariantName)
	{
		Text_DetailVariantName->SetText(ResolvedVariantDisplayName);
		Text_DetailVariantName->SetVisibility(
			ResolvedVariantDisplayName.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailVariantDescription)
	{
		Text_DetailVariantDescription->SetText(ResolvedVariantDescription);
		Text_DetailVariantDescription->SetVisibility(
			ResolvedVariantDescription.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (Text_DetailActionSummary)
	{
		Text_DetailActionSummary->SetText(ResolvedActionSummary);
		Text_DetailActionSummary->SetVisibility(
			ResolvedActionSummary.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

void UGridTalentDetailWidget::ApplyAcquisitionPresentation()
{
	const bool bCanAcquireSimple = CanRequestSimpleAcquisition();
	const bool bCanAcquireVariant = CanRequestVariantAcquisition();
	const bool bAnyPending = bAcquireConfirmationPending || bVariantSelectionPending;

	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->SetVisibility(
			bCanAcquireSimple && !bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_ChooseVariant)
	{
		Button_ChooseVariant->SetVisibility(
			bCanAcquireVariant && !bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Combo_VariantChoice)
	{
		Combo_VariantChoice->SetVisibility(
			bInitialized && NodeView.Variants.Num() > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->SetVisibility(
			bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_ConfirmAcquire->SetIsEnabled(
			bAcquireConfirmationPending || (bVariantSelectionPending && NodeView.Variants.ContainsByPredicate(
			[this](const FGridTalentVariantView& Variant)
			{
				return Variant.ChoiceId == SelectedVariantChoiceId && Variant.bAvailable && !Variant.bSelected;
			})));
	}
	if (Button_CancelAcquire)
	{
		Button_CancelAcquire->SetVisibility(
			bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Text_AcquirePrompt)
	{
		FText Prompt = FText::GetEmpty();
		if (bAcquireConfirmationPending)
		{
			Prompt = FText::Format(
				NSLOCTEXT("GridTalentDetail", "ConfirmAcquirePrompt", "Confirmer l’acquisition de « {0} » ?"),
				ResolvedDisplayName);
		}
		else if (bVariantSelectionPending)
		{
			FText SelectedLabel;
			if (!SelectedVariantChoiceId.IsNone() &&
				GetVariantDisplayLabel(SelectedVariantChoiceId, SelectedLabel))
			{
				Prompt = FText::Format(
					NSLOCTEXT("GridTalentDetail", "ConfirmVariantPrompt", "Confirmer « {0} » pour « {1} » ?"),
					SelectedLabel,
					ResolvedDisplayName);
			}
			else
			{
				Prompt = FText::Format(
					NSLOCTEXT("GridTalentDetail", "ChooseVariantPrompt", "Choisissez une variante pour « {0} »."),
					ResolvedDisplayName);
			}
		}
		Text_AcquirePrompt->SetText(Prompt);
	}
	if (Text_AcquireFeedback)
	{
		Text_AcquireFeedback->SetText(AcquisitionFeedback);
	}
}
