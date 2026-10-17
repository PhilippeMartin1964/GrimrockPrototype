#include "UI/GridSkillsWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Texture2D.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassVisualAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "UI/GridSkillDetailWidget.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridSkillsWidgetSessionUndoInternal.h"
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentNodeWidget.h"
#include "UI/GridTalentVariantFlyoutWidget.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UI/UILocalizationText.h"

void UGridSkillsWidget::InitializeSkillsWidget(AGrimrockPartyPawn* InPartyPawn)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
	}

	OwningPartyPawn = InPartyPawn;
	InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;

	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
		InventoryComponent->OnPartyInventoryChanged.AddDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
	}

	RefreshSkills();
}

void UGridSkillsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindDesignerShell();
	ApplyDesignerPresentation();
	ApplySkillDetailPresentation();
}

void UGridSkillsWidget::NativeDestruct()
{
	CloseTalentVariantFlyout();
	UnbindDesignerShell();

	if (InventoryComponent)
	{
		InventoryComponent->OnPartyInventoryChanged.RemoveDynamic(this, &UGridSkillsWidget::HandlePartyInventoryChanged);
	}

	Super::NativeDestruct();
}

void UGridSkillsWidget::BindDesignerShell()
{
	Button_SkillsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
	Button_SkillsTab->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);

	Button_TalentsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
	Button_TalentsTab->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);

	Branch_Left->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Left->OnTalentNodeClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Center->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Center->OnTalentNodeClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Right->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	Branch_Right->OnTalentNodeClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);

	if (Detail_Talent)
	{
		Detail_Talent->OnAcquireConfirmed.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentAcquireConfirmed);
		Detail_Talent->OnAcquireConfirmed.AddUniqueDynamic(this, &UGridSkillsWidget::HandleTalentAcquireConfirmed);
	}

	if (Button_SortName)
	{
		Button_SortName->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortNameClicked);
		Button_SortName->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSortNameClicked);
	}
	if (Button_SortAttribute)
	{
		Button_SortAttribute->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortAttributeClicked);
		Button_SortAttribute->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSortAttributeClicked);
	}
	if (Button_SortAffinity)
	{
		Button_SortAffinity->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortAffinityClicked);
		Button_SortAffinity->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSortAffinityClicked);
	}
	if (Button_SortRank)
	{
		Button_SortRank->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortRankClicked);
		Button_SortRank->OnClicked.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSortRankClicked);
	}
}

void UGridSkillsWidget::UnbindDesignerShell()
{
	if (Button_SkillsTab)
	{
		Button_SkillsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSkillsTabClicked);
	}
	if (Button_TalentsTab)
	{
		Button_TalentsTab->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentsTabClicked);
	}
	if (Branch_Left)
	{
		Branch_Left->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	}
	if (Branch_Center)
	{
		Branch_Center->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	}
	if (Branch_Right)
	{
		Branch_Right->OnTalentNodeClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentNodeClicked);
	}
	if (Detail_Talent)
	{
		Detail_Talent->OnAcquireConfirmed.RemoveDynamic(this, &UGridSkillsWidget::HandleTalentAcquireConfirmed);
	}
	if (Button_SortName)
	{
		Button_SortName->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortNameClicked);
	}
	if (Button_SortAttribute)
	{
		Button_SortAttribute->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortAttributeClicked);
	}
	if (Button_SortAffinity)
	{
		Button_SortAffinity->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortAffinityClicked);
	}
	if (Button_SortRank)
	{
		Button_SortRank->OnClicked.RemoveDynamic(this, &UGridSkillsWidget::HandleSortRankClicked);
	}
}

FReply UGridSkillsWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	// The flyout is a top-level viewport sibling, not a child of this widget.
	// Any click that reaches the Skills hierarchy is therefore outside the popup.
	CloseTalentVariantFlyout();
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UGridSkillsWidget::ClearView()
{
	CloseTalentVariantFlyout();
	View = FGridSkillsPageView();
	SelectedSkillDetail = FGridSkillDetailView();
}

const URPGSkillAsset* UGridSkillsWidget::ResolveCanonicalSkillDefinition(FName SkillId) const
{
	if (SkillId.IsNone())
	{
		return nullptr;
	}

	TArray<const URPGSkillAsset*> Definitions;
	FGridSkillsPageService::ResolveCanonicalSkillDefinitions(Definitions);
	for (const URPGSkillAsset* Definition : Definitions)
	{
		if (IsValid(Definition) && Definition->SkillId == SkillId)
		{
			return Definition;
		}
	}
	return nullptr;
}

void UGridSkillsWidget::RefreshSkills()
{
	if (bRefreshInProgress)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bRefreshInProgress, true);

	const FGuid PreviousCharacterId = View.CharacterId;
	const FName PreviousSkillId = SelectedSkillId;
	ClearView();

	TArray<const URPGSkillAsset*> SkillDefinitions;
	if (InventoryComponent)
	{
		FGridSkillsPageService::ResolveCanonicalSkillDefinitions(SkillDefinitions);

		FGridSkillsPageView Candidate;
		if (FGridSkillsPageService::TryBuildSelectedCharacterView(InventoryComponent, SkillDefinitions, Candidate))
		{
			for (FGridSkillEntryView& Entry : Candidate.Skills)
			{
				const FString SessionKey =
					MakeSkillAllocationSessionKey(Candidate.CharacterId, Entry.SkillId);
				GridSkillsWidgetSessionUndo::ReconcileHistory(
					SessionKey,
					Entry.Rank,
					bSkillAllocationTransactionInProgress,
					SessionPurchasedSkillRanks,
					SessionSkillRankFloors);

				const int32 SessionPurchaseCount =
					GetSessionPurchasedSkillRankCount(Candidate.CharacterId, Entry.SkillId);
				const int32 SessionFloor =
					GetSessionSkillRankFloor(Candidate.CharacterId, Entry.SkillId);
				Entry.bCanDecreaseRank =
					SessionPurchaseCount > 0 &&
					SessionFloor >= 0 &&
					Entry.Rank == SessionFloor + SessionPurchaseCount;

				const URPGSkillPresentationAsset* PresentationCatalog =
					IsValid(SkillPresentationAsset) && SkillPresentationAsset->IsValidDefinition()
						? SkillPresentationAsset.Get()
						: nullptr;
				Entry.ClassAffinity =
					PresentationCatalog
						? PresentationCatalog->ResolveClassAffinity(Entry.SkillId, Candidate.ClassId)
						: EGridSkillClassAffinity::Neutral;
			}
			View = MoveTemp(Candidate);
		}
	}

	const FName ResolvedSkillId = FGridSkillsPageService::ResolveSkillSelection(View, PreviousSkillId);
	const bool bSkillSelectionChanged = SelectedSkillId != ResolvedSkillId;
	SelectedSkillId = ResolvedSkillId;
	RefreshSelectedSkillDetail(SkillDefinitions);
	if (bSkillSelectionChanged)
	{
		OnSkillSelectionChanged.Broadcast(SelectedSkillId);
	}

	if (!SelectedTalentNodeId.IsNone())
	{
		const bool bCharacterChanged =
			PreviousCharacterId.IsValid() && (!View.IsValid() || PreviousCharacterId != View.CharacterId);
		const FGridTalentNodeView* RefreshedNode = FindTalentNode(SelectedTalentNodeId);
		if (bCharacterChanged || !RefreshedNode)
		{
			ClearTalentSelection();
		}
		else if (!SelectedTalentChoiceId.IsNone())
		{
			const bool bChoiceStillExists = RefreshedNode->Variants.ContainsByPredicate(
				[this](const FGridTalentVariantView& Variant)
				{
					return Variant.ChoiceId == SelectedTalentChoiceId;
				});
			if (!bChoiceStillExists)
			{
				ClearTalentSelection();
			}
			else if (RefreshedNode->SelectedChoiceId == SelectedTalentChoiceId)
			{
				SelectedTalentChoiceId = NAME_None;
			}
		}
	}

	ApplyDesignerPresentation();
	ApplySkillDetailPresentation();
	ApplyTalentDetailPresentation();
	OnSkillsRefreshed.Broadcast();
}

const FGridTalentNodeView* UGridSkillsWidget::FindTalentNode(FName TalentNodeId) const
{
	if (TalentNodeId.IsNone())
	{
		return nullptr;
	}

	for (const FGridTalentBranchView& Branch : View.TalentTree.Branches)
	{
		if (const FGridTalentNodeView* Node = Branch.Nodes.FindByPredicate(
			[TalentNodeId](const FGridTalentNodeView& Candidate)
			{
				return Candidate.TalentNodeId == TalentNodeId;
			}))
		{
			return Node;
		}
	}
	return nullptr;
}

void UGridSkillsWidget::PublishProgressionNotification(const FRPGProgressionNotificationView& Notification)
{
	if (!Notification.IsValid())
	{
		return;
	}

	LastProgressionNotification = Notification;
	if (Notification_Progression)
	{
		Notification_Progression->ShowNotification(Notification);
	}
	OnProgressionNotification.Broadcast(LastProgressionNotification);
}

void UGridSkillsWidget::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	(void)CharacterIndex;
	RefreshSkills();
}
