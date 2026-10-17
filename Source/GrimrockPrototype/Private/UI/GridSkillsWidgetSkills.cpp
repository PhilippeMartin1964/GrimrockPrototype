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
#include "Internationalization/TextComparison.h"
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
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentNodeWidget.h"
#include "UI/GridTalentVariantFlyoutWidget.h"
#include "UI/GridRPGNotificationWidget.h"
#include "UI/RPGProgressionFeedbackService.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UI/UILocalizationText.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridSkillsUI, Log, All);

namespace GridSkillsWidgetPrivate
{
	int32 NormalizeComparison(int32 Comparison)
	{
		return Comparison < 0 ? -1 : (Comparison > 0 ? 1 : 0);
	}

	int32 CompareLocalizedText(const FText& Left, const FText& Right)
	{
		return NormalizeComparison(
			FTextComparison::CompareToCaseIgnored(Left.ToString(), Right.ToString()));
	}

	int32 CompareSkillNames(const FGridSkillEntryView& Left, const FGridSkillEntryView& Right)
	{
		const int32 NameComparison =
			CompareLocalizedText(
				UGridSkillEntryWidget::ResolveDisplayName(Left),
				UGridSkillEntryWidget::ResolveDisplayName(Right));
		if (NameComparison != 0)
		{
			return NameComparison;
		}

		// Technical identity is only the deterministic final tie-breaker.
		return NormalizeComparison(
			Left.SkillId.ToString().Compare(
				Right.SkillId.ToString(),
				ESearchCase::CaseSensitive));
	}

	int32 GetAffinitySortPriority(EGridSkillClassAffinity Affinity)
	{
		switch (Affinity)
		{
			case EGridSkillClassAffinity::Recommended:
				return 0;
			case EGridSkillClassAffinity::Useful:
				return 1;
			case EGridSkillClassAffinity::Neutral:
				return 2;
			case EGridSkillClassAffinity::Discouraged:
				return 3;
			default:
				return 4;
		}
	}

	int32 ComparePrimaryColumn(
		const FGridSkillEntryView& Left,
		const FGridSkillEntryView& Right,
		EGridSkillSortColumn Column)
	{
		switch (Column)
		{
			case EGridSkillSortColumn::Attribute:
				return CompareLocalizedText(
					UGridSkillEntryWidget::ResolveAttributeLabel(Left),
					UGridSkillEntryWidget::ResolveAttributeLabel(Right));

			case EGridSkillSortColumn::Affinity:
			{
				const int32 LeftPriority = GetAffinitySortPriority(Left.ClassAffinity);
				const int32 RightPriority = GetAffinitySortPriority(Right.ClassAffinity);
				return LeftPriority < RightPriority ? -1 : (LeftPriority > RightPriority ? 1 : 0);
			}

			case EGridSkillSortColumn::Rank:
				return Left.Rank < Right.Rank ? -1 : (Left.Rank > Right.Rank ? 1 : 0);

			case EGridSkillSortColumn::Name:
			default:
				return CompareSkillNames(Left, Right);
		}
	}

	int32 CompareSkillEntries(
		const FGridSkillEntryView& Left,
		const FGridSkillEntryView& Right,
		EGridSkillSortColumn Column,
		EGridSkillSortDirection Direction)
	{
		const int32 PrimaryComparison = ComparePrimaryColumn(Left, Right, Column);
		if (PrimaryComparison != 0)
		{
			return Direction == EGridSkillSortDirection::Ascending
				? PrimaryComparison
				: -PrimaryComparison;
		}

		// Non-name ties stay alphabetic in both directions.
		return Column == EGridSkillSortColumn::Name
			? 0
			: CompareSkillNames(Left, Right);
	}
}

void UGridSkillsWidget::BeginSkillAllocationSession()
{
	SessionPurchasedSkillRanks.Reset();
	SessionSkillRankFloors.Reset();
	RefreshSkills();
}

void UGridSkillsWidget::RefreshSelectedSkillDetail(const TArray<const URPGSkillAsset*>& SkillDefinitions)
{
	SelectedSkillDetail = FGridSkillDetailView();
	if (!InventoryComponent || SelectedSkillId.IsNone())
	{
		return;
	}

	FGridSkillsPageService::TryBuildSkillDetailView(
		InventoryComponent,
		View,
		SelectedSkillId,
		SkillDefinitions,
		SkillPresentationAsset.Get(),
		SelectedSkillDetail);
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

bool UGridSkillsWidget::CommitSkillRankIncrease(FName SkillId, FText& OutFeedback)
{
	OutFeedback = FUILocalizationText::Get(TEXT("Progression.Skill.AllocationFailed"));

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
	bool bCommitted = false;
	{
		TGuardValue<bool> TransactionGuard(bSkillAllocationTransactionInProgress, true);
		bCommitted =
			FRPGSkillPointService::TryPurchaseNextRank(
				InventoryComponent,
				View.CharacterIndex,
				Definition,
				Result);
	}

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
	OutFeedback = FUILocalizationText::Get(TEXT("Progression.Skill.CancellationFailed"));

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
		TGuardValue<bool> TransactionGuard(bSkillAllocationTransactionInProgress, true);
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

void UGridSkillsWidget::RefreshSkillSummaryPresentation()
{
	const bool bHasCharacter = View.IsValid();
	const FText UnavailableValue = FText::FromString(TEXT("—"));

	Text_AvailableSkillPoints->SetText(
		bHasCharacter
			? FText::AsNumber(View.RemainingSkillPoints)
			: UnavailableValue);

	Text_CurrentSkillRankCap->SetText(
		bHasCharacter
			? FText::AsNumber(View.SkillRankCap)
			: UnavailableValue);
}

void UGridSkillsWidget::ApplySkillDetailPresentation()
{
	if (!Detail_Skill)
	{
		return;
	}

	if (!SelectedSkillDetail.IsValid() ||
		!Detail_Skill->InitializeSkillDetail(SelectedSkillDetail))
	{
		Detail_Skill->ClearSkillDetail();
	}
}

void UGridSkillsWidget::SortSkillEntries(
	TArray<FGridSkillEntryView>& InOutEntries,
	EGridSkillSortColumn Column,
	EGridSkillSortDirection Direction)
{
	InOutEntries.Sort(
		[Column, Direction](const FGridSkillEntryView& Left, const FGridSkillEntryView& Right)
		{
			return GridSkillsWidgetPrivate::CompareSkillEntries(
				Left,
				Right,
				Column,
				Direction) < 0;
		});
}
void UGridSkillsWidget::SetSkillSort(
	EGridSkillSortColumn Column,
	EGridSkillSortDirection Direction)
{
	SkillSortColumn = Column;
	SkillSortDirection = Direction;
	RebuildSkillEntryWidgets();
	RefreshSkillSortIndicators();
}

void UGridSkillsWidget::ToggleSkillSortColumn(EGridSkillSortColumn Column)
{
	if (SkillSortColumn == Column)
	{
		SkillSortDirection =
			SkillSortDirection == EGridSkillSortDirection::Ascending
				? EGridSkillSortDirection::Descending
				: EGridSkillSortDirection::Ascending;
	}
	else
	{
		SkillSortColumn = Column;
		SkillSortDirection = EGridSkillSortDirection::Ascending;
	}

	RebuildSkillEntryWidgets();
	RefreshSkillSortIndicators();
}

void UGridSkillsWidget::HandleSortNameClicked()
{
	ToggleSkillSortColumn(EGridSkillSortColumn::Name);
}

void UGridSkillsWidget::HandleSortAttributeClicked()
{
	ToggleSkillSortColumn(EGridSkillSortColumn::Attribute);
}

void UGridSkillsWidget::HandleSortAffinityClicked()
{
	ToggleSkillSortColumn(EGridSkillSortColumn::Affinity);
}

void UGridSkillsWidget::HandleSortRankClicked()
{
	ToggleSkillSortColumn(EGridSkillSortColumn::Rank);
}

void UGridSkillsWidget::RefreshSkillSortIndicators()
{
	const FText AscendingGlyph = FText::FromString(TEXT("▲"));
	const FText DescendingGlyph = FText::FromString(TEXT("▼"));

	auto ApplyIndicator = [this, &AscendingGlyph, &DescendingGlyph](
		UTextBlock* Indicator,
		EGridSkillSortColumn Column)
	{
		if (!Indicator)
		{
			return;
		}

		const bool bActive = SkillSortColumn == Column;
		Indicator->SetText(
			bActive
				? (SkillSortDirection == EGridSkillSortDirection::Ascending
					? AscendingGlyph
					: DescendingGlyph)
				: FText::GetEmpty());
		Indicator->SetVisibility(
			bActive ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	};

	ApplyIndicator(Text_SortNameIndicator, EGridSkillSortColumn::Name);
	ApplyIndicator(Text_SortAttributeIndicator, EGridSkillSortColumn::Attribute);
	ApplyIndicator(Text_SortAffinityIndicator, EGridSkillSortColumn::Affinity);
	ApplyIndicator(Text_SortRankIndicator, EGridSkillSortColumn::Rank);
}
UTexture2D* UGridSkillsWidget::ResolveCachedSkillIcon(
	URPGSkillPresentationAsset* PresentationCatalog,
	FName SkillId)
{
	if (CachedSkillIconPresentationAsset != PresentationCatalog)
	{
		CachedSkillIconTextures.Reset();
		CachedSkillIconPresentationAsset = PresentationCatalog;
	}

	if (!PresentationCatalog || SkillId.IsNone())
	{
		return nullptr;
	}

	if (const TObjectPtr<UTexture2D>* CachedIcon = CachedSkillIconTextures.Find(SkillId))
	{
		return CachedIcon->Get();
	}

	UTexture2D* IconTexture = nullptr;
	if (const FRPGSkillPresentationDefinition* Presentation =
		PresentationCatalog->FindSkill(SkillId))
	{
		IconTexture = Presentation->Icon.Get();
		if (!IconTexture && !Presentation->Icon.IsNull())
		{
			IconTexture = Presentation->Icon.LoadSynchronous();
		}
	}

	CachedSkillIconTextures.Add(SkillId, IconTexture);
	return IconTexture;
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

	URPGSkillPresentationAsset* PresentationCatalog =
		IsValid(SkillPresentationAsset) && SkillPresentationAsset->IsValidDefinition()
			? SkillPresentationAsset.Get()
			: nullptr;

	TArray<FGridSkillEntryView> SortedEntries = View.Skills;
	SortSkillEntries(SortedEntries, SkillSortColumn, SkillSortDirection);

	for (const FGridSkillEntryView& Entry : SortedEntries)
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

		EntryWidget->SetSkillIcon(
			ResolveCachedSkillIcon(PresentationCatalog, Entry.SkillId));
		EntryWidget->SetSelected(Entry.SkillId == SelectedSkillId);

		EntryWidget->OnSelectSkillRequested.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillSelectionRequested);
		EntryWidget->OnIncreaseSkillRequested.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillIncreaseRequested);
		EntryWidget->OnDecreaseSkillRequested.AddUniqueDynamic(this, &UGridSkillsWidget::HandleSkillDecreaseRequested);
		Panel_SkillEntries->AddChild(EntryWidget);
	}
}

void UGridSkillsWidget::RefreshSkillEntrySelection()
{
	if (!Panel_SkillEntries)
	{
		return;
	}

	const int32 ChildCount = Panel_SkillEntries->GetChildrenCount();
	for (int32 Index = 0; Index < ChildCount; ++Index)
	{
		if (UGridSkillEntryWidget* EntryWidget =
			Cast<UGridSkillEntryWidget>(Panel_SkillEntries->GetChildAt(Index)))
		{
			EntryWidget->SetSelected(
				!SelectedSkillId.IsNone() &&
				EntryWidget->Entry.SkillId == SelectedSkillId);
		}
	}
}

void UGridSkillsWidget::HandleSkillSelectionRequested(FName SkillId)
{
	SelectSkill(SkillId);
}

void UGridSkillsWidget::HandleSkillIncreaseRequested(FName SkillId)
{
	SelectSkill(SkillId);
	FText Feedback;
	CommitSkillRankIncrease(SkillId, Feedback);
}

void UGridSkillsWidget::HandleSkillDecreaseRequested(FName SkillId)
{
	SelectSkill(SkillId);
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

bool UGridSkillsWidget::SelectSkill(FName SkillId)
{
	if (SkillId.IsNone() ||
		!View.Skills.ContainsByPredicate(
			[SkillId](const FGridSkillEntryView& Entry)
			{
				return Entry.SkillId == SkillId;
			}))
	{
		return false;
	}

	const bool bChanged = SelectedSkillId != SkillId;
	SelectedSkillId = SkillId;

	if (InventoryComponent)
	{
		TArray<const URPGSkillAsset*> SkillDefinitions;
		GetCanonicalSkillDefinitions(SkillDefinitions);
		RefreshSelectedSkillDetail(SkillDefinitions);
	}
	else
	{
		SelectedSkillDetail = FGridSkillDetailView();
	}

	RefreshSkillEntrySelection();
	ApplySkillDetailPresentation();
	if (bChanged)
	{
		OnSkillSelectionChanged.Broadcast(SelectedSkillId);
	}
	return true;
}

void UGridSkillsWidget::ClearSkillSelection()
{
	if (SelectedSkillId.IsNone() && !SelectedSkillDetail.IsValid())
	{
		return;
	}

	SelectedSkillId = NAME_None;
	SelectedSkillDetail = FGridSkillDetailView();
	RefreshSkillEntrySelection();
	ApplySkillDetailPresentation();
	OnSkillSelectionChanged.Broadcast(NAME_None);
}

bool UGridSkillsWidget::GetSelectedSkillDetail(FGridSkillDetailView& OutDetail) const
{
	OutDetail = FGridSkillDetailView();
	if (!SelectedSkillDetail.IsValid())
	{
		return false;
	}

	OutDetail = SelectedSkillDetail;
	return true;
}

