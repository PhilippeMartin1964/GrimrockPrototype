#include "UI/GridEventFeedWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GridEventFeedWidget"

UGridEventFeedWidget::UGridEventFeedWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentPanelHeight = ClampPanelHeight(InitialPanelHeight, MinPanelHeight, MaxPanelHeight);
}

void UGridEventFeedWidget::SetPanelHeight(float InHeight)
{
	CurrentPanelHeight = ClampPanelHeight(InHeight, MinPanelHeight, MaxPanelHeight);
	SynchronizeShell();
}

float UGridEventFeedWidget::GetPanelHeight() const
{
	return CurrentPanelHeight;
}

float UGridEventFeedWidget::GetPanelWidth() const
{
	return FMath::Max(1.0f, PanelWidth);
}

float UGridEventFeedWidget::GetMinPanelHeight() const
{
	return FMath::Max(1.0f, FMath::Min(MinPanelHeight, MaxPanelHeight));
}

float UGridEventFeedWidget::GetMaxPanelHeight() const
{
	return FMath::Max(GetMinPanelHeight(), FMath::Max(MinPanelHeight, MaxPanelHeight));
}

void UGridEventFeedWidget::SetCategoryEnabled(EGridHudEventCategory Category, bool bEnabled)
{
	bool* Target = nullptr;
	switch (Category)
	{
		case EGridHudEventCategory::Dungeon:
			Target = &bShowEvents;
			break;
		case EGridHudEventCategory::Combat:
			Target = &bShowCombat;
			break;
		case EGridHudEventCategory::Dialogue:
			Target = &bShowDiscussion;
			break;
		default:
			break;
	}

	if (!Target || *Target == bEnabled)
	{
		return;
	}

	*Target = bEnabled;
	RebuildEntryPresentation();
	SynchronizeShell();
}

void UGridEventFeedWidget::ToggleCategory(EGridHudEventCategory Category)
{
	SetCategoryEnabled(Category, !IsCategoryEnabled(Category));
}

bool UGridEventFeedWidget::IsCategoryEnabled(EGridHudEventCategory Category) const
{
	switch (Category)
	{
		case EGridHudEventCategory::Dungeon:
			return bShowEvents;
		case EGridHudEventCategory::Combat:
			return bShowCombat;
		case EGridHudEventCategory::Dialogue:
			return bShowDiscussion;
		default:
			return false;
	}
}

void UGridEventFeedWidget::FilterEntries(
	const TArray<FGridHudEventEntry>& SourceEntries,
	bool bInShowEvents,
	bool bInShowCombat,
	bool bInShowDiscussion,
	TArray<FGridHudEventEntry>& OutEntries)
{
	OutEntries.Reset();
	OutEntries.Reserve(SourceEntries.Num());

	for (const FGridHudEventEntry& Entry : SourceEntries)
	{
		const bool bVisible =
			(Entry.Category == EGridHudEventCategory::Dungeon && bInShowEvents) ||
			(Entry.Category == EGridHudEventCategory::Combat && bInShowCombat) ||
			(Entry.Category == EGridHudEventCategory::Dialogue && bInShowDiscussion);
		if (bVisible)
		{
			OutEntries.Add(Entry);
		}
	}
}

float UGridEventFeedWidget::ClampPanelHeight(float InHeight, float InMinHeight, float InMaxHeight)
{
	const float SafeMin = FMath::Max(1.0f, FMath::Min(InMinHeight, InMaxHeight));
	const float SafeMax = FMath::Max(SafeMin, FMath::Max(InMinHeight, InMaxHeight));
	return FMath::Clamp(InHeight, SafeMin, SafeMax);
}

bool UGridEventFeedWidget::IsScrollAtEnd(float CurrentOffset, float EndOffset, float Tolerance)
{
	return FMath::Max(0.0f, EndOffset) - FMath::Max(0.0f, CurrentOffset) <= FMath::Max(0.0f, Tolerance);
}

void UGridEventFeedWidget::BeginResizeAtScreenY(float ScreenY)
{
	bResizeDragging = true;
	ResizeStartScreenY = ScreenY;
	ResizeStartHeight = CurrentPanelHeight;
}

void UGridEventFeedWidget::UpdateResizeAtScreenY(float ScreenY)
{
	if (!bResizeDragging)
	{
		return;
	}

	SetPanelHeight(ResizeStartHeight + (ResizeStartScreenY - ScreenY));
}

void UGridEventFeedWidget::EndResize()
{
	bResizeDragging = false;
}

void UGridEventFeedWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	CurrentPanelHeight = ClampPanelHeight(CurrentPanelHeight, MinPanelHeight, MaxPanelHeight);
	SynchronizeShell();
}

TSharedRef<SWidget> UGridEventFeedWidget::RebuildWidget()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	return SAssignNew(NativeSizeBox, SBox)
		.WidthOverride(GetPanelWidth())
		.HeightOverride(GetPanelHeight())
		[
			SNew(SBorder)
			.Padding(FMargin(6.0f))
			.BorderBackgroundColor(ShellBackgroundColor)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 4.0f)
				[
					SNew(SButton)
						.ContentPadding(FMargin(0.0f))
						.ToolTipText(LOCTEXT("ResizeTooltip", "Redimensionner le flux"))
						.OnPressed(FSimpleDelegate::CreateUObject(this, &UGridEventFeedWidget::HandleResizePressed))
						.OnReleased(FSimpleDelegate::CreateUObject(this, &UGridEventFeedWidget::HandleResizeReleased))
						[
							SNew(SBox)
								.HeightOverride(8.0f)
						]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SAssignNew(NativeScrollBox, SScrollBox)
						.OnUserScrolled_Lambda([this](float CurrentOffset)
						{
							HandleNativeUserScrolled(CurrentOffset);
						})
						+ SScrollBox::Slot()
						[
							SAssignNew(NativeEntryList, SVerticalBox)
						]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 0.0f, 2.0f, 0.0f)
						[
							SNew(SButton)
								.HAlign(HAlign_Center)
								.Text_Lambda([this]() { return BuildFilterText(EGridHudEventCategory::Dungeon); })
								.OnClicked_Lambda([this]()
								{
									ToggleCategory(EGridHudEventCategory::Dungeon);
									return FReply::Handled();
								})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(2.0f, 0.0f)
						[
							SNew(SButton)
								.HAlign(HAlign_Center)
								.Text_Lambda([this]() { return BuildFilterText(EGridHudEventCategory::Combat); })
								.OnClicked_Lambda([this]()
								{
									ToggleCategory(EGridHudEventCategory::Combat);
									return FReply::Handled();
								})
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(2.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
								.HAlign(HAlign_Center)
								.Text_Lambda([this]() { return BuildFilterText(EGridHudEventCategory::Dialogue); })
								.OnClicked_Lambda([this]()
								{
									ToggleCategory(EGridHudEventCategory::Dialogue);
									return FReply::Handled();
								})
						]
				]
			]
		];
}

void UGridEventFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	BindControls();
	BindEventFeed();
	SynchronizeShell();
	RebuildEntryPresentation(true);
}

void UGridEventFeedWidget::NativeDestruct()
{
	EndResize();
	UnbindEventFeed();
	UnbindControls();
	Super::NativeDestruct();
}

void UGridEventFeedWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bResizeDragging && FSlateApplication::IsInitialized())
	{
		UpdateResizeAtScreenY(static_cast<float>(FSlateApplication::Get().GetCursorPos().Y));
	}
}

void UGridEventFeedWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	NativeSizeBox.Reset();
	NativeScrollBox.Reset();
	NativeEntryList.Reset();
}

void UGridEventFeedWidget::SynchronizeShell()
{
	PanelWidth = FMath::Max(1.0f, PanelWidth);
	CurrentPanelHeight = ClampPanelHeight(CurrentPanelHeight, MinPanelHeight, MaxPanelHeight);

	if (SizeBox_EventFeed)
	{
		SizeBox_EventFeed->SetWidthOverride(PanelWidth);
		SizeBox_EventFeed->SetHeightOverride(CurrentPanelHeight);
	}
	if (Border_EventFeed)
	{
		Border_EventFeed->SetBrushColor(ShellBackgroundColor);
		Border_EventFeed->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (ScrollBox_Entries)
	{
		ScrollBox_Entries->SetVisibility(ESlateVisibility::Visible);
	}
	if (Text_FilterEvents)
	{
		Text_FilterEvents->SetText(BuildFilterText(EGridHudEventCategory::Dungeon));
	}
	if (Text_FilterCombat)
	{
		Text_FilterCombat->SetText(BuildFilterText(EGridHudEventCategory::Combat));
	}
	if (Text_FilterDiscussion)
	{
		Text_FilterDiscussion->SetText(BuildFilterText(EGridHudEventCategory::Dialogue));
	}
	if (NativeSizeBox.IsValid())
	{
		NativeSizeBox->SetWidthOverride(PanelWidth);
		NativeSizeBox->SetHeightOverride(CurrentPanelHeight);
	}
}

void UGridEventFeedWidget::BindControls()
{
	if (Button_FilterEvents)
	{
		Button_FilterEvents->OnClicked.AddUniqueDynamic(this, &UGridEventFeedWidget::HandleFilterEventsClicked);
	}
	if (Button_FilterCombat)
	{
		Button_FilterCombat->OnClicked.AddUniqueDynamic(this, &UGridEventFeedWidget::HandleFilterCombatClicked);
	}
	if (Button_FilterDiscussion)
	{
		Button_FilterDiscussion->OnClicked.AddUniqueDynamic(this, &UGridEventFeedWidget::HandleFilterDiscussionClicked);
	}
	if (Button_ResizeHandle)
	{
		Button_ResizeHandle->OnPressed.AddUniqueDynamic(this, &UGridEventFeedWidget::HandleResizePressed);
		Button_ResizeHandle->OnReleased.AddUniqueDynamic(this, &UGridEventFeedWidget::HandleResizeReleased);
	}
	if (ScrollBox_Entries)
	{
		ScrollBox_Entries->OnUserScrolled.AddUniqueDynamic(this, &UGridEventFeedWidget::HandleBlueprintUserScrolled);
	}
}

void UGridEventFeedWidget::UnbindControls()
{
	if (Button_FilterEvents)
	{
		Button_FilterEvents->OnClicked.RemoveDynamic(this, &UGridEventFeedWidget::HandleFilterEventsClicked);
	}
	if (Button_FilterCombat)
	{
		Button_FilterCombat->OnClicked.RemoveDynamic(this, &UGridEventFeedWidget::HandleFilterCombatClicked);
	}
	if (Button_FilterDiscussion)
	{
		Button_FilterDiscussion->OnClicked.RemoveDynamic(this, &UGridEventFeedWidget::HandleFilterDiscussionClicked);
	}
	if (Button_ResizeHandle)
	{
		Button_ResizeHandle->OnPressed.RemoveDynamic(this, &UGridEventFeedWidget::HandleResizePressed);
		Button_ResizeHandle->OnReleased.RemoveDynamic(this, &UGridEventFeedWidget::HandleResizeReleased);
	}
	if (ScrollBox_Entries)
	{
		ScrollBox_Entries->OnUserScrolled.RemoveDynamic(this, &UGridEventFeedWidget::HandleBlueprintUserScrolled);
	}
}

void UGridEventFeedWidget::BindEventFeed()
{
	UnbindEventFeed();
	if (UWorld* World = GetWorld())
	{
		if (UGridEventFeedSubsystem* Subsystem = World->GetSubsystem<UGridEventFeedSubsystem>())
		{
			EventFeedSubsystem = Subsystem;
			Subsystem->OnEventPublished.AddUniqueDynamic(this, &UGridEventFeedWidget::HandleEventPublished);
		}
	}
}

void UGridEventFeedWidget::UnbindEventFeed()
{
	if (UGridEventFeedSubsystem* Subsystem = EventFeedSubsystem.Get())
	{
		Subsystem->OnEventPublished.RemoveDynamic(this, &UGridEventFeedWidget::HandleEventPublished);
	}
	EventFeedSubsystem.Reset();
}

void UGridEventFeedWidget::RebuildEntryPresentation(bool bForceScrollToEnd)
{
	const float PreviousOffset = GetPresentationScrollOffset();
	const bool bWasAtEnd = bForceScrollToEnd ||
		IsScrollAtEnd(PreviousOffset, GetPresentationScrollEndOffset());

	TArray<FGridHudEventEntry> VisibleEntries;
	if (const UGridEventFeedSubsystem* Subsystem = EventFeedSubsystem.Get())
	{
		FilterEntries(Subsystem->GetEntries(), bShowEvents, bShowCombat, bShowDiscussion, VisibleEntries);
	}

	if (ScrollBox_Entries)
	{
		ScrollBox_Entries->ClearChildren();
		for (const FGridHudEventEntry& Entry : VisibleEntries)
		{
			UTextBlock* Row = WidgetTree
				? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
				: NewObject<UTextBlock>(this);
			if (!Row)
			{
				continue;
			}
			Row->SetText(BuildEntryText(Entry));
			Row->SetAutoWrapText(true);
			Row->SetVisibility(ESlateVisibility::HitTestInvisible);
			ScrollBox_Entries->AddChild(Row);
		}
	}

	if (NativeEntryList.IsValid())
	{
		NativeEntryList->ClearChildren();
		for (const FGridHudEventEntry& Entry : VisibleEntries)
		{
			NativeEntryList->AddSlot()
				.AutoHeight()
				.Padding(2.0f, 2.0f, 6.0f, 6.0f)
				[
					SNew(STextBlock)
						.AutoWrapText(true)
						.Text(BuildEntryText(Entry))
				];
		}
	}

	if (bWasAtEnd)
	{
		bFollowNewestEntries = true;
		ScrollPresentationToEnd();
	}
	else
	{
		bFollowNewestEntries = false;
		if (ScrollBox_Entries)
		{
			ScrollBox_Entries->SetScrollOffset(PreviousOffset);
		}
		if (NativeScrollBox.IsValid())
		{
			NativeScrollBox->SetScrollOffset(PreviousOffset);
		}
	}
}

void UGridEventFeedWidget::ScrollPresentationToEnd()
{
	if (ScrollBox_Entries)
	{
		ScrollBox_Entries->ScrollToEnd();
	}
	if (NativeScrollBox.IsValid())
	{
		NativeScrollBox->ScrollToEnd();
	}
}

float UGridEventFeedWidget::GetPresentationScrollOffset() const
{
	if (ScrollBox_Entries)
	{
		return ScrollBox_Entries->GetScrollOffset();
	}
	return NativeScrollBox.IsValid() ? NativeScrollBox->GetScrollOffset() : 0.0f;
}

float UGridEventFeedWidget::GetPresentationScrollEndOffset() const
{
	if (ScrollBox_Entries)
	{
		return ScrollBox_Entries->GetScrollOffsetOfEnd();
	}
	return NativeScrollBox.IsValid() ? NativeScrollBox->GetScrollOffsetOfEnd() : 0.0f;
}

FText UGridEventFeedWidget::BuildEntryText(const FGridHudEventEntry& Entry) const
{
	TArray<FText> Lines;
	if (!Entry.Header.IsEmpty())
	{
		Lines.Add(Entry.Header);
	}

	if (!Entry.Message.IsEmpty())
	{
		Lines.Add(Entry.Message);
	}
	else if (!Entry.MessageFragments.IsEmpty())
	{
		TArray<FText> FragmentTexts;
		FragmentTexts.Reserve(Entry.MessageFragments.Num());
		for (const FGridHudEventTextFragment& Fragment : Entry.MessageFragments)
		{
			FragmentTexts.Add(Fragment.Text);
		}
		Lines.Add(FText::Join(FText::GetEmpty(), FragmentTexts));
	}

	for (const FText& Detail : Entry.Details)
	{
		if (!Detail.IsEmpty())
		{
			Lines.Add(Detail);
		}
	}

	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

FText UGridEventFeedWidget::BuildFilterText(EGridHudEventCategory Category) const
{
	const bool bEnabled = IsCategoryEnabled(Category);
	switch (Category)
	{
		case EGridHudEventCategory::Dungeon:
			return FText::Format(
				LOCTEXT("EventsFilterFormat", "Événements [{0}]"),
				bEnabled ? LOCTEXT("FilterOnEvents", "ON") : LOCTEXT("FilterOffEvents", "OFF"));
		case EGridHudEventCategory::Combat:
			return FText::Format(
				LOCTEXT("CombatFilterFormat", "Combat [{0}]"),
				bEnabled ? LOCTEXT("FilterOnCombat", "ON") : LOCTEXT("FilterOffCombat", "OFF"));
		case EGridHudEventCategory::Dialogue:
			return FText::Format(
				LOCTEXT("DiscussionFilterFormat", "Discussion [{0}]"),
				bEnabled ? LOCTEXT("FilterOnDiscussion", "ON") : LOCTEXT("FilterOffDiscussion", "OFF"));
		default:
			return FText::GetEmpty();
	}
}

void UGridEventFeedWidget::HandleNativeUserScrolled(float CurrentOffset)
{
	bFollowNewestEntries = IsScrollAtEnd(CurrentOffset, GetPresentationScrollEndOffset());
}

void UGridEventFeedWidget::BeginResizeFromCursor()
{
	if (FSlateApplication::IsInitialized())
	{
		BeginResizeAtScreenY(static_cast<float>(FSlateApplication::Get().GetCursorPos().Y));
	}
}

void UGridEventFeedWidget::HandleFilterEventsClicked()
{
	ToggleCategory(EGridHudEventCategory::Dungeon);
}

void UGridEventFeedWidget::HandleFilterCombatClicked()
{
	ToggleCategory(EGridHudEventCategory::Combat);
}

void UGridEventFeedWidget::HandleFilterDiscussionClicked()
{
	ToggleCategory(EGridHudEventCategory::Dialogue);
}

void UGridEventFeedWidget::HandleResizePressed()
{
	BeginResizeFromCursor();
}

void UGridEventFeedWidget::HandleResizeReleased()
{
	EndResize();
}

void UGridEventFeedWidget::HandleBlueprintUserScrolled(float CurrentOffset)
{
	bFollowNewestEntries = IsScrollAtEnd(CurrentOffset, GetPresentationScrollEndOffset());
}

void UGridEventFeedWidget::HandleEventPublished(const FGridHudEventEntry& Entry)
{
	if (!IsCategoryEnabled(Entry.Category))
	{
		return;
	}

	const bool bShouldFollow = bFollowNewestEntries &&
		IsScrollAtEnd(GetPresentationScrollOffset(), GetPresentationScrollEndOffset());
	RebuildEntryPresentation(bShouldFollow);
}

#undef LOCTEXT_NAMESPACE
