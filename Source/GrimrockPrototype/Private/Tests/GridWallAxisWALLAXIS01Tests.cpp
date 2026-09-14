#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"

namespace
{
	bool IsRotation(const FTransform& Transform, float ExpectedYaw)
	{
		return Transform.Rotator().Equals(FRotator(0.0f, ExpectedYaw, 0.0f), 0.01f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWallAxis01CanonicalBoundaryFrameTest,
	"Grimrock.WorldObjects.WALL_AXIS01.CanonicalBoundaryFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWallAxis01CanonicalBoundaryFrameTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	struct FExpectedBoundary
	{
		EGridEdge Edge;
		FVector Location;
		FVector Normal;
		FVector Tangent;
		float Yaw;
	};

	const FVector CellOrigin(200.0f, 400.0f, 0.0f);
	constexpr float CellSize = 200.0f;
	const FExpectedBoundary Cases[] = {
		{EGridEdge::North, FVector(300.0f, 600.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(-1.0f, 0.0f, 0.0f), 90.0f},
		{EGridEdge::East, FVector(400.0f, 500.0f, 0.0f), FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), 0.0f},
		{EGridEdge::South, FVector(300.0f, 400.0f, 0.0f), FVector(0.0f, -1.0f, 0.0f), FVector(1.0f, 0.0f, 0.0f), -90.0f},
		{EGridEdge::West, FVector(200.0f, 500.0f, 0.0f), FVector(-1.0f, 0.0f, 0.0f), FVector(0.0f, -1.0f, 0.0f), 180.0f},
	};

	for (const FExpectedBoundary& Case : Cases)
	{
		GridDirectionUtils::FBoundaryFrame Frame;
		TestTrue(TEXT("Canonical boundary frame resolves"), GridDirectionUtils::ResolveBoundaryFrame(Case.Edge, Frame));
		TestTrue(TEXT("Canonical boundary normal matches edge"), Frame.Normal.Equals(Case.Normal, 0.01f));
		TestTrue(TEXT("Canonical boundary tangent follows mesh local Y"), Frame.Tangent.Equals(Case.Tangent, 0.01f));
		TestTrue(TEXT("Canonical boundary yaw matches GridDirectionUtils::ToYaw"), FMath::IsNearlyEqual(Frame.Rotation.Yaw, Case.Yaw, 0.01f));

		FTransform Transform;
		TestTrue(TEXT("Canonical boundary transform resolves"),
			GridDirectionUtils::ResolveBoundaryTransform(CellOrigin, CellSize, Case.Edge, 0.0f, 0.0f, 0.0f, Transform));
		TestTrue(TEXT("Canonical boundary location is exact"), Transform.GetLocation().Equals(Case.Location, 0.01f));
		TestTrue(TEXT("Canonical boundary rotation is exact"), IsRotation(Transform, Case.Yaw));
	}

	GridDirectionUtils::FBoundaryFrame InvalidFrame;
	TestFalse(TEXT("None has no wall boundary frame"), GridDirectionUtils::ResolveBoundaryFrame(EGridEdge::None, InvalidFrame));
	FTransform InvalidTransform;
	TestFalse(TEXT("None has no wall boundary transform"),
		GridDirectionUtils::ResolveBoundaryTransform(CellOrigin, CellSize, EGridEdge::None, 0.0f, 0.0f, 0.0f, InvalidTransform));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWallAxis01MountedOffsetsTest,
	"Grimrock.WorldObjects.WALL_AXIS01.MountedOffsets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWallAxis01MountedOffsetsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	struct FExpectedMounted
	{
		EGridEdge Edge;
		FVector Location;
		float Yaw;
	};

	const FVector CellOrigin(200.0f, 400.0f, 0.0f);
	constexpr float CellSize = 200.0f;
	constexpr float Vertical = 110.0f;
	constexpr float Inset = 6.0f;
	constexpr float AlongWall = 25.0f;
	const FExpectedMounted Cases[] = {
		{EGridEdge::North, FVector(325.0f, 594.0f, 110.0f), 90.0f},
		{EGridEdge::East, FVector(394.0f, 475.0f, 110.0f), 0.0f},
		{EGridEdge::South, FVector(275.0f, 406.0f, 110.0f), -90.0f},
		{EGridEdge::West, FVector(206.0f, 525.0f, 110.0f), 180.0f},
	};

	for (const FExpectedMounted& Case : Cases)
	{
		FTransform Transform;
		TestTrue(TEXT("Mounted wall transform resolves"),
			GridDirectionUtils::ResolveBoundaryTransform(CellOrigin, CellSize, Case.Edge, Vertical, Inset, AlongWall, Transform));
		TestTrue(TEXT("Mounted wall U/V/N semantics are preserved"), Transform.GetLocation().Equals(Case.Location, 0.01f));
		TestTrue(TEXT("Mounted wall uses canonical Y-axis yaw"), IsRotation(Transform, Case.Yaw));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
