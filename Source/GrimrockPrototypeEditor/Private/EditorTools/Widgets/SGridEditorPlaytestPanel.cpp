#include "EditorTools/Widgets/SGridEditorPlaytestPanel.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "Runtime/GridLevelRuntimeActor.h"

#include "Editor.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SGridEditorPlaytestPanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;

	ChildSlot[BuildPanel()];
}

AGridLevelEditorActor* SGridEditorPlaytestPanel::GetEditorActor() const
{
	if (OnGetEditorActor.IsBound())
	{
		return OnGetEditorActor.Execute();
	}

	return EditorActor.Get();
}

void SGridEditorPlaytestPanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

TSharedRef<SWidget> SGridEditorPlaytestPanel::BuildPanel()
{
	AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor)
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("No GridLevelEditorActor found."))).AutoWrapText(true);
	}

	const UGridLevelAsset* LevelAsset = CurrentEditorActor->LevelAsset;
	const bool bHasValidStart = LevelAsset && LevelAsset->IsStartCellValid();
	const FString FacingText = LevelAsset
		? (StaticEnum<EGridEdge>()
				? StaticEnum<EGridEdge>()->GetNameStringByValue(static_cast<int64>(LevelAsset->StartFacing))
				: FString(TEXT("Unknown")))
		: FString(TEXT("None"));

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(SCheckBox)
				.IsChecked(CurrentEditorActor->bAutoPreparePIE ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda(
					[this](ECheckBoxState NewState)
					{
						if (AGridLevelEditorActor* Editor = GetEditorActor())
						{
							Editor->Modify();
							Editor->bAutoPreparePIE = NewState == ECheckBoxState::Checked;
							RequestRefresh();
						}
					})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Auto Prepare PIE")))
				]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 7.f)
		[
			SNew(SCheckBox)
				.IsChecked(CurrentEditorActor->bAbortPIEOnPreparationError ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda(
					[this](ECheckBoxState NewState)
					{
						if (AGridLevelEditorActor* Editor = GetEditorActor())
						{
							Editor->Modify();
							Editor->bAbortPIEOnPreparationError = NewState == ECheckBoxState::Checked;
							RequestRefresh();
						}
					})
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("Abort PIE On Error")))
				]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Current LevelAsset")),
				LevelAsset ? FText::FromString(LevelAsset->GetName()) : FText::FromString(TEXT("None")))
		];

	if (!LevelAsset)
	{
		Root->AddSlot()
			.AutoHeight()
			.Padding(0.f, 5.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("No LevelAsset assigned.")))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.25f, 0.18f, 1.f)))
			];
	}
	else
	{
		Root->AddSlot()
			.AutoHeight()
			[
				GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
					FText::FromString(TEXT("Start Cell")),
					FText::Format(
						FText::FromString(TEXT("X={0} Y={1} Facing={2} Valid={3}")),
						FText::AsNumber(LevelAsset->StartCellX),
						FText::AsNumber(LevelAsset->StartCellY),
						FText::FromString(FacingText),
						bHasValidStart ? FText::FromString(TEXT("true")) : FText::FromString(TEXT("false"))))
			];

		if (!bHasValidStart)
		{
			Root->AddSlot()
				.AutoHeight()
				.Padding(0.f, 5.f, 0.f, 0.f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(TEXT("Warning: StartCell is invalid. PIE auto preparation will fail.")))
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.55f, 0.18f, 1.f)))
				];
		}
	}

	TSharedRef<SVerticalBox> ActionButtons = SNew(SVerticalBox);

	const auto AddActionButton = [&ActionButtons](TSharedRef<SWidget> Button)
	{
		ActionButtons->AddSlot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				Button
			];
	};

	AddActionButton(
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Set Start From Selection")),
			FOnClicked::CreateLambda(
				[this]()
				{
					if (AGridLevelEditorActor* Editor = GetEditorActor())
					{
						Editor->SetStartFromSelection();
						RequestRefresh();
						if (GEditor)
						{
							GEditor->RedrawAllViewports();
						}
					}
					return FReply::Handled();
				})));

	AddActionButton(
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Log PIE Readiness")),
			FOnClicked::CreateLambda(
				[this]()
				{
					if (AGridLevelEditorActor* Editor = GetEditorActor())
					{
						if (Editor->PreviewRuntimeActor)
						{
							Editor->PreviewRuntimeActor->LogPIEReadinessDiagnostics();
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("Log PIE Readiness failed: PreviewRuntimeActor is null."));
						}
					}
					return FReply::Handled();
				})));

	AddActionButton(
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Debug Prepare PIE")),
			FOnClicked::CreateLambda(
				[this]()
				{
					if (AGridLevelEditorActor* Editor = GetEditorActor())
					{
						Editor->PreparePIETestFromStart();
						RequestRefresh();
					}
					return FReply::Handled();
				})));

	Root->AddSlot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 0.f)
		[
			ActionButtons
		];

	return Root;
}

#endif
