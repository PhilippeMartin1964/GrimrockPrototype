#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridInventoryTypes.h"
#include "UObject/UnrealType.h"

namespace GridTD0734Characterization
{
	const FProperty* FindCharacterProperty(const TCHAR* PropertyName)
	{
		return FindFProperty<FProperty>(FGridCharacterInventoryState::StaticStruct(), PropertyName);
	}

	bool IsDurableCharacterProperty(const TCHAR* PropertyName)
	{
		const FProperty* Property = FindCharacterProperty(PropertyName);
		return Property && !Property->HasAnyPropertyFlags(CPF_Transient);
	}

	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	bool PrimaryAssetIdUsesBusinessId(const FPrimaryAssetId& PrimaryAssetId, FName BusinessId)
	{
		return PrimaryAssetId.IsValid() && PrimaryAssetId.PrimaryAssetName == BusinessId;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0734CharacterDurableIdentityDuplicationTest,
	"Grimrock.TechnicalDebt.TD07_3_4.Characterization.CharacterDurableIdentityDuplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0734CharacterDurableIdentityDuplicationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0734Characterization;

	TestTrue(TEXT("ClassId is durable"), IsDurableCharacterProperty(TEXT("ClassId")));
	TestTrue(TEXT("ClassDisplayName is also durable today"), IsDurableCharacterProperty(TEXT("ClassDisplayName")));
	TestTrue(TEXT("ClassDefinition is also durable today"), IsDurableCharacterProperty(TEXT("ClassDefinition")));

	TestTrue(TEXT("RaceId is durable"), IsDurableCharacterProperty(TEXT("RaceId")));
	TestTrue(TEXT("RaceDisplayName is also durable today"), IsDurableCharacterProperty(TEXT("RaceDisplayName")));

	TestTrue(TEXT("PortraitGender is durable"), IsDurableCharacterProperty(TEXT("PortraitGender")));
	TestTrue(TEXT("PortraitVariantId is durable"), IsDurableCharacterProperty(TEXT("PortraitVariantId")));
	TestTrue(TEXT("Portrait soft reference is also durable today"), IsDurableCharacterProperty(TEXT("Portrait")));
	TestTrue(TEXT("ClassIcon soft reference is also durable today"), IsDurableCharacterProperty(TEXT("ClassIcon")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0734PrimaryAssetIdentityGapTest,
	"Grimrock.TechnicalDebt.TD07_3_4.Characterization.PrimaryAssetIdentityGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0734PrimaryAssetIdentityGapTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0734Characterization;

	URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(GetTransientPackage(), TEXT("TD0734_ClassAsset"));
	ClassDefinition->ClassId = TEXT("TD0734_Fighter");

	URPGRaceAsset* RaceDefinition = NewObject<URPGRaceAsset>(GetTransientPackage(), TEXT("TD0734_RaceAsset"));
	RaceDefinition->RaceId = TEXT("TD0734_Human");

	URPGClassVisualAsset* ClassVisual = NewObject<URPGClassVisualAsset>(GetTransientPackage(), TEXT("TD0734_ClassVisual"));
	ClassVisual->ClassId = ClassDefinition->ClassId;

	URPGCharacterPortraitSetAsset* PortraitSet =
		NewObject<URPGCharacterPortraitSetAsset>(GetTransientPackage(), TEXT("TD0734_PortraitSet"));
	PortraitSet->RaceId = RaceDefinition->RaceId;

	TestTrue(TEXT("Class PrimaryAssetId is canonically keyed by ClassId"),
		PrimaryAssetIdUsesBusinessId(ClassDefinition->GetPrimaryAssetId(), ClassDefinition->ClassId));
	TestTrue(TEXT("Race PrimaryAssetId is canonically keyed by RaceId"),
		PrimaryAssetIdUsesBusinessId(RaceDefinition->GetPrimaryAssetId(), RaceDefinition->RaceId));
	TestTrue(TEXT("ClassVisual PrimaryAssetId is canonically keyed by ClassId"),
		PrimaryAssetIdUsesBusinessId(ClassVisual->GetPrimaryAssetId(), ClassVisual->ClassId));
	TestTrue(TEXT("PortraitSet PrimaryAssetId is canonically keyed by RaceId"),
		PrimaryAssetIdUsesBusinessId(PortraitSet->GetPrimaryAssetId(), PortraitSet->RaceId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0734CreationCopiesPresentationStateTest,
	"Grimrock.TechnicalDebt.TD07_3_4.Characterization.CreationCopiesPresentationState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0734CreationCopiesPresentationStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0734Characterization;

	FString CustomRecruitSource;
	FString StoryCompanionSource;
	TestTrue(TEXT("Custom recruit source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/RPG/RPGCustomRecruitService.cpp"), CustomRecruitSource));
	TestTrue(TEXT("Story companion source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/RPG/RPGStoryCompanionService.cpp"), StoryCompanionSource));

	const TCHAR* CopiedFields[] = {
		TEXT("Candidate.RaceDisplayName ="),
		TEXT("Candidate.ClassDisplayName ="),
		TEXT("Candidate.Portrait ="),
		TEXT("Candidate.ClassIcon =")
	};

	for (const TCHAR* CopiedField : CopiedFields)
	{
		TestTrue(FString::Printf(TEXT("Custom recruit copies %s into durable character state"), CopiedField),
			CustomRecruitSource.Contains(CopiedField));
		TestTrue(FString::Printf(TEXT("Story companion copies %s into durable character state"), CopiedField),
			StoryCompanionSource.Contains(CopiedField));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD0734VisualFallbackDuplicationTest,
	"Grimrock.TechnicalDebt.TD07_3_4.Characterization.VisualFallbackDuplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD0734VisualFallbackDuplicationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD0734Characterization;

	FString InventoryVisualSource;
	FString ClassIconSource;
	TestTrue(TEXT("Inventory visual source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentVisuals.cpp"), InventoryVisualSource));
	TestTrue(TEXT("Inventory class-icon source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/UI/GridInventoryWidgetClassIcon.cpp"), ClassIconSource));

	TestTrue(TEXT("Visual read model copies durable Portrait"),
		InventoryVisualSource.Contains(TEXT("OutSelection.Portrait = CharacterState.Portrait")));
	TestTrue(TEXT("Visual read model copies durable ClassIcon"),
		InventoryVisualSource.Contains(TEXT("OutSelection.ClassIcon = CharacterState.ClassIcon")));
	TestTrue(TEXT("Class visual lookup is already keyed by ClassId"),
		ClassIconSource.Contains(TEXT("FindClassVisualForClass(VisualSelection.ClassId)")));
	TestTrue(TEXT("UI still falls back to the character-stored ClassIcon"),
		ClassIconSource.Contains(TEXT("VisualSelection.ClassIcon")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
