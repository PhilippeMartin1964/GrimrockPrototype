#include "UI/GridTalentNodeWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

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

	FLinearColor NodeStateTint(EGridTalentNodeState State)
	{
		float Luminance = 0.30f;
		switch (State)
		{
			case EGridTalentNodeState::Acquired:           Luminance = 1.00f; break;
			case EGridTalentNodeState::Available:          Luminance = 0.82f; break;
			case EGridTalentNodeState::LockedPoints:       Luminance = 0.55f; break;
			case EGridTalentNodeState::LockedLevel:        Luminance = 0.42f; break;
			case EGridTalentNodeState::LockedPrerequisite: Luminance = 0.32f; break;
			case EGridTalentNodeState::LockedExclusive:    Luminance = 0.24f; break;
			default:                                       Luminance = 0.30f; break;
		}
		return FLinearColor(Luminance, Luminance, Luminance, 1.0f);
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
	bInitialized = true;
	ApplyNodePresentation();
	return true;
}

void UGridTalentNodeWidget::ClearTalentNode()
{
	NodeView = FGridTalentNodeView();
	ResolvedDisplayName = FText::GetEmpty();
	ResolvedDescription = FText::GetEmpty();
	bInitialized = false;

	if (Text_TalentTier) Text_TalentTier->SetText(FText::GetEmpty());
	if (Text_TalentName) Text_TalentName->SetText(FText::GetEmpty());
	if (Text_TalentLevel) Text_TalentLevel->SetText(FText::GetEmpty());
	if (Text_TalentState) Text_TalentState->SetText(FText::GetEmpty());
	if (Text_VariantCount) Text_VariantCount->SetText(FText::GetEmpty());
	SetTalentIcon(nullptr);
	SetToolTipText(FText::GetEmpty());
}

void UGridTalentNodeWidget::SetTalentIcon(UTexture2D* IconTexture)
{
	if (Border_TalentNode)
	{
		Border_TalentNode->SetBrushFromTexture(IconTexture);
	}
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
		Border_TalentNode->SetBrushColor(NodeStateTint(NodeView.State));
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
