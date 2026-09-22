#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AssetRegistryInterface.h"
#include "Modules/ModuleManager.h"

namespace GridEditorUIAssetClean01Private
{
	struct FUiAssetCandidate
	{
		const TCHAR* Label;
		const TCHAR* PackagePath;
	};

	static const FUiAssetCandidate Candidates[] = {
		{ TEXT("TopTabNormal"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Buttons/TopTabs/T_ButtonTab_Normal_480x100") },
		{ TEXT("TopTabHovered"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Buttons/TopTabs/T_ButtonTab_Hovered_480x100") },
		{ TEXT("TopTabPressed"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Buttons/TopTabs/T_ButtonTab_Pressed_480x100") },
		{ TEXT("TopTabDisabled"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Buttons/TopTabs/T_ButtonTab_Disabled_480x100") },
		{ TEXT("TopTabSelected"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Buttons/TopTabs/T_ButtonTab_Selected_480x100") },
		{ TEXT("ItemTooltipComparisonRow"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemTooltipComparisonRow") },
		{ TEXT("BorderCharacterLegacyName"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Icons/T_BorderCharacter") },
		{ TEXT("BorderCharacterCanonicalName"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Icons/T_Border_Character") },
		{ TEXT("RootFrame"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Buttons/T_RootFrame") },
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorUIAssetClean01ReferenceAuditTest, "Grimrock.Editor.UIAssetClean01.ReferenceAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorUIAssetClean01ReferenceAuditTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridEditorUIAssetClean01Private;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	AddInfo(TEXT("[UI-ASSET-CLEAN01] Read-only AssetRegistry audit. No asset is deleted by this test."));
	AddInfo(TEXT("[UI-ASSET-CLEAN01] Candidate is deletion-ready only when Exists=true and Referencers=0, then confirm once in Unreal Reference Viewer before deletion."));

	for (const FUiAssetCandidate& Candidate : Candidates)
	{
		const FName PackageName(Candidate.PackagePath);
		TArray<FAssetData> PackageAssets;
		const bool bAssetQuerySucceeded = AssetRegistry.GetAssetsByPackageName(PackageName, PackageAssets, true, false);
		const bool bExists = bAssetQuerySucceeded && !PackageAssets.IsEmpty();

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

		const bool bDeletionReady = bExists && bReferencerQuerySucceeded && Referencers.IsEmpty();

		AddInfo(FString::Printf(
			TEXT("[UI-ASSET-CLEAN01] Asset=%s Label=%s Exists=%s ReferencerQuery=%s Referencers=%d DeletionReady=%s"),
			Candidate.PackagePath,
			Candidate.Label,
			bExists ? TEXT("true") : TEXT("false"),
			bReferencerQuerySucceeded ? TEXT("true") : TEXT("false"),
			Referencers.Num(),
			bDeletionReady ? TEXT("true") : TEXT("false")));

		for (const FName Referencer : Referencers)
		{
			AddInfo(FString::Printf(TEXT("[UI-ASSET-CLEAN01]   Referencer=%s"), *Referencer.ToString()));
		}
	}

	return true;
}

#endif
