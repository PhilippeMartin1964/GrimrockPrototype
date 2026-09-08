#include "EditorTools/GridLevelEditorActor.h"

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
