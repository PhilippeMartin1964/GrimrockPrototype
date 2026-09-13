#if WITH_EDITOR

#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorUndoClient.h"
#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EngineUtils.h"

namespace
{
	bool bGridEditorUndoRedoRefreshQueued = false;

	class FGridEditorUndoBridge final : public FSelfRegisteringEditorUndoClient
	{
	public:
		virtual void PostUndo(bool bSuccess) override
		{
			if (bSuccess)
			{
				QueueGridEditorRefresh();
			}
		}

		virtual void PostRedo(bool bSuccess) override
		{
			if (bSuccess)
			{
				QueueGridEditorRefresh();
			}
		}

	private:
		static void QueueGridEditorRefresh()
		{
			if (bGridEditorUndoRedoRefreshQueued)
			{
				return;
			}

			bGridEditorUndoRedoRefreshQueued = true;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float /*DeltaTime*/)
			{
				bGridEditorUndoRedoRefreshQueued = false;
				RefreshGridEditorAfterUndoRedo();
				return false;
			}));
		}

		static void RefreshGridEditorAfterUndoRedo()
		{
			if (!GEditor || !GLevelEditorModeTools().IsModeActive(FGridLevelEdMode::EM_GridLevelEdModeId))
			{
				return;
			}

			UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
			if (!EditorWorld)
			{
				return;
			}

			for (TActorIterator<AGridLevelEditorActor> It(EditorWorld); It; ++It)
			{
				AGridLevelEditorActor* EditorActor = *It;
				if (!EditorActor)
				{
					continue;
				}

				if (UGridLevelAsset* LevelAsset = EditorActor->LevelAsset.Get())
				{
					if (EditorActor->LastSelectedObjectId.IsValid() && !LevelAsset->ContainsTypedPlacementId(EditorActor->LastSelectedObjectId))
					{
						EditorActor->LastSelectedObjectId.Invalidate();
					}
					if (EditorActor->HoveredObjectId.IsValid() && !LevelAsset->ContainsTypedPlacementId(EditorActor->HoveredObjectId))
					{
						EditorActor->HoveredObjectId.Invalidate();
					}
					if (EditorActor->PendingLinkSourceObjectId.IsValid() && !LevelAsset->ContainsTypedPlacementId(EditorActor->PendingLinkSourceObjectId))
					{
						EditorActor->ClearPendingLinkSource();
					}
				}

				// Rebuild on the next editor tick: by then Undo/Redo has completely
				// restored the authoritative UGridLevelAsset state. This avoids the
				// stale-redo preview that previously required pressing Load Default.
				EditorActor->RebuildPreview();
			}

			GEditor->RedrawAllViewports(true);
		}
	};

	FGridEditorUndoBridge GGridEditorUndoBridge;
}

#endif // WITH_EDITOR
