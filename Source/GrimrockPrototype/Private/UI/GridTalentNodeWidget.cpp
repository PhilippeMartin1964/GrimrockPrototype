#include "UI/GridTalentNodeWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/UILocalizationText.h"
#include "UI/RPGTalentNodeStyleSettings.h"

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
			FUILocalizationText::Get(TEXT("Talent.Node.LevelFormat"));
		Text_TalentLevel->SetText(FormatMinimumLevel(LevelFormat, NodeView.MinimumLevel));
	}
	FRPGTalentNodeStateVisualStyle StateStyle;
	const URPGTalentNodeStyleSettings* StyleSettings = GetDefault<URPGTalentNodeStyleSettings>();
	const bool bHasStateStyle =
		StyleSettings && StyleSettings->TryGetStyle(NodeView.State, StateStyle);

	if (Text_TalentState)
	{
		Text_TalentState->SetText(
			FUILocalizationText::Get(StateTextId(NodeView.State)));
		if (bHasStateStyle)
		{
			Text_TalentState->SetColorAndOpacity(FSlateColor(StateStyle.TextColor));
		}
	}
	if (Text_VariantCount)
	{
		Text_VariantCount->SetText(
			NodeView.bHasExclusiveVariants
				? FText::FromString(FString::Printf(TEXT("×%d"), NodeView.Variants.Num()))
				: FText::GetEmpty());
	}
	if (Border_TalentNode && bHasStateStyle)
	{
		Border_TalentNode->SetBrushColor(StateStyle.NodeTint);
	}
}

void UGridTalentNodeWidget::HandleNodeClicked()
{
	if (bInitialized)
	{
		OnTalentNodeClicked.Broadcast(NodeView.TalentNodeId);
	}
}
