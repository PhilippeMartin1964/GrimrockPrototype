#if WITH_EDITOR

#include "Containers/Ticker.h"
#include "Core/GridLevelAsset.h"
#include "Editor.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EngineUtils.h"
#include "UObject/CoreUObjectDelegates.h"
#include "UObject/Transactor.h"

namespace
{
	FDelegateHandle ObjectTransactedHandle;
	FTSTicker::FDelegateHandle RefreshTickerHandle;
	TArray<TWeakObjectPtr<UGridLevelAsset>> PendingLevelAssets;
	bool bBridgeStarted = false;

	bool IsPendingLevelAsset(const UGridLevelAsset* LevelAsset)
	{
		return LevelAsset && PendingLevelAssets.ContainsByPredicate(
			[LevelAsset](const TWeakObjectPtr<UGridLevelAsset>& PendingAsset)
			{
				return PendingAsset.Get() == LevelAsset;
			});
	}

	void SanitizeEditorSelectionAfterUndoRedo(AGridLevelEditorActor& EditorActor, UGridLevelAsset& LevelAsset)
	{
		if (EditorActor.LastSelectedObjectId.IsValid() && !LevelAsset.ContainsTypedPlacementId(EditorActor.LastSelectedObjectId))
		{
			EditorActor.LastSelectedObjectId.Invalidate();
		}
		if (EditorActor.HoveredObjectId.IsValid() && !LevelAsset.ContainsTypedPlacementId(EditorActor.HoveredObjectId))
		{
			EditorActor.HoveredObjectId.Invalidate();
		}
		if (EditorActor.PendingLinkSourceObjectId.IsValid() && !LevelAsset.ContainsTypedPlacementId(EditorActor.PendingLinkSourceObjectId))
		{
			EditorActor.ClearPendingLinkSource();
		}
	}

	bool RefreshAffectedGridEditorPreviews(float /*DeltaTime*/)
	{
		RefreshTickerHandle = FTSTicker::FDelegateHandle();

		if (!bBridgeStarted || !GEditor || GEditor->PlayWorld)
		{
			PendingLevelAssets.Reset();
			return false;
		}

		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		if (!EditorWorld || EditorWorld->WorldType != EWorldType::Editor)
		{
			PendingLevelAssets.Reset();
			return false;
		}

		bool bRefreshedAnyPreview = false;
		for (TActorIterator<AGridLevelEditorActor> It(EditorWorld); It; ++It)
		{
			AGridLevelEditorActor* EditorActor = *It;
			if (!EditorActor)
			{
				continue;
			}

			UGridLevelAsset* LevelAsset = EditorActor->LevelAsset.Get();
			if (!IsPendingLevelAsset(LevelAsset))
			{
				continue;
			}

			SanitizeEditorSelectionAfterUndoRedo(*EditorActor, *LevelAsset);

			// OnObjectTransacted is emitted while the native transaction is still
			// applying. Rebuilding here, on the following editor tick, guarantees
			// the UGridLevelAsset is fully restored and avoids calling Modify()
			// while GIsTransacting is true.
			EditorActor->RebuildPreview();
			bRefreshedAnyPreview = true;
		}

		PendingLevelAssets.Reset();
		if (bRefreshedAnyPreview)
		{
			GEditor->RedrawAllViewports(true);
		}
		return false;
	}

	void QueueGridEditorRefresh(UGridLevelAsset* LevelAsset)
	{
		if (!LevelAsset)
		{
			return;
		}

		PendingLevelAssets.AddUnique(TWeakObjectPtr<UGridLevelAsset>(LevelAsset));
		if (!RefreshTickerHandle.IsValid())
		{
			RefreshTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&RefreshAffectedGridEditorPreviews));
		}
	}

	void HandleObjectTransacted(UObject* Object, const FTransactionObjectEvent& TransactionEvent)
	{
		if (!bBridgeStarted || TransactionEvent.GetEventType() != ETransactionObjectEventType::UndoRedo)
		{
			return;
		}

		if (UGridLevelAsset* LevelAsset = Cast<UGridLevelAsset>(Object))
		{
			QueueGridEditorRefresh(LevelAsset);
		}
	}
}

namespace GridEditorUndoBridge
{
	void Startup()
	{
		if (bBridgeStarted)
		{
			return;
		}

		bBridgeStarted = true;
		ObjectTransactedHandle = FCoreUObjectDelegates::OnObjectTransacted.AddStatic(&HandleObjectTransacted);
	}

	void Shutdown()
	{
		bBridgeStarted = false;

		if (ObjectTransactedHandle.IsValid())
		{
			FCoreUObjectDelegates::OnObjectTransacted.Remove(ObjectTransactedHandle);
			ObjectTransactedHandle = FDelegateHandle();
		}

		if (RefreshTickerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(RefreshTickerHandle);
			RefreshTickerHandle = FTSTicker::FDelegateHandle();
		}

		PendingLevelAssets.Reset();
	}
}

#endif // WITH_EDITOR
