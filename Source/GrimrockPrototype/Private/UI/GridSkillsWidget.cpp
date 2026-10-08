#include "UI/GridSkillsWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillPointService.h"
#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridSkillsUI, Log, All);

namespace GridSkillsWidgetPrivate
{
	const TCHAR* TalentPresentationAssetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation");

	UGridTalentBranchWidget* BranchWidgetByIndex(
		int32 Index,
		UGridTalentBranchWidget* Left,
		UGridTalentBranchWidget* Center,
		UGridTalentBranchWidget* Right)
	{
		switch (Index)
		{
			case 0: return Left;
			case 1: return Center;
			case 2: return Right;
			default: return nullptr;
		}
	}
}

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
}

void UGridSkillsWidget::NativeDestruct()
{
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
}

void UGridSkillsWidget::ClearView()
{
	View = FGridSkillsPageView();
}

void UGridSkillsWidget::BeginSkillAllocationSession()
{
	SessionPurchasedSkillRanks.Reset();
	SessionSkillRankFloors.Reset();
	RefreshSkills();
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

FString UGridSkillsWidget::MakeSkillAllocationSessionKey(const FGuid& CharacterId, FName SkillId) const
{
	return CharacterId.IsValid() && !SkillId.IsNone()
		? FString::Printf(TEXT("%s|%s"), *CharacterId.ToString(EGuidFormats::Digits), *SkillId.ToString())
		: FString();
}

int32 UGridSkillsWidget::GetSessionPurchasedSkillRankCount(const FGuid& CharacterId, FName SkillId) const
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	return Key.IsEmpty() ? 0 : SessionPurchasedSkillRanks.FindRef(Key);
}

int32 UGridSkillsWidget::GetSessionSkillRankFloor(const FGuid& CharacterId, FName SkillId) const
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	if (Key.IsEmpty())
	{
		return INDEX_NONE;
	}

	const int32* Floor = SessionSkillRankFloors.Find(Key);
	return Floor ? *Floor : INDEX_NONE;
}

void UGridSkillsWidget::RecordSessionSkillPurchase(const FGuid& CharacterId, FName SkillId, int32 PreviousRank)
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	if (Key.IsEmpty() || PreviousRank < 0)
	{
		return;
	}

	SessionSkillRankFloors.FindOrAdd(Key, PreviousRank);
	++SessionPurchasedSkillRanks.FindOrAdd(Key);
}

void UGridSkillsWidget::ConsumeSessionSkillPurchase(const FGuid& CharacterId, FName SkillId)
{
	const FString Key = MakeSkillAllocationSessionKey(CharacterId, SkillId);
	if (Key.IsEmpty())
	{
		return;
	}

	int32* Count = SessionPurchasedSkillRanks.Find(Key);
	if (!Count)
	{
		return;
	}

	--(*Count);
	if (*Count <= 0)
	{
		SessionPurchasedSkillRanks.Remove(Key);
		SessionSkillRankFloors.Remove(Key);
	}
}

void UGridSkillsWidget::RefreshSkills()
{
	if (bRefreshInProgress)
	{
		return;
	}
	TGuardValue<bool> RefreshGuard(bRefreshInProgress, true);

	const FGuid PreviousCharacterId = View.CharacterId;
	ClearView();
	if (InventoryComponent)
	{
		TArray<const URPGSkillAsset*> SkillDefinitions;
		FGridSkillsPageService::ResolveCanonicalSkillDefinitions(SkillDefinitions);

		FGridSkillsPageView Candidate;
		if (FGridSkillsPageService::TryBuildSelectedCharacterView(InventoryComponent, SkillDefinitions, Candidate))
		{
			for (FGridSkillEntryView& Entry : Candidate.Skills)
			{
				const int32 SessionPurchaseCount =
					GetSessionPurchasedSkillRankCount(Candidate.CharacterId, Entry.SkillId);
				const int32 SessionFloor =
					GetSessionSkillRankFloor(Candidate.CharacterId, Entry.SkillId);
				Entry.bCanDecreaseRank =
					SessionPurchaseCount > 0 &&
					SessionFloor >= 0 &&
					Entry.Rank > SessionFloor;
			}
			View = MoveTemp(Candidate);
		}
	}

	if (!SelectedTalentNodeId.IsNone())
	{
		const bool bCharacterChanged =
			PreviousCharacterId.IsValid() && (!View.IsValid() || PreviousCharacterId != View.CharacterId);
		if (bCharacterChanged || !FindTalentNode(SelectedTalentNodeId))
		{
			ClearTalentSelection();
		}
	}

	ApplyDesignerPresentation();
	ApplyTalentDetailPresentation();
	OnSkillsRefreshed.Broadcast();
}

bool UGridSkillsWidget::GetCurrentClassPresentation(FRPGClassPresentationDefinition& OutPresentation) const
{
	OutPresentation = FRPGClassPresentationDefinition();
	if (!View.IsValid() || View.ClassId.IsNone())
	{
		return false;
	}

	const URPGTalentPresentationAsset* Catalog =
		LoadObject<URPGTalentPresentationAsset>(nullptr, GridSkillsWidgetPrivate::TalentPresentationAssetPath);
	return Catalog && Catalog->GetClassPresentation(View.ClassId, OutPresentation);
}

bool UGridSkillsWidget::GetPresentedTalentBranch(
	int32 VisualIndex,
	FGridTalentBranchView& OutBranch,
	FRPGTalentBranchPresentationDefinition& OutPresentation) const
{
	OutBranch = FGridTalentBranchView();
	OutPresentation = FRPGTalentBranchPresentationDefinition();

	FRPGClassPresentationDefinition ClassPresentation;
	if (GetCurrentClassPresentation(ClassPresentation) && ClassPresentation.Branches.IsValidIndex(VisualIndex))
	{
		OutPresentation = ClassPresentation.Branches[VisualIndex];
		if (const FGridTalentBranchView* Branch = View.TalentTree.Branches.FindByPredicate(
			[&OutPresentation](const FGridTalentBranchView& Candidate)
			{
				return Candidate.TalentBranchId == OutPresentation.TalentBranchId;
			}))
		{
			OutBranch = *Branch;
			return true;
		}
		return false;
	}

	if (!View.TalentTree.Branches.IsValidIndex(VisualIndex))
	{
		return false;
	}

	OutBranch = View.TalentTree.Branches[VisualIndex];
	OutPresentation.TalentBranchId = OutBranch.TalentBranchId;
	OutPresentation.DisplayName = FText::FromName(OutBranch.TalentBranchId);
	return true;
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

bool UGridSkillsWidget::SelectTalentNode(FName TalentNodeId)
{
	if (!View.IsValid() || !FindTalentNode(TalentNodeId))
	{
		return false;
	}

	if (SelectedTalentNodeId == TalentNodeId)
	{
		return true;
	}

	SelectedTalentNodeId = TalentNodeId;
	OnTalentSelectionChanged.Broadcast(SelectedTalentNodeId);
	return true;
}

void UGridSkillsWidget::ClearTalentSelection()
{
	if (SelectedTalentNodeId.IsNone())
	{
		return;
	}

	SelectedTalentNodeId = NAME_None;
	OnTalentSelectionChanged.Broadcast(NAME_None);
}

bool UGridSkillsWidget::GetSelectedTalentNode(FGridTalentNodeView& OutNode) const
{
	OutNode = FGridTalentNodeView();
	if (const FGridTalentNodeView* Node = FindTalentNode(SelectedTalentNodeId))
	{
		OutNode = *Node;
		return true;
	}
	return false;
}

bool UGridSkillsWidget::CommitConfirmedTalentChoice(FName ChoiceId, FText& OutFeedback)
{
	OutFeedback = FText::FromString(TEXT("Acquisition impossible."));

	FGridTalentNodeView SelectedNode;
	if (!IsValid(InventoryComponent) ||
		!View.IsValid() ||
		!GetSelectedTalentNode(SelectedNode) ||
		ChoiceId.IsNone())
	{
		FRPGClassProgressionCommitResult InvalidResult;
		InvalidResult.RejectReason = ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
		const FRPGProgressionNotificationView Notification =
			FRPGProgressionFeedbackService::MakeTalentCommitNotification(InvalidResult, FText::GetEmpty());
		OutFeedback = Notification.Message;
		PublishProgressionNotification(Notification);
		return false;
	}

	const FGridTalentVariantView* RequestedVariant = SelectedNode.Variants.FindByPredicate(
		[ChoiceId](const FGridTalentVariantView& Variant)
		{
			return Variant.ChoiceId == ChoiceId;
		});
	if (!RequestedVariant)
	{
		FRPGClassProgressionCommitResult InvalidResult;
		InvalidResult.RejectReason = ERPGClassProgressionCommitRejectReason::UnknownChoice;
		const FRPGProgressionNotificationView Notification =
			FRPGProgressionFeedbackService::MakeTalentCommitNotification(InvalidResult, FText::GetEmpty());
		OutFeedback = Notification.Message;
		PublishProgressionNotification(Notification);
		return false;
	}

	const FText RequestedDisplayName =
		RequestedVariant->DisplayName.IsEmpty() ? FText::FromName(ChoiceId) : RequestedVariant->DisplayName;

	FRPGClassProgressionCommitResult Result;
	const bool bCommitted = FRPGClassProgressionTransactionService::TryCommitChoices(
		InventoryComponent, View.CharacterIndex, { ChoiceId }, Result);

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeTalentCommitNotification(Result, RequestedDisplayName);
	OutFeedback = Notification.Message;
	PublishProgressionNotification(Notification);

	if (!bCommitted)
	{
		RefreshSkills();
		return false;
	}

	return true;
}

bool UGridSkillsWidget::CommitSkillRankIncrease(FName SkillId, FText& OutFeedback)
{
	OutFeedback = FText::FromString(TEXT("Attribution de compétence impossible."));

	const URPGSkillAsset* Definition = ResolveCanonicalSkillDefinition(SkillId);
	FText SkillDisplayName = FText::FromName(SkillId);
	if (const FGridSkillEntryView* Entry = View.Skills.FindByPredicate(
		[SkillId](const FGridSkillEntryView& Candidate)
		{
			return Candidate.SkillId == SkillId;
		}))
	{
		SkillDisplayName = Entry->DisplayName.IsEmpty() ? FText::FromName(SkillId) : Entry->DisplayName;
	}

	const FGuid SessionCharacterId = View.CharacterId;
	FRPGSkillPointMutationResult Result;
	const bool bCommitted =
		FRPGSkillPointService::TryPurchaseNextRank(InventoryComponent, View.CharacterIndex, Definition, Result);

	if (bCommitted)
	{
		RecordSessionSkillPurchase(SessionCharacterId, SkillId, Result.PreviousRank);
	}

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeSkillRankPurchaseNotification(Result, SkillDisplayName);
	OutFeedback = Notification.Message;
	PublishProgressionNotification(Notification);

	// The transaction broadcasts inventory changes synchronously, before the
	// UI-session history is updated. Refresh once more to project the undo state.
	RefreshSkills();
	return bCommitted;
}

bool UGridSkillsWidget::CommitSkillRankDecrease(FName SkillId, FText& OutFeedback)
{
	OutFeedback = FText::FromString(TEXT("Annulation de compétence impossible."));

	const URPGSkillAsset* Definition = ResolveCanonicalSkillDefinition(SkillId);
	FText SkillDisplayName = FText::FromName(SkillId);
	if (const FGridSkillEntryView* Entry = View.Skills.FindByPredicate(
		[SkillId](const FGridSkillEntryView& Candidate)
		{
			return Candidate.SkillId == SkillId;
		}))
	{
		SkillDisplayName = Entry->DisplayName.IsEmpty() ? FText::FromName(SkillId) : Entry->DisplayName;
	}

	const FGuid SessionCharacterId = View.CharacterId;
	const int32 SessionPurchaseCount = GetSessionPurchasedSkillRankCount(SessionCharacterId, SkillId);
	const int32 SessionFloor = GetSessionSkillRankFloor(SessionCharacterId, SkillId);

	FRPGSkillPointMutationResult Result;
	bool bCommitted = false;
	if (SessionPurchaseCount <= 0 || SessionFloor < 0)
	{
		Result.SkillId = SkillId;
		Result.RejectReason = ERPGSkillPointMutationRejectReason::NoSessionPurchaseToUndo;
	}
	else
	{
		bCommitted = FRPGSkillPointService::TryRefundPurchasedRank(
			InventoryComponent,
			View.CharacterIndex,
			Definition,
			SessionFloor,
			Result);
	}

	if (bCommitted)
	{
		ConsumeSessionSkillPurchase(SessionCharacterId, SkillId);
	}

	const FRPGProgressionNotificationView Notification =
		FRPGProgressionFeedbackService::MakeSkillRankRefundNotification(Result, SkillDisplayName);
	OutFeedback = Notification.Message;
	PublishProgressionNotification(Notification);

	RefreshSkills();
	return bCommitted;
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

void UGridSkillsWidget::ApplyDesignerPresentation()
{
	RebuildSkillEntryWidgets();

	if (!View.IsValid())
	{
		Text_CharacterName->SetText(FText::FromString(TEXT("Aucun personnage sélectionné")));
		Text_ClassLevel->SetText(FText::GetEmpty());
		Text_TalentPoints->SetText(FText::FromString(TEXT("Points de talent : —")));
		if (Text_SkillPoints)
		{
			Text_SkillPoints->SetText(FText::FromString(TEXT("Points de compétence : —")));
		}

		Branch_Left->ClearTalentBranch();
		Branch_Center->ClearTalentBranch();
		Branch_Right->ClearTalentBranch();
		return;
	}

	Text_CharacterName->SetText(
		View.CharacterName.IsEmpty() ? FText::FromString(TEXT("Personnage sélectionné")) : View.CharacterName);

	const FString ClassName =
		View.ClassDisplayName.IsEmpty() ? View.ClassId.ToString() : View.ClassDisplayName.ToString();
	Text_ClassLevel->SetText(
		FText::FromString(FString::Printf(TEXT("%s — Niveau %d"), *ClassName, View.CharacterLevel)));

	Text_TalentPoints->SetText(
		FText::FromString(FString::Printf(TEXT("Points de talent : %d"), View.RemainingTalentPoints)));
	if (Text_SkillPoints)
	{
		Text_SkillPoints->SetText(
			FText::FromString(FString::Printf(
				TEXT("Points de compétence : %d  —  Rang max : %d"),
				View.RemainingSkillPoints,
				View.SkillRankCap)));
	}

	FRPGClassPresentationDefinition ClassPresentation;
	if (GetCurrentClassPresentation(ClassPresentation))
	{
		Text_ClassLevel->SetColorAndOpacity(FSlateColor(ClassPresentation.AccentColor));
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UGridTalentBranchWidget* BranchWidget = GridSkillsWidgetPrivate::BranchWidgetByIndex(
			Index, Branch_Left, Branch_Center, Branch_Right);
		if (!BranchWidget)
		{
			continue;
		}

		FGridTalentBranchView BranchView;
		FRPGTalentBranchPresentationDefinition BranchPresentation;
		if (GetPresentedTalentBranch(Index, BranchView, BranchPresentation))
		{
			BranchWidget->InitializeTalentBranch(BranchView, BranchPresentation);
		}
		else
		{
			BranchWidget->ClearTalentBranch();
		}
	}
}

void UGridSkillsWidget::ApplyTalentDetailPresentation()
{
	if (!Detail_Talent)
	{
		return;
	}

	FGridTalentNodeView SelectedNode;
	if (!GetSelectedTalentNode(SelectedNode))
	{
		Detail_Talent->ClearTalentDetail();
		return;
	}

	FRPGClassPresentationDefinition ClassPresentation;
	if (!GetCurrentClassPresentation(ClassPresentation))
	{
		Detail_Talent->ClearTalentDetail();
		return;
	}

	const FRPGTalentBranchPresentationDefinition* BranchPresentation =
		ClassPresentation.FindBranch(SelectedNode.TalentBranchId);
	if (!BranchPresentation ||
		!Detail_Talent->InitializeTalentDetail(SelectedNode, *BranchPresentation))
	{
		Detail_Talent->ClearTalentDetail();
	}
}

void UGridSkillsWidget::RebuildSkillEntryWidgets()
{
	if (Text_EmptySkills)
	{
		Text_EmptySkills->SetVisibility(View.Skills.IsEmpty() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (!Panel_SkillEntries)
	{
		return;
	}

	Panel_SkillEntries->ClearChildren();
	if (View.Skills.IsEmpty())
	{
		return;
	}

	if (!SkillEntryWidgetClass)
	{
		UE_LOG(LogGridSkillsUI, Warning, TEXT("WBP_GridSkills has Skills data but no SkillEntryWidgetClass."));
		return;
	}

	for (const FGridSkillEntryView& Entry : View.Skills)
	{
		UGridSkillEntryWidget* EntryWidget = CreateWidget<UGridSkillEntryWidget>(this, SkillEntryWidgetClass);
		if (!EntryWidget)
		{
			UE_LOG(LogGridSkillsUI, Warning, TEXT("Failed to create Skill row for %s."), *Entry.SkillId.ToString());
			continue;
		}

		if (!EntryWidget->InitializeSkillEntry(Entry))
		{
			UE_LOG(LogGridSkillsUI, Warning, TEXT("Rejected invalid Skill row projection for %s."), *Entry.SkillId.ToString());
			continue;
		}

		EntryWidget->OnIncreaseSkillRequested.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillIncreaseRequested);
		EntryWidget->OnDecreaseSkillRequested.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillDecreaseRequested);
		Panel_SkillEntries->AddChild(EntryWidget);
	}
}

void UGridSkillsWidget::ShowSkillsTab()
{
	Switcher_SkillsTalents->SetActiveWidgetIndex(0);
}

void UGridSkillsWidget::ShowTalentsTab()
{
	Switcher_SkillsTalents->SetActiveWidgetIndex(1);
}

void UGridSkillsWidget::HandleSkillsTabClicked()
{
	ShowSkillsTab();
}

void UGridSkillsWidget::HandleTalentsTabClicked()
{
	ShowTalentsTab();
}

void UGridSkillsWidget::HandleTalentNodeClicked(FName TalentNodeId)
{
	if (SelectTalentNode(TalentNodeId))
	{
		ApplyTalentDetailPresentation();
	}
}

void UGridSkillsWidget::HandleTalentAcquireConfirmed(FName ChoiceId)
{
	FText Feedback;
	const bool bCommitted = CommitConfirmedTalentChoice(ChoiceId, Feedback);
	if (Detail_Talent)
	{
		// UI-RPG05: the toast is the sole success feedback surface.
		// Keep inline detail feedback only for a rejected transaction.
		Detail_Talent->SetAcquisitionFeedback(bCommitted ? FText::GetEmpty() : Feedback);
	}
}

void UGridSkillsWidget::HandleSkillIncreaseRequested(FName SkillId)
{
	FText Feedback;
	CommitSkillRankIncrease(SkillId, Feedback);
}

void UGridSkillsWidget::HandleSkillDecreaseRequested(FName SkillId)
{
	FText Feedback;
	CommitSkillRankDecrease(SkillId, Feedback);
}

int32 UGridSkillsWidget::GetSkillEntryCount() const
{
	return View.Skills.Num();
}

bool UGridSkillsWidget::GetSkillEntry(int32 EntryIndex, FGridSkillEntryView& OutEntry) const
{
	if (!View.Skills.IsValidIndex(EntryIndex))
	{
		OutEntry = FGridSkillEntryView();
		return false;
	}

	OutEntry = View.Skills[EntryIndex];
	return true;
}

int32 UGridSkillsWidget::GetTalentEntryCount() const
{
	return View.Talents.Num();
}

bool UGridSkillsWidget::GetTalentEntry(int32 EntryIndex, FGridTalentEntryView& OutEntry) const
{
	if (!View.Talents.IsValidIndex(EntryIndex))
	{
		OutEntry = FGridTalentEntryView();
		return false;
	}

	OutEntry = View.Talents[EntryIndex];
	return true;
}

void UGridSkillsWidget::HandlePartyInventoryChanged(int32 CharacterIndex)
{
	(void)CharacterIndex;
	RefreshSkills();
}
