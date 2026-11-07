#include "Misc/AutomationTest.h"
#include "Core/GridFireAudioClusters.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFireAudioClustersTests,
	"Grimrock.RPG.FIREFX01.FireAudioClusters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFireAudioClustersTests::RunTest(const FString&)
{
	TArray<FIntPoint> Cells = {FIntPoint(3, 1), FIntPoint(2, 1),
		FIntPoint(8, 8), FIntPoint(8, 9), FIntPoint(12, 2)};
	TArray<FGridFireAudioCluster> Clusters;
	FGridFireAudioClusters::Build(Cells, 4, Clusters);
	TestEqual(TEXT("One voice per contiguous region"), Clusters.Num(), 3);
	if (Clusters.Num() == 3)
	{
		TestEqual(TEXT("Stable first anchor"), Clusters[0].Anchor, FIntPoint(2, 1));
		TestEqual(TEXT("First region size"), Clusters[0].CellCount, 2);
		TestTrue(TEXT("Centroid on grid"), Clusters[0].CenterInCells.Equals(FVector2D(3.f, 1.5f), .001f));
	}
	FGridFireAudioClusters::Build(Cells, 1, Clusters);
	TestEqual(TEXT("Audio voice budget"), Clusters.Num(), 1);
	FGridFireAudioClusters::Build(Cells, 0, Clusters);
	TestEqual(TEXT("Audio disabled cleanly"), Clusters.Num(), 0);
	return true;
}
#endif
