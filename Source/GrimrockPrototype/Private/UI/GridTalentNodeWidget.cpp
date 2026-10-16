#include "UI/GridTalentNodeWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/RPGTalentLocalizationText.h"

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

	FText FormatMinimumLevel(const FText& Pattern, int32 Level)
	{
		if (Pattern.IsEmpty())
		{
			return FText::GetEmpty();
		}
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Level"), FText::AsNumber(Level));
		return FText::Format(Pattern, Arguments);
	}

	FName StateTextId(EGridTalentNodeState State)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired: return TEXT("Talent.Node.State.Acquired");
			case EGridTalentNodeState::Available: return TEXT("Talent.Node.State.Available");
			case EGridTalentNodeState::LockedLevel: return TEXT("Talent.Node.State.LockedLevel");
			case EGridTalentNodeState::LockedPrerequisite: return TEXT("Talent.Node.State.LockedPrerequisite");
			case EGridTalentNodeState::LockedPoints: return TEXT("Talent.Node.State.LockedPoints");
			case EGridTalentNodeState::LockedExclusive: return TEXT("Talent.Node.State.LockedExclusive");
			default: return NAME_None;
		}
	}

	FLinearColor NodeStateTextColor(EGridTalentNodeState State)
	{
		switch (State)
		{
			case EGridTalentNodeState::Acquired:
				return FLinearColor(0.30f, 0.90f, 0.30f, 1.0f);
			case EGridTalentNodeState::Available:
				return FLinearColor(0.45f, 1.00f, 0.45f, 1.0f);
			default:
				return FLinearColor(0.65f, 0.65f, 0.65f, 1.0f);
		}
	}

	FLinearColor NodeStateTint(EGridTalentNodeState State)
	{
		float Luminance = 0.20f;
		switch (State)
		{
			case EGridTalentNodeState::Acquired:           Luminance = 1.00f; break;
			case EGridTalentNodeState::Available:          Luminance = 0.50f; break;
			case EGridTalentNodeState::LockedPoints:       Luminance = 0.02f; break;
			case EGridTalentNodeState::LockedLevel:        Luminance = 0.02f; break;
			case EGridTalentNodeState::LockedPrerequisite: Luminance = 0.02f; break;
			case EGridTalentNodeState::LockedExclusive:    Luminance = 0.02f; break;
			default:                                       Luminance = 0.02f; break;
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
	ClearVariantIcons();

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

void UGridTalentNodeWidget::SetVariantIcon(FName ChoiceId, UTexture2D* IconTexture)
{
	if (!ChoiceId.IsNone())
	{
		VariantIcons.FindOrAdd(ChoiceId) = IconTexture;
	}
}

void UGridTalentNodeWidget::ClearVariantIcons()
{
	VariantIcons.Reset();
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
		const FText LevelFormat =
			FRPGTalentLocalizationText::Get(TEXT("Talent.Node.LevelFormat"));
		Text_TalentLevel->SetText(FormatMinimumLevel(LevelFormat, NodeView.MinimumLevel));
	}
	if (Text_TalentState)
	{
		Text_TalentState->SetText(
			FRPGTalentLocalizationText::Get(StateTextId(NodeView.State)));
		Text_TalentState->SetColorAndOpacity(FSlateColor(NodeStateTextColor(NodeView.State)));
	}
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
}

void UGridTalentNodeWidget::HandleNodeClicked()
{
	if (bInitialized)
	{
		OnTalentNodeClicked.Broadcast(NodeView.TalentNodeId);
	}
}
