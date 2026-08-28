#include "EditorTools/Widgets/SGridEditorValidationPanel.h"

#if WITH_EDITOR

#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "EditorTools/GridEditorLuaService.h"

#include "HAL/PlatformApplicationMisc.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"

namespace
{
	FText GetValidationSeverityText(EGridLevelValidationSeverity Severity)
	{
		switch (Severity)
		{
			case EGridLevelValidationSeverity::Error:
				return FText::FromString(TEXT("Error"));
			case EGridLevelValidationSeverity::Warning:
				return FText::FromString(TEXT("Warning"));
			case EGridLevelValidationSeverity::Info:
			default:
				return FText::FromString(TEXT("Info"));
		}
	}

	FSlateColor GetValidationSeverityColor(EGridLevelValidationSeverity Severity)
	{
		switch (Severity)
		{
			case EGridLevelValidationSeverity::Error:
				return FSlateColor(FLinearColor(0.95f, 0.25f, 0.20f, 1.f));

			case EGridLevelValidationSeverity::Warning:
				return FSlateColor(FLinearColor(1.0f, 0.72f, 0.20f, 1.f));

			case EGridLevelValidationSeverity::Info:
			default:
				return FSlateColor(FLinearColor(0.25f, 0.75f, 1.f, 1.f));
		}
	}

	int32 GetValidationSeveritySortOrder(EGridLevelValidationSeverity Severity)
	{
		switch (Severity)
		{
			case EGridLevelValidationSeverity::Error:
				return 0;
			case EGridLevelValidationSeverity::Warning:
				return 1;
			case EGridLevelValidationSeverity::Info:
			default:
				return 2;
		}
	}


	FText GetValidationHeadlineText(int32 ErrorCount, int32 WarningCount)
	{
		if (ErrorCount > 0)
		{
			return FText::FromString(TEXT("INVALID"));
		}
		if (WarningCount > 0)
		{
			return FText::FromString(TEXT("VALID WITH WARNINGS"));
		}
		return FText::FromString(TEXT("VALID"));
	}

	FSlateColor GetValidationHeadlineColor(int32 ErrorCount, int32 WarningCount)
	{
		if (ErrorCount > 0)
		{
			return FSlateColor(FLinearColor(0.95f, 0.25f, 0.20f, 1.f));
		}
		if (WarningCount > 0)
		{
			return FSlateColor(FLinearColor(1.0f, 0.72f, 0.20f, 1.f));
		}
		return FSlateColor(FLinearColor(0.35f, 0.85f, 0.45f, 1.f));
	}
}

void FGridEditorValidationPanelState::CountValidationErrorsWarnings(int32& OutErrorCount, int32& OutWarningCount) const
{
	int32 InfoCount = 0;
	CountValidationMessages(OutErrorCount, OutWarningCount, InfoCount);
}

void FGridEditorValidationPanelState::CountValidationMessages(int32& OutErrorCount, int32& OutWarningCount, int32& OutInfoCount) const
{
	OutErrorCount = 0;
	OutWarningCount = 0;
	OutInfoCount = 0;

	for (const FGridLevelValidationMessage& ValidationMessage : ValidationMessages)
	{
		if (ValidationMessage.Severity == EGridLevelValidationSeverity::Error)
		{
			++OutErrorCount;
		}
		else if (ValidationMessage.Severity == EGridLevelValidationSeverity::Warning)
		{
			++OutWarningCount;
		}
		else
		{
			++OutInfoCount;
		}
	}
}

FText FGridEditorValidationPanelState::GetValidationStatusText() const
{
	if (!bValidationHasRun)
	{
		return FText::FromString(TEXT("Not run"));
	}

	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;
	CountValidationMessages(ErrorCount, WarningCount, InfoCount);

	return FText::Format(
		FText::FromString(TEXT("{0} errors, {1} warnings, {2} infos")), FText::AsNumber(ErrorCount), FText::AsNumber(WarningCount), FText::AsNumber(InfoCount));
}

bool SGridEditorValidationPanel::DoesMessageMatchSearch(const FGridLevelValidationMessage& ValidationMessage) const
{
	FString Search = GetValidationState().SearchText;
	Search.TrimStartAndEndInline();
	if (Search.IsEmpty())
	{
		return true;
	}

	const auto Matches = [&Search](const FString& Candidate)
	{
		return Candidate.Contains(Search, ESearchCase::IgnoreCase);
	};

	if (Matches(ValidationMessage.Message) || Matches(ValidationMessage.Category.ToString()))
	{
		return true;
	}

	if (ValidationMessage.OptionalObjectId.IsValid() && Matches(ValidationMessage.OptionalObjectId.ToString()))
	{
		return true;
	}
	if (ValidationMessage.SourceObjectId.IsValid() && Matches(ValidationMessage.SourceObjectId.ToString()))
	{
		return true;
	}
	if (ValidationMessage.TargetObjectId.IsValid() && Matches(ValidationMessage.TargetObjectId.ToString()))
	{
		return true;
	}

	if (ValidationMessage.CellX != INDEX_NONE && ValidationMessage.CellY != INDEX_NONE)
	{
		const FString CellText = FString::Printf(TEXT("%d,%d"), ValidationMessage.CellX, ValidationMessage.CellY);
		const FString CellTextWithSpace = FString::Printf(TEXT("%d, %d"), ValidationMessage.CellX, ValidationMessage.CellY);
		if (Matches(CellText) || Matches(CellTextWithSpace))
		{
			return true;
		}
	}

	if (ValidationMessage.Edge != EGridEdge::None)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			if (Matches(EdgeEnum->GetNameStringByValue(static_cast<int64>(ValidationMessage.Edge))) ||
				Matches(EdgeEnum->GetDisplayNameTextByValue(static_cast<int64>(ValidationMessage.Edge)).ToString()))
			{
				return true;
			}
		}
	}

	return false;
}

bool SGridEditorValidationPanel::ShouldShowMessage(const FGridLevelValidationMessage& ValidationMessage) const
{
	const FGridEditorValidationPanelState& CurrentValidationState = GetValidationState();
	bool bSeverityVisible = false;

	switch (ValidationMessage.Severity)
	{
		case EGridLevelValidationSeverity::Error:
			bSeverityVisible = CurrentValidationState.bShowErrors;
			break;

		case EGridLevelValidationSeverity::Warning:
			bSeverityVisible = CurrentValidationState.bShowWarnings;
			break;

		case EGridLevelValidationSeverity::Info:
		default:
			bSeverityVisible = CurrentValidationState.bShowInfos;
			break;
	}

	return bSeverityVisible && DoesMessageMatchSearch(ValidationMessage);
}

void SGridEditorValidationPanel::OnSearchTextChanged(const FText& NewText)
{
	GetValidationState().SearchText = NewText.ToString();
	RequestRefresh();
}

FReply SGridEditorValidationPanel::OnClearFiltersClicked()
{
	FGridEditorValidationPanelState& State = GetValidationState();
	State.bShowErrors = true;
	State.bShowWarnings = true;
	State.bShowInfos = true;
	State.SearchText.Reset();
	RequestRefresh();
	return FReply::Handled();
}

FReply SGridEditorValidationPanel::OnCopySummaryClicked() const
{
	const FGridEditorValidationPanelState& CurrentValidationState = GetValidationState();
	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;
	CurrentValidationState.CountValidationMessages(ErrorCount, WarningCount, InfoCount);

	FString Summary = FString::Printf(TEXT("Level validation: %d errors, %d warnings, %d infos"), ErrorCount, WarningCount, InfoCount);
	for (const FGridLevelValidationMessage& ValidationMessage : CurrentValidationState.ValidationMessages)
	{
		Summary += FString::Printf(TEXT("\n[%s] [%s] %s"), *GetValidationSeverityText(ValidationMessage.Severity).ToString(),
			*ValidationMessage.Category.ToString(), *ValidationMessage.Message);
	}

	FPlatformApplicationMisc::ClipboardCopy(*Summary);
	return FReply::Handled();
}

FReply SGridEditorValidationPanel::OnSelectObjectClicked(FGuid ObjectId, bool bFocus)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		if (CurrentEditorActor->SelectObjectById(ObjectId) && bFocus)
		{
			CurrentEditorActor->FocusSelectedObject();
		}
		RequestRefresh();
	}
	return FReply::Handled();
}

FReply SGridEditorValidationPanel::OnSelectCellClicked(int32 CellX, int32 CellY)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->SelectCellFromOverview(CellX, CellY);
		RequestRefresh();
	}
	return FReply::Handled();
}

void AddValidationFilter(
	const TSharedRef<SHorizontalBox>& FilterRow, const FText& Label, int32 Count, TFunction<ECheckBoxState()> GetState, TFunction<void(ECheckBoxState)> SetState)
{
	FilterRow->AddSlot()
		.AutoWidth()
		.Padding(0.f, 0.f, 10.f, 0.f)
		[
			SNew(SCheckBox)
				.IsChecked_Lambda(MoveTemp(GetState))
				.OnCheckStateChanged_Lambda(MoveTemp(SetState))
				[
					SNew(STextBlock)
						.Text(FText::Format(FText::FromString(TEXT("{0} ({1})")), Label, FText::AsNumber(Count)))
				]
		];
}

void AddValidationObjectAction(const TSharedRef<SHorizontalBox>& ActionRow, const FText& Label, TFunction<FReply()> OnClicked)
{
	ActionRow->AddSlot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)[SNew(SButton).Text(Label).OnClicked_Lambda(MoveTemp(OnClicked))];
}

FText GetValidationLocationText(const FGridLevelValidationMessage& ValidationMessage)
{
	if (ValidationMessage.CellX == INDEX_NONE || ValidationMessage.CellY == INDEX_NONE)
	{
		return FText::GetEmpty();
	}

	const UEnum* EdgeEnum = StaticEnum<EGridEdge>();
	const FText EdgeText = ValidationMessage.Edge == EGridEdge::None || !EdgeEnum
		? FText::GetEmpty()
		: EdgeEnum->GetDisplayNameTextByValue(static_cast<int64>(ValidationMessage.Edge));

	return EdgeText.IsEmpty()
		? FText::Format(FText::FromString(TEXT("Cell: ({0}, {1})")), FText::AsNumber(ValidationMessage.CellX), FText::AsNumber(ValidationMessage.CellY))
		: FText::Format(FText::FromString(TEXT("Cell: ({0}, {1}) | Edge: {2}")), FText::AsNumber(ValidationMessage.CellX),
			  FText::AsNumber(ValidationMessage.CellY), EdgeText);
}

FText GetShortValidationObjectText(const TCHAR* Prefix, FGuid ObjectId)
{
	return FText::Format(FText::FromString(TEXT("{0}: {1}")), FText::FromString(Prefix), FText::FromString(ObjectId.ToString().Left(8)));
}

FText GetValidationSummaryText(const FGridEditorValidationPanelState& ValidationState)
{
	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;
	ValidationState.CountValidationMessages(ErrorCount, WarningCount, InfoCount);
	return FText::Format(FText::FromString(TEXT("Errors: {0} | Warnings: {1} | Infos: {2}")), FText::AsNumber(ErrorCount), FText::AsNumber(WarningCount),
		FText::AsNumber(InfoCount));
}

void SGridEditorValidationPanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	ValidationState = InArgs._ValidationState;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;

	ChildSlot[BuildValidationSection()];
}

AGridLevelEditorActor* SGridEditorValidationPanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}

	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

FGridEditorValidationPanelState& SGridEditorValidationPanel::GetValidationState() const
{
	static FGridEditorValidationPanelState FallbackValidationState;

	return ValidationState.IsValid() ? *ValidationState : FallbackValidationState;
}

void SGridEditorValidationPanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

TSharedRef<SWidget> SGridEditorValidationPanel::BuildValidationSection()
{
	const FGridEditorValidationPanelState& CurrentValidationState = GetValidationState();
	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox);

	Root->AddSlot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(0.f, 0.f, 4.f, 0.f)
			[
				GridEditorWidgetHelpers::BuildGridActionButton(
					FText::FromString(TEXT("Refresh Validation")),
					FOnClicked::CreateSP(this, &SGridEditorValidationPanel::OnValidateLevelClicked))
			]

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
					.Text(FText::FromString(TEXT("Copy Summary")))
					.IsEnabled(CurrentValidationState.bValidationHasRun)
					.OnClicked(FOnClicked::CreateSP(this, &SGridEditorValidationPanel::OnCopySummaryClicked))
			]
		];

	if (!CurrentValidationState.bValidationHasRun)
	{
		Root->AddSlot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("No validation run yet.")))
					.AutoWrapText(true)
			];
		return Root;
	}

	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	int32 InfoCount = 0;
	CurrentValidationState.CountValidationMessages(ErrorCount, WarningCount, InfoCount);

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(SBorder)
				.Padding(FMargin(8.f, 6.f))
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.f, 0.f, 10.f, 0.f)
					[
						SNew(STextBlock)
							.Text(GetValidationHeadlineText(ErrorCount, WarningCount))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
							.ColorAndOpacity(GetValidationHeadlineColor(ErrorCount, WarningCount))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text(GetValidationSummaryText(CurrentValidationState))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
					]
				]
		];

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(SSearchBox)
				.HintText(FText::FromString(TEXT("Search message, category, cell, edge or object id...")))
				.InitialText(FText::FromString(CurrentValidationState.SearchText))
				.OnTextChanged(this, &SGridEditorValidationPanel::OnSearchTextChanged)
		];

	TSharedRef<SHorizontalBox> FilterRow = SNew(SHorizontalBox);
	AddValidationFilter(
		FilterRow, FText::FromString(TEXT("Errors")), ErrorCount,
		[this]()
		{
			return GetValidationState().bShowErrors ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		},
		[this](ECheckBoxState State)
		{
			GetValidationState().bShowErrors = State == ECheckBoxState::Checked;
			RequestRefresh();
		});
	AddValidationFilter(
		FilterRow, FText::FromString(TEXT("Warnings")), WarningCount,
		[this]()
		{
			return GetValidationState().bShowWarnings ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		},
		[this](ECheckBoxState State)
		{
			GetValidationState().bShowWarnings = State == ECheckBoxState::Checked;
			RequestRefresh();
		});
	AddValidationFilter(
		FilterRow, FText::FromString(TEXT("Infos")), InfoCount,
		[this]()
		{
			return GetValidationState().bShowInfos ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		},
		[this](ECheckBoxState State)
		{
			GetValidationState().bShowInfos = State == ECheckBoxState::Checked;
			RequestRefresh();
		});

	FilterRow->AddSlot()
		.AutoWidth()
		.Padding(4.f, 0.f, 0.f, 0.f)
		[
			SNew(SButton)
				.Text(FText::FromString(TEXT("Clear Filters")))
				.OnClicked(FOnClicked::CreateSP(this, &SGridEditorValidationPanel::OnClearFiltersClicked))
		];

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 0.f)
		[
			FilterRow
		];

	if (CurrentValidationState.ValidationMessages.Num() == 0)
	{
		Root->AddSlot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("No validation messages.")))
					.AutoWrapText(true)
			];
		return Root;
	}

	TArray<const FGridLevelValidationMessage*> SortedMessages;
	for (const FGridLevelValidationMessage& ValidationMessage : CurrentValidationState.ValidationMessages)
	{
		if (ShouldShowMessage(ValidationMessage))
		{
			SortedMessages.Add(&ValidationMessage);
		}
	}

	SortedMessages.StableSort(
		[](const FGridLevelValidationMessage& Left, const FGridLevelValidationMessage& Right)
		{
			const int32 LeftOrder = GetValidationSeveritySortOrder(Left.Severity);
			const int32 RightOrder = GetValidationSeveritySortOrder(Right.Severity);
			return LeftOrder == RightOrder ? Left.Category.LexicalLess(Right.Category) : LeftOrder < RightOrder;
		});

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 6.f, 0.f, 0.f)
		[
			SNew(STextBlock)
				.Text(FText::Format(
					FText::FromString(TEXT("Showing {0} of {1} validation messages")),
					FText::AsNumber(SortedMessages.Num()),
					FText::AsNumber(CurrentValidationState.ValidationMessages.Num())))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.68f, 0.68f, 1.f)))
		];

	if (SortedMessages.Num() == 0)
	{
		Root->AddSlot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("No messages match the active filters or search.")))
					.AutoWrapText(true)
			];
		return Root;
	}

	for (const FGridLevelValidationMessage* ValidationMessagePtr : SortedMessages)
	{
		const FGridLevelValidationMessage& ValidationMessage = *ValidationMessagePtr;
		const FString ShortObjectId = ValidationMessage.OptionalObjectId.IsValid() ? ValidationMessage.OptionalObjectId.ToString().Left(8) : FString();
		const FText LocationText = GetValidationLocationText(ValidationMessage);
		TSharedRef<SHorizontalBox> ActionRow = SNew(SHorizontalBox);
		TSet<FGuid> ActionObjectIds;

		auto AddObjectActions = [this, &ActionRow, &ActionObjectIds](FGuid ObjectId, const TCHAR* SelectLabel, const TCHAR* FocusLabel)
		{
			if (!ObjectId.IsValid() || ActionObjectIds.Contains(ObjectId))
			{
				return;
			}
			ActionObjectIds.Add(ObjectId);
			AddValidationObjectAction(ActionRow, FText::FromString(SelectLabel),
				[this, ObjectId]()
				{
					return OnSelectObjectClicked(ObjectId, false);
				});
			AddValidationObjectAction(ActionRow, FText::FromString(FocusLabel),
				[this, ObjectId]()
				{
					return OnSelectObjectClicked(ObjectId, true);
				});
		};

		AddObjectActions(ValidationMessage.OptionalObjectId, TEXT("Select Object"), TEXT("Focus Object"));
		AddObjectActions(ValidationMessage.SourceObjectId, TEXT("Select Source"), TEXT("Focus Source"));
		AddObjectActions(ValidationMessage.TargetObjectId, TEXT("Select Target"), TEXT("Focus Target"));
		if (ActionObjectIds.Num() == 0 && ValidationMessage.CellX != INDEX_NONE && ValidationMessage.CellY != INDEX_NONE)
		{
			AddValidationObjectAction(ActionRow, FText::FromString(TEXT("Select Cell")),
				[this, CellX = ValidationMessage.CellX, CellY = ValidationMessage.CellY]()
				{
					return OnSelectCellClicked(CellX, CellY);
				});
		}

		Root->AddSlot().AutoHeight().Padding(
			0.f, 6.f, 0.f, 0.f)[SNew(SBorder).Padding(6.f).BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))[SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)

				  + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)[SNew(STextBlock)
							.Text(GetValidationSeverityText(ValidationMessage.Severity))
							.ColorAndOpacity(GetValidationSeverityColor(ValidationMessage.Severity))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))]

				  + SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)[SNew(STextBlock)
							.Text(FText::FromName(ValidationMessage.Category))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.75f, 0.85f)))
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))]

				  + SHorizontalBox::Slot().FillWidth(1.f)[SNew(STextBlock).Text(FText::FromString(ValidationMessage.Message)).AutoWrapText(true)]]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, ShortObjectId.IsEmpty() ? 0.f : 4.f, 0.f, 0.f)[SNew(STextBlock)
					  .Visibility(ShortObjectId.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
					  .Text(FText::Format(FText::FromString(TEXT("Object: {0}")), FText::FromString(ShortObjectId)))]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, LocationText.IsEmpty() ? 0.f : 3.f, 0.f,
				  0.f)[SNew(STextBlock).Visibility(LocationText.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible).Text(LocationText)]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, ValidationMessage.SourceObjectId.IsValid() ? 3.f : 0.f, 0.f, 0.f)[SNew(STextBlock)
					  .Visibility(ValidationMessage.SourceObjectId.IsValid() ? EVisibility::Visible : EVisibility::Collapsed)
					  .Text(GetShortValidationObjectText(TEXT("Source"), ValidationMessage.SourceObjectId))]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, ValidationMessage.TargetObjectId.IsValid() ? 3.f : 0.f, 0.f, 0.f)[SNew(STextBlock)
					  .Visibility(ValidationMessage.TargetObjectId.IsValid() ? EVisibility::Visible : EVisibility::Collapsed)
					  .Text(GetShortValidationObjectText(TEXT("Target"), ValidationMessage.TargetObjectId))]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, ActionObjectIds.Num() > 0 ? 5.f : 0.f, 0.f, 0.f)[ActionRow]]];
	}


	return Root;
}

FReply SGridEditorValidationPanel::OnValidateLevelClicked()
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		FGridEditorValidationPanelState& CurrentValidationState = GetValidationState();
		CurrentValidationState.bValidationHasRun = true;
		CurrentValidationState.ValidationMessages = GridEditorLuaService::ValidateCurrentLevelWithLua(*CurrentEditorActor);
		RequestRefresh();
	}

	return FReply::Handled();
}

#endif
