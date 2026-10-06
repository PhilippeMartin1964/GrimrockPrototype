#include "RPG/RPGMageAuthoringCommandlet.h"

#include "RPG/RPGMageAuthoring.h"

URPGMageAuthoringCommandlet::URPGMageAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGMageAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	FString Error;
	if (!FRPGMageAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.4F2] Mage authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.4F2] Mage production assets authored successfully."));
	return 0;
}
