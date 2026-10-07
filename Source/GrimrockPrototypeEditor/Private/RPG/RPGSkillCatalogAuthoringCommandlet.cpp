#include "RPG/RPGSkillCatalogAuthoringCommandlet.h"

#include "RPG/RPGSkillCatalogAuthoring.h"

URPGSkillCatalogAuthoringCommandlet::URPGSkillCatalogAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGSkillCatalogAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;

	FString Error;
	if (!FRPGSkillCatalogAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-RPG06.1] Échec de l'authoring du catalogue Skills : %s"), *Error);
		return 1;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG06.1] Catalogue Skills matérialisé : 25 URPGSkillAsset canoniques."));
	return 0;
}
