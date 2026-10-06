#include "RPG/RPGAlchemistAuthoringCommandlet.h"

#include "RPG/RPGAlchemistAuthoring.h"

URPGAlchemistAuthoringCommandlet::URPGAlchemistAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGAlchemistAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	FString Error;
	if (!FRPGAlchemistAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.6C] Alchemist authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.6C] Alchemist production assets authored successfully."));
	return 0;
}
