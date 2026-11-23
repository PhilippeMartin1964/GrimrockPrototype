#include "EditorTools/GridEditorActorLookup.h"

#include "Editor.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "EngineUtils.h"

AGridLevelEditorActor* GridEditorActorLookup::FindFirstInWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AGridLevelEditorActor> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AGridLevelEditorActor* GridEditorActorLookup::FindInEditorWorld()
{
	return GEditor ? FindFirstInWorld(GEditor->GetEditorWorldContext().World()) : nullptr;
}
