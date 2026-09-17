#include "EditorTools/GridLevelEditorActor.h"

#if WITH_EDITOR
#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "UnrealClient.h"
#include "EditorTools/GridLevelEdMode.h"
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
#if WITH_EDITOR
		const FScopedTransaction Transaction(FText::FromString(TEXT("Edit Grid Object")));
#endif
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
	FLevelEditorViewportClient* FindOverviewNavigationViewport()
	{
		if (!GEditor)
		{
			return nullptr;
		}

		const TArray<FLevelEditorViewportClient*>& LevelViewportClients = GEditor->GetLevelViewportClients();
		FViewport* ActiveViewport = GEditor->GetActiveViewport();

		if (ActiveViewport)
		{
			for (FLevelEditorViewportClient* ViewportClient : LevelViewportClients)
			{
				if (ViewportClient && ViewportClient->IsPerspective() && ActiveViewport->GetClient() == ViewportClient)
				{
					return ViewportClient;
				}
			}
		}

		for (FLevelEditorViewportClient* ViewportClient : LevelViewportClients)
		{
			if (ViewportClient && ViewportClient->IsPerspective())
			{
				return ViewportClient;
			}
		}

		return nullptr;
	}

	TUniquePtr<FScopedTransaction> GridEditorGestureTransaction;
	FTSTicker::FDelegateHandle GridEditorGestureTickerHandle;

	bool IsGridEditorPaintGestureActive()
	{
		if (FEdMode* ActiveMode = GLevelEditorModeTools().GetActiveMode(FGridLevelEdMode::EM_GridLevelEdModeId))
		{
			return static_cast<const FGridLevelEdMode*>(ActiveMode)->IsPaintGestureActive();
		}
		return false;
	}

	void EndGridEditorGestureTransaction()
	{
		GridEditorGestureTransaction.Reset();
		if (GridEditorGestureTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GridEditorGestureTickerHandle);
			GridEditorGestureTickerHandle = FTSTicker::FDelegateHandle();
		}
	}

	bool TickGridEditorGestureTransaction(float /*DeltaTime*/)
	{
		if (!GridEditorGestureTransaction || !IsGridEditorPaintGestureActive())
		{
			GridEditorGestureTransaction.Reset();
			GridEditorGestureTickerHandle = FTSTicker::FDelegateHandle();
			return false;
		}
		return true;
	}

	void BeginGridEditorGestureTransaction(const TCHAR* Description)
	{
		if (!GridEditorGestureTransaction)
		{
			GridEditorGestureTransaction = MakeUnique<FScopedTransaction>(FText::FromString(Description));
		}
		if (!GridEditorGestureTickerHandle.IsValid())
		{
			GridEditorGestureTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&TickGridEditorGestureTransaction));
		}
	}

	template <typename TEdit>
	void RunGridEditorTransaction(const TCHAR* Description, TEdit&& Edit)
	{
		// GE-UNDO02: one continuous viewport gesture is one native Unreal
		// transaction. Panel/button actions still receive an immediate transaction.
		if (IsGridEditorPaintGestureActive())
		{
			BeginGridEditorGestureTransaction(Description);
			Edit();
			return;
		}

		EndGridEditorGestureTransaction();
		const FScopedTransaction Transaction(FText::FromString(Description));
		Edit();
	}

	void CancelGridEditorPaintGesture()
	{
		EndGridEditorGestureTransaction();
		if (FEdMode* ActiveMode = GLevelEditorModeTools().GetActiveMode(FGridLevelEdMode::EM_GridLevelEdModeId))
		{
			static_cast<FGridLevelEdMode*>(ActiveMode)->CancelActivePaintGesture();
		}
	}

	FString GetEraseObjectIdentifier(const UGridLevelAsset& LevelAsset, const FGuid& ObjectId)
	{
		const FName LogicId = LevelAsset.GetTypedPlacementLogicId(ObjectId);
		if (!LogicId.IsNone())
		{
			return LogicId.ToString();
		}
		if (const FGridWorldObjectInstance* WorldObjectInstance = LevelAsset.FindWorldObjectInstanceById(ObjectId))
		{
			if (!WorldObjectInstance->WorldObjectDefinitionId.IsNone()) return WorldObjectInstance->WorldObjectDefinitionId.ToString();
		}
		else if (const FGridLooseItemInstance* LooseItemInstance = LevelAsset.FindLooseItemInstanceById(ObjectId))
		{
			if (!LooseItemInstance->PaletteEntryId.IsNone()) return LooseItemInstance->PaletteEntryId.ToString();
		}
		else if (const FGridMonsterSpawnInstance* MonsterSpawn = LevelAsset.FindMonsterSpawnInstanceById(ObjectId))
		{
			if (!MonsterSpawn->PaletteEntryId.IsNone()) return MonsterSpawn->PaletteEntryId.ToString();
		}
		else if (const FGridItemSpawnInstance* ItemSpawn = LevelAsset.FindItemSpawnInstanceById(ObjectId))
		{
			if (!ItemSpawn->PaletteEntryId.IsNone()) return ItemSpawn->PaletteEntryId.ToString();
		}
		else if (const FGridLogicObjectInstance* LogicInstance = LevelAsset.FindLogicObjectInstanceById(ObjectId))
		{
			if (!LogicInstance->PaletteEntryId.IsNone()) return LogicInstance->PaletteEntryId.ToString();
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
		int32 LinkCount = 0;
		for (const FGridObjectLink& Link : LevelAsset.Links)
		{
			if (Link.SourceObjectId == ObjectId || Link.TargetObjectId == ObjectId)
			{
				++LinkCount;
			}
		}
		return LinkCount;
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
		if (!ChooseEraseObject(EditorActor, CandidateIds, ObjectId))
		{
			CancelGridEditorPaintGesture();
			return false;
		}

		const bool bConfirmed = ConfirmEraseObject(EditorActor, ObjectId);
		// The modal chooser/confirmation consumes its own mouse input. End the
		// originating viewport gesture before returning so it cannot resume as a
		// paint drag over whatever lies beneath the closed popup.
		CancelGridEditorPaintGesture();
		if (!bConfirmed)
		{
			return false;
		}
		return DeleteEraseObject(EditorActor, ObjectId);
	}
#endif
}

#if WITH_EDITOR
bool AGridLevelEditorActor::FocusSelectedCellInEditorViewport()
{
	if (!LevelAsset)
	{
		return false;
	}

	FLevelEditorViewportClient* ViewportClient = FindOverviewNavigationViewport();
	if (!ViewportClient)
	{
		return false;
	}

	const float CellSize = FMath::Max(LevelAsset->CellSize, 1.f);
	const FVector FocusPoint = GetSelectionPreviewCenter(CellSize * 0.75f);
	const FRotator PreservedRotation = ViewportClient->GetViewRotation();
	const float FocusDistance = CellSize * 1.5f;

	ViewportClient->SetViewLocation(FocusPoint - PreservedRotation.Vector() * FocusDistance);
	ViewportClient->SetViewRotation(PreservedRotation);
	ViewportClient->SetLookAtLocation(FocusPoint, false);
	ViewportClient->Invalidate();
	return true;
}

#endif

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
