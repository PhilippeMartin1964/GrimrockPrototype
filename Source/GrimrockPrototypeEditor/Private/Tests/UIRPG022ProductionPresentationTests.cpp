#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "UI/RPGTalentPresentationAsset.h"
#include "UI/UIRPGTalentPresentationAuthoring.h"

namespace UIRPG022Production
{
	struct FClassSpec
	{
		FName ClassId;
		const TCHAR* ClassObjectPath;
		TArray<FName> OrderedBranchIds;
	};

	const FClassSpec Specs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior"),
			{ TEXT("Guardian"), TEXT("Breaker"), TEXT("WeaponMaster") } },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"),
			{ TEXT("Assassin"), TEXT("Shadow"), TEXT("Saboteur") } },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"),
			{ TEXT("Marksman"), TEXT("Hunter"), TEXT("Scout") } },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage"),
			{ TEXT("Evoker"), TEXT("Arcanist"), TEXT("SurfaceWeaver") } },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest"),
			{ TEXT("Restoration"), TEXT("Protection"), TEXT("Exorcism") } },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist"),
			{ TEXT("Grenadier"), TEXT("Apothecary"), TEXT("Transmuter") } }
	};

	URPGTalentPresentationAsset* LoadCatalog()
	{
		return LoadObject<URPGTalentPresentationAsset>(
			nullptr, FUIRPGTalentPresentationAuthoring::ObjectPath());
	}

	TSet<FName> CollectGameplayBranchIds(const URPGClassAsset& ClassAsset)
	{
		TSet<FName> Result;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset.ProgressionChoices)
		{
			Result.Add(Choice.TalentBranchId);
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG022CatalogLoadsTest,
	"Grimrock.UI.RPG02.ProductionPresentation.CatalogLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG022CatalogLoadsTest::RunTest(const FString&)
{
	using namespace UIRPG022Production;
	URPGTalentPresentationAsset* Catalog = LoadCatalog();
	TestNotNull(TEXT("Canonical DA_RPGTalentPresentation loads"), Catalog);
	if (!Catalog) return false;

	TestTrue(TEXT("Canonical presentation catalog is valid"), Catalog->IsValidDefinition());
	TestEqual(TEXT("Catalog contains six class presentations"), Catalog->Classes.Num(), 6);

	int32 BranchCount = 0;
	for (const FRPGClassPresentationDefinition& ClassPresentation : Catalog->Classes)
	{
		BranchCount += ClassPresentation.Branches.Num();
	}
	TestEqual(TEXT("Catalog contains eighteen branch presentations"), BranchCount, 18);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG022MatchesGameplayBranchesTest,
	"Grimrock.UI.RPG02.ProductionPresentation.MatchesGameplayBranches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG022MatchesGameplayBranchesTest::RunTest(const FString&)
{
	using namespace UIRPG022Production;
	URPGTalentPresentationAsset* Catalog = LoadCatalog();
	if (!TestNotNull(TEXT("Canonical presentation catalog loads"), Catalog)) return false;

	for (const FClassSpec& Spec : Specs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ClassObjectPath);
		TestNotNull(*FString::Printf(TEXT("%s gameplay class loads"), *Spec.ClassId.ToString()), ClassAsset);
		const FRPGClassPresentationDefinition* Presentation = Catalog->FindClass(Spec.ClassId);
		TestNotNull(*FString::Printf(TEXT("%s presentation exists"), *Spec.ClassId.ToString()), Presentation);
		if (!ClassAsset || !Presentation) continue;

		const TSet<FName> GameplayBranches = CollectGameplayBranchIds(*ClassAsset);
		TestEqual(*FString::Printf(TEXT("%s gameplay exposes three structural branches"), *Spec.ClassId.ToString()), GameplayBranches.Num(), 3);
		TestEqual(*FString::Printf(TEXT("%s presentation exposes three branches"), *Spec.ClassId.ToString()), Presentation->Branches.Num(), 3);

		for (const FRPGTalentBranchPresentationDefinition& Branch : Presentation->Branches)
		{
			TestTrue(*FString::Printf(TEXT("%s presentation branch %s exists in gameplay data"),
				*Spec.ClassId.ToString(), *Branch.TalentBranchId.ToString()), GameplayBranches.Contains(Branch.TalentBranchId));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG022CanonicalOrderAndLabelsTest,
	"Grimrock.UI.RPG02.ProductionPresentation.CanonicalOrderAndLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPG022CanonicalOrderAndLabelsTest::RunTest(const FString&)
{
	using namespace UIRPG022Production;
	URPGTalentPresentationAsset* Catalog = LoadCatalog();
	if (!TestNotNull(TEXT("Canonical presentation catalog loads"), Catalog)) return false;

	for (const FClassSpec& Spec : Specs)
	{
		const FRPGClassPresentationDefinition* Presentation = Catalog->FindClass(Spec.ClassId);
		TestNotNull(*FString::Printf(TEXT("%s presentation exists"), *Spec.ClassId.ToString()), Presentation);
		if (!Presentation || Presentation->Branches.Num() != 3) continue;

		for (int32 Index = 0; Index < 3; ++Index)
		{
			TestEqual(*FString::Printf(TEXT("%s branch index %d keeps canonical visual order"), *Spec.ClassId.ToString(), Index),
				Presentation->Branches[Index].TalentBranchId, Spec.OrderedBranchIds[Index]);
			TestFalse(*FString::Printf(TEXT("%s branch index %d has a display label"), *Spec.ClassId.ToString(), Index),
				Presentation->Branches[Index].DisplayName.IsEmpty());
		}
	}
	return true;
}

#endif
