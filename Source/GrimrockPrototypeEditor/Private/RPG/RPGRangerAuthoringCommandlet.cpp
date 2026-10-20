#include "RPG/RPGRangerAuthoringCommandlet.h"

#include "RPG/RPGRangerAuthoring.h"

URPGRangerAuthoringCommandlet::URPGRangerAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGRangerAuthoringCommandlet::Main(const FString& Params)
{
	const bool bTitlesOnly = Params.Contains(TEXT("-FavoredEnemyTitlesOnly"), ESearchCase::IgnoreCase);
	FString Error;
	const bool bSuccess = bTitlesOnly
		? FRPGRangerAuthoring::SyncProductionFavoredEnemyTitles(Error)
		: FRPGRangerAuthoring::AuthorProductionAssets(Error);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[RPG03.9.3] Ranger authoring failed: %s"), *Error);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[RPG03.9.3] Ranger %s completed successfully."),
		bTitlesOnly ? TEXT("Favored Enemy title synchronization") : TEXT("production authoring"));
	return 0;
}
