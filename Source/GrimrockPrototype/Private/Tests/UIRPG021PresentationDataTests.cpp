#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/RPGTalentPresentationAsset.h"

namespace UIRPG021Tests
{
	FRPGTalentBranchPresentationDefinition MakeBranch(FName BranchId, const TCHAR* DisplayName)
	{
		FRPGTalentBranchPresentationDefinition Branch;
		Branch.TalentBranchId = BranchId;
		Branch.DisplayName = FText::FromString(DisplayName);
		Branch.ShortDescription = FText::FromString(TEXT("Presentation-only branch."));
		return Branch;
	}

	FRPGClassPresentationDefinition MakeClass(
		FName ClassId, FName LeftId, FName CenterId, FName RightId)
	{
		FRPGClassPresentationDefinition ClassPresentation;
		ClassPresentation.ClassId = ClassId;
		ClassPresentation.Branches = {
			MakeBranch(LeftId, TEXT("Left")),
			MakeBranch(CenterId, TEXT("Center")),
			MakeBranch(RightId, TEXT("Right"))
		};
		return ClassPresentation;
	}

	URPGTalentPresentationAsset* MakeSixClassCatalog()
	{
		URPGTalentPresentationAsset* Catalog = NewObject<URPGTalentPresentationAsset>();
		Catalog->Classes = {
			MakeClass(TEXT("Warrior"), TEXT("Guardian"), TEXT("Breaker"), TEXT("WeaponMaster")),
			MakeClass(TEXT("Rogue"), TEXT("Assassin"), TEXT("Shadow"), TEXT("Saboteur")),
			MakeClass(TEXT("Ranger"), TEXT("Marksman"), TEXT("Hunter"), TEXT("Scout")),
			MakeClass(TEXT("Mage"), TEXT("Evoker"), TEXT("Arcanist"), TEXT("SurfaceWeaver")),
			MakeClass(TEXT("Priest"), TEXT("Restoration"), TEXT("Protection"), TEXT("Exorcism")),
			MakeClass(TEXT("Alchemist"), TEXT("Grenadier"), TEXT("Apothecary"), TEXT("Transmuter"))
		};
		return Catalog;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG021ValidCatalogTest,
	"Grimrock.UI.RPG02.PresentationData.ValidSixClassCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG021ValidCatalogTest::RunTest(const FString&)
{
	using namespace UIRPG021Tests;
	URPGTalentPresentationAsset* Catalog = MakeSixClassCatalog();

	TestTrue(TEXT("Six-class presentation catalog is valid"), Catalog->IsValidDefinition());
	TestEqual(TEXT("Catalog contains six classes"), Catalog->Classes.Num(), 6);

	FRPGClassPresentationDefinition Warrior;
	TestTrue(TEXT("Warrior presentation resolves"), Catalog->GetClassPresentation(TEXT("Warrior"), Warrior));
	TestEqual(TEXT("Warrior exposes exactly three branches"), Warrior.Branches.Num(), 3);

	FRPGTalentBranchPresentationDefinition Guardian;
	TestTrue(TEXT("Guardian presentation resolves through class scope"),
		Catalog->GetBranchPresentation(TEXT("Warrior"), TEXT("Guardian"), Guardian));
	TestEqual(TEXT("Resolved branch keeps its structural id"), Guardian.TalentBranchId, FName(TEXT("Guardian")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG021VisualOrderTest,
	"Grimrock.UI.RPG02.PresentationData.LeftCenterRightOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG021VisualOrderTest::RunTest(const FString&)
{
	using namespace UIRPG021Tests;
	URPGTalentPresentationAsset* Catalog = MakeSixClassCatalog();

	FRPGClassPresentationDefinition Warrior;
	TestTrue(TEXT("Warrior presentation resolves"), Catalog->GetClassPresentation(TEXT("Warrior"), Warrior));
	if (Warrior.Branches.Num() != 3)
	{
		return false;
	}

	TestEqual(TEXT("Index 0 is left branch"), Warrior.Branches[0].TalentBranchId, FName(TEXT("Guardian")));
	TestEqual(TEXT("Index 1 is center branch"), Warrior.Branches[1].TalentBranchId, FName(TEXT("Breaker")));
	TestEqual(TEXT("Index 2 is right branch"), Warrior.Branches[2].TalentBranchId, FName(TEXT("WeaponMaster")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG021DuplicateClassTest,
	"Grimrock.UI.RPG02.PresentationData.RejectDuplicateClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG021DuplicateClassTest::RunTest(const FString&)
{
	using namespace UIRPG021Tests;
	URPGTalentPresentationAsset* Catalog = MakeSixClassCatalog();
	const FRPGClassPresentationDefinition DuplicateClass = Catalog->Classes[0];
	Catalog->Classes.Add(DuplicateClass);

	TestFalse(TEXT("Duplicate ClassId is rejected"), Catalog->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG021DuplicateBranchTest,
	"Grimrock.UI.RPG02.PresentationData.RejectDuplicateBranch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG021DuplicateBranchTest::RunTest(const FString&)
{
	using namespace UIRPG021Tests;
	URPGTalentPresentationAsset* Catalog = MakeSixClassCatalog();
	Catalog->Classes[0].Branches[2].TalentBranchId = Catalog->Classes[0].Branches[0].TalentBranchId;

	TestFalse(TEXT("Duplicate branch id inside one class is rejected"), Catalog->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG021ThreeBranchContractTest,
	"Grimrock.UI.RPG02.PresentationData.RequireExactlyThreeBranches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG021ThreeBranchContractTest::RunTest(const FString&)
{
	using namespace UIRPG021Tests;
	URPGTalentPresentationAsset* Catalog = MakeSixClassCatalog();
	Catalog->Classes[0].Branches.RemoveAt(2);

	TestFalse(TEXT("A class with fewer than three presentation branches is rejected"), Catalog->IsValidDefinition());

	Catalog = MakeSixClassCatalog();
	Catalog->Classes[0].Branches.Add(MakeBranch(TEXT("Fourth"), TEXT("Fourth")));
	TestFalse(TEXT("A class with more than three presentation branches is rejected"), Catalog->IsValidDefinition());
	return true;
}

#endif
