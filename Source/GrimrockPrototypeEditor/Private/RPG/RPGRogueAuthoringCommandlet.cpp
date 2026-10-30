#include "RPG/RPGRogueAuthoringCommandlet.h"

#include "RPG/RPGRogueAuthoring.h"
#include "RPG/RPGStartingDisarmToolAuthoring.h"
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
	if (FParse::Param(*Params, TEXT("StartingDisarmTool")))
	{
		const bool bApply = FParse::Param(*Params, TEXT("Apply"));
		if (!FRPGStartingDisarmToolAuthoring::Run(bApply, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.16.1] Disarm tool authoring failed: %s"), *Error);
			return 1;
		}
		return 0;
	}

	if (!FRPGRogueAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.2] Rogue authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.2] Rogue production assets authored successfully."));
	return 0;
}
