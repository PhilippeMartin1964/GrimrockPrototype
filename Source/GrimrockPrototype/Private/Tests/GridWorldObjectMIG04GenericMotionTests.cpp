#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "UObject/UnrealType.h"

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

	void TestBehaviorPropertyRemoved(FAutomationTestBase& Test, UStruct* Struct, const TCHAR* PropertyName)
	{
		if (!Struct)
		{
			Test.AddError(FString::Printf(TEXT("Missing struct while checking removed behavior property %s."), PropertyName));
			return;
		}

		Test.TestNull(
			*FString::Printf(TEXT("%s no longer exposes legacy reflected behavior property %s"), *Struct->GetName(), PropertyName),
			Struct->FindPropertyByName(FName(PropertyName)));
	}

	void TestEditableBehaviorProperty(FAutomationTestBase& Test, UStruct* Struct, const TCHAR* PropertyName)
	{
		FProperty* Property = Struct ? Struct->FindPropertyByName(FName(PropertyName)) : nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s gameplay behavior property exists"), PropertyName), Property))
		{
			return;
		}
		Test.TestTrue(*FString::Printf(TEXT("%s remains editable"), PropertyName), Property->HasAnyPropertyFlags(CPF_Edit));
		Test.TestFalse(*FString::Printf(TEXT("%s remains serialized behavior"), PropertyName), Property->HasAnyPropertyFlags(CPF_Transient));
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
	TestPropertyRemoved(*this, AGridDoorActor::StaticClass(), TEXT("OpenHeight"));

	TestNotNull(TEXT("Button keeps runtime motion duration cache"), AGridButtonActor::StaticClass()->FindPropertyByName(TEXT("PressDuration")));
	TestNotNull(TEXT("Lever keeps runtime motion duration cache"), AGridLeverActor::StaticClass()->FindPropertyByName(TEXT("ToggleDuration")));
	TestNotNull(TEXT("Pressure plate keeps runtime motion duration cache"), AGridPressurePlateActor::StaticClass()->FindPropertyByName(TEXT("MoveDuration")));
	TestNotNull(TEXT("Pit keeps runtime motion duration cache"), AGridPitTrapdoorActor::StaticClass()->FindPropertyByName(TEXT("MoveDuration")));
	TestNotNull(TEXT("Door keeps runtime motion duration cache"), AGridDoorActor::StaticClass()->FindPropertyByName(TEXT("MoveDuration")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG04BehaviorSchemaAuthorityTest,
	"Grimrock.WorldObjects.MIG04.BehaviorSchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG04BehaviorSchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* ButtonAnimation = FGridButtonAnimationParams::StaticStruct();
	TestBehaviorPropertyRemoved(*this, ButtonAnimation, TEXT("ButtonPressDistance"));
	TestBehaviorPropertyRemoved(*this, ButtonAnimation, TEXT("ButtonPressDuration"));
	TestBehaviorPropertyRemoved(*this, ButtonAnimation, TEXT("ButtonReleaseDuration"));
	TestEditableBehaviorProperty(*this, ButtonAnimation, TEXT("ButtonHoldTime"));

	UScriptStruct* DoorAnimation = FGridDoorAnimationParams::StaticStruct();
	TestBehaviorPropertyRemoved(*this, DoorAnimation, TEXT("OpenHeight"));
	TestBehaviorPropertyRemoved(*this, DoorAnimation, TEXT("MoveDuration"));
	TestEditableBehaviorProperty(*this, DoorAnimation, TEXT("bHasChainMechanism"));
	TestEditableBehaviorProperty(*this, DoorAnimation, TEXT("ChainPullDistance"));
	TestEditableBehaviorProperty(*this, DoorAnimation, TEXT("ChainPullDuration"));

	UScriptStruct* Behavior = FGridObjectBehaviorParams::StaticStruct();
	TestBehaviorPropertyRemoved(*this, Behavior, TEXT("PitAnimation"));
	TestBehaviorPropertyRemoved(*this, Behavior, TEXT("LeverAnimation"));
	TestBehaviorPropertyRemoved(*this, Behavior, TEXT("PressurePlateAnimation"));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
