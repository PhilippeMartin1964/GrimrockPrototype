#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCatalogAuthoring.h"

namespace UIRPG061SkillCatalogAuthoringTests
{
	const FRPGCanonicalSkillDefinition* FindDefinition(
		const TArray<FRPGCanonicalSkillDefinition>& Definitions,
		FName SkillId)
	{
		return Definitions.FindByPredicate(
			[SkillId](const FRPGCanonicalSkillDefinition& Definition)
			{
				return Definition.SkillId == SkillId;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPG061CanonicalSkillCatalogTest,
	"Grimrock.UI.RPG06.Skills.CanonicalCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG061CanonicalSkillCatalogTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace UIRPG061SkillCatalogAuthoringTests;

	FString Error;
	TestTrue(TEXT("Canonical Skill catalog validates"), FRPGSkillCatalogAuthoring::ValidateCanonicalCatalog(Error));
	TestTrue(TEXT("Canonical Skill catalog validation returns no error"), Error.IsEmpty());

	TArray<FRPGCanonicalSkillDefinition> Definitions;
	FRPGSkillCatalogAuthoring::GetCanonicalDefinitions(Definitions);
	TestEqual(TEXT("Canonical catalog contains exactly twenty-five Skills"), Definitions.Num(), 25);

	TSet<FName> SeenIds;
	int32 TrainedOnlyCount = 0;
	for (const FRPGCanonicalSkillDefinition& Definition : Definitions)
	{
		TestFalse(TEXT("Canonical SkillId is not None"), Definition.SkillId.IsNone());
		TestFalse(TEXT("Canonical DisplayName is not empty"), Definition.DisplayName.IsEmpty());
		TestTrue(TEXT("Canonical governing attribute is explicit"), Definition.GoverningAttribute != ERPGSkillGoverningAttribute::None);
		TestFalse(TEXT("Canonical SkillId is unique"), SeenIds.Contains(Definition.SkillId));
		SeenIds.Add(Definition.SkillId);
		if (!Definition.bAllowUntrainedChecks)
		{
			++TrainedOnlyCount;
		}
	}
	TestEqual(TEXT("Exactly six Skills require training"), TrainedOnlyCount, 6);

	const FRPGCanonicalSkillDefinition* HeavyWeapons = FindDefinition(Definitions, TEXT("Skill_HeavyWeapons"));
	TestTrue(TEXT("Heavy Weapons contract matches rules"), HeavyWeapons &&
		HeavyWeapons->DisplayName.EqualTo(FText::FromString(TEXT("Armes lourdes"))) &&
		HeavyWeapons->GoverningAttribute == ERPGSkillGoverningAttribute::Strength &&
		HeavyWeapons->bAllowUntrainedChecks);

	const FRPGCanonicalSkillDefinition* Lockpicking = FindDefinition(Definitions, TEXT("Skill_Lockpicking"));
	TestTrue(TEXT("Lockpicking contract matches rules"), Lockpicking &&
		Lockpicking->DisplayName.EqualTo(FText::FromString(TEXT("Crochetage"))) &&
		Lockpicking->GoverningAttribute == ERPGSkillGoverningAttribute::Dexterity &&
		!Lockpicking->bAllowUntrainedChecks);

	const FRPGCanonicalSkillDefinition* Religion = FindDefinition(Definitions, TEXT("Skill_Religion"));
	TestTrue(TEXT("Religion contract matches rules"), Religion &&
		Religion->GoverningAttribute == ERPGSkillGoverningAttribute::Wisdom &&
		!Religion->bAllowUntrainedChecks);

	const FRPGCanonicalSkillDefinition* Deception = FindDefinition(Definitions, TEXT("Skill_Deception"));
	TestTrue(TEXT("Deception contract matches rules"), Deception &&
		Deception->GoverningAttribute == ERPGSkillGoverningAttribute::Charisma &&
		Deception->bAllowUntrainedChecks);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPG061SkillAuthoringPreservesExtensionsTest,
	"Grimrock.UI.RPG06.Skills.AuthoringPreservesExtensions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG061SkillAuthoringPreservesExtensionsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FRPGCanonicalSkillDefinition Lockpicking;
	TestTrue(
		TEXT("Lockpicking canonical definition resolves"),
		FRPGSkillCatalogAuthoring::TryGetCanonicalDefinition(TEXT("Skill_Lockpicking"), Lockpicking));

	URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(GetTransientPackage());
	Skill->Description = FText::FromString(TEXT("Description métier conservée."));

	FRPGSkillRequirementGrant Grant;
	Grant.MinimumRank = 2;
	Grant.GrantedRequirementIds = { TEXT("Requirement_Test_Lockpicking") };
	Skill->RequirementGrants.Add(Grant);

	FRPGSkillCatalogAuthoring::ConfigureSkill(*Skill, Lockpicking);

	TestTrue(TEXT("Authored Skill matches canonical fields"), FRPGSkillCatalogAuthoring::IsCanonicalSkill(*Skill, Lockpicking));
	TestTrue(TEXT("Authored Skill remains structurally valid"), Skill->IsValidDefinition());
	TestTrue(
		TEXT("Description is preserved"),
		Skill->Description.EqualTo(FText::FromString(TEXT("Description métier conservée."))));
	TestEqual(TEXT("RequirementGrants are preserved"), Skill->RequirementGrants.Num(), 1);
	if (Skill->RequirementGrants.Num() == 1)
	{
		TestEqual(TEXT("Requirement threshold is preserved"), Skill->RequirementGrants[0].MinimumRank, 2);
		TestTrue(
			TEXT("RequirementId is preserved"),
			Skill->RequirementGrants[0].GrantedRequirementIds.Contains(TEXT("Requirement_Test_Lockpicking")));
	}

	const FPrimaryAssetId PrimaryAssetId = Skill->GetPrimaryAssetId();
	TestEqual(TEXT("PrimaryAsset type remains RPGSkill"), PrimaryAssetId.PrimaryAssetType, FPrimaryAssetType(TEXT("RPGSkill")));
	TestEqual(TEXT("PrimaryAsset name remains SkillId"), PrimaryAssetId.PrimaryAssetName, FName(TEXT("Skill_Lockpicking")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPG062AProductionSkillCatalogTest,
	"Grimrock.UI.RPG06.Skills.ProductionCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG062AProductionSkillCatalogTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FRPGCanonicalSkillDefinition> Definitions;
	FRPGSkillCatalogAuthoring::GetCanonicalDefinitions(Definitions);
	TestEqual(TEXT("Twenty-five canonical Skill definitions are expected"), Definitions.Num(), 25);

	int32 LoadedCount = 0;
	for (const FRPGCanonicalSkillDefinition& Definition : Definitions)
	{
		const FString AssetName = FString::Printf(TEXT("DA_%s"), *Definition.SkillId.ToString());
		const FString ObjectPath = FString::Printf(
			TEXT("%s/%s.%s"),
			FRPGSkillCatalogAuthoring::ProductionFolder(),
			*AssetName,
			*AssetName);

		URPGSkillAsset* Skill = LoadObject<URPGSkillAsset>(nullptr, *ObjectPath);
		TestNotNull(*FString::Printf(TEXT("%s production asset loads"), *Definition.SkillId.ToString()), Skill);
		if (!Skill)
		{
			continue;
		}

		++LoadedCount;
		TestTrue(
			*FString::Printf(TEXT("%s production asset matches canonical fields"), *Definition.SkillId.ToString()),
			FRPGSkillCatalogAuthoring::IsCanonicalSkill(*Skill, Definition));
		TestTrue(
			*FString::Printf(TEXT("%s production asset is structurally valid"), *Definition.SkillId.ToString()),
			Skill->IsValidDefinition());
	}

	TestEqual(TEXT("All twenty-five production Skill assets load"), LoadedCount, 25);
	return true;
}

#endif
