#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

#include "UI/RPGTalentPresentationAsset.h"

namespace UIRPGTalentCodeAudit028
{
	bool LoadProjectSource(const TCHAR* RelativePath, FString& OutSource)
	{
		return FFileHelper::LoadFileToString(
			OutSource,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit028ReflectedContractTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentPresentationDataAudit.ReflectedContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit028ReflectedContractTest::RunTest(const FString&)
{
	UScriptStruct* ClassStruct = FRPGClassPresentationDefinition::StaticStruct();
	UScriptStruct* BranchStruct = FRPGTalentBranchPresentationDefinition::StaticStruct();
	if (!TestNotNull(TEXT("Class presentation struct exists"), ClassStruct) ||
		!TestNotNull(TEXT("Branch presentation struct exists"), BranchStruct))
	{
		return false;
	}

	for (const TCHAR* PropertyName : {
		TEXT("ClassId"),
		TEXT("AccentColor"),
		TEXT("Branches")
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("Class presentation keeps live property %s"), PropertyName),
			FindFProperty<FProperty>(ClassStruct, FName(PropertyName)));
	}

	for (const TCHAR* PropertyName : {
		TEXT("PortraitTexture"),
		TEXT("ClassEmblemTexture"),
		TEXT("PrimaryColor"),
		TEXT("SecondaryColor"),
		TEXT("GlowColor"),
		TEXT("MotifTextures")
	})
	{
		TestNull(
			*FString::Printf(TEXT("Class presentation removes obsolete property %s"), PropertyName),
			FindFProperty<FProperty>(ClassStruct, FName(PropertyName)));
	}

	for (const TCHAR* PropertyName : {
		TEXT("TalentBranchId"),
		TEXT("DisplayName"),
		TEXT("ShortDescription"),
		TEXT("AccentColor"),
		TEXT("NodePresentationOverrides")
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("Branch presentation keeps live/intentional property %s"), PropertyName),
			FindFProperty<FProperty>(BranchStruct, FName(PropertyName)));
	}

	TestNull(
		TEXT("Branch presentation removes obsolete BranchEmblemTexture"),
		FindFProperty<FProperty>(BranchStruct, TEXT("BranchEmblemTexture")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit028SourceOwnershipTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentPresentationDataAudit.SourceOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit028SourceOwnershipTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit028;

	FString HeaderSource;
	FString AuthoringSource;
	FString VisualHeaderSource;
	TestTrue(
		TEXT("RPGTalentPresentationAsset.h loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Public/UI/RPGTalentPresentationAsset.h"),
			HeaderSource));
	TestTrue(
		TEXT("UIRPGTalentPresentationAuthoring.cpp loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototypeEditor/Private/UI/UIRPGTalentPresentationAuthoring.cpp"),
			AuthoringSource));
	TestTrue(
		TEXT("RPGClassVisualAsset.h loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Public/RPG/RPGClassVisualAsset.h"),
			VisualHeaderSource));

	for (const TCHAR* DeadName : {
		TEXT("PortraitTexture"),
		TEXT("ClassEmblemTexture"),
		TEXT("PrimaryColor"),
		TEXT("SecondaryColor"),
		TEXT("GlowColor"),
		TEXT("MotifTextures"),
		TEXT("BranchEmblemTexture")
	})
	{
		TestFalse(
			*FString::Printf(TEXT("Talent presentation contract no longer owns %s"), DeadName),
			HeaderSource.Contains(DeadName));
		TestFalse(
			*FString::Printf(TEXT("Talent presentation authoring no longer writes %s"), DeadName),
			AuthoringSource.Contains(DeadName));
	}

	TestTrue(TEXT("Talent presentation keeps class AccentColor"),
		HeaderSource.Contains(TEXT("FLinearColor AccentColor")));
	TestTrue(TEXT("Talent presentation keeps serialized branch ShortDescription"),
		HeaderSource.Contains(TEXT("FText ShortDescription")));
	TestTrue(TEXT("Class visual authority owns ClassIcon"),
		VisualHeaderSource.Contains(TEXT("TSoftObjectPtr<UTexture2D> ClassIcon")));
	TestTrue(TEXT("Class visual authority owns Banner"),
		VisualHeaderSource.Contains(TEXT("TSoftObjectPtr<UTexture2D> Banner")));
	TestTrue(TEXT("Class visual authority owns Flag"),
		VisualHeaderSource.Contains(TEXT("TSoftObjectPtr<UTexture2D> Flag")));
	TestTrue(TEXT("Class visual authority owns branch visual records"),
		VisualHeaderSource.Contains(TEXT("TalentBranchVisuals")));
	TestTrue(TEXT("Class visual authority owns node visual records"),
		VisualHeaderSource.Contains(TEXT("TalentNodeVisuals")));
	TestTrue(TEXT("Class visual authority owns variant visual records"),
		VisualHeaderSource.Contains(TEXT("TalentVariantVisuals")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
