#include "EditorTools/Widgets/SGridEditorDungeonLevelsPanel.h"

#if WITH_EDITOR

#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"

#include "Editor.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

void SGridEditorDungeonLevelsPanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;

	ChildSlot[BuildPanel()];
}

AGridLevelEditorActor* SGridEditorDungeonLevelsPanel::GetEditorActor() const
{
	if (OnGetEditorActor.IsBound())
	{
		return OnGetEditorActor.Execute();
	}

	return EditorActor.Get();
}

void SGridEditorDungeonLevelsPanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

TSharedRef<SWidget> SGridEditorDungeonLevelsPanel::BuildPanel()
{
	AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor)
	{
		return SNew(STextBlock).Text(FText::FromString(TEXT("No GridLevelEditorActor found."))).AutoWrapText(true);
	}

	UGridDungeonAsset* DungeonAsset = CurrentEditorActor->DungeonAsset.Get();
	if (!DungeonAsset)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("No DungeonAsset assigned on BP_GridLevelEditorActor.")))
					.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 3.f, 0.f, 0.f)
			[
				SNew(STextBlock)
					.Text(FText::FromString(TEXT("Mono-LevelAsset editing remains available.")))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f)))
			];
	}

	const FText DungeonName = DungeonAsset->DungeonName.IsEmpty() ? FText::FromString(DungeonAsset->GetName()) : DungeonAsset->DungeonName;

	TSharedRef<SVerticalBox> Root = SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Dungeon")), DungeonName)
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Default Level Id")), FText::FromName(DungeonAsset->DefaultLevelId))
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Current Level Id")), FText::FromName(CurrentEditorActor->CurrentDungeonLevelId))
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Current LevelAsset")),
				CurrentEditorActor->LevelAsset ? FText::FromString(CurrentEditorActor->LevelAsset->GetPathName()) : FText::FromString(TEXT("None")))
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Levels")), FText::AsNumber(DungeonAsset->Levels.Num()))
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(
				FText::FromString(TEXT("Auto PIE Prepare")),
				CurrentEditorActor->bAutoPreparePIE ? FText::FromString(TEXT("On")) : FText::FromString(TEXT("Off")))
		];

	TSharedRef<SHorizontalBox> ActionButtons = SNew(SHorizontalBox);

	ActionButtons->AddSlot().Padding(0.f, 0.f, 4.f, 4.f)
	[
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Load Default")),
			FOnClicked::CreateLambda(
				[this]()
				{
					if (AGridLevelEditorActor* Editor = GetEditorActor())
					{
						Editor->LoadDefaultDungeonLevelInEditor();
						RequestRefresh();
						if (GEditor)
						{
							GEditor->RedrawAllViewports();
						}
					}
					return FReply::Handled();
				}))
	];

	ActionButtons->AddSlot().Padding(0.f, 0.f, 4.f, 4.f)
	[
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Reload Current")),
			FOnClicked::CreateLambda(
				[this]()
				{
					if (AGridLevelEditorActor* Editor = GetEditorActor())
					{
						Editor->ApplyCurrentDungeonLevel();
						RequestRefresh();
						if (GEditor)
						{
							GEditor->RedrawAllViewports();
						}
					}
					return FReply::Handled();
				}))
	];

	ActionButtons->AddSlot().Padding(0.f, 0.f, 4.f, 4.f)
	[
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Log Dungeon")),
			FOnClicked::CreateLambda(
				[this]()
				{
					if (AGridLevelEditorActor* Editor = GetEditorActor())
					{
						Editor->LogDungeonDiagnostics();
					}
					return FReply::Handled();
				}))
	];

	ActionButtons->AddSlot().Padding(0.f, 0.f, 4.f, 4.f)
	[
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("Log Transitions")),
			FOnClicked::CreateLambda(
				[this]()
				{
					if (AGridLevelEditorActor* Editor = GetEditorActor())
					{
						Editor->LogDungeonTransitionDiagnostics();
					}
					return FReply::Handled();
				}))
	];

	ActionButtons->AddSlot().Padding(0.f, 0.f, 4.f, 4.f)
	[
		GridEditorWidgetHelpers::BuildGridActionButton(
			FText::FromString(TEXT("New Level")),
			FOnClicked::CreateSP(this, &SGridEditorDungeonLevelsPanel::HandleCreateDungeonLevelClicked))
	];

	Root->AddSlot().AutoHeight().Padding(0.f, 6.f, 0.f, 5.f)
	[
		ActionButtons
	];

	TSharedRef<SVerticalBox> LevelList = SNew(SVerticalBox);
	for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
	{
		const bool bSelected = Entry.LevelId == CurrentEditorActor->CurrentDungeonLevelId;
		const bool bHasLevelAsset = Entry.LevelAsset != nullptr;
		const bool bCanSelect = Entry.bEnabled && bHasLevelAsset;
		const FText DisplayName = Entry.DisplayName.IsEmpty() ? FText::FromName(Entry.LevelId) : Entry.DisplayName;
		const FText ButtonText = FText::Format(
			FText::FromString(TEXT("({0},{1},{2}) {3}")),
			FText::AsNumber(Entry.LogicalPosition.X),
			FText::AsNumber(Entry.LogicalPosition.Y),
			FText::AsNumber(Entry.LogicalPosition.Z),
			DisplayName);

		const FLinearColor ButtonColor = bSelected
			? FLinearColor(0.30f, 0.50f, 0.90f, 1.f)
			: (!Entry.bEnabled
				? FLinearColor(0.35f, 0.35f, 0.35f, 1.f)
				: (!bHasLevelAsset ? FLinearColor(0.90f, 0.55f, 0.16f, 1.f) : FLinearColor::White));

		LevelList->AddSlot().AutoHeight().Padding(0.f, 1.f, 0.f, 3.f)
		[
			SNew(SButton)
				.IsEnabled(bCanSelect)
				.ButtonColorAndOpacity(ButtonColor)
				.HAlign(HAlign_Left)
				.ContentPadding(FMargin(7.f, 4.f))
				.OnClicked(FOnClicked::CreateSP(this, &SGridEditorDungeonLevelsPanel::HandleSelectDungeonLevel, Entry.LevelId))
				[
					SNew(STextBlock).Text(ButtonText).AutoWrapText(true)
				]
		];
	}

	Root->AddSlot().AutoHeight()
	[
		LevelList
	];

	return Root;
}

FReply SGridEditorDungeonLevelsPanel::HandleSelectDungeonLevel(FName LevelId)
{
	if (AGridLevelEditorActor* CurrentEditorActor = GetEditorActor())
	{
		CurrentEditorActor->Modify();
		CurrentEditorActor->CurrentDungeonLevelId = LevelId;
		CurrentEditorActor->ApplyCurrentDungeonLevel();
		RequestRefresh();

		if (GEditor)
		{
			GEditor->RedrawAllViewports();
		}
	}

	return FReply::Handled();
}

FReply SGridEditorDungeonLevelsPanel::HandleCreateDungeonLevelClicked()
{
	AGridLevelEditorActor* CurrentEditorActor = GetEditorActor();
	if (!CurrentEditorActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("New Level failed: no GridLevelEditorActor found."));
		return FReply::Handled();
	}

	UGridDungeonAsset* DungeonAsset = CurrentEditorActor->DungeonAsset.Get();
	if (!DungeonAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("New Level failed: DungeonAsset is null."));
		return FReply::Handled();
	}

	int32 SuggestedZ = 0;
	for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
	{
		SuggestedZ = FMath::Max(SuggestedZ, Entry.LogicalPosition.Z + 1);
	}

	FString SuggestedLevelId = SuggestedZ > 0 ? FString::Printf(TEXT("New_Level_%02d"), SuggestedZ) : FString(TEXT("New_Level"));
	FString SuggestedDisplayName = SuggestedZ > 0 ? FString::Printf(TEXT("New Level %02d"), SuggestedZ) : FString(TEXT("New Level"));

	int32 UniqueSuffix = SuggestedZ;
	while (DungeonAsset->Levels.ContainsByPredicate(
		[&SuggestedLevelId](const FGridDungeonLevelEntry& Entry)
		{
			return Entry.LevelId == FName(*SuggestedLevelId);
		}))
	{
		++UniqueSuffix;
		SuggestedLevelId = FString::Printf(TEXT("New_Level_%02d"), UniqueSuffix);
		SuggestedDisplayName = FString::Printf(TEXT("New Level %02d"), UniqueSuffix);
	}

	TSharedPtr<SEditableTextBox> LevelIdTextBox;
	TSharedPtr<SEditableTextBox> DisplayNameTextBox;
	TSharedPtr<SSpinBox<int32>> LogicalXSpinBox;
	TSharedPtr<SSpinBox<int32>> LogicalYSpinBox;
	TSharedPtr<SSpinBox<int32>> LogicalZSpinBox;
	TSharedPtr<STextBlock> ErrorTextBlock;
	TSharedPtr<SWindow> DialogWindow;

	const auto BuildLabeledRow = [](const FText& Label, TSharedRef<SWidget> ValueWidget)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 4.f)
			[
				SNew(STextBlock).MinDesiredWidth(130.f).Text(Label)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 0.f, 4.f)
			[
				ValueWidget
			];
	};

	DialogWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Create New Dungeon Level")))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	DialogWindow->SetContent(
		SNew(SBorder)
		.Padding(12.f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildLabeledRow(
					FText::FromString(TEXT("Level Id")),
					SAssignNew(LevelIdTextBox, SEditableTextBox)
						.Text(FText::FromString(SuggestedLevelId)))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildLabeledRow(
					FText::FromString(TEXT("Display Name")),
					SAssignNew(DisplayNameTextBox, SEditableTextBox)
						.Text(FText::FromString(SuggestedDisplayName)))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildLabeledRow(
					FText::FromString(TEXT("Logical Position X")),
					SAssignNew(LogicalXSpinBox, SSpinBox<int32>)
						.MinValue(-128)
						.MaxValue(128)
						.Value(0))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildLabeledRow(
					FText::FromString(TEXT("Logical Position Y")),
					SAssignNew(LogicalYSpinBox, SSpinBox<int32>)
						.MinValue(-128)
						.MaxValue(128)
						.Value(0))
			]

			+ SVerticalBox::Slot().AutoHeight()
			[
				BuildLabeledRow(
					FText::FromString(TEXT("Logical Position Z")),
					SAssignNew(LogicalZSpinBox, SSpinBox<int32>)
						.MinValue(-128)
						.MaxValue(128)
						.Value(SuggestedZ))
			]

			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 7.f)
			[
				SAssignNew(ErrorTextBlock, STextBlock)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.25f, 0.18f, 1.f)))
			]

			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
						.Text(FText::FromString(TEXT("Create")))
						.OnClicked_Lambda(
							[this, DialogWindow, LevelIdTextBox, DisplayNameTextBox, LogicalXSpinBox, LogicalYSpinBox, LogicalZSpinBox, ErrorTextBlock]()
							{
								AGridLevelEditorActor* Editor = GetEditorActor();
								if (!Editor || !Editor->DungeonAsset)
								{
									if (ErrorTextBlock.IsValid())
									{
										ErrorTextBlock->SetText(FText::FromString(TEXT("DungeonAsset is no longer available.")));
									}
									return FReply::Handled();
								}

								UGridDungeonAsset* CurrentDungeonAsset = Editor->DungeonAsset.Get();

								FString LevelIdString = LevelIdTextBox.IsValid() ? LevelIdTextBox->GetText().ToString() : FString();
								LevelIdString.TrimStartAndEndInline();
								LevelIdString.ReplaceInline(TEXT(" "), TEXT("_"));
								LevelIdString.ReplaceInline(TEXT("-"), TEXT("_"));

								FString NormalizedLevelId;
								NormalizedLevelId.Reserve(LevelIdString.Len());
								for (const TCHAR Character : LevelIdString)
								{
									if (FChar::IsAlnum(Character) || Character == TEXT('_'))
									{
										NormalizedLevelId.AppendChar(Character);
									}
								}
								while (NormalizedLevelId.Contains(TEXT("__")))
								{
									NormalizedLevelId.ReplaceInline(TEXT("__"), TEXT("_"));
								}
								while (NormalizedLevelId.StartsWith(TEXT("_")))
								{
									NormalizedLevelId.RightChopInline(1);
								}
								while (NormalizedLevelId.EndsWith(TEXT("_")))
								{
									NormalizedLevelId.LeftChopInline(1);
								}

								if (NormalizedLevelId.IsEmpty())
								{
									if (ErrorTextBlock.IsValid())
									{
										ErrorTextBlock->SetText(FText::FromString(TEXT("Level Id is required.")));
									}
									return FReply::Handled();
								}

								const FName NewLevelId(*NormalizedLevelId);
								const FIntVector LogicalPosition(
									LogicalXSpinBox.IsValid() ? LogicalXSpinBox->GetValue() : 0,
									LogicalYSpinBox.IsValid() ? LogicalYSpinBox->GetValue() : 0,
									LogicalZSpinBox.IsValid() ? LogicalZSpinBox->GetValue() : 0);

								for (const FGridDungeonLevelEntry& Entry : CurrentDungeonAsset->Levels)
								{
									if (Entry.LevelId == NewLevelId)
									{
										if (ErrorTextBlock.IsValid())
										{
											ErrorTextBlock->SetText(FText::Format(
												FText::FromString(TEXT("Level Id '{0}' already exists.")),
												FText::FromName(NewLevelId)));
										}
										return FReply::Handled();
									}

									if (Entry.LogicalPosition == LogicalPosition)
									{
										if (ErrorTextBlock.IsValid())
										{
											ErrorTextBlock->SetText(FText::Format(
												FText::FromString(TEXT("Logical Position ({0},{1},{2}) is already used by LevelId '{3}'.")),
												FText::AsNumber(LogicalPosition.X),
												FText::AsNumber(LogicalPosition.Y),
												FText::AsNumber(LogicalPosition.Z),
												FText::FromName(Entry.LevelId)));
										}
										return FReply::Handled();
									}
								}

								FString DisplayNameString = DisplayNameTextBox.IsValid() ? DisplayNameTextBox->GetText().ToString() : FString();
								DisplayNameString.TrimStartAndEndInline();
								const FText DisplayName = DisplayNameString.IsEmpty() ? FText::FromName(NewLevelId) : FText::FromString(DisplayNameString);

								FString Error;
								if (!Editor->CreateAndAddDungeonLevel(NewLevelId, DisplayName, LogicalPosition, Error))
								{
									if (ErrorTextBlock.IsValid())
									{
										ErrorTextBlock->SetText(FText::FromString(Error));
									}
									return FReply::Handled();
								}

								if (DialogWindow.IsValid())
								{
									DialogWindow->RequestDestroyWindow();
								}

								RequestRefresh();
								if (GEditor)
								{
									GEditor->RedrawAllViewports();
								}

								return FReply::Handled();
							})
				]

				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
						.Text(FText::FromString(TEXT("Cancel")))
						.OnClicked_Lambda(
							[DialogWindow]()
							{
								if (DialogWindow.IsValid())
								{
									DialogWindow->RequestDestroyWindow();
								}
								return FReply::Handled();
							})
				]
			]
		]);

	FSlateApplication::Get().AddWindow(DialogWindow.ToSharedRef());
	return FReply::Handled();
}

#endif
