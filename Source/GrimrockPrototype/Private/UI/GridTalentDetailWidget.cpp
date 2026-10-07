#include "UI/GridTalentDetailWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
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

	FText VariantText(const FGridTalentNodeView& Node)
	{
		if (Node.Variants.Num() <= 1)
		{
			return FText::GetEmpty();
		}

		TArray<FString> Names;
		Names.Reserve(Node.Variants.Num());
		for (const FGridTalentVariantView& Variant : Node.Variants)
		{
			Names.Add(Variant.DisplayName.IsEmpty() ? Variant.ChoiceId.ToString() : Variant.DisplayName.ToString());
		}
		return FText::FromString(FString::Printf(TEXT("Variantes : %s"), *FString::Join(Names, TEXT(" / "))));
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
	AcquisitionFeedback = FText::GetEmpty();

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

bool UGridTalentDetailWidget::BeginAcquireConfirmation()
{
	if (!CanRequestSimpleAcquisition())
	{
		return false;
	}

	bAcquireConfirmationPending = true;
	AcquisitionFeedback = FText::GetEmpty();
	ApplyAcquisitionPresentation();
	return true;
}

void UGridTalentDetailWidget::CancelAcquireConfirmation()
{
	bAcquireConfirmationPending = false;
	AcquisitionFeedback = FText::GetEmpty();
	ApplyAcquisitionPresentation();
}

bool UGridTalentDetailWidget::ConfirmAcquire()
{
	if (!bAcquireConfirmationPending || !bInitialized || NodeView.Variants.Num() != 1)
	{
		return false;
	}

	const FName ChoiceId = NodeView.Variants[0].ChoiceId;
	if (ChoiceId.IsNone())
	{
		return false;
	}

	bAcquireConfirmationPending = false;
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

void UGridTalentDetailWidget::HandleConfirmAcquireClicked()
{
	ConfirmAcquire();
}

void UGridTalentDetailWidget::HandleCancelAcquireClicked()
{
	CancelAcquireConfirmation();
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
		Text_DetailVariants->SetText(VariantText(NodeView));
	}
}

void UGridTalentDetailWidget::ApplyAcquisitionPresentation()
{
	const bool bCanAcquire = CanRequestSimpleAcquisition();

	if (Button_AcquireTalent)
	{
		Button_AcquireTalent->SetVisibility(
			bCanAcquire && !bAcquireConfirmationPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_ConfirmAcquire)
	{
		Button_ConfirmAcquire->SetVisibility(
			bAcquireConfirmationPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Button_CancelAcquire)
	{
		Button_CancelAcquire->SetVisibility(
			bAcquireConfirmationPending ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (Text_AcquirePrompt)
	{
		Text_AcquirePrompt->SetText(
			bAcquireConfirmationPending
				? FText::Format(
					NSLOCTEXT("GridTalentDetail", "ConfirmAcquirePrompt", "Confirmer l’acquisition de « {0} » ?"),
					ResolvedDisplayName)
				: FText::GetEmpty());
	}
	if (Text_AcquireFeedback)
	{
		Text_AcquireFeedback->SetText(AcquisitionFeedback);
	}
}
