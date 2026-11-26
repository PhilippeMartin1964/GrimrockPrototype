#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../EditorTools/Inspection/GridItemWorkshopSession.h"
#include "../EditorTools/Inspection/GridItemWorkshopOrbit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorItemWorkshopCatalogueTest,
	"Grimrock.Editor.Items.Inspection.Catalogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorItemWorkshopCatalogueTest::RunTest(const FString&)
{
	FGridItemWorkshopSession Session;
	if (!TestTrue(TEXT("Configured production registry"), Session.Rebuild()))
	{
		AddError(Session.GetError());
		return false;
	}
	TestEqual(TEXT("Materialized definitions"), Session.GetEntries().Num(), 48);
	TArray<TSharedPtr<FGridWorkshopEntry>> Filtered;
	Session.Filter(TEXT(""), EGridWorkshopFilter::Armaments, Filtered);
	TestEqual(TEXT("ARM filter"), Filtered.Num(), 22);
	Session.Filter(TEXT("ARM014"), EGridWorkshopFilter::Armaments, Filtered);
	if (TestEqual(TEXT("Archetype search"), Filtered.Num(), 1))
	{
		TestEqual(TEXT("ARM014 item identity"), Filtered[0]->Id, FName(TEXT("Weapon_StraightSword_01")));
	}
	Session.Filter(TEXT("Stone"), EGridWorkshopFilter::All, Filtered);
	TestTrue(TEXT("Non-ARM is searchable"), Filtered.ContainsByPredicate(
		[](const TSharedPtr<FGridWorkshopEntry>& Entry) { return Entry->Id == FName(TEXT("Stone")); }));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorItemWorkshopSelectionTest,
	"Grimrock.Editor.Items.Inspection.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorItemWorkshopSelectionTest::RunTest(const FString&)
{
	FGridItemWorkshopSession Session;
	if (!TestTrue(TEXT("Configured registry"), Session.Rebuild()))
	{
		return false;
	}
	TestTrue(TEXT("Select ARM014"), Session.Select(TEXT("Weapon_StraightSword_01")));
	for (int32 N = 1; N <= 20; ++N)
	{
		TestTrue(TEXT("Select exact level"), Session.SelectLevel(N));
		TestEqual(TEXT("No adjacent-level fallback"), Session.GetView().Item.ItemLevel, N);
	}
	TestTrue(TEXT("Refresh retains valid selection"), Session.Rebuild());
	TestEqual(TEXT("Level retained after refresh"), Session.GetSelectedLevel(), 20);
	TestFalse(TEXT("N21 rejected"), Session.SelectLevel(21));
	TestTrue(TEXT("Select unlevelled stone"), Session.Select(TEXT("Stone")));
	TestEqual(TEXT("Stone internal level"), Session.GetSelectedLevel(), 0);
	TestFalse(TEXT("No level control for stone"), Session.GetView().HasLevel());
	TestFalse(TEXT("No simulated ARM level for stone"), Session.SelectLevel(2));
	TestFalse(TEXT("Unknown identity fails closed"), Session.Select(TEXT("Inspection_Unknown")));
	TestFalse(TEXT("Prior valid view cleared"), Session.GetView().IsValid());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorItemWorkshopOrbitTest,
	"Grimrock.Editor.Items.Inspection.Orbit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorItemWorkshopOrbitTest::RunTest(const FString&)
{
	FGridItemWorkshopOrbit Orbit;
	const FVector Center(74.0, -39.0, 61.0);
	const FBoxSphereBounds Bounds(Center, FVector(35.0, 4.0, 10.0), 37.0);
	Orbit.SetBounds(Bounds, true);
	const double Distance = Orbit.GetDistance();
	const FVector Before = Orbit.CameraLocation();
	Orbit.Drag(FVector2D(160.0, -45.0));
	TestTrue(TEXT("Camera moves around object, not around itself"), !Orbit.CameraLocation().Equals(Before));
	TestTrue(TEXT("Geometric centre stays fixed"), Orbit.Target.Equals(Center));
	TestTrue(TEXT("Orbit radius stays constant"), FMath::IsNearlyEqual(FVector::Dist(Orbit.CameraLocation(), Center), Distance));
	TestTrue(TEXT("Camera faces geometric centre"), (Center - Orbit.CameraLocation()).GetSafeNormal().Equals(Orbit.Orientation.GetAxisX()));
	const FQuat AfterDrag = Orbit.Orientation;
	Orbit.SetBounds(Bounds, false);
	TestTrue(TEXT("Level change preserves orientation"), Orbit.Orientation.Equals(AfterDrag));
	TestTrue(TEXT("Level change preserves distance"), FMath::IsNearlyEqual(Orbit.GetDistance(), Distance));
	Orbit.Zoom(1.0);
	TestTrue(TEXT("Wheel forward zooms in"), Orbit.GetDistance() < Distance);
	TestTrue(TEXT("Zoom retains aim"), (Center - Orbit.CameraLocation()).GetSafeNormal().Equals(Orbit.Orientation.GetAxisX()));
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Orbit.Zoom(1.0);
	}
	TestTrue(TEXT("Zoom clamps outside bounds"), FMath::IsNearlyEqual(Orbit.GetDistance(), Orbit.MinDistance()));
	TestTrue(TEXT("Optical detail zoom reaches minimum FOV"), FMath::IsNearlyEqual(Orbit.ViewFov(), FGridItemWorkshopOrbit::DetailFovDegrees, 0.001));
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Orbit.Zoom(-1.0);
	}
	TestTrue(TEXT("Zoom out is bounded"), FMath::IsNearlyEqual(Orbit.GetDistance(), Orbit.MaxDistance()));
	// A point on the initially visible face must follow the mouse in
	// screen space. Test directions independently, not just angle signs.
	const FVector FrontPoint = -FVector::ForwardVector;
	Orbit.SetBounds(Bounds, true, FRotator::ZeroRotator);
	Orbit.Drag(FVector2D(100.0, 0.0));
	TestTrue(TEXT("Drag right moves the visible face right"), FVector::DotProduct(FrontPoint, Orbit.Orientation.GetAxisY()) > 0.0);
	Orbit.SetBounds(Bounds, true, FRotator::ZeroRotator);
	Orbit.Drag(FVector2D(-100.0, 0.0));
	TestTrue(TEXT("Drag left moves the visible face left"), FVector::DotProduct(FrontPoint, Orbit.Orientation.GetAxisY()) < 0.0);
	Orbit.SetBounds(Bounds, true, FRotator::ZeroRotator);
	Orbit.Drag(FVector2D(0.0, 100.0));
	TestTrue(TEXT("Drag down moves the visible face down"), FVector::DotProduct(FrontPoint, Orbit.Orientation.GetAxisZ()) < 0.0);
	Orbit.SetBounds(Bounds, true, FRotator::ZeroRotator);
	Orbit.Drag(FVector2D(0.0, -100.0));
	TestTrue(TEXT("Drag up moves the visible face up"), FVector::DotProduct(FrontPoint, Orbit.Orientation.GetAxisZ()) > 0.0);

	// Rotate through both poles and the back side on each screen axis.
	// Check the actual orientation sent through the viewport rotator API.
	for (const FVector2D Axis : { FVector2D(1.0, 0.0), FVector2D(0.0, 1.0) })
	{
		Orbit.SetBounds(Bounds, true, FRotator::ZeroRotator);
		const FQuat Initial = Orbit.Orientation;
		const FVector StartLocation = Orbit.CameraLocation();
		for (int32 Step = 1; Step <= 36; ++Step)
		{
			const FQuat Previous = Orbit.Orientation;
			Orbit.Drag(Axis * (10.0 / 0.35));
			const double Angle = FMath::RadiansToDegrees(Previous.AngularDistance(Orbit.Orientation));
			TestTrue(TEXT("Continuous ten-degree step, including poles"), FMath::IsNearlyEqual(Angle, 10.0, 0.001));
			TestTrue(TEXT("Quaternion survives viewport rotator conversion"), Orbit.ViewRotation().Quaternion().Equals(Orbit.Orientation, 0.001));
			TestTrue(TEXT("Centre remains in front"), (Center - Orbit.CameraLocation()).GetSafeNormal().Equals(Orbit.ViewRotation().Vector(), 0.001));
			if (Step == 18)
			{
				TestTrue(TEXT("Half-turn reaches opposite side"), (Orbit.CameraLocation() - Center).Equals(Center - StartLocation, 0.001));
			}
			if (Step == 27)
			{
				TestTrue(TEXT("Rotation continues beyond 180 degrees"), FMath::Abs(FVector::DotProduct((Orbit.CameraLocation() - Center).GetSafeNormal(), (StartLocation - Center).GetSafeNormal())) < 0.001);
			}
		}
		TestTrue(TEXT("Full 360-degree rotation returns to start"), Orbit.Orientation.Equals(Initial, 0.001));
		TestTrue(TEXT("Full turn retains radius"), Orbit.CameraLocation().Equals(StartLocation, 0.001));
	}
	Orbit.SetBounds(Bounds, true, FRotator::ZeroRotator);
	Orbit.Drag(FVector2D(0.0, 180.0 / 0.35));
	const FVector RightBefore = Orbit.Orientation.GetAxisY();
	const FVector ForwardBefore = Orbit.Orientation.GetAxisX();
	Orbit.Drag(FVector2D(100.0, 0.0));
	TestTrue(TEXT("Screen-right remains screen-right upside down"), FVector::DotProduct(-ForwardBefore, Orbit.Orientation.GetAxisY()) > 0.0);
	TestTrue(TEXT("Upside-down orbit has continuous screen basis"), FVector::DotProduct(RightBefore, Orbit.Orientation.GetAxisY()) > 0.0);
	Orbit.SetBounds(FBoxSphereBounds(FVector(-17.0, 2.0, 0.0), FVector(1.0), 2.0), true);
	TestTrue(TEXT("New object is recentered"), Orbit.Target.Equals(FVector(-17.0, 2.0, 0.0)));
	TestTrue(TEXT("Small object near clip safety"), Orbit.GetDistance() >= Orbit.MinDistance());
	Orbit.Clear();
	TestFalse(TEXT("No target after clearing"), Orbit.bReady);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorItemWorkshopDetailNavigationTest,
	"Grimrock.Editor.Items.Inspection.DetailNavigation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorItemWorkshopDetailNavigationTest::RunTest(const FString&)
{
	FGridItemWorkshopOrbit Orbit;
	const FVector Center(12.0, -37.0, 25.0);
	const FBoxSphereBounds Sword(Center, FVector(0.8, 5.0, 55.0), 56.0);
	Orbit.SetBounds(Sword, true, FRotator::ZeroRotator);
	TestTrue(TEXT("Broadside minimum follows thickness, not sword length"), FMath::IsNearlyEqual(Orbit.MinDistance(), 1.3, 0.001));
	for (int32 Index = 0; Index < 150; ++Index)
	{
		Orbit.Zoom(1.0);
	}
	TestTrue(TEXT("Physical camera reaches half a centimetre from the broadside bounds"), FMath::IsNearlyEqual(Orbit.GetDistance(), 1.3, 0.001));
	TestTrue(TEXT("Close-up has a locally safe near plane"), FGridItemWorkshopOrbit::NearClipCm < FGridItemWorkshopOrbit::SurfaceClearanceCm);
	TestTrue(TEXT("Extra detail is available without crossing the mesh"), Orbit.ViewFov() <= 2.001);
	const FVector CameraBefore = Orbit.CameraLocation();
	const FQuat RotationBefore = Orbit.Orientation;
	const double DistanceBefore = Orbit.GetDistance();
	Orbit.Pan(FVector2D(100.0, 80.0), 1000.0);
	const FVector Shift = Orbit.CameraLocation() - CameraBefore;
	TestTrue(TEXT("Right drag shifts camera left"), FVector::DotProduct(Shift, RotationBefore.GetAxisY()) < 0.0);
	TestTrue(TEXT("Down drag shifts camera up"), FVector::DotProduct(Shift, RotationBefore.GetAxisZ()) > 0.0);
	TestTrue(TEXT("Pan has no depth motion"), FMath::Abs(FVector::DotProduct(Shift, RotationBefore.GetAxisX())) < 0.001);
	TestTrue(TEXT("Pan retains geometric pivot"), Orbit.Target.Equals(Center));
	TestTrue(TEXT("Pan retains rotation"), Orbit.Orientation.Equals(RotationBefore));
	TestTrue(TEXT("Pan retains distance"), FMath::IsNearlyEqual(Orbit.GetDistance(), DistanceBefore));
	TestTrue(TEXT("Camera faces panned aim without forced recenter"), (Orbit.AimLocation() - Orbit.CameraLocation()).GetSafeNormal().Equals(Orbit.Orientation.GetAxisX()));
	const FVector2D SavedPan = Orbit.PanOffset;
	const double SavedFov = Orbit.ViewFov();
	Orbit.SetBounds(Sword, false);
	TestTrue(TEXT("Level change preserves pan"), Orbit.PanOffset.Equals(SavedPan));
	TestTrue(TEXT("Level change preserves detail magnification"), FMath::IsNearlyEqual(Orbit.ViewFov(), SavedFov));
	Orbit.Pan(FVector2D(-100.0, -80.0), 1000.0);
	TestTrue(TEXT("Opposite drag cancels pan"), Orbit.PanOffset.IsNearlyZero(0.001));
	Orbit.Drag(FVector2D(0.0, 90.0 / 0.35));
	TestTrue(TEXT("Close orbit remains outside longitudinal bounds"), Orbit.GetDistance() >= Orbit.MinDistance());
	TestTrue(TEXT("Longitudinal minimum uses actual depth"), FMath::IsNearlyEqual(Orbit.MinDistance(), 55.5, 0.001));
	Orbit.Pan(FVector2D(500.0, -300.0), 1000.0);
	Orbit.SetBounds(Sword, true);
	TestTrue(TEXT("Recenter clears pan"), Orbit.PanOffset.IsNearlyZero());
	TestTrue(TEXT("Recenter resets detail FOV"), FMath::IsNearlyEqual(Orbit.ViewFov(), FGridItemWorkshopOrbit::DefaultFovDegrees));
	Orbit.SetBounds(FBoxSphereBounds(Center, FVector(0.2), 0.35), true, FRotator::ZeroRotator);
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Orbit.Zoom(1.0);
	}
	TestTrue(TEXT("Tiny item has sub-centimetre physical viewing distance"), Orbit.GetDistance() < 1.0);
	return !HasAnyErrors();
}

#endif
