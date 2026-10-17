#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "UI/GridEventFeedWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD01ReadableHeaderPolishTest, "Grimrock.UI.EventHUD01.PresentationPolish.ReadableHeader",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD01ReadableHeaderPolishTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>();
	Definition->DefinitionId = FName(TEXT("WallInscription"));
	Definition->DisplayName = FText::FromString(TEXT("Wall Inscription"));

	TestEqual(TEXT("WallInscription overrides the authored English display name"),
		GridEventFeedPublisher::ResolveReadableHeader(Definition).ToString(),
		FString(TEXT("Écriture murale :")));

	Definition->DefinitionId = FName(TEXT("ReadablePlaque"));
	Definition->DisplayName = FText::FromString(TEXT("Plaque commémorative"));
	TestEqual(TEXT("Other readable definitions preserve their authored display name"),
		GridEventFeedPublisher::ResolveReadableHeader(Definition).ToString(),
		FString(TEXT("Plaque commémorative")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD01StyleDefaultsPolishTest, "Grimrock.UI.EventHUD01.PresentationPolish.StyleDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD01StyleDefaultsPolishTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridEventFeedWidget* Widget = NewObject<UGridEventFeedWidget>();
	if (!TestNotNull(TEXT("Event Feed widget exists"), Widget))
	{
		return false;
	}

	TestEqual(TEXT("Default shell opacity is more legible"), Widget->ShellBackgroundColor.A, 0.92f);
	TestEqual(TEXT("Default event text font size is explicit"), Widget->EntryFont.Size, 16.0f);
	TestEqual(TEXT("Default filter font size is explicit"), Widget->FilterFont.Size, 14.0f);
	TestTrue(TEXT("Event text has an explicit font object"), Widget->EntryFont.FontObject != nullptr);
	TestTrue(TEXT("Filter text has an explicit font object"), Widget->FilterFont.FontObject != nullptr);

	return true;
}

#endif
