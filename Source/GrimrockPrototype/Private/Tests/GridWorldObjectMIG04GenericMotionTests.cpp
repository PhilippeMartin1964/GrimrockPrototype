#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GridPressurePlateActor.h"

namespace
{
	void TestPropertyRemoved(FAutomationTestBase& Test, UClass* Class, const TCHAR* PropertyName)
	{
		if (!Class)
		{
			Test.AddError(FString::Printf(TEXT("Missing class while checking removed property %s."), PropertyName));
			return;
		}

		Test.TestNull(
			*FString::Printf(TEXT("%s no longer exposes specialized runtime geometry property %s"), *Class->GetName(), PropertyName),
			Class->FindPropertyByName(FName(PropertyName)));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG04RuntimeGenericMotionContractTest,
	"Grimrock.WorldObjects.MIG04.RuntimeGenericMotionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG04RuntimeGenericMotionContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestPropertyRemoved(*this, AGridButtonActor::StaticClass(), TEXT("PressDistance"));

	TestPropertyRemoved(*this, AGridLeverActor::StaticClass(), TEXT("LeverOffPitch"));
	TestPropertyRemoved(*this, AGridLeverActor::StaticClass(), TEXT("LeverOnPitch"));

	TestPropertyRemoved(*this, AGridPressurePlateActor::StaticClass(), TEXT("ReleasedHeightAboveFloor"));
	TestPropertyRemoved(*this, AGridPressurePlateActor::StaticClass(), TEXT("PressedHeightAboveFloor"));

	TestPropertyRemoved(*this, AGridPitTrapdoorActor::StaticClass(), TEXT("LeftHingeLocation"));
	TestPropertyRemoved(*this, AGridPitTrapdoorActor::StaticClass(), TEXT("RightHingeLocation"));
	TestPropertyRemoved(*this, AGridPitTrapdoorActor::StaticClass(), TEXT("OpenAngleDegrees"));
	TestPropertyRemoved(*this, AGridPitTrapdoorActor::StaticClass(), TEXT("LeftHingeComponent"));
	TestPropertyRemoved(*this, AGridPitTrapdoorActor::StaticClass(), TEXT("RightHingeComponent"));

	TestNotNull(TEXT("Button keeps runtime motion duration cache"), AGridButtonActor::StaticClass()->FindPropertyByName(TEXT("PressDuration")));
	TestNotNull(TEXT("Lever keeps runtime motion duration cache"), AGridLeverActor::StaticClass()->FindPropertyByName(TEXT("ToggleDuration")));
	TestNotNull(TEXT("Pressure plate keeps runtime motion duration cache"), AGridPressurePlateActor::StaticClass()->FindPropertyByName(TEXT("MoveDuration")));
	TestNotNull(TEXT("Pit keeps runtime motion duration cache"), AGridPitTrapdoorActor::StaticClass()->FindPropertyByName(TEXT("MoveDuration")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
