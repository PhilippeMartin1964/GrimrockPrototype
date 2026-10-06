#include "RPG/RPGPriestAuthoringCommandlet.h"

#include "RPG/RPGPriestAuthoring.h"

URPGPriestAuthoringCommandlet::URPGPriestAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGPriestAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	FString Error;
	if (!FRPGPriestAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.5C] Priest authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.5C] Priest production assets authored successfully."));
	return 0;
}
