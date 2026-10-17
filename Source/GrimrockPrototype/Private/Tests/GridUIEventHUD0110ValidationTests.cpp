#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Runtime/GridEventFeedSubsystem.h"
#include "UI/GridEventFeedWidget.h"
#include "UI/GridPersistentHudWidget.h"

namespace
{
	struct FGridEventHUD0110WorldScope
	{
		UWorld* World = nullptr;

		FGridEventHUD0110WorldScope(const TCHAR* Prefix)
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("%s_%s"), Prefix, *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);

			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridEventHUD0110WorldScope()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
			}
		}
	};

	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(
			OutText,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	bool SaveClassContainsEventFeedState()
	{
		for (TFieldIterator<FProperty> It(UGrimrockPartySaveGame::StaticClass()); It; ++It)
		{
			const FProperty* Property = *It;
			if (!Property)
			{
				continue;
			}

			if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
			{
				if (StructProperty->Struct == FGridHudEventEntry::StaticStruct())
				{
					return true;
				}
			}
			if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
			{
				if (const FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProperty->Inner))
				{
					if (InnerStruct->Struct == FGridHudEventEntry::StaticStruct())
					{
						return true;
					}
				}
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD0110StressBufferTest, "Grimrock.UI.EventHUD01.Validation.StressBuffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0110StressBufferTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridEventFeedBuffer Buffer;
	constexpr int32 PublishCount = 10000;

	for (int32 Index = 1; Index <= PublishCount; ++Index)
	{
		FGridHudEventEntry Entry;
		Entry.Category = static_cast<EGridHudEventCategory>((Index - 1) % 3);
		Entry.Semantic = Index % 17 == 0 ? EGridHudEventSemantic::Fire : EGridHudEventSemantic::Information;
		Entry.Visibility = EGridEventVisibility::Always;
		Entry.EventCode = FName(*FString::Printf(TEXT("Stress.%05d"), Index));
		Entry.Message = FText::FromString(FString::Printf(TEXT("Stress event %d"), Index));

		if (Index % 10 == 0)
		{
			for (int32 DetailIndex = 0; DetailIndex < 32; ++DetailIndex)
			{
				Entry.Details.Add(FText::FromString(FString::Printf(TEXT("Detail %d"), DetailIndex)));
			}
		}

		if (!Buffer.TryPublish(Entry, true))
		{
			AddError(FString::Printf(TEXT("Stress event %d was unexpectedly rejected."), Index));
			return false;
		}
	}

	const TArray<FGridHudEventEntry>& Entries = Buffer.GetEntries();
	TestEqual(TEXT("Stress run remains capped at 200 logical events"), Entries.Num(), 200);
	if (Entries.Num() == 200)
	{
		TestEqual(TEXT("Stress FIFO evicts exactly the oldest 9800 events"), Entries[0].SequenceId, int64(9801));
		TestEqual(TEXT("Stress FIFO retains the newest sequence"), Entries.Last().SequenceId, int64(PublishCount));
		TestEqual(TEXT("Oldest retained event code matches sequence"), Entries[0].EventCode, FName(TEXT("Stress.09801")));
		TestEqual(TEXT("Newest retained event code matches sequence"), Entries.Last().EventCode, FName(TEXT("Stress.10000")));
		TestEqual(TEXT("Multi-line combat-style details still belong to one logical event"), Entries.Last().Details.Num(), 32);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD0110FilterStressTest, "Grimrock.UI.EventHUD01.Validation.FilterStress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0110FilterStressTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FGridHudEventEntry> Source;
	Source.Reserve(600);
	for (int32 Index = 0; Index < 600; ++Index)
	{
		FGridHudEventEntry Entry;
		Entry.SequenceId = Index + 1;
		Entry.Category = static_cast<EGridHudEventCategory>(Index % 3);
		Entry.Message = FText::FromString(FString::Printf(TEXT("Entry %d"), Index + 1));
		Source.Add(Entry);
	}

	TArray<FGridHudEventEntry> Filtered;
	UGridEventFeedWidget::FilterEntries(Source, true, false, true, Filtered);

	TestEqual(TEXT("Filtering never mutates the 600-entry source projection"), Source.Num(), 600);
	TestEqual(TEXT("Hiding Combat keeps the other two thirds"), Filtered.Num(), 400);
	for (int32 Index = 1; Index < Filtered.Num(); ++Index)
	{
		if (!TestTrue(TEXT("Filtered projection preserves chronological order"), Filtered[Index - 1].SequenceId < Filtered[Index].SequenceId))
		{
			break;
		}
	}
	TestTrue(TEXT("No Combat entry leaks through disabled filter"),
		!Filtered.ContainsByPredicate([](const FGridHudEventEntry& Entry)
		{
			return Entry.Category == EGridHudEventCategory::Combat;
		}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD0110WorldResetTest, "Grimrock.UI.EventHUD01.Validation.WorldReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0110WorldResetTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridEventHUD0110WorldScope FirstWorld(TEXT("EVENTHUD0110_A"));
	FGridEventHUD0110WorldScope SecondWorld(TEXT("EVENTHUD0110_B"));
	if (!TestNotNull(TEXT("First world exists"), FirstWorld.World) ||
		!TestNotNull(TEXT("Second world exists"), SecondWorld.World))
	{
		return false;
	}

	UGridEventFeedSubsystem* FirstFeed = FirstWorld.World->GetSubsystem<UGridEventFeedSubsystem>();
	UGridEventFeedSubsystem* SecondFeed = SecondWorld.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("First world has Event Feed subsystem"), FirstFeed) ||
		!TestNotNull(TEXT("Second world has Event Feed subsystem"), SecondFeed))
	{
		return false;
	}

	FGridHudEventEntry Entry;
	Entry.Visibility = EGridEventVisibility::Always;
	Entry.EventCode = TEXT("WorldA.Event");
	Entry.Message = FText::FromString(TEXT("World A"));
	TestTrue(TEXT("First world accepts its event"), FirstFeed->PublishEvent(Entry, true));
	TestEqual(TEXT("First world contains one event"), FirstFeed->GetEntries().Num(), 1);
	TestEqual(TEXT("Second world starts with an empty Event Feed"), SecondFeed->GetEntries().Num(), 0);

	Entry.EventCode = TEXT("WorldB.Event");
	Entry.Message = FText::FromString(TEXT("World B"));
	TestTrue(TEXT("Second world independently accepts its first event"), SecondFeed->PublishEvent(Entry, true));
	TestEqual(TEXT("Second world first sequence restarts at one"), SecondFeed->GetEntries()[0].SequenceId, int64(1));
	TestEqual(TEXT("First world history remains isolated"), FirstFeed->GetEntries().Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD0110SecretVisibilityTest, "Grimrock.UI.EventHUD01.Validation.SecretVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0110SecretVisibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridEventHUD0110WorldScope Scope(TEXT("EVENTHUD0110_SECRET"));
	if (!TestNotNull(TEXT("World exists"), Scope.World))
	{
		return false;
	}
	UGridEventFeedSubsystem* Feed = Scope.World->GetSubsystem<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}

	FGridHudEventEntry Secret;
	Secret.Category = EGridHudEventCategory::Dungeon;
	Secret.Semantic = EGridHudEventSemantic::Secret;
	Secret.EventCode = TEXT("Dungeon.Secret.Internal");
	Secret.Message = FText::FromString(TEXT("Internal secret"));

	Secret.Visibility = EGridEventVisibility::Hidden;
	TestFalse(TEXT("Hidden secret never enters the player feed"), Feed->PublishEvent(Secret, true));

	Secret.Visibility = EGridEventVisibility::Observable;
	Secret.EventCode = TEXT("Dungeon.Secret.Remote");
	TestFalse(TEXT("Unobserved secret effect does not spoil the player"), Feed->PublishEvent(Secret, false));

	Secret.EventCode = TEXT("Dungeon.SecretDoor.Discovered");
	Secret.Message = FText::FromString(TEXT("Passage secret découvert."));
	TestTrue(TEXT("Observed discovered secret is player-facing"), Feed->PublishEvent(Secret, true));

	TestEqual(TEXT("Only the legitimately observed secret is retained"), Feed->GetEntries().Num(), 1);
	if (Feed->GetEntries().Num() == 1)
	{
		TestEqual(TEXT("Retained secret is the discovery event"), Feed->GetEntries()[0].EventCode, FName(TEXT("Dungeon.SecretDoor.Discovered")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD0110ProductionWbpTest, "Grimrock.UI.EventHUD01.Validation.ProductionWBP",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0110ProductionWbpTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* FeedClass = LoadClass<UGridEventFeedWidget>(
		nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/EventFeed/WBP_GridEventFeed.WBP_GridEventFeed_C"));
	UClass* PersistentHudClass = LoadClass<UGridPersistentHudWidget>(
		nullptr,
		TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_GridPersistentHud.WBP_GridPersistentHud_C"));

	if (!TestNotNull(TEXT("Production WBP_GridEventFeed loads"), FeedClass) ||
		!TestNotNull(TEXT("Production WBP_GridPersistentHud loads"), PersistentHudClass))
	{
		return false;
	}

	TestTrue(TEXT("Production Event Feed derives from UGridEventFeedWidget"),
		FeedClass->IsChildOf(UGridEventFeedWidget::StaticClass()));
	TestNotEqual(TEXT("Production Event Feed is not the native base class"), FeedClass, UGridEventFeedWidget::StaticClass());
	TestNotNull(TEXT("Production Event Feed is an authored generated class"), FeedClass->ClassGeneratedBy);

	const UGridPersistentHudWidget* PersistentDefaults =
		Cast<UGridPersistentHudWidget>(PersistentHudClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Persistent HUD defaults exist"), PersistentDefaults))
	{
		return false;
	}
	TestEqual(
		TEXT("Production Persistent HUD is wired to WBP_GridEventFeed"),
		PersistentDefaults->EventFeedWidgetClass.Get(),
		FeedClass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD0110PersistenceBoundaryTest, "Grimrock.UI.EventHUD01.Validation.PersistenceBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0110PersistenceBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestFalse(TEXT("Party SaveGame contains no direct Event Feed entry state"), SaveClassContainsEventFeedState());

	for (TFieldIterator<FProperty> It(FGridHudEventEntry::StaticStruct()); It; ++It)
	{
		const FProperty* Property = *It;
		if (Property)
		{
			TestFalse(
				*FString::Printf(TEXT("Event Feed field '%s' is not SaveGame"), *Property->GetName()),
				Property->HasAnyPropertyFlags(CPF_SaveGame));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD0110InputArchitectureTest, "Grimrock.UI.EventHUD01.Validation.InputArchitecture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD0110InputArchitectureTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FString WidgetSource;
	FString PersistentHudSource;
	FString SubsystemSource;
	FString PublisherSource;
	TestTrue(TEXT("Event Feed widget source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/UI/GridEventFeedWidget.cpp"), WidgetSource));
	TestTrue(TEXT("Persistent HUD source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/UI/GridPersistentHudWidget.cpp"), PersistentHudSource));
	TestTrue(TEXT("Event Feed subsystem source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridEventFeedSubsystem.cpp"), SubsystemSource));
	TestTrue(TEXT("Event Feed publisher source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridEventFeedPublisher.cpp"), PublisherSource));

	TestFalse(TEXT("Event Feed widget never changes the gameplay input mode"), WidgetSource.Contains(TEXT("SetInputMode(")));
	TestFalse(TEXT("Persistent HUD Event Feed integration never changes the gameplay input mode"), PersistentHudSource.Contains(TEXT("SetInputMode(")));
	TestFalse(TEXT("Event Feed widget never owns bShowMouseCursor"), WidgetSource.Contains(TEXT("bShowMouseCursor")));
	TestTrue(TEXT("Event Feed shell passes through non-control hit tests"),
		WidgetSource.Contains(TEXT("SetVisibility(ESlateVisibility::SelfHitTestInvisible)")));
	TestTrue(TEXT("Generated event rows never intercept world clicks"),
		WidgetSource.Contains(TEXT("Row->SetVisibility(ESlateVisibility::HitTestInvisible)")));

	TestFalse(TEXT("Event Feed authority does not write to Journal"), SubsystemSource.Contains(TEXT("Journal")));
	TestFalse(TEXT("Event Feed publisher does not write to Journal"), PublisherSource.Contains(TEXT("Journal")));

	TArray<FString> RuntimeFiles;
	IFileManager::Get().FindFilesRecursive(
		RuntimeFiles,
		*FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/Runtime")),
		TEXT("*.cpp"),
		true,
		false,
		false);
	IFileManager::Get().FindFilesRecursive(
		RuntimeFiles,
		*FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Public/Runtime")),
		TEXT("*.h"),
		true,
		false,
		false);

	bool bPresentationCouplingFound = false;
	for (const FString& Filename : RuntimeFiles)
	{
		FString Source;
		if (!FFileHelper::LoadFileToString(Source, *Filename))
		{
			AddError(FString::Printf(TEXT("Unable to read runtime source '%s'."), *Filename));
			bPresentationCouplingFound = true;
			continue;
		}
		if (Source.Contains(TEXT("WBP_GridEventFeed")) ||
			Source.Contains(TEXT("#include \"UI/GridEventFeedWidget.h\"")))
		{
			AddError(FString::Printf(
				TEXT("Runtime business code directly depends on Event Feed presentation: %s"),
				*Filename));
			bPresentationCouplingFound = true;
		}
	}

	TestFalse(TEXT("Runtime business code has no direct Event Feed presentation dependency"), bPresentationCouplingFound);
	return !bPresentationCouplingFound;
}

#endif
