#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AssetRegistryInterface.h"
#include "Modules/ModuleManager.h"

namespace GridEditorALIGNA3AssetReferenceAuditPrivate
{
	struct FAssetAuditCandidate
	{
		const TCHAR* Label;
		const TCHAR* PackagePath;
		bool bExpectedToExist;
	};

	static const FAssetAuditCandidate Candidates[] = {
		{ TEXT("BlueGemPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_BlueGemPickup"), false },
		{ TEXT("KeyCopperPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_KeyCopperPickup"), false },
		{ TEXT("KeyIronPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_KeyIronPickup"), false },
		{ TEXT("ShurikenPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_ShurikenPickup"), false },
		{ TEXT("StonePickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_StonePickup"), false },
		{ TEXT("TestNotePickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_TestNotePickup"), false },
		{ TEXT("MonsterSpawn"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_MonsterSpawn"), true },
		{ TEXT("ArchetypeCustomRecruiterService"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Archetype_CustomRecruiter_Service"), true },
		{ TEXT("ArchetypeStoryCompanionRecruit"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Archetype_StoryCompanion_Recruit"), true },
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorALIGNA3AssetReferenceAuditTest, "Grimrock.Editor.ALIGN_A3.AssetReferenceAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorALIGNA3AssetReferenceAuditTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridEditorALIGNA3AssetReferenceAuditPrivate;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	AddInfo(TEXT("[ALIGN-A3] Policy=AssetRegistry on-disk referencers only; removed legacy pickup definitions must stay absent; retained authoring definitions must stay present."));

	for (const FAssetAuditCandidate& Candidate : Candidates)
	{
		const FName PackageName(Candidate.PackagePath);
		TArray<FAssetData> PackageAssets;
		const bool bAssetQuerySucceeded = AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets, true, false);
		const bool bExists = bAssetQuerySucceeded && !PackageAssets.IsEmpty();

		const FString ExistenceAssertion = FString::Printf(
			TEXT("ALIGN-A3 expected %s: %s"),
			Candidate.bExpectedToExist ? TEXT("asset to exist") : TEXT("legacy asset to be absent"),
			Candidate.Label);

		if (Candidate.bExpectedToExist)
		{
			TestTrue(*ExistenceAssertion, bExists);
		}
		else
		{
			TestFalse(*ExistenceAssertion, bExists);
		}

		TArray<FName> Referencers;
		bool bReferencerQuerySucceeded = false;
		if (bExists)
		{
			bReferencerQuerySucceeded = AssetRegistry.GetReferencers(
				PackageName,
				Referencers,
				UE::AssetRegistry::EDependencyCategory::All,
				UE::AssetRegistry::FDependencyQuery());
		}

		Referencers.Sort([](const FName& A, const FName& B)
		{
			return A.ToString() < B.ToString();
		});

		AddInfo(FString::Printf(
			TEXT("[ALIGN-A3] Asset=%s Label=%s ExpectedExists=%s Exists=%s ReferencerQuery=%s Referencers=%d"),
			Candidate.PackagePath,
			Candidate.Label,
			Candidate.bExpectedToExist ? TEXT("true") : TEXT("false"),
			bExists ? TEXT("true") : TEXT("false"),
			bReferencerQuerySucceeded ? TEXT("true") : TEXT("false"),
			Referencers.Num()));

		for (const FName Referencer : Referencers)
		{
			AddInfo(FString::Printf(TEXT("[ALIGN-A3]   Referencer=%s"), *Referencer.ToString()));
		}
	}

	return true;
}

#endif
