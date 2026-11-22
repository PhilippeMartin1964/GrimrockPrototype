#include "RPG/RPGRogueAuthoringCommandlet.h"

#include "RPG/RPGRogueAuthoring.h"
#include "Misc/Parse.h"

URPGRogueAuthoringCommandlet::URPGRogueAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGRogueAuthoringCommandlet::Main(const FString& Params)
{
	FString Error;
	// Rogue class/status authoring is still valid. The legacy tool
	// DataAsset must never be recreated through a stale commandlet flag.
	if (FParse::Param(*Params, TEXT("StartingDisarmTool")))
	{
		UE_LOG(LogTemp, Error, TEXT("Legacy DisarmTool authoring retired; ItemSystem is authoritative."));
		return 1;
	}

	if (!FRPGRogueAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.2] Rogue authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.2] Rogue production assets authored successfully."));
	return 0;
}
