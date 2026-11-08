#include "Misc/AutomationTest.h"
#include "Core/GridGlassShardMotion.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFlaskGlass01BallisticsTest,
	"Grimrock.RPG.FLASK.GLASS01.ShardBallistics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFlaskGlass01BallisticsTest::RunTest(const FString&)
{
	const FVector Impact(120.f, 80.f, 14.f);
	FGridGlassShardState Shard = FGridGlassShardMotion::MakeInitial(2, 14, Impact, 1.f, 155.f);
	TestTrue(TEXT("Initial shard velocity points upward"), Shard.Velocity.Z > 0.f);
	const float StartZ = Shard.Position.Z;
	FGridGlassShardMotion::Advance(Shard, 1.f / 30.f);
	TestTrue(TEXT("Shard initially rises after impact"), Shard.Position.Z > StartZ);
	TestTrue(TEXT("Shard spins while traveling"), !Shard.Rotation.IsNearlyZero());
	for (int32 Tick = 0; Tick < 60; ++Tick)
		FGridGlassShardMotion::Advance(Shard, 1.f / 30.f);
	TestTrue(TEXT("No shard passes below the impact floor"), Shard.Position.Z >= Shard.GroundZ);
	TestTrue(TEXT("Floor bounce cannot exceed two impacts"), Shard.Bounces <= 2);
	TestTrue(TEXT("Finite shard position"), !Shard.Position.ContainsNaN());
	FGridGlassShardState A = FGridGlassShardMotion::MakeInitial(3, 14, Impact, 1.f, 155.f);
	FGridGlassShardState B = FGridGlassShardMotion::MakeInitial(3, 14, Impact, 1.f, 155.f);
	TestTrue(TEXT("Shard seed is deterministic"), A.Velocity.Equals(B.Velocity));
	TestFalse(TEXT("Different fragments have different trajectories"),
		A.Velocity.Equals(Shard.Velocity));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFlaskGlass01ProductionAssetTest,
	"Grimrock.RPG.FLASK.GLASS01.ProductionResources",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFlaskGlass01ProductionAssetTest::RunTest(const FString&)
{
	const AGridLevelRuntimeActor* Defaults = GetDefault<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Native runtime defaults"), Defaults)) return false;
	TestNotNull(TEXT("Builtin shard mesh loads"), Defaults->GlassShardMesh.LoadSynchronous());
	TestNotNull(TEXT("Game-owned glass material loads"), Defaults->GlassShardMaterial.LoadSynchronous());
	TestTrue(TEXT("Reasonable shard count"), Defaults->GlassShardCount >= 8 && Defaults->GlassShardCount <= 24);
	TestTrue(TEXT("Speed authoring remains positive"), Defaults->GlassShardLaunchSpeedCmPerSec > 0.f);
	TestNotNull(TEXT("Existing optional Niagara override remains"),
		AGridLevelRuntimeActor::StaticClass()->FindPropertyByName(TEXT("FlaskGlassBreakVfx")));
	return true;
}
#endif
