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
	const double Distance = Orbit.Distance;
	const FVector Before = Orbit.CameraLocation();
	Orbit.Drag(FVector2D(160.0, -45.0));
	TestTrue(TEXT("Camera moves around object, not around itself"), !Orbit.CameraLocation().Equals(Before));
	TestTrue(TEXT("Geometric centre stays fixed"), Orbit.Target.Equals(Center));
	TestTrue(TEXT("Orbit radius stays constant"), FMath::IsNearlyEqual(FVector::Dist(Orbit.CameraLocation(), Center), Distance));
	TestTrue(TEXT("Camera faces geometric centre"), (Center - Orbit.CameraLocation()).GetSafeNormal().Equals(Orbit.Rotation.Vector()));
	const FRotator AfterDrag = Orbit.Rotation;
	Orbit.SetBounds(Bounds, false);
	TestTrue(TEXT("Level change preserves orientation"), Orbit.Rotation.Equals(AfterDrag));
	TestTrue(TEXT("Level change preserves distance"), FMath::IsNearlyEqual(Orbit.Distance, Distance));
	Orbit.Zoom(1.0);
	TestTrue(TEXT("Wheel forward zooms in"), Orbit.Distance < Distance);
	TestTrue(TEXT("Zoom retains aim"), (Center - Orbit.CameraLocation()).GetSafeNormal().Equals(Orbit.Rotation.Vector()));
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Orbit.Zoom(1.0);
	}
	TestTrue(TEXT("Zoom clamps outside bounds"), FMath::IsNearlyEqual(Orbit.Distance, Orbit.MinDistance()));
	for (int32 Index = 0; Index < 100; ++Index)
	{
		Orbit.Zoom(-1.0);
	}
	TestTrue(TEXT("Zoom out is bounded"), FMath::IsNearlyEqual(Orbit.Distance, Orbit.MaxDistance()));
	Orbit.Drag(FVector2D(0.0, 2000.0));
	TestTrue(TEXT("Pitch stays below vertical singularity"), Orbit.Rotation.Pitch <= 89.0);
	Orbit.SetBounds(FBoxSphereBounds(FVector(-17.0, 2.0, 0.0), FVector(1.0), 2.0), true);
	TestTrue(TEXT("New object is recentered"), Orbit.Target.Equals(FVector(-17.0, 2.0, 0.0)));
	TestTrue(TEXT("Small object near clip safety"), Orbit.Distance >= Orbit.MinDistance());
	Orbit.Clear();
	TestFalse(TEXT("No target after clearing"), Orbit.bReady);
	return !HasAnyErrors();
}

#endif
