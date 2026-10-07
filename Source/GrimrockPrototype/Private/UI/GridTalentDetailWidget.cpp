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
	ApplyDetailPresentation();
	ApplyAcquisitionPresentation();
	return true;
}

void UGridTalentDetailWidget::ClearTalentDetail()
{
	NodeView = FGridTalentNodeView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedDescription = FText::GetEmpty();
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
	ApplyAcquisitionPresentation();
	return true;
}

bool UGridTalentDetailWidget::SelectVariantChoice(FName ChoiceId)
{
	if (!bVariantSelectionPending || ChoiceId.IsNone())
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

	SelectedVariantChoiceId = ChoiceId;
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
		Text_DetailDescription->SetText(ResolvedDescription);
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
			bVariantSelectionPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->SetVisibility(
			bAnyPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button_ConfirmAcquire->SetIsEnabled(
			bAcquireConfirmationPending || (bVariantSelectionPending && !SelectedVariantChoiceId.IsNone()));
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
