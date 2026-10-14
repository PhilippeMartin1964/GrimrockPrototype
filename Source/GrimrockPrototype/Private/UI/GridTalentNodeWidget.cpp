#include "UI/GridTalentNodeWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/MenuAnchor.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/GridTalentVariantFlyoutWidget.h"
#include "UI/RPGTalentDetailTextSet.h"

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
	if (MenuAnchor_Variants)
	{
		MenuAnchor_Variants->OnGetUserMenuContentEvent.Unbind();
		MenuAnchor_Variants->OnGetUserMenuContentEvent.BindDynamic(
			this, &UGridTalentNodeWidget::BuildVariantFlyoutContent);
		MenuAnchor_Variants->OnMenuOpenChanged.RemoveDynamic(
			this, &UGridTalentNodeWidget::HandleVariantMenuOpenChanged);
		MenuAnchor_Variants->OnMenuOpenChanged.AddUniqueDynamic(
			this, &UGridTalentNodeWidget::HandleVariantMenuOpenChanged);
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
	if (MenuAnchor_Variants)
	{
		MenuAnchor_Variants->Close();
		MenuAnchor_Variants->OnGetUserMenuContentEvent.Unbind();
		MenuAnchor_Variants->OnMenuOpenChanged.RemoveDynamic(
			this, &UGridTalentNodeWidget::HandleVariantMenuOpenChanged);
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
	if (MenuAnchor_Variants) MenuAnchor_Variants->Close();

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
			URPGTalentDetailTextSet::ResolveProductionText(TEXT("Talent.Node.LevelFormat"));
		Text_TalentLevel->SetText(
			LevelFormat.IsEmpty()
				? FText::GetEmpty()
				: FText::Format(LevelFormat, FText::AsNumber(NodeView.MinimumLevel)));
	}
	if (Text_TalentState)
	{
		Text_TalentState->SetText(
			URPGTalentDetailTextSet::ResolveProductionText(StateTextId(NodeView.State)));
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
	if (!bInitialized)
	{
		return;
	}

	if (NodeView.bHasExclusiveVariants && NodeView.SelectedChoiceId.IsNone() &&
		MenuAnchor_Variants && VariantFlyoutWidgetClass)
	{
		MenuAnchor_Variants->ToggleOpen(false);
		return;
	}

	OnTalentNodeClicked.Broadcast(NodeView.TalentNodeId);
}

UUserWidget* UGridTalentNodeWidget::BuildVariantFlyoutContent()
{
	if (!bInitialized || !NodeView.bHasExclusiveVariants || !NodeView.SelectedChoiceId.IsNone() ||
		!VariantFlyoutWidgetClass || !GetWorld())
	{
		return nullptr;
	}

	UGridTalentVariantFlyoutWidget* Flyout =
		CreateWidget<UGridTalentVariantFlyoutWidget>(GetWorld(), VariantFlyoutWidgetClass);
	if (!Flyout || !Flyout->InitializeFlyout(NodeView, VariantIcons))
	{
		return nullptr;
	}

	Flyout->OnVariantChosen.AddUniqueDynamic(this, &UGridTalentNodeWidget::HandleVariantChosen);
	return Flyout;
}

void UGridTalentNodeWidget::HandleVariantChosen(FName ChoiceId)
{
	if (!bInitialized || ChoiceId.IsNone())
	{
		return;
	}

	if (MenuAnchor_Variants)
	{
		MenuAnchor_Variants->Close();
	}
	OnTalentVariantClicked.Broadcast(NodeView.TalentNodeId, ChoiceId);
}

void UGridTalentNodeWidget::HandleVariantMenuOpenChanged(bool bIsOpen)
{
	if (bIsOpen)
	{
		// The anchor can remain hovered while its popup is active. Suppress the
		// source Talent tooltip so it cannot visually leak through the flyout.
		SetToolTipText(FText::GetEmpty());
		return;
	}

	if (bInitialized)
	{
		ApplyNodePresentation();
	}
}
