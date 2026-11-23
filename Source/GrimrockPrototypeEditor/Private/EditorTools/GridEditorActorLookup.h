#pragma once

class UWorld;
class AGridLevelEditorActor;

namespace GridEditorActorLookup
{
	// Keeps UE iterator order and validity/active-level filtering. Selection and
	// current level do not alter the existing first-actor policy. No world fallback.
	AGridLevelEditorActor* FindFirstInWorld(UWorld* World);
	AGridLevelEditorActor* FindInEditorWorld();
}
