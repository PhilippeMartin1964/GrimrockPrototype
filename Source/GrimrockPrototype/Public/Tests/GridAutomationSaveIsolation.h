#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

// Only for the development Automation harness. Normal game and ordinary PIE
// never opt into this directory; no SaveGame or gameplay schema changes.
namespace GridAutomationSaveIsolation
{
	inline FString FullDirectory(const FString& Path)
	{
		FString Result = FPaths::ConvertRelativePathToFull(Path);
		FPaths::NormalizeDirectoryName(Result);
		return Result;
	}

	inline bool IsIsolated(const FString& Root, const FString& ActualSaved, const FString& ProjectDir)
	{
		if (Root.IsEmpty() || FPaths::IsRelative(Root))
		{
			return false;
		}
		const FString FullRoot = FullDirectory(Root);
		const FString ExpectedSaved = FullDirectory(FullRoot / TEXT("Saved"));
		const FString LiveSaved = FullDirectory(ProjectDir / TEXT("Saved"));
		const FString LiveSaves = LiveSaved / TEXT("SaveGames");
		return FPaths::GetCleanFilename(FullRoot).StartsWith(TEXT("GrimrockAutomation-"), ESearchCase::CaseSensitive)
			&& FullDirectory(ActualSaved).Equals(ExpectedSaved, ESearchCase::IgnoreCase)
			&& !ExpectedSaved.Equals(LiveSaved, ESearchCase::IgnoreCase)
			&& !FullRoot.Equals(LiveSaves, ESearchCase::IgnoreCase)
			&& !FullRoot.StartsWith(LiveSaves + TEXT("/"), ESearchCase::IgnoreCase);
	}

	inline bool ReadRoot(FString& Root)
	{
		return FParse::Value(FCommandLine::Get(), TEXT("GrimrockAutomationUserDir="), Root);
	}

	inline bool IsConfigured()
	{
		FString Root;
		return ReadRoot(Root) && IsIsolated(Root, FPaths::ProjectSavedDir(), FPaths::ProjectDir());
	}

	inline bool Require(FAutomationTestBase& Test)
	{
		return Test.TestTrue(TEXT("Disk Automation requires a dedicated -UserDir; use Scripts/ValidateUE.ps1"), IsConfigured());
	}

	inline void DeleteOwnedSlot(FAutomationTestBase& Test, const FString& Slot, int32 UserIndex)
	{
		if (!Require(Test) || Slot.IsEmpty())
		{
			return;
		}
		if (UGameplayStatics::DoesSaveGameExist(Slot, UserIndex))
		{
			Test.TestTrue(TEXT("Owned test slot deleted"), UGameplayStatics::DeleteGameInSlot(Slot, UserIndex));
		}
		Test.TestFalse(TEXT("Owned test slot absent after cleanup"), UGameplayStatics::DoesSaveGameExist(Slot, UserIndex));
	}
}

#endif
