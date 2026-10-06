#include "RPG/RPGRangerAuthoringCommandlet.h"

#include "RPG/RPGRangerAuthoring.h"

URPGRangerAuthoringCommandlet::URPGRangerAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGRangerAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	FString Error;
	if (!FRPGRangerAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.3] Ranger authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.3] Ranger production assets authored successfully."));
	return 0;
}
