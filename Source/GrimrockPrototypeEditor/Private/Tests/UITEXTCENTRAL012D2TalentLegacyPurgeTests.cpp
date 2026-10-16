#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

namespace UITEXTCENTRAL012D2Purge
{
	bool HasForbiddenToken(
		const FString& Filename,
		const TArray<FString>& ForbiddenTokens,
		FString& OutToken)
	{
		FString Source;
		if (!FFileHelper::LoadFileToString(Source, *Filename))
		{
			return false;
		}

		for (const FString& Token : ForbiddenTokens)
		{
			if (Source.Contains(Token, ESearchCase::CaseSensitive))
			{
				OutToken = Token;
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUITEXTCENTRAL012D2TalentLegacyPurgeTest,
	"Game.UI.TextCentral.CENTRAL01.TalentLegacyPurge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITEXTCENTRAL012D2TalentLegacyPurgeTest::RunTest(const FString&)
{
	using namespace UITEXTCENTRAL012D2Purge;

	const FString LegacyPackageName =
		FString(TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/")) +
		TEXT("DA_RPGTalent") +
		TEXT("DetailTextSet");
	TestFalse(
		TEXT("Legacy Talent text package is physically absent"),
		FPackageName::DoesPackageExist(LegacyPackageName));

	const FString PublicUiRoot =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Public/UI"));
	const FString PrivateUiRoot =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/UI"));
	const TArray<FString> RemovedSourceFiles = {
		FPaths::Combine(PublicUiRoot, FString(TEXT("RPGTalent")) + TEXT("DetailTextSet.h")),
		FPaths::Combine(PrivateUiRoot, FString(TEXT("RPGTalent")) + TEXT("DetailTextSet.cpp")),
		FPaths::Combine(PublicUiRoot, FString(TEXT("RPGTalent")) + TEXT("LocalizationText.h")),
		FPaths::Combine(PrivateUiRoot, FString(TEXT("RPGTalent")) + TEXT("LocalizationText.cpp"))
	};
	for (const FString& Filename : RemovedSourceFiles)
	{
		TestFalse(
			*FString::Printf(TEXT("Removed legacy source stays absent: %s"), *Filename),
			IFileManager::Get().FileExists(*Filename));
	}

	const TArray<FString> ForbiddenTokens = {
		FString(TEXT("RPGTalent")) + TEXT("DetailTextSet"),
		FString(TEXT("RPGTalent")) + TEXT("LocalizationText"),
		FString(TEXT("GrantedRecipe")) + TEXT("TextIds"),
		FString(TEXT("Talent")) + TEXT(".Recipe.")
	};

	TArray<FString> SourceFiles;
	IFileManager::Get().FindFilesRecursive(
		SourceFiles,
		*FPaths::Combine(FPaths::ProjectDir(), TEXT("Source")),
		TEXT("*.cpp"),
		true,
		false);
	TArray<FString> HeaderFiles;
	IFileManager::Get().FindFilesRecursive(
		HeaderFiles,
		*FPaths::Combine(FPaths::ProjectDir(), TEXT("Source")),
		TEXT("*.h"),
		true,
		false);
	SourceFiles.Append(HeaderFiles);

	for (const FString& Filename : SourceFiles)
	{
		FString FoundToken;
		if (HasForbiddenToken(Filename, ForbiddenTokens, FoundToken))
		{
			AddError(FString::Printf(
				TEXT("Legacy token '%s' remains in source file '%s'."),
				*FoundToken,
				*Filename));
		}
	}

	return true;
}

#endif
