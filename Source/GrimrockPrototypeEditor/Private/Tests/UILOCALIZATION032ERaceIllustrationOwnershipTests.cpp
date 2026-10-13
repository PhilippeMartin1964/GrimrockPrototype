#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterTypes.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUILOCALIZATION032ERaceIllustrationOwnershipTest,
	"Game.UI.Localization.LOCALIZATION03.RaceIllustrationOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUILOCALIZATION032ERaceIllustrationOwnershipTest::RunTest(const FString&)
{
	UScriptStruct* Struct = FRPGRaceIllustrationOption::StaticStruct();
	if (!TestNotNull(TEXT("Race illustration option struct is reflected"), Struct))
	{
		return false;
	}

	for (const TCHAR* RequiredProperty : {
		TEXT("RaceId"),
		TEXT("Gender"),
		TEXT("Illustration"),
		TEXT("RaceEnvironment")
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("Race illustration keeps required visual property '%s'"), RequiredProperty),
			FindFProperty<FProperty>(Struct, RequiredProperty));
	}

	TestNull(
		TEXT("Race illustration no longer owns duplicated authored DisplayName"),
		FindFProperty<FProperty>(Struct, TEXT("DisplayName")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
