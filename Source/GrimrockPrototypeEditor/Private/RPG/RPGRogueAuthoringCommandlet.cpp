#include "RPG/RPGRogueAuthoringCommandlet.h"

#include "RPG/RPGRogueAuthoring.h"

URPGRogueAuthoringCommandlet::URPGRogueAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGRogueAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	FString Error;
	if (!FRPGRogueAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.2] Rogue authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.2] Rogue production assets authored successfully."));
	return 0;
}
