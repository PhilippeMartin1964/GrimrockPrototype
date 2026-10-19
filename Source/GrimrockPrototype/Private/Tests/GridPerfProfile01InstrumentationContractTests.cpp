#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridPerfProfile01InstrumentationContractTest,
    "Grimrock.Performance.Profile01.InstrumentationContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPerfProfile01InstrumentationContractTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    struct FTraceScopeContract
    {
        const TCHAR* RelativePath;
        const TCHAR* Scope;
    };

    // These timers are explicitly consumed by the manual PERF-PROFILE01
    // Unreal Insights protocol. Detect accidental removal or renaming.
    const FTraceScopeContract Contracts[] = {
        { TEXT("Source/GrimrockPrototype/Private/UI/GridEventFeedWidget.cpp"),
          TEXT("PERF_PROFILE01_EventFeed_AppendRow") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridEventFeedWidget.cpp"),
          TEXT("PERF_PROFILE01_EventFeed_RebuildAllRows") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridEventFeedWidget.cpp"),
          TEXT("PERF_PROFILE01_EventFeed_SynchronizePublication") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridEventFeedWidget.cpp"),
          TEXT("PERF_PROFILE01_EventFeed_DeferredScroll") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp"),
          TEXT("PERF_PROFILE01_HUD_NativeTick") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp"),
          TEXT("PERF_PROFILE01_HUD_RefreshFromSources") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp"),
          TEXT("PERF_PROFILE01_HUD_RefreshPartyPortraits") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp"),
          TEXT("PERF_PROFILE01_HUD_SynchronizeEventFeedLayout") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp"),
          TEXT("PERF_PROFILE01_HUD_SynchronizeActionBar") },
        { TEXT("Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp"),
          TEXT("PERF_PROFILE01_HUD_RefreshActionWidgets") },
        { TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp"),
          TEXT("PERF_PROFILE01_RPG_ProgressionPointsRead") },
        { TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp"),
          TEXT("PERF_PROFILE01_RPG_CharacterSummary") },
        { TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp"),
          TEXT("PERF_PROFILE01_RPG_PortraitViews") },
        { TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp"),
          TEXT("PERF_PROFILE01_RPG_OverloadScan") },
        { TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp"),
          TEXT("PERF_PROFILE01_RPG_CharacterOverloaded") },
        { TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawn.cpp"),
          TEXT("PERF_PROFILE01_Pawn_ProgressionPoll") },
        { TEXT("Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawnMovement.cpp"),
          TEXT("PERF_PROFILE01_Pawn_TryStartMove") }
    };

    int32 Verified = 0;
    for (const FTraceScopeContract& Contract : Contracts)
    {
        FString Source;
        const FString SourcePath = FPaths::Combine(FPaths::ProjectDir(), Contract.RelativePath);
        if (!TestTrue(*FString::Printf(TEXT("Instrumented source exists: %s"), Contract.RelativePath),
            FFileHelper::LoadFileToString(Source, *SourcePath)))
        {
            continue;
        }
        const FString Expected = FString::Printf(
            TEXT("TRACE_CPUPROFILER_EVENT_SCOPE(%s)"), Contract.Scope);
        if (TestTrue(*FString::Printf(TEXT("Expected Insights timer: %s"), Contract.Scope),
            Source.Contains(Expected, ESearchCase::CaseSensitive)))
        {
            ++Verified;
        }
    }
    TestEqual(TEXT("All PERF-PROFILE01 scopes remain wired"), Verified,
        static_cast<int32>(UE_ARRAY_COUNT(Contracts)));

    FString Script;
    const FString ScriptPath = FPaths::Combine(FPaths::ProjectDir(),
        TEXT("Scripts/CapturePerfProfile01.ps1"));
    if (TestTrue(TEXT("Local trace launcher is available"),
        FFileHelper::LoadFileToString(Script, *ScriptPath)))
    {
        TestTrue(TEXT("Launcher enables CPU, frame and bookmark channels"),
            Script.Contains(TEXT("cpu,frame,bookmark,log")));
        TestTrue(TEXT("Launcher requests a tracefile, not a remote trace server"),
            Script.Contains(TEXT("-tracefile=")));
        TestTrue(TEXT("Launcher has a non-launching preflight path"),
            Script.Contains(TEXT("PreflightOnly")));
    }

    AddInfo(TEXT("This verifies instrumentation wiring, NOT CPU performance or valid .utrace output; capture and inspect on the user's UE5.5.4 machine."));
    return Verified == UE_ARRAY_COUNT(Contracts);
}

#endif // WITH_DEV_AUTOMATION_TESTS
