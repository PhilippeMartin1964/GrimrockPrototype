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

	void TestTransientBridgeProperty(FAutomationTestBase& Test, UStruct* Struct, const TCHAR* PropertyName)
	{
		if (!Struct)
		{
			Test.AddError(FString::Printf(TEXT("Missing struct while checking transient bridge property %s."), PropertyName));
			return;
		}

		FProperty* Property = Struct->FindPropertyByName(FName(PropertyName));
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s bridge property exists until MIG09"), PropertyName), Property))
		{
			return;
		}
		Test.TestTrue(*FString::Printf(TEXT("%s is transient"), PropertyName), Property->HasAnyPropertyFlags(CPF_Transient));
		Test.TestFalse(*FString::Printf(TEXT("%s is not editable authoring data"), PropertyName), Property->HasAnyPropertyFlags(CPF_Edit));
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

	TestNotNull(TEXT("Button keeps runtime motion duration cache"), AGridButtonActor::StaticClass()->FindPropertyByName(TEXT("PressDuration")));
	TestNotNull(TEXT("Lever keeps runtime motion duration cache"), AGridLeverActor::StaticClass()->FindPropertyByName(TEXT("ToggleDuration")));
	TestNotNull(TEXT("Pressure plate keeps runtime motion duration cache"), AGridPressurePlateActor::StaticClass()->FindPropertyByName(TEXT("MoveDuration")));
	TestNotNull(TEXT("Pit keeps runtime motion duration cache"), AGridPitTrapdoorActor::StaticClass()->FindPropertyByName(TEXT("MoveDuration")));

	FProperty* DoorOpenHeight = AGridDoorActor::StaticClass()->FindPropertyByName(TEXT("OpenHeight"));
	TestNotNull(TEXT("Door retains only a transient MIG09 compatibility OpenHeight cache"), DoorOpenHeight);
	if (DoorOpenHeight)
	{
		TestTrue(TEXT("Door OpenHeight cache is transient"), DoorOpenHeight->HasAnyPropertyFlags(CPF_Transient));
		TestFalse(TEXT("Door OpenHeight cache is not editable"), DoorOpenHeight->HasAnyPropertyFlags(CPF_Edit));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridWorldObjectMIG04BehaviorSchemaAuthorityTest,
	"Grimrock.WorldObjects.MIG04.BehaviorSchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWorldObjectMIG04BehaviorSchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* PitAnimation = FGridPitAnimationParams::StaticStruct();
	TestTransientBridgeProperty(*this, PitAnimation, TEXT("LeftHingeLocation"));
	TestTransientBridgeProperty(*this, PitAnimation, TEXT("RightHingeLocation"));
	TestTransientBridgeProperty(*this, PitAnimation, TEXT("OpenAngleDegrees"));
	TestTransientBridgeProperty(*this, PitAnimation, TEXT("MoveDuration"));

	UScriptStruct* LeverAnimation = FGridLeverAnimationParams::StaticStruct();
	TestTransientBridgeProperty(*this, LeverAnimation, TEXT("LeverOffPitch"));
	TestTransientBridgeProperty(*this, LeverAnimation, TEXT("LeverOnPitch"));
	TestTransientBridgeProperty(*this, LeverAnimation, TEXT("ToggleDuration"));

	UScriptStruct* PressurePlateAnimation = FGridPressurePlateAnimationParams::StaticStruct();
	TestTransientBridgeProperty(*this, PressurePlateAnimation, TEXT("ReleasedHeightAboveFloor"));
	TestTransientBridgeProperty(*this, PressurePlateAnimation, TEXT("PressedHeightAboveFloor"));
	TestTransientBridgeProperty(*this, PressurePlateAnimation, TEXT("MoveDuration"));

	UScriptStruct* ButtonAnimation = FGridButtonAnimationParams::StaticStruct();
	TestTransientBridgeProperty(*this, ButtonAnimation, TEXT("ButtonPressDistance"));
	TestTransientBridgeProperty(*this, ButtonAnimation, TEXT("ButtonPressDuration"));
	TestTransientBridgeProperty(*this, ButtonAnimation, TEXT("ButtonReleaseDuration"));
	TestEditableBehaviorProperty(*this, ButtonAnimation, TEXT("ButtonHoldTime"));

	UScriptStruct* DoorAnimation = FGridDoorAnimationParams::StaticStruct();
	TestTransientBridgeProperty(*this, DoorAnimation, TEXT("OpenHeight"));
	TestTransientBridgeProperty(*this, DoorAnimation, TEXT("MoveDuration"));
	TestEditableBehaviorProperty(*this, DoorAnimation, TEXT("bHasChainMechanism"));
	TestEditableBehaviorProperty(*this, DoorAnimation, TEXT("ChainPullDistance"));
	TestEditableBehaviorProperty(*this, DoorAnimation, TEXT("ChainPullDuration"));

	UScriptStruct* Behavior = FGridObjectBehaviorParams::StaticStruct();
	TestTransientBridgeProperty(*this, Behavior, TEXT("PitAnimation"));
	TestTransientBridgeProperty(*this, Behavior, TEXT("LeverAnimation"));
	TestTransientBridgeProperty(*this, Behavior, TEXT("PressurePlateAnimation"));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
