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
	};

	static const FAssetAuditCandidate Candidates[] = {
		{ TEXT("BlueGemPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_BlueGemPickup") },
		{ TEXT("KeyCopperPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_KeyCopperPickup") },
		{ TEXT("KeyIronPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_KeyIronPickup") },
		{ TEXT("ShurikenPickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_ShurikenPickup") },
		{ TEXT("StonePickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_StonePickup") },
		{ TEXT("TestNotePickup"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Object_TestNotePickup") },
		{ TEXT("MonsterSpawn"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_MonsterSpawn") },
		{ TEXT("ArchetypeCustomRecruiterService"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Archetype_CustomRecruiter_Service") },
		{ TEXT("ArchetypeStoryCompanionRecruit"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Archetype_StoryCompanion_Recruit") },
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

	AddInfo(TEXT("[ALIGN-A3] Policy=AssetRegistry on-disk referencers only; Referencers=0 never implies DELETE_SAFE and requires code/string/dynamic-load checks."));

	for (const FAssetAuditCandidate& Candidate : Candidates)
	{
		const FName PackageName(Candidate.PackagePath);
		TArray<FAssetData> PackageAssets;
		const bool bAssetQuerySucceeded = AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets, true, false);
		const bool bExists = bAssetQuerySucceeded && !PackageAssets.IsEmpty();
		TestTrue(*FString::Printf(TEXT("ALIGN-A3 candidate exists: %s"), Candidate.Label), bExists);

		TArray<FName> Referencers;
		const bool bReferencerQuerySucceeded = AssetRegistry.GetReferencers(
			PackageName,
			Referencers,
			UE::AssetRegistry::EDependencyCategory::All,
			UE::AssetRegistry::FDependencyQuery());

		Referencers.Sort([](const FName& A, const FName& B)
		{
			return A.ToString() < B.ToString();
		});

		AddInfo(FString::Printf(
			TEXT("[ALIGN-A3] Asset=%s Label=%s Exists=%s ReferencerQuery=%s Referencers=%d"),
			Candidate.PackagePath,
			Candidate.Label,
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
