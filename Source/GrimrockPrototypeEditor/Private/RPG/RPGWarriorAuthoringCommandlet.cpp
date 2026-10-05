#include "RPG/RPGWarriorAuthoringCommandlet.h"

#include "RPG/RPGWarriorAuthoring.h"

URPGWarriorAuthoringCommandlet::URPGWarriorAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGWarriorAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	FString Error;
	if (!FRPGWarriorAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.1] Warrior authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.1] Warrior production assets authored successfully."));
	return 0;
}
