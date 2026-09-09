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
	FString LinksSource;
	FString LuaSource;
	TestTrue(TEXT("GridActivationComponent header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h"), Header));
	TestTrue(
		TEXT("GridActivationComponent source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp"), Source));
	TestTrue(TEXT("Activation Links source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponentLinks.cpp"), LinksSource));
	TestTrue(TEXT("Activation Lua source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponentLua.cpp"), LuaSource));

	const int32 HeaderLines = CountLines(Header);
	const int32 SourceLines = CountLines(Source);
	const int32 ScopedMethodOccurrences = CountOccurrences(Source, TEXT("UGridActivationComponent::"));
	const int32 LogCalls = CountOccurrences(Source, TEXT("UE_LOG("));
	const int32 LogTempCalls = CountOccurrences(Source, TEXT("UE_LOG(LogTemp"));
	const int32 LogGridActivationCalls = CountOccurrences(Source, TEXT("UE_LOG(LogGridActivation"));

	AddInfo(FString::Printf(TEXT("TD07.4 surface: header=%d/%d source=%d/%d methods=%d UE_LOG=%d LogTemp=%d LogGridActivation=%d"), HeaderLines, Header.Len(),
		SourceLines, Source.Len(), ScopedMethodOccurrences, LogCalls, LogTempCalls, LogGridActivationCalls));

	// MIG09 split the implementation into independent compilation units. Line/log counts are
	// observations, not a requirement to preserve the former monolithic implementation.
	for (const FString* Unit : { &Source, &LinksSource, &LuaSource })
	{
		TestTrue(TEXT("Each activation unit includes its own component declaration"), Unit->Contains(TEXT("#include \"Runtime/GridActivationComponent.h\"")));
		TestTrue(TEXT("Each activation unit owns its log category"), Unit->Contains(TEXT("DEFINE_LOG_CATEGORY_STATIC(LogGridActivation, Log, All);")));
		TestEqual(TEXT("Each activation unit uses no LogTemp"), CountOccurrences(*Unit, TEXT("UE_LOG(LogTemp")), 0);
	}
	FString BuildRules;
	TestTrue(TEXT("Runtime build rules load"), LoadProjectFile(TEXT("Source/GrimrockPrototype/GrimrockPrototype.Build.cs"), BuildRules));
	TestTrue(TEXT("Runtime compilation remains non-unity"), BuildRules.Contains(TEXT("bUseUnity = false;")));
	for (const TCHAR* Collection : { TEXT("WorldObjectInstances"), TEXT("LooseItemInstances"), TEXT("MonsterSpawns"), TEXT("ItemSpawns"), TEXT("LogicObjects") })
	{
		TestTrue(*FString::Printf(TEXT("Activation indexes native collection %s"), Collection), Source.Contains(Collection));
	}
	AddInfo(FString::Printf(TEXT("Split units: core=%d lines links=%d lines lua=%d lines"), SourceLines, CountLines(LinksSource), CountLines(LuaSource)));
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
	FString LinksSource;
	FString LuaSource;
	TestTrue(TEXT("Activation Links source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponentLinks.cpp"), LinksSource));
	TestTrue(TEXT("Activation Lua source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridActivationComponentLua.cpp"), LuaSource));
	const FString AllSources = Source + LinksSource + LuaSource;
	// Preserve every historical responsibility, checking its current owner and unique definition.
	const auto CheckResponsibility = [this, &AllSources](const FString& Owner, const TCHAR* Symbol)
	{
		const FString Definition = FString::Printf(TEXT("UGridActivationComponent::%s("), Symbol);
		TestTrue(*FString::Printf(TEXT("Responsibility %s is implemented by its owning unit"), Symbol), Owner.Contains(Definition));
		TestEqual(*FString::Printf(TEXT("Responsibility %s has exactly one implementation"), Symbol), CountOccurrences(AllSources, *Definition), 1);
	};

	for (const TCHAR* RequiredSymbol : { TEXT("TryInteractAtEdge"), TEXT("RefreshPressurePlatesAtCell"), TEXT("ProcessTriggersAtCell"),
			 TEXT("ActivateReadableObject"), TEXT("ActivateReceptacle"), TEXT("RebuildIndexes"), TEXT("GetDebugSummary") })
	{
		CheckResponsibility(Source, RequiredSymbol);
	}
	for (const TCHAR* RequiredSymbol : { TEXT("ExecuteLinksFromObjectForEventInternal"), TEXT("ApplyLinkCommand"),
		TEXT("EvaluateGridObjectLinkCondition"), TEXT("ApplyQuestLinkCommand"), TEXT("IsStoryCompanionAlreadyActive") })
	{
		CheckResponsibility(LinksSource, RequiredSymbol);
	}
	for (const TCHAR* RequiredSymbol : { TEXT("ReloadLuaRuntime"), TEXT("ExecuteLuaCallbackLink"), TEXT("ExecuteLuaIssuedCommand") })
	{
		CheckResponsibility(LuaSource, RequiredSymbol);
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
