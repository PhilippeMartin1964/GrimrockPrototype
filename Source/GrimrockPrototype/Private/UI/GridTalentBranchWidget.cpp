#include "UI/GridTalentBranchWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/GridTalentNodeWidget.h"

void UGridTalentBranchWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindNodeEvents();
}

void UGridTalentBranchWidget::NativeDestruct()
{
	UnbindNodeEvents();
	Super::NativeDestruct();
}

void UGridTalentBranchWidget::BindNodeEvents()
{
	for (int32 Tier = 1; Tier <= 5; ++Tier)
	{
		if (UGridTalentNodeWidget* NodeWidget = GetTalentNodeWidgetForTier(Tier))
		{
			NodeWidget->OnTalentNodeClicked.RemoveDynamic(this, &UGridTalentBranchWidget::HandleTalentNodeClicked);
			NodeWidget->OnTalentNodeClicked.AddUniqueDynamic(this, &UGridTalentBranchWidget::HandleTalentNodeClicked);
		}
	}
}

void UGridTalentBranchWidget::UnbindNodeEvents()
{
	for (int32 Tier = 1; Tier <= 5; ++Tier)
	{
		if (UGridTalentNodeWidget* NodeWidget = GetTalentNodeWidgetForTier(Tier))
		{
			NodeWidget->OnTalentNodeClicked.RemoveDynamic(this, &UGridTalentBranchWidget::HandleTalentNodeClicked);
		}
	}
}

void UGridTalentBranchWidget::HandleTalentNodeClicked(FName TalentNodeId)
{
	if (bInitialized && !TalentNodeId.IsNone())
	{
		OnTalentNodeClicked.Broadcast(TalentNodeId);
	}
}

bool UGridTalentBranchWidget::InitializeTalentBranch(
	const FGridTalentBranchView& InBranchView,
	const FRPGTalentBranchPresentationDefinition& InPresentation)
{
	ClearTalentBranch();

	if (InBranchView.TalentBranchId.IsNone() ||
		InBranchView.TalentBranchId != InPresentation.TalentBranchId ||
		InBranchView.Nodes.Num() != 5)
	{
		return false;
	}

	TSet<int32> SeenTiers;
	for (const FGridTalentNodeView& Node : InBranchView.Nodes)
	{
		if (Node.TalentBranchId != InBranchView.TalentBranchId ||
			Node.Tier < 1 ||
			Node.Tier > 5 ||
			SeenTiers.Contains(Node.Tier))
		{
			return false;
		}
		SeenTiers.Add(Node.Tier);
	}

	if (SeenTiers.Num() != 5)
	{
		return false;
	}

	BranchView = InBranchView;
	BranchPresentation = InPresentation;
	bInitialized = true;

	for (const FGridTalentNodeView& Node : BranchView.Nodes)
	{
		UGridTalentNodeWidget* NodeWidget = GetTalentNodeWidgetForTier(Node.Tier);
		if (!NodeWidget || !NodeWidget->InitializeTalentNode(Node, BranchPresentation))
		{
			ClearTalentBranch();
			return false;
		}
	}

	ApplyBranchPresentation();
	return true;
}

void UGridTalentBranchWidget::ClearTalentBranch()
{
	BranchView = FGridTalentBranchView();
	BranchPresentation = FRPGTalentBranchPresentationDefinition();
	bInitialized = false;

	if (Text_BranchName) Text_BranchName->SetText(FText::GetEmpty());
	if (Text_BranchProgress) Text_BranchProgress->SetText(FText::GetEmpty());
	SetBranchBackground(nullptr);

	for (int32 Tier = 1; Tier <= 5; ++Tier)
	{
		if (UGridTalentNodeWidget* NodeWidget = GetTalentNodeWidgetForTier(Tier))
		{
			NodeWidget->ClearTalentNode();
		}
	}
}

void UGridTalentBranchWidget::SetBranchBackground(UTexture2D* BackgroundTexture)
{
	Image_BranchBackground->SetBrushFromTexture(BackgroundTexture, false);
	Image_BranchBackground->SetVisibility(
		BackgroundTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

UGridTalentNodeWidget* UGridTalentBranchWidget::GetTalentNodeWidgetForTier(int32 Tier) const
{
	switch (Tier)
	{
		case 1: return Node_Tier1;
		case 2: return Node_Tier2;
		case 3: return Node_Tier3;
		case 4: return Node_Tier4;
		case 5: return Node_Tier5;
		default: return nullptr;
	}
}

void UGridTalentBranchWidget::ApplyBranchPresentation()
{
	if (!bInitialized)
	{
		return;
	}

	if (Text_BranchName)
	{
		Text_BranchName->SetText(BranchPresentation.DisplayName);
		Text_BranchName->SetColorAndOpacity(FSlateColor(BranchPresentation.AccentColor));
	}

	if (Text_BranchProgress)
	{
		Text_BranchProgress->SetText(
			FText::FromString(FString::Printf(TEXT("%d / 5"), BranchView.AcquiredNodeCount)));
	}

	if (Border_BranchAccent)
	{
		Border_BranchAccent->SetBrushColor(BranchPresentation.AccentColor);
	}
}
