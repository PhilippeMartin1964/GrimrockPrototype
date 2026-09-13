#include "EditorTools/GridLevelEditorActor.h"

#if WITH_EDITOR
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#endif

namespace
{
	// Apply an authoring edit directly to one concrete placement. No snapshot or cache is created.
	template <typename TEdit>
	bool EditGridPlacementAuthoring(UGridLevelAsset* LevelAsset, FGuid ObjectId, TEdit&& Edit)
	{
		if (!LevelAsset || !LevelAsset->ContainsTypedPlacementId(ObjectId)) return false;
		LevelAsset->Modify();
		if (FGridWorldObjectInstance* WorldObjectInstance = LevelAsset->FindWorldObjectInstanceById(ObjectId)) Edit(*WorldObjectInstance);
		else if (FGridLooseItemInstance* LooseItemInstance = LevelAsset->FindLooseItemInstanceById(ObjectId)) Edit(*LooseItemInstance);
		else if (FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset->FindMonsterSpawnInstanceById(ObjectId)) Edit(*MonsterSpawn);
		else if (FGridItemSpawnInstance* ItemSpawn = LevelAsset->FindItemSpawnInstanceById(ObjectId)) Edit(*ItemSpawn);
		else if (FGridLogicObjectInstance* LogicInstance = LevelAsset->FindLogicObjectInstanceById(ObjectId)) Edit(*LogicInstance);
		LevelAsset->MarkPackageDirty();
		return true;
	}

#if WITH_EDITOR
	FString GetEraseObjectIdentifier(const UGridLevelAsset& LevelAsset, const FGuid& ObjectId)
	{
		const FName LogicId = LevelAsset.GetTypedPlacementLogicId(ObjectId);
		if (!LogicId.IsNone())
		{
			return LogicId.ToString();
		}
		if (const FGridWorldObjectInstance* Instance = LevelAsset.FindWorldObjectInstanceById(ObjectId))
		{
			if (!Instance->WorldObjectDefinitionId.IsNone()) return Instance->WorldObjectDefinitionId.ToString();
		}
		else if (const FGridLooseItemInstance* Instance = LevelAsset.FindLooseItemInstanceById(ObjectId))
		{
			if (!Instance->PaletteEntryId.IsNone()) return Instance->PaletteEntryId.ToString();
		}
		else if (const FGridMonsterSpawnInstance* Instance = LevelAsset.FindMonsterSpawnInstanceById(ObjectId))
		{
			if (!Instance->PaletteEntryId.IsNone()) return Instance->PaletteEntryId.ToString();
		}
		else if (const FGridItemSpawnInstance* Instance = LevelAsset.FindItemSpawnInstanceById(ObjectId))
		{
			if (!Instance->PaletteEntryId.IsNone()) return Instance->PaletteEntryId.ToString();
		}
		else if (const FGridLogicObjectInstance* Instance = LevelAsset.FindLogicObjectInstanceById(ObjectId))
		{
			if (!Instance->PaletteEntryId.IsNone()) return Instance->PaletteEntryId.ToString();
		}
		return ObjectId.ToString(EGuidFormats::Digits).Left(8);
	}

	FText BuildEraseObjectLabel(const AGridLevelEditorActor& EditorActor, const FGuid& ObjectId)
	{
		const UGridLevelAsset* LevelAsset = EditorActor.LevelAsset.Get();
		if (!LevelAsset)
		{
			return FText::FromString(TEXT("Unknown object"));
		}

		const UEnum* TypeEnum = StaticEnum<EGridLevelObjectType>();
		const EGridLevelObjectType Type = LevelAsset->GetTypedPlacementType(ObjectId);
		const FString TypeText = TypeEnum ? TypeEnum->GetDisplayNameTextByValue(static_cast<int64>(Type)).ToString() : TEXT("Object");
		const FString Identifier = GetEraseObjectIdentifier(*LevelAsset, ObjectId);

		int32 CellX = INDEX_NONE;
		int32 CellY = INDEX_NONE;
		EGridEdge Edge = EGridEdge::None;
		LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge);
		FString EdgeSuffix;
		if (Edge != EGridEdge::None)
		{
			const UEnum* EdgeEnum = StaticEnum<EGridEdge>();
			const FString EdgeText = EdgeEnum ? EdgeEnum->GetDisplayNameTextByValue(static_cast<int64>(Edge)).ToString() : TEXT("Edge");
			EdgeSuffix = FString::Printf(TEXT(" [%s]"), *EdgeText);
		}

		const FString SelectedSuffix = EditorActor.LastSelectedObjectId == ObjectId ? TEXT(" (selected)") : TEXT("");
		return FText::FromString(FString::Printf(TEXT("%s - %s%s%s"), *TypeText, *Identifier, *EdgeSuffix, *SelectedSuffix));
	}

	int32 CountEraseObjectLinks(const UGridLevelAsset& LevelAsset, const FGuid& ObjectId)
	{
		return LevelAsset.Links.CountByPredicate(
			[&ObjectId](const FGridObjectLink& Link)
			{
				return Link.SourceObjectId == ObjectId || Link.TargetObjectId == ObjectId;
			});
	}

	bool ChooseEraseObject(const AGridLevelEditorActor& EditorActor, const TArray<FGuid>& CandidateIds, FGuid& OutObjectId)
	{
		OutObjectId.Invalidate();
		if (CandidateIds.Num() == 0)
		{
			return false;
		}
		if (CandidateIds.Num() == 1)
		{
			OutObjectId = CandidateIds[0];
			return true;
		}
		if (!FSlateApplication::IsInitialized())
		{
			return false;
		}

		TSharedPtr<SVerticalBox> CandidateList;
		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(FText::FromString(TEXT("Choose object to delete")))
			.SizingRule(ESizingRule::Autosized)
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			[
				SNew(SBorder)
				.Padding(12.0f)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(
							TEXT("%d objects share cell (%d,%d). Choose exactly one object to delete."),
							CandidateIds.Num(), EditorActor.SelectedCellX, EditorActor.SelectedCellY)))
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(CandidateList, SVerticalBox)
					]
				];

		TWeakPtr<SWindow> WeakWindow = Window;
		TArray<FGuid> OrderedIds = CandidateIds;
		if (EditorActor.LastSelectedObjectId.IsValid() && OrderedIds.Remove(EditorActor.LastSelectedObjectId) > 0)
		{
			OrderedIds.Insert(EditorActor.LastSelectedObjectId, 0);
		}
		for (const FGuid& CandidateId : OrderedIds)
		{
			CandidateList->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
			[
				SNew(SButton)
				.Text(BuildEraseObjectLabel(EditorActor, CandidateId))
				.ContentPadding(FMargin(10.0f, 4.0f))
				.OnClicked_Lambda([&OutObjectId, CandidateId, WeakWindow]() -> FReply
				{
					OutObjectId = CandidateId;
					if (const TSharedPtr<SWindow> PinnedWindow = WeakWindow.Pin())
					{
						PinnedWindow->RequestDestroyWindow();
					}
					return FReply::Handled();
				})
			];
		}
		CandidateList->AddSlot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Cancel")))
			.HAlign(HAlign_Center)
			.OnClicked_Lambda([WeakWindow]() -> FReply
			{
				if (const TSharedPtr<SWindow> PinnedWindow = WeakWindow.Pin())
				{
					PinnedWindow->RequestDestroyWindow();
				}
				return FReply::Handled();
			})
		];

		FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow(), false);
		return OutObjectId.IsValid();
	}

	bool ConfirmEraseObject(const AGridLevelEditorActor& EditorActor, const FGuid& ObjectId)
	{
		const UGridLevelAsset* LevelAsset = EditorActor.LevelAsset.Get();
		if (!LevelAsset || !LevelAsset->ContainsTypedPlacementId(ObjectId))
		{
			return false;
		}
		int32 CellX = INDEX_NONE;
		int32 CellY = INDEX_NONE;
		EGridEdge Edge = EGridEdge::None;
		LevelAsset->TryGetTypedPlacementLocation(ObjectId, CellX, CellY, Edge);

		FString Message = FString::Printf(
			TEXT("Delete '%s' from cell (%d,%d)?"),
			*BuildEraseObjectLabel(EditorActor, ObjectId).ToString(), CellX, CellY);
		const int32 LinkCount = CountEraseObjectLinks(*LevelAsset, ObjectId);
		if (LinkCount > 0)
		{
			Message += FString::Printf(
				TEXT("\n\nWarning: this object owns or receives %d logical link%s. Those links will also be removed."),
				LinkCount, LinkCount == 1 ? TEXT("") : TEXT("s"));
		}
		Message += TEXT("\n\nThe deletion can be undone with Ctrl+Z.");
		return FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(Message)) == EAppReturnType::Yes;
	}

	bool DeleteEraseObject(AGridLevelEditorActor& EditorActor, const FGuid& ObjectId)
	{
		if (!EditorActor.LevelAsset || !EditorActor.LevelAsset->ContainsTypedPlacementId(ObjectId))
		{
			return false;
		}

		const FScopedTransaction Transaction(FText::FromString(TEXT("Delete Grid Object")));
		EditorActor.Modify();
		EditorActor.LevelAsset->Modify();
		if (!EditorActor.LevelAsset->RemoveObjectById(ObjectId))
		{
			return false;
		}
		if (EditorActor.LastSelectedObjectId == ObjectId)
		{
			EditorActor.LastSelectedObjectId.Invalidate();
		}
		if (EditorActor.HoveredObjectId == ObjectId)
		{
			EditorActor.HoveredObjectId.Invalidate();
		}
		if (EditorActor.PendingLinkSourceObjectId == ObjectId)
		{
			EditorActor.ClearPendingLinkSource();
		}
		EditorActor.RebuildPreview();
		return true;
	}

	bool HandleTargetedObjectErase(AGridLevelEditorActor& EditorActor, const TArray<FGuid>& CandidateIds)
	{
		FGuid ObjectId;
		if (!ChooseEraseObject(EditorActor, CandidateIds, ObjectId) || !ConfirmEraseObject(EditorActor, ObjectId))
		{
			return false;
		}
		return DeleteEraseObject(EditorActor, ObjectId);
	}
#endif
}

// MON19.2.1R — Décomposition structurelle de AGridLevelEditorActor.
//
// Cette unité de traduction reste volontairement unique afin de préserver
// exactement la visibilité des helpers privés, les blocs WITH_EDITOR et
// l'ordre historique des définitions. Les fichiers .inl ne sont jamais
// compilés séparément par UnrealBuildTool.
#include "GridLevelEditorActorParts/GridLevelEditorActor_CoreDungeon.inl"
#include "GridLevelEditorActorParts/GridLevelEditorActor_EditingObjectsLinks.inl"
#include "GridLevelEditorActorParts/GridLevelEditorActor_Validation.inl"
#include "GridLevelEditorActorParts/GridLevelEditorActor_InteractionViewport.inl"
