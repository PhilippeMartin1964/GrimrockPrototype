#include "RPG/RPGAlchemistAuthoringCommandlet.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "Misc/Parse.h"

URPGAlchemistAuthoringCommandlet::URPGAlchemistAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGAlchemistAuthoringCommandlet::Main(const FString& Params)
{
	FString Error;
	// Historical item authoring can no longer write DA_Item_* packages.
	// This commandlet remains for the separate class/status authoring path.
	if (FParse::Param(*Params, TEXT("StartingHealthPotion")) ||
		FParse::Param(*Params, TEXT("StartingSurfaceCatalyst")))
	{
		UE_LOG(LogTemp, Error, TEXT("Legacy item-authoring modes were retired; configure DA_ItemSystem instead."));
		return 1;
	}

	if (!FRPGAlchemistAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.6C] Alchemist authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.6C] Alchemist production assets authored successfully."));
	return 0;
}
