#include "UI/GridTalentNodeWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

namespace GridTalentNodeWidgetPrivate
{
	FText TierText(int32 Tier)
	{
		switch (Tier)
		{
			case 1: return FText::FromString(TEXT("I"));
			case 2: return FText::FromString(TEXT("II"));
			case 3: return FText::FromString(TEXT("III"));
			case 4: return FText::FromString(TEXT("IV"));
			case 5: return FText::FromString(TEXT("V"));
			default: return FText::FromString(TEXT("?"));
		}
	}

	FText StateText(EGridTalentNodeState State)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired: return FText::FromString(TEXT("Acquis"));
			case EGridTalentNodeState::Available: return FText::FromString(TEXT("Disponible"));
			case EGridTalentNodeState::LockedLevel: return FText::FromString(TEXT("Niveau"));
			case EGridTalentNodeState::LockedPrerequisite: return FText::FromString(TEXT("Prérequis"));
			case EGridTalentNodeState::LockedPoints: return FText::FromString(TEXT("Points"));
			case EGridTalentNodeState::LockedExclusive: return FText::FromString(TEXT("Exclusif"));
			default: return FText::GetEmpty();
		}
	}

	FLinearColor NodeColor(EGridTalentNodeState State, const FLinearColor& Accent)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired:
				return FLinearColor(Accent.R, Accent.G, Accent.B, 1.0f);
			case EGridTalentNodeState::Available:
				return FLinearColor(Accent.R * 0.72f, Accent.G * 0.72f, Accent.B * 0.72f, 1.0f);
			case EGridTalentNodeState::LockedLevel:
				return FLinearColor(0.12f, 0.12f, 0.13f, 1.0f);
			case EGridTalentNodeState::LockedPrerequisite:
				return FLinearColor(0.10f, 0.10f, 0.11f, 1.0f);
			case EGridTalentNodeState::LockedPoints:
				return FLinearColor(0.16f, 0.12f, 0.08f, 1.0f);
			case EGridTalentNodeState::LockedExclusive:
				return FLinearColor(0.14f, 0.09f, 0.12f, 1.0f);
			default:
				return FLinearColor(0.10f, 0.10f, 0.11f, 1.0f);
		}
	}
}

void UGridTalentNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_TalentNode)
	{
		Button_TalentNode->OnClicked.RemoveDynamic(this, &UGridTalentNodeWidget::HandleNodeClicked);
		Button_TalentNode->OnClicked.AddDynamic(this, &UGridTalentNodeWidget::HandleNodeClicked);
	}

	if (bInitialized)
	{
		ApplyNodePresentation();
	}
}

void UGridTalentNodeWidget::NativeDestruct()
{
	if (Button_TalentNode)
	{
		Button_TalentNode->OnClicked.RemoveDynamic(this, &UGridTalentNodeWidget::HandleNodeClicked);
	}

	Super::NativeDestruct();
}

bool UGridTalentNodeWidget::InitializeTalentNode(
	const FGridTalentNodeView& InNodeView,
	const FRPGTalentBranchPresentationDefinition& InBranchPresentation)
{
	ClearTalentNode();

	if (InNodeView.TalentNodeId.IsNone() ||
		InNodeView.TalentBranchId.IsNone() ||
		InNodeView.TalentBranchId != InBranchPresentation.TalentBranchId ||
		InNodeView.Tier < 1 ||
		InNodeView.Tier > 5 ||
		InNodeView.DisplayName.IsEmpty() ||
		(InNodeView.bHasExclusiveVariants && InNodeView.Variants.Num() < 2) ||
		(!InNodeView.bHasExclusiveVariants && !InNodeView.Variants.IsEmpty()))
	{
		return false;
	}

	NodeView = InNodeView;
	ResolvedDisplayName = NodeView.DisplayName;
	ResolvedDescription = NodeView.Principle;
	BranchAccentColor = InBranchPresentation.AccentColor;
	bInitialized = true;
	ApplyNodePresentation();
	return true;
}

void UGridTalentNodeWidget::ClearTalentNode()
{
	NodeView = FGridTalentNodeView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedDescription = FText::GetEmpty();
	BranchAccentColor = FLinearColor::White;
	bInitialized = false;

	if (Text_TalentTier) Text_TalentTier->SetText(FText::GetEmpty());
	if (Text_TalentName) Text_TalentName->SetText(FText::GetEmpty());
	if (Text_TalentLevel) Text_TalentLevel->SetText(FText::GetEmpty());
	if (Text_TalentState) Text_TalentState->SetText(FText::GetEmpty());
	if (Text_VariantCount) Text_VariantCount->SetText(FText::GetEmpty());
	SetToolTipText(FText::GetEmpty());
}



void UGridTalentNodeWidget::ApplyNodePresentation()
{
	if (!bInitialized)
	{
		return;
	}

	using namespace GridTalentNodeWidgetPrivate;

	if (Text_TalentTier) Text_TalentTier->SetText(TierText(NodeView.Tier));
	if (Text_TalentName) Text_TalentName->SetText(ResolvedDisplayName);
	if (Text_TalentLevel)
	{
		Text_TalentLevel->SetText(FText::FromString(FString::Printf(TEXT("Niv. %d"), NodeView.MinimumLevel)));
	}
	if (Text_TalentState) Text_TalentState->SetText(StateText(NodeView.State));
	if (Text_VariantCount)
	{
		Text_VariantCount->SetText(
			NodeView.bHasExclusiveVariants
				? FText::FromString(FString::Printf(TEXT("×%d"), NodeView.Variants.Num()))
				: FText::GetEmpty());
	}
	if (Border_TalentNode)
	{
		Border_TalentNode->SetBrushColor(NodeColor(NodeView.State, BranchAccentColor));
	}

	if (!ResolvedDescription.IsEmpty())
	{
		SetToolTipText(FText::Format(
			NSLOCTEXT("GridTalentNode", "TalentTooltip", "{0}\n{1}"),
			ResolvedDisplayName,
			ResolvedDescription));
	}
	else
	{
		SetToolTipText(ResolvedDisplayName);
	}
}

void UGridTalentNodeWidget::HandleNodeClicked()
{
	if (bInitialized)
	{
		OnTalentNodeClicked.Broadcast(NodeView.TalentNodeId);
	}
}
