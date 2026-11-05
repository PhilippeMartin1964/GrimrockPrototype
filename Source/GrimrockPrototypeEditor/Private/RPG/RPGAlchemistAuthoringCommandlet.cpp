#include "RPG/RPGAlchemistAuthoringCommandlet.h"

#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGStartingHealthPotionAuthoring.h"
#include "RPG/RPGStartingSurfaceCatalystAuthoring.h"
#include "RPG/RPGSurfaceCatalystVisualAuthoring.h"
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
	if (FParse::Param(*Params, TEXT("StartingHealthPotion")))
	{
		const bool bApply = FParse::Param(*Params, TEXT("Apply"));
		if (!FRPGStartingHealthPotionAuthoring::Run(bApply, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.15] Starting health potion authoring failed: %s"), *Error);
			return 1;
		}
		return 0;
	}

	if (FParse::Param(*Params, TEXT("StartingSurfaceCatalyst")))
	{
		const bool bApply = FParse::Param(*Params, TEXT("Apply"));
		if (!FRPGStartingSurfaceCatalystAuthoring::Run(bApply, Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.2] Surface catalyst authoring failed: %s"), *Error);
			return 1;
		}
		return 0;
	}

	if (FParse::Param(*Params, TEXT("UpgradeSurfaceCatalystNative")))
	{
		if (!FRPGStartingSurfaceCatalystAuthoring::UpgradeExistingProduction(
			FParse::Param(*Params, TEXT("Apply")), Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.3D] DataAsset migration failed: %s"), *Error);
			return 1;
		}
		return 0;
	}

	if (FParse::Param(*Params, TEXT("SurfaceCatalystVisuals")))
	{
		if (!FRPGSurfaceCatalystVisualAuthoring::Run(FParse::Param(*Params, TEXT("Apply")), Error))
		{
			UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.17.2C] Catalyst visual production failed: %s"), *Error);
			return 1;
		}
		return 0;
	}

	if (!FRPGAlchemistAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.6C] Alchemist authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.6C] Alchemist production assets authored successfully."));
	return 0;
}
