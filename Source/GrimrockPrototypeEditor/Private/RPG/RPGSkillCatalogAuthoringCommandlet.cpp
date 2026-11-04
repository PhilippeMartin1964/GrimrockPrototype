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
	FString RequestedSkill;
	FParse::Value(*Params, TEXT("SkillId="), RequestedSkill);
	const FName OnlySkillId = RequestedSkill.IsEmpty() ? NAME_None : FName(*RequestedSkill);
	FString Error;
	if (!FRPGSkillCatalogAuthoring::AuthorProductionAssets(Error, OnlySkillId))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-RPG06.1] Échec de l'authoring du catalogue Skills : %s"), *Error);
		return 1;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[UI-RPG06.1] Authoring Skills canonique : %s."),
		OnlySkillId.IsNone() ? TEXT("25 DataAssets") : *OnlySkillId.ToString());
	return 0;
}
