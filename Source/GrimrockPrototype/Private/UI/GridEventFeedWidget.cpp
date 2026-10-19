#include "UI/GridEventFeedWidget.h"
#include "UI/GridNavigationTooltipWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Runtime/GridEventFeedSubsystem.h"

#define LOCTEXT_NAMESPACE "GridEventFeedWidget"

UGridEventFeedWidget::UGridEventFeedWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CurrentPanelHeight = ClampPanelHeight(InitialPanelHeight, MinPanelHeight, MaxPanelHeight);

	static ConstructorHelpers::FObjectFinder<UFont> EventFeedFontFinder(
		TEXT("/Game/GrimrockPrototype/Core/Fonts/F_AlegreyaSans.F_AlegreyaSans"));
	if (EventFeedFontFinder.Succeeded())
	{
		EntryFont = FSlateFontInfo(EventFeedFontFinder.Object, 16);
	}
	else
	{
		EntryFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 16);
	}
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

FLinearColor UGridEventFeedWidget::ResolveSemanticColor(
	EGridHudEventSemantic Semantic,
	const FGridEventFeedSemanticPalette& Palette)
{
	switch (Semantic)
	{
		case EGridHudEventSemantic::Information: return Palette.Information;
		case EGridHudEventSemantic::Interaction: return Palette.Interaction;
		case EGridHudEventSemantic::Locked: return Palette.Locked;
		case EGridHudEventSemantic::Mechanism: return Palette.Mechanism;
		case EGridHudEventSemantic::Secret: return Palette.Secret;
		case EGridHudEventSemantic::Danger: return Palette.Danger;
		case EGridHudEventSemantic::PhysicalDamage: return Palette.PhysicalDamage;
		case EGridHudEventSemantic::Fire: return Palette.Fire;
		case EGridHudEventSemantic::Ice: return Palette.Ice;
		case EGridHudEventSemantic::Lightning: return Palette.Lightning;
		case EGridHudEventSemantic::Poison: return Palette.Poison;
		case EGridHudEventSemantic::Arcane: return Palette.Arcane;
		case EGridHudEventSemantic::Healing: return Palette.Healing;
		case EGridHudEventSemantic::Buff: return Palette.Buff;
		case EGridHudEventSemantic::Debuff: return Palette.Debuff;
		case EGridHudEventSemantic::Dialogue: return Palette.Dialogue;
		case EGridHudEventSemantic::Holy: return Palette.Holy;
		case EGridHudEventSemantic::Necrotic: return Palette.Necrotic;
		case EGridHudEventSemantic::Default:
		default:
			return Palette.Default;
	}
}

FLinearColor UGridEventFeedWidget::GetSemanticColor(EGridHudEventSemantic Semantic) const
{
	return ResolveSemanticColor(Semantic, SemanticPalette);
}

void UGridEventFeedWidget::BeginResizeAtScreenY(float ScreenY, float ViewportScale)
{
	bResizeDragging = true;
	ResizeStartScreenY = ScreenY;
	ResizeStartHeight = CurrentPanelHeight;
	ResizeViewportScale = FMath::Max(KINDA_SMALL_NUMBER, ViewportScale);
}

void UGridEventFeedWidget::UpdateResizeAtScreenY(float ScreenY)
{
	if (!bResizeDragging)
	{
		return;
	}

	const float ScreenDelta = ResizeStartScreenY - ScreenY;
	SetPanelHeight(ResizeStartHeight + ScreenDeltaToViewportDelta(ScreenDelta, ResizeViewportScale));
}

void UGridEventFeedWidget::EndResize()
{
	bResizeDragging = false;
}

float UGridEventFeedWidget::ScreenDeltaToViewportDelta(float ScreenDelta, float ViewportScale)
{
	return ScreenDelta / FMath::Max(KINDA_SMALL_NUMBER, ViewportScale);
}

void UGridEventFeedWidget::SetSharedTooltipWidgetClass(
	TSubclassOf<UGridNavigationTooltipWidget> InTooltipWidgetClass)
{
	SharedTooltipWidgetClass = InTooltipWidgetClass;
}

void UGridEventFeedWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	CurrentPanelHeight = ClampPanelHeight(CurrentPanelHeight, MinPanelHeight, MaxPanelHeight);
	SynchronizeShell();
}

void UGridEventFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// The project renders Cursor_Default through its custom cursor widget.
	// Hit-testable Event Feed controls must not let Slate re-expose the platform cursor.
	SetCursor(EMouseCursor::None);
	if (Button_ResizeHandle)
	{
		Button_ResizeHandle->SetCursor(EMouseCursor::None);
	}
	if (Button_FilterEvents)
	{
		Button_FilterEvents->SetCursor(EMouseCursor::None);
	}
	if (Button_FilterCombat)
	{
		Button_FilterCombat->SetCursor(EMouseCursor::None);
	}
	if (Button_FilterDiscussion)
	{
		Button_FilterDiscussion->SetCursor(EMouseCursor::None);
	}

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

void UGridEventFeedWidget::SynchronizeShell()
{
	PanelWidth = FMath::Max(1.0f, PanelWidth);
	CurrentPanelHeight = ClampPanelHeight(CurrentPanelHeight, MinPanelHeight, MaxPanelHeight);

	if (SizeBox_EventFeed)
	{
		SizeBox_EventFeed->SetWidthOverride(PanelWidth);
		SizeBox_EventFeed->SetHeightOverride(CurrentPanelHeight);
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
			UHorizontalBox* Row = WidgetTree
				? WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass())
				: NewObject<UHorizontalBox>(this);
			UTextBlock* MessageText = WidgetTree
				? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
				: NewObject<UTextBlock>(this);
			if (!Row || !MessageText)
			{
				continue;
			}

			if (Entry.bHasGameplayTimestamp)
			{
				UTextBlock* TimestampText = WidgetTree
					? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass())
					: NewObject<UTextBlock>(this);
				if (TimestampText)
				{
					TimestampText->SetText(FormatGameplayTimestamp(Entry.GameplayElapsedSeconds));
					TimestampText->SetFont(EntryFont);
					TimestampText->SetColorAndOpacity(FSlateColor(TimestampColor));
					TimestampText->SetAutoWrapText(false);
					TimestampText->SetVisibility(ESlateVisibility::HitTestInvisible);
					if (UHorizontalBoxSlot* TimestampSlot = Row->AddChildToHorizontalBox(TimestampText))
					{
						TimestampSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
						TimestampSlot->SetVerticalAlignment(VAlign_Top);
					}
				}
			}

			MessageText->SetText(BuildEntryText(Entry));
			MessageText->SetFont(EntryFont);
			MessageText->SetColorAndOpacity(FSlateColor(GetSemanticColor(Entry.Semantic)));
			MessageText->SetAutoWrapText(true);
			MessageText->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UHorizontalBoxSlot* MessageSlot = Row->AddChildToHorizontalBox(MessageText))
			{
				MessageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				MessageSlot->SetVerticalAlignment(VAlign_Top);
			}

			const FText TooltipText = BuildEntryTooltipText(Entry);
			if (Entry.Category == EGridHudEventCategory::Combat && !TooltipText.IsEmpty())
			{
				if (SharedTooltipWidgetClass)
				{
					if (UGridNavigationTooltipWidget* TooltipWidget =
						CreateWidget<UGridNavigationTooltipWidget>(this, SharedTooltipWidgetClass))
					{
						TooltipWidget->InitializeTooltip(TooltipText);
						TooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
						Row->SetToolTip(TooltipWidget);
					}
				}
				else
				{
					Row->SetToolTipText(TooltipText);
				}
				Row->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				Row->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			ScrollBox_Entries->AddChild(Row);
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
	}
}

void UGridEventFeedWidget::ScrollPresentationToEnd()
{
	if (ScrollBox_Entries)
	{
		ScrollBox_Entries->ScrollToEnd();
	}
}

float UGridEventFeedWidget::GetPresentationScrollOffset() const
{
	if (ScrollBox_Entries)
	{
		return ScrollBox_Entries->GetScrollOffset();
	}
	return 0.0f;
}

float UGridEventFeedWidget::GetPresentationScrollEndOffset() const
{
	if (ScrollBox_Entries)
	{
		return ScrollBox_Entries->GetScrollOffsetOfEnd();
	}
	return 0.0f;
}

FText UGridEventFeedWidget::FormatGameplayTimestamp(int64 GameplayElapsedSeconds)
{
	const int64 SafeSeconds = FMath::Max<int64>(0, GameplayElapsedSeconds);
	const int64 Day = SafeSeconds / 86400;
	const int64 SecondsWithinDay = SafeSeconds % 86400;
	const int64 Hour = SecondsWithinDay / 3600;
	const int64 Minute = (SecondsWithinDay % 3600) / 60;
	const int64 Second = SecondsWithinDay % 60;

	FFormatNamedArguments Args;
	Args.Add(TEXT("Day"), FText::AsNumber(Day));
	Args.Add(TEXT("Hour"), FText::FromString(FString::Printf(TEXT("%02lld"), static_cast<long long>(Hour))));
	Args.Add(TEXT("Minute"), FText::FromString(FString::Printf(TEXT("%02lld"), static_cast<long long>(Minute))));
	Args.Add(TEXT("Second"), FText::FromString(FString::Printf(TEXT("%02lld"), static_cast<long long>(Second))));
	return FText::Format(LOCTEXT("GameplayTimestampFormat", "[J {Day}, {Hour}:{Minute}:{Second}]"), Args);
}

FText UGridEventFeedWidget::BuildEntryText(const FGridHudEventEntry& Entry)
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

	// EVENTHUD01.8P: combat calculations are inspectable in a tooltip instead of
	// expanding every resolved attack into a large permanent block.
	if (Entry.Category != EGridHudEventCategory::Combat)
	{
		for (const FText& Detail : Entry.Details)
		{
			if (!Detail.IsEmpty())
			{
				Lines.Add(Detail);
			}
		}
	}

	return FText::Join(FText::FromString(TEXT("\n")), Lines);
}

FText UGridEventFeedWidget::BuildEntryTooltipText(const FGridHudEventEntry& Entry)
{
	if (Entry.Details.IsEmpty())
	{
		return FText::GetEmpty();
	}

	TArray<FText> Lines;
	if (!Entry.Header.IsEmpty())
	{
		Lines.Add(Entry.Header);
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

void UGridEventFeedWidget::BeginResizeFromCursor()
{
	if (FSlateApplication::IsInitialized())
	{
		const float ViewportScale =
			FMath::Max(KINDA_SMALL_NUMBER, UWidgetLayoutLibrary::GetViewportScale(this));
		BeginResizeAtScreenY(
			static_cast<float>(FSlateApplication::Get().GetCursorPos().Y),
			ViewportScale);
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

	// A new visible event always brings the newest entry into view.
	// Wheel scrolling remains available until the next published event.
	RebuildEntryPresentation(true);
}

#undef LOCTEXT_NAMESPACE
