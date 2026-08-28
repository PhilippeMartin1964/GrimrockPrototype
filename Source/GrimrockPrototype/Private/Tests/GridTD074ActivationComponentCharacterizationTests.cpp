#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"

namespace GridTD074ActivationCharacterization
{
	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	int32 CountOccurrences(const FString& Text, const TCHAR* Needle)
	{
		if (!Needle || !*Needle)
		{
			return 0;
		}

		int32 Count = 0;
		int32 SearchFrom = 0;
		const FString Token(Needle);
		while (true)
		{
			const int32 FoundAt = Text.Find(Token, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchFrom);
			if (FoundAt == INDEX_NONE)
			{
				break;
			}
			++Count;
			SearchFrom = FoundAt + Token.Len();
		}
		return Count;
	}

	int32 CountLines(const FString& Text)
	{
		if (Text.IsEmpty())
		{
			return 0;
		}

		int32 Count = 1;
		for (const TCHAR Character : Text)
		{
			if (Character == TEXT('\n'))
			{
				++Count;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD074ActivationSurfaceCharacterizationTest, "Grimrock.TechnicalDebt.TD07_4.Characterization.SurfaceMetrics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD074ActivationSurfaceCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD074ActivationCharacterization;

	FString Header;
	FString Source;
	TestTrue(TEXT("GridActivationComponent header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h"), Header));
	TestTrue(
		TEXT("GridActivationComponent source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp"), Source));

	const int32 HeaderLines = CountLines(Header);
	const int32 SourceLines = CountLines(Source);
	const int32 ScopedMethodOccurrences = CountOccurrences(Source, TEXT("UGridActivationComponent::"));
	const int32 LogCalls = CountOccurrences(Source, TEXT("UE_LOG("));
	const int32 LogTempCalls = CountOccurrences(Source, TEXT("UE_LOG(LogTemp"));
	const int32 LogGridActivationCalls = CountOccurrences(Source, TEXT("UE_LOG(LogGridActivation"));

	AddInfo(FString::Printf(
		TEXT("TD07.4 surface: header=%d/%d source=%d/%d methods=%d UE_LOG=%d LogTemp=%d LogGridActivation=%d"),
		HeaderLines, Header.Len(), SourceLines, Source.Len(), ScopedMethodOccurrences, LogCalls, LogTempCalls, LogGridActivationCalls));

	TestTrue(TEXT("Activation component is currently a concentrated source file"), SourceLines >= 1500);
	TestTrue(TEXT("Activation component has at least forty scoped implementations"), ScopedMethodOccurrences >= 40);
	TestEqual(TEXT("Activation uses no LogTemp"), LogTempCalls, 0);
	TestEqual(TEXT("Activation uses 38 LogGridActivation calls"), LogGridActivationCalls, 38);
	TestTrue(TEXT("LogGridActivation category is declared"), Source.Contains(TEXT("DEFINE_LOG_CATEGORY_STATIC(LogGridActivation, Log, All);")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD074ActivationResponsibilitiesCharacterizationTest, "Grimrock.TechnicalDebt.TD07_4.Characterization.ResponsibilityMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD074ActivationResponsibilitiesCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD074ActivationCharacterization;

	FString Source;
	TestTrue(
		TEXT("GridActivationComponent source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp"), Source));

	for (const TCHAR* RequiredSymbol : { TEXT("TryInteractAtEdge"), TEXT("RefreshPressurePlatesAtCell"), TEXT("ProcessTriggersAtCell"),
			 TEXT("ExecuteLinksFromObjectForEventInternal"), TEXT("ApplyLinkCommand"), TEXT("EvaluateGridObjectLinkCondition"), TEXT("ApplyQuestLinkCommand"),
			 TEXT("ReloadLuaRuntime"), TEXT("ExecuteLuaCallbackLink"), TEXT("ExecuteLuaIssuedCommand"), TEXT("IsStoryCompanionAlreadyActive"),
			 TEXT("ActivateReadableObject"), TEXT("ActivateReceptacle"), TEXT("RebuildIndexes"), TEXT("GetDebugSummary") })
	{
		TestTrue(*FString::Printf(TEXT("Responsibility symbol %s remains in ActivationComponent"), RequiredSymbol), Source.Contains(RequiredSymbol));
	}

	AddInfo(TEXT(
		"TD07.4 responsibility families: interaction; plates/triggers; Event->Command; conditions; command adapters; Quest; Lua; recruitment; readable/receptacle; indexing/debug."));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD074ActivationAuthorityCharacterizationTest, "Grimrock.TechnicalDebt.TD07_4.Characterization.AuthorityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD074ActivationAuthorityCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD074ActivationCharacterization;

	TestTrue(TEXT("Activation remains an ActorComponent"), UGridActivationComponent::StaticClass()->IsChildOf(UActorComponent::StaticClass()));

	FString RuntimeHeader;
	FString RuntimeSource;
	TestTrue(TEXT("RuntimeActor header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h"), RuntimeHeader));
	TestTrue(TEXT("RuntimeActor source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp"), RuntimeSource));

	TestEqual(TEXT("RuntimeActor declares one ActivationComponent member"),
		CountOccurrences(RuntimeHeader, TEXT("TObjectPtr<UGridActivationComponent> ActivationComponent")), 1);
	TestEqual(TEXT("RuntimeActor constructs exactly one UGridActivationComponent"),
		CountOccurrences(RuntimeSource, TEXT("CreateDefaultSubobject<UGridActivationComponent>")), 1);

	FString ActivationHeader;
	TestTrue(TEXT("Activation header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h"), ActivationHeader));
	TestTrue(TEXT("Event->Command dispatch remains owned by ActivationComponent"),
		ActivationHeader.Contains(TEXT("bool ApplyLinkCommand(const FGridObjectLink& LinkData);")));
	TestTrue(TEXT("Active object runtime state remains owned by ActivationComponent"), ActivationHeader.Contains(TEXT("TSet<FGuid> ActiveObjectIds;")));
	TestTrue(TEXT("Dispatch recursion/budget guard remains owned by ActivationComponent"), ActivationHeader.Contains(TEXT("RuntimeActionBudgetRemaining")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD074ActivationCoverageCharacterizationTest, "Grimrock.TechnicalDebt.TD07_4.Characterization.RegressionCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD074ActivationCoverageCharacterizationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	for (const TCHAR* RelativePath : { TEXT("Source/GrimrockPrototype/Private/Tests/GridTD0132EventCommandRuntimeHardeningTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/GridMON192LogicPrimitiveTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/GridMON1924VariableLinkConditionTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/GridMON194LuaBridgeTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/GridMON1971LuaLogicIdTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/GridMON198ProductionPuzzleTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/GridMonsterMON13SpawnTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/RPGStoryCompanionMON204EventCommandTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/RPGStoryCompanionMON204OfferSuppressionTests.cpp"),
			 TEXT("Source/GrimrockPrototype/Private/Tests/RPGCustomRecruitMON205EventCommandTests.cpp") })
	{
		TestTrue(*FString::Printf(TEXT("Regression coverage file exists: %s"), RelativePath),
			FPaths::FileExists(FPaths::Combine(FPaths::ProjectDir(), RelativePath)));
	}

	AddInfo(TEXT("TD07.4 found established regression coverage around Event->Command, Logic, Lua, monsters, story recruitment and custom recruitment."));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
