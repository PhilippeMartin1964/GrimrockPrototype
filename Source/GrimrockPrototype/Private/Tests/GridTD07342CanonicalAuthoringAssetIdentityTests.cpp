#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "RPG/RPGRaceAsset.h"

namespace GridTD07342Normalization
{
	TSoftObjectPtr<UTexture2D> MakeFakeTexture(const TCHAR* Path)
	{
		return TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Path));
	}

	URPGClassVisualAsset* MakeClassVisual(FName ClassId)
	{
		URPGClassVisualAsset* Visual = NewObject<URPGClassVisualAsset>(GetTransientPackage());
		Visual->ClassId = ClassId;
		Visual->ClassIcon = MakeFakeTexture(TEXT("/Game/TD07342/FakeClassIcon.FakeClassIcon"));
		return Visual;
	}

	URPGCharacterPortraitSetAsset* MakePortraitSet(FName RaceId)
	{
		URPGCharacterPortraitSetAsset* Set = NewObject<URPGCharacterPortraitSetAsset>(GetTransientPackage());
		Set->RaceId = RaceId;

		FRPGCharacterPortraitVariant Variant;
		Variant.VariantId = TEXT("Portrait01");
		Variant.Portrait = MakeFakeTexture(TEXT("/Game/TD07342/FakePortrait.FakePortrait"));
		Set->MalePortraits.Add(Variant);
		return Set;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07342ClassIdentityContractTest, "Grimrock.TechnicalDebt.TD07_3_4_2.Normalization.ClassIdentityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07342ClassIdentityContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGClassAsset* First = NewObject<URPGClassAsset>(GetTransientPackage(), TEXT("TD07342_ClassFileA"));
	URPGClassAsset* Second = NewObject<URPGClassAsset>(GetTransientPackage(), TEXT("TD07342_ClassFileB"));
	First->ClassId = TEXT("Fighter");
	Second->ClassId = TEXT("Fighter");

	const FPrimaryAssetId Expected(FPrimaryAssetType(TEXT("RPGClass")), FName(TEXT("Fighter")));
	TestTrue(TEXT("Class primary identity is independent from UObject name"), First->GetPrimaryAssetId() == Second->GetPrimaryAssetId());
	TestTrue(TEXT("Class primary identity uses ClassId"), First->GetPrimaryAssetId() == Expected);
	TestTrue(TEXT("Resolver builds the same canonical Class id"), FRPGAuthoringIdentityResolver::MakeClassPrimaryAssetId(TEXT("Fighter")) == Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07342RaceIdentityContractTest, "Grimrock.TechnicalDebt.TD07_3_4_2.Normalization.RaceIdentityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07342RaceIdentityContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGRaceAsset* First = NewObject<URPGRaceAsset>(GetTransientPackage(), TEXT("TD07342_RaceFileA"));
	URPGRaceAsset* Second = NewObject<URPGRaceAsset>(GetTransientPackage(), TEXT("TD07342_RaceFileB"));
	First->RaceId = TEXT("Human");
	Second->RaceId = TEXT("Human");

	const FPrimaryAssetId Expected(FPrimaryAssetType(TEXT("RPGRace")), FName(TEXT("Human")));
	TestTrue(TEXT("Race primary identity is independent from UObject name"), First->GetPrimaryAssetId() == Second->GetPrimaryAssetId());
	TestTrue(TEXT("Race primary identity uses RaceId"), First->GetPrimaryAssetId() == Expected);
	TestTrue(TEXT("Resolver builds the same canonical Race id"), FRPGAuthoringIdentityResolver::MakeRacePrimaryAssetId(TEXT("Human")) == Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07342VisualIdentityContractTest, "Grimrock.TechnicalDebt.TD07_3_4_2.Normalization.VisualIdentityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07342VisualIdentityContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07342Normalization;

	URPGClassVisualAsset* Visual = MakeClassVisual(TEXT("Mage"));
	URPGCharacterPortraitSetAsset* PortraitSet = MakePortraitSet(TEXT("Elf"));

	TestTrue(TEXT("Class visual is keyed by ClassId"),
		Visual->GetPrimaryAssetId() == FPrimaryAssetId(FPrimaryAssetType(TEXT("RPGClassVisual")), FName(TEXT("Mage"))));
	TestTrue(TEXT("Portrait set is keyed by RaceId"),
		PortraitSet->GetPrimaryAssetId() == FPrimaryAssetId(FPrimaryAssetType(TEXT("RPGPortraitSet")), FName(TEXT("Elf"))));

	TestTrue(TEXT("Matching ClassVisual is accepted"), FRPGAuthoringIdentityResolver::IsMatchingClassVisual(TEXT("Mage"), Visual));
	TestFalse(TEXT("Mismatched ClassVisual is rejected"), FRPGAuthoringIdentityResolver::IsMatchingClassVisual(TEXT("Fighter"), Visual));
	TestTrue(TEXT("Matching PortraitSet is accepted"), FRPGAuthoringIdentityResolver::IsMatchingPortraitSet(TEXT("Elf"), PortraitSet));
	TestFalse(TEXT("Mismatched PortraitSet is rejected"), FRPGAuthoringIdentityResolver::IsMatchingPortraitSet(TEXT("Human"), PortraitSet));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07342StrictResolverContractTest, "Grimrock.TechnicalDebt.TD07_3_4_2.Normalization.StrictResolverContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07342StrictResolverContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(GetTransientPackage());
	ClassDefinition->ClassId = TEXT("Rogue");
	URPGRaceAsset* RaceDefinition = NewObject<URPGRaceAsset>(GetTransientPackage());
	RaceDefinition->RaceId = TEXT("Dwarf");

	TestTrue(TEXT("Matching class definition is accepted"), FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(TEXT("Rogue"), ClassDefinition));
	TestFalse(TEXT("Mismatched class definition is rejected"), FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(TEXT("Mage"), ClassDefinition));
	TestTrue(TEXT("Matching race definition is accepted"), FRPGAuthoringIdentityResolver::IsMatchingRaceDefinition(TEXT("Dwarf"), RaceDefinition));
	TestFalse(TEXT("Mismatched race definition is rejected"), FRPGAuthoringIdentityResolver::IsMatchingRaceDefinition(TEXT("Human"), RaceDefinition));

	TestFalse(TEXT("NAME_None does not create a Class primary id"), FRPGAuthoringIdentityResolver::MakeClassPrimaryAssetId(NAME_None).IsValid());
	TestFalse(TEXT("NAME_None does not create a Race primary id"), FRPGAuthoringIdentityResolver::MakeRacePrimaryAssetId(NAME_None).IsValid());
	TestNull(TEXT("NAME_None Class cannot resolve"), FRPGAuthoringIdentityResolver::ResolveClassById(NAME_None));
	TestNull(TEXT("NAME_None Race cannot resolve"), FRPGAuthoringIdentityResolver::ResolveRaceById(NAME_None));
	TestNull(TEXT("NAME_None ClassVisual cannot resolve"), FRPGAuthoringIdentityResolver::ResolveClassVisualByClassId(NAME_None));
	TestNull(TEXT("NAME_None PortraitSet cannot resolve"), FRPGAuthoringIdentityResolver::ResolvePortraitSetByRaceId(NAME_None));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
