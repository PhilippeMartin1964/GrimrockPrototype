#include "EditorTools/GridLevelEditorActor.h"

namespace
{
	bool ApplyGridEditorObjectSnapshotToAuthority(UGridLevelAsset* LevelAsset, const FGridLevelObjectData& EditedObject)
	{
		if (!LevelAsset || !EditedObject.ObjectId.IsValid())
		{
			return false;
		}

		if (LevelAsset->bTypedPlacementStorageAuthoritative)
		{
			return LevelAsset->AddObject(EditedObject) == EditedObject.ObjectId;
		}

		FGridLevelObjectData* LegacyObject = LevelAsset->Objects.FindByPredicate(
			[&EditedObject](const FGridLevelObjectData& Object)
			{
				return Object.ObjectId == EditedObject.ObjectId;
			});
		if (!LegacyObject)
		{
			return false;
		}

		*LegacyObject = EditedObject;
		return true;
	}
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
