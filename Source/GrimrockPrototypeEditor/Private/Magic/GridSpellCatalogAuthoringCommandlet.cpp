#include "Magic/GridSpellCatalogAuthoringCommandlet.h"

#include "Magic/GridSpellCatalogAuthoring.h"

UGridSpellCatalogAuthoringCommandlet::UGridSpellCatalogAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UGridSpellCatalogAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;

	FString Error;
	if (!FGridSpellCatalogAuthoring::AuthorProductionAssets(Error))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[UI-RPG-CONTENT-TEXT05.5] Échec de la matérialisation des Spells : %s"),
			*Error);
		return 1;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG-CONTENT-TEXT05.5] 4 UGridSpellDefinitionAsset de production matérialisés."));
	return 0;
}
