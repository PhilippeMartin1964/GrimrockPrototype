#include "EditorTools/Widgets/SGridEditorObjectIdentityPanel.h"

#if WITH_EDITOR

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridEditorLuaService.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EditorTools/Widgets/GridEditorWidgetHelpers.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateColor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SGridEditorObjectIdentityPanel::Construct(const FArguments& InArgs)
{
	EditorActor = InArgs._EditorActor;
	OnGetEditorActor = InArgs._OnGetEditorActor;
	OnRequestRefresh = InArgs._OnRequestRefresh;
	ChildSlot[BuildRoot()];
}

AGridLevelEditorActor* SGridEditorObjectIdentityPanel::GetEditorActor() const
{
	if (EditorActor.IsValid())
	{
		return EditorActor.Get();
	}
	return OnGetEditorActor.IsBound() ? OnGetEditorActor.Execute() : nullptr;
}

void SGridEditorObjectIdentityPanel::RequestRefresh() const
{
	if (OnRequestRefresh.IsBound())
	{
		OnRequestRefresh.Execute();
	}
}

void SGridEditorObjectIdentityPanel::Rebuild()
{
	ChildSlot[BuildRoot()];
}

void SGridEditorObjectIdentityPanel::SetStatus(const FString& Text, bool bSuccess)
{
	StatusText = Text;
	bStatusSuccess = bSuccess;
}

FText SGridEditorObjectIdentityPanel::GetTypeText(FGuid ObjectId) const
{
	const AGridLevelEditorActor* Actor = GetEditorActor();
	const UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	if (!LevelAsset)
	{
		return FText::FromString(TEXT("None"));
	}
	const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
	const EGridLevelObjectType Type = LevelAsset->GetTypedPlacementType(ObjectId);
	return TypeEnum ? TypeEnum->GetDisplayNameTextByValue(static_cast<int64>(Type)) : FText::FromString(TEXT("Unknown"));
}

FText SGridEditorObjectIdentityPanel::GetPositionText(FGuid ObjectId) const
{
	const AGridLevelEditorActor* Actor = GetEditorActor();
	const UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	if (!LevelAsset)
	{
		return FText::FromString(TEXT("Unknown"));
	}
	int32 X = INDEX_NONE;
	int32 Y = INDEX_NONE;
	EGridEdge Edge = EGridEdge::None;
	if (!LevelAsset->TryGetTypedPlacementLocation(ObjectId, X, Y, Edge))
	{
		return FText::FromString(TEXT("Unknown"));
	}
	const UEnum* EdgeEnum = StaticEnum<EGridEdge>();
	if (Edge != EGridEdge::None && EdgeEnum)
	{
		return FText::Format(
			FText::FromString(TEXT("({0},{1}) {2}")),
			FText::AsNumber(X),
			FText::AsNumber(Y),
			GridEditorWidgetHelpers::GetGridEnumDisplayText(EdgeEnum, static_cast<int64>(Edge)));
	}
	return FText::Format(FText::FromString(TEXT("({0},{1})")), FText::AsNumber(X), FText::AsNumber(Y));
}

FText SGridEditorObjectIdentityPanel::GetDefinitionText(FGuid ObjectId) const
{
	const AGridLevelEditorActor* Actor = GetEditorActor();
	const UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	if (!LevelAsset)
	{
		return FText::FromString(TEXT("None"));
	}
	if (const FGridWorldObjectInstance* Instance = LevelAsset->FindWorldObjectInstanceById(ObjectId))
	{
		return Instance->WorldObjectDefinitionId.IsNone() ? FText::FromString(TEXT("None")) : FText::FromName(Instance->WorldObjectDefinitionId);
	}
	if (const FGridLooseItemInstance* Instance = LevelAsset->FindLooseItemInstanceById(ObjectId))
	{
		return Instance->PaletteEntryId.IsNone() ? FText::FromString(TEXT("Item")) : FText::FromName(Instance->PaletteEntryId);
	}
	if (const FGridMonsterSpawnInstance* Instance = LevelAsset->FindMonsterSpawnInstanceById(ObjectId))
	{
		return Instance->PaletteEntryId.IsNone() ? FText::FromString(TEXT("Monster Spawn")) : FText::FromName(Instance->PaletteEntryId);
	}
	if (const FGridItemSpawnInstance* Instance = LevelAsset->FindItemSpawnInstanceById(ObjectId))
	{
		return Instance->PaletteEntryId.IsNone() ? FText::FromString(TEXT("Item Spawn")) : FText::FromName(Instance->PaletteEntryId);
	}
	if (const FGridLogicObjectInstance* Instance = LevelAsset->FindLogicObjectInstanceById(ObjectId))
	{
		return Instance->PaletteEntryId.IsNone() ? FText::FromString(TEXT("Logic")) : FText::FromName(Instance->PaletteEntryId);
	}
	return FText::FromString(TEXT("None"));
}

TSharedRef<SWidget> SGridEditorObjectIdentityPanel::BuildRoot()
{
	AGridLevelEditorActor* Actor = GetEditorActor();
	UGridLevelAsset* LevelAsset = Actor ? Actor->LevelAsset.Get() : nullptr;
	const FGuid ObjectId = Actor ? Actor->LastSelectedObjectId : FGuid();
	if (!LevelAsset || !LevelAsset->ContainsTypedPlacementId(ObjectId))
	{
		return SNew(SBorder)
			.Padding(6.f)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Select a Grid object to edit its identity.")))
			];
	}

	TSharedRef<SVerticalBox> ContentRoot = SNew(SVerticalBox);

	int32 CellX = INDEX_NONE;
	int32 CellY = INDEX_NONE;
	EGridEdge CellEdge = EGridEdge::None;
	if (LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, CellEdge))
	{
		const TArray<FGuid> CellObjectIds = LevelAsset->GetTypedPlacementIdsAtCell(CellX, CellY);
		if (CellObjectIds.Num() > 1)
		{
			TSharedRef<SVerticalBox> CellSelector = SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(
							TEXT("%d objects share cell (%d,%d). Choose the object you want to edit."),
							CellObjectIds.Num(), CellX, CellY)))
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.f)))
				];

			for (const FGuid CandidateId : CellObjectIds)
			{
				const bool bCurrent = CandidateId == ObjectId;
				const FName CandidateLogicId = LevelAsset->GetTypedPlacementLogicId(CandidateId);
				FString Identifier = CandidateLogicId.IsNone() ? GetDefinitionText(CandidateId).ToString() : CandidateLogicId.ToString();
				if (Identifier.IsEmpty() || Identifier == TEXT("None"))
				{
					Identifier = CandidateId.ToString(EGuidFormats::Digits).Left(8);
				}
				const FText CandidateLabel = FText::FromString(FString::Printf(
					TEXT("%s — %s"), *GetTypeText(CandidateId).ToString(), *Identifier));

				CellSelector->AddSlot().AutoHeight().Padding(0.f, 1.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
					[
						SNew(STextBlock)
							.Text(CandidateLabel)
							.Font(FCoreStyle::GetDefaultFontStyle(bCurrent ? "Bold" : "Regular", 8))
							.AutoWrapText(true)
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SButton)
							.Text(FText::FromString(bCurrent ? TEXT("Selected") : TEXT("Select")))
							.IsEnabled(!bCurrent)
							.ContentPadding(FMargin(8.f, 2.f))
							.ToolTipText(FText::FromString(bCurrent ? TEXT("This object is currently selected.") : TEXT("Select this object on the current cell.")))
							.OnClicked_Lambda([this, CandidateId]() -> FReply
							{
								if (AGridLevelEditorActor* CurrentActor = GetEditorActor())
								{
									if (CurrentActor->SelectObjectById(CandidateId))
									{
										RequestRefresh();
									}
									else
									{
										Rebuild();
									}
								}
								return FReply::Handled();
							})
					]
				];
			}

			ContentRoot->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
			[
				GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Objects on Cell")), CellSelector)
			];
		}
	}

	const FName LogicId = LevelAsset->GetTypedPlacementLogicId(ObjectId);
	TSharedRef<SVerticalBox> IdentityRoot = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridPropertyRow(
				FText::FromString(TEXT("Logic Id")),
				SNew(SEditableTextBox)
					.Text(LogicId.IsNone() ? FText::GetEmpty() : FText::FromName(LogicId))
					.HintText(FText::FromString(TEXT("e.g. GuardianDoor")))
					.ToolTipText(FText::FromString(TEXT("Unique readable identity used by Lua and Events & Actions.")))
					.OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type)
					{
						AGridLevelEditorActor* CurrentActor = GetEditorActor();
						if (!CurrentActor)
						{
							return;
						}
						const FString Trimmed = NewText.ToString().TrimStartAndEnd();
						FString Error;
						const bool bOk = GridEditorLuaService::SetSelectedObjectLogicId(
							*CurrentActor,
							Trimmed.IsEmpty() ? NAME_None : FName(*Trimmed),
							Error);
						SetStatus(bOk ? TEXT("Logic Id updated.") : Error, bOk);
						if (bOk)
						{
							RequestRefresh();
						}
						else
						{
							Rebuild();
						}
					}))]
		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Type")), GetTypeText(ObjectId))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Definition")), GetDefinitionText(ObjectId))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			GridEditorWidgetHelpers::BuildGridReadOnlyPropertyRow(FText::FromString(TEXT("Position")), GetPositionText(ObjectId))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(TEXT("Logic Id is optional until another object or Lua script needs to reference this instance.")))
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.68f, 0.68f, 1.f)))
		];

	if (!StatusText.IsEmpty())
	{
		IdentityRoot->AddSlot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
		[
			SNew(STextBlock)
				.Text(FText::FromString(StatusText))
				.AutoWrapText(true)
				.ColorAndOpacity(bStatusSuccess
					? FSlateColor(FLinearColor(0.35f, 0.85f, 0.45f, 1.f))
					: FSlateColor(FLinearColor(1.f, 0.35f, 0.25f, 1.f)))
		];
	}

	ContentRoot->AddSlot().AutoHeight()
	[
		GridEditorWidgetHelpers::BuildGridPanelSection(FText::FromString(TEXT("Identity")), IdentityRoot)
	];

	return ContentRoot;
}

#endif
