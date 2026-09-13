#if WITH_EDITOR

#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorUndoClient.h"
#include "EditorTools/GridLevelEdMode.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EngineUtils.h"

namespace
{
	class FGridEditorUndoBridge final : public FSelfRegisteringEditorUndoClient
	{
	public:
		virtual void PostUndo(bool bSuccess) override
		{
			if (bSuccess)
			{
				RefreshGridEditorAfterUndoRedo();
			}
		}

		virtual void PostRedo(bool bSuccess) override
		{
			if (bSuccess)
			{
				RefreshGridEditorAfterUndoRedo();
			}
		}

	private:
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

				EditorActor->RebuildPreview();
			}

			GEditor->RedrawAllViewports(true);
		}
	};

	FGridEditorUndoBridge GGridEditorUndoBridge;
}

#endif // WITH_EDITOR
