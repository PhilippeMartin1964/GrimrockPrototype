#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "UI/RPGTalentPresentationAsset.h"

namespace UIRPGTalentCodeAudit025
{
	const TCHAR* CanonicalPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation");

	bool LoadProjectSource(const TCHAR* RelativePath, FString& OutSource)
	{
		return FFileHelper::LoadFileToString(
			OutSource,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit025CanonicalResolverTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentPresentationDependency.CanonicalResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit025CanonicalResolverTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit025;

	TestEqual(
		TEXT("Talent presentation canonical object path is centralized"),
		FString(URPGTalentPresentationAsset::CanonicalObjectPath()),
		FString(CanonicalPath));

	const URPGTalentPresentationAsset* Catalog = URPGTalentPresentationAsset::ResolveCanonical();
	if (!TestNotNull(TEXT("Canonical Talent presentation catalog resolves"), Catalog))
	{
		return false;
	}
	TestTrue(TEXT("Canonical Talent presentation catalog is structurally valid"), Catalog->IsValidDefinition());
	TestEqual(TEXT("Canonical Talent presentation catalog keeps six production classes"), Catalog->Classes.Num(), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit025RuntimeSourceBoundaryTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentPresentationDependency.RuntimeSourceBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit025RuntimeSourceBoundaryTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit025;

	const TCHAR* Consumers[] = {
		TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsPageServiceTalentDetail.cpp"),
		TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetTalents.cpp")
	};

	for (const TCHAR* RelativePath : Consumers)
	{
		FString Source;
		if (!TestTrue(
			*FString::Printf(TEXT("%s loads"), RelativePath),
			LoadProjectSource(RelativePath, Source)))
		{
			continue;
		}

		TestTrue(
			*FString::Printf(TEXT("%s uses the canonical Talent presentation resolver"), RelativePath),
			Source.Contains(TEXT("URPGTalentPresentationAsset::ResolveCanonical()")));
		TestFalse(
			*FString::Printf(TEXT("%s does not hard-code the production Talent presentation asset"), RelativePath),
			Source.Contains(TEXT("DA_RPGTalentPresentation")));
		TestFalse(
			*FString::Printf(TEXT("%s does not load the Talent presentation asset directly"), RelativePath),
			Source.Contains(TEXT("LoadObject<URPGTalentPresentationAsset>")));
	}

	FString AssetSource;
	TestTrue(
		TEXT("RPGTalentPresentationAsset.cpp loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/UI/RPGTalentPresentationAsset.cpp"),
			AssetSource));
	TestTrue(
		TEXT("Canonical asset class owns the production object path"),
		AssetSource.Contains(CanonicalPath));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit025PrivateUiPathUniquenessTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentPresentationDependency.PrivateUiPathUniqueness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit025PrivateUiPathUniquenessTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit025;

	const FString UiSourceRoot =
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/GrimrockPrototype/Private/UI"));
	TArray<FString> SourceFiles;
	IFileManager::Get().FindFilesRecursive(
		SourceFiles,
		*UiSourceRoot,
		TEXT("*.cpp"),
		true,
		false,
		false);

	int32 PathOwnerCount = 0;
	for (const FString& Filename : SourceFiles)
	{
		FString Source;
		if (!FFileHelper::LoadFileToString(Source, *Filename) ||
			!Source.Contains(CanonicalPath))
		{
			continue;
		}

		++PathOwnerCount;
		TestTrue(
			TEXT("Only RPGTalentPresentationAsset.cpp may own the canonical runtime path"),
			Filename.EndsWith(TEXT("RPGTalentPresentationAsset.cpp"), ESearchCase::CaseSensitive));
	}

	TestEqual(TEXT("Exactly one Private/UI source owns the canonical runtime path"), PathOwnerCount, 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
