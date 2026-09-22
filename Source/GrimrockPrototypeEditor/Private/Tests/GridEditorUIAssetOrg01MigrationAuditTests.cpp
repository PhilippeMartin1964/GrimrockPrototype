#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"

namespace GridEditorUIAssetOrg01Private
{
	struct FUiAssetMove
	{
		const TCHAR* Label;
		const TCHAR* SourcePackage;
		const TCHAR* TargetPackage;
	};

	static const FUiAssetMove Moves[] = {
		{ TEXT("CharacterSheet"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_CharacterSheet"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_CharacterSheet") },
		{ TEXT("InventoryBag"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_InventoryBag"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_InventoryBag") },
		{ TEXT("InventorySlot"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_InventorySlot"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_InventorySlot") },
		{ TEXT("PartyMember"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_PartyMember"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_PartyMember") },
		{ TEXT("ItemActionMenu"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemActionMenu"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemActionMenu") },
		{ TEXT("ItemActionButton"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemActionButton"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemActionButton") },
		{ TEXT("ItemReadPanel"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemReadPanel"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemReadPanel") },
		{ TEXT("ItemTooltip"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemTooltip"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltip") },
		{ TEXT("ItemTooltipComparisonRow"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemTooltipComparisonRow"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipComparisonRow") },
		{ TEXT("ItemTooltipStatLine"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ItemTooltipStatLine"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipStatLine") },
		{ TEXT("GrimrockMenu"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GrimrockMenu"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GrimrockMenu") },
		{ TEXT("GridSkills"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridSkills"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSkills") },
		{ TEXT("GridSpellbook"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridSpellbook"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSpellbook") },
		{ TEXT("GridSpellbookEntry"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridSpellbookEntry"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSpellbookEntry") },
		{ TEXT("GridJournal"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridJournal"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridJournal") },
		{ TEXT("GridMap"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridMap"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridMap") },
		{ TEXT("GridRecipes"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridRecipes"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridRecipes") },
		{ TEXT("GridCodex"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridCodex"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridCodex") },
		{ TEXT("GridMouseCursor"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_GridMouseCursor"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Interaction/WBP_GridMouseCursor") },
		{ TEXT("ReadableMessage"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/WBP_ReadableMessage"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Interaction/WBP_ReadableMessage") },
		{ TEXT("CarolingiaFont"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/F_Carolingia"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Fonts/F_Carolingia") },
		{ TEXT("CarolingiaResource"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/carolingia"), TEXT("/Game/GrimrockPrototype/Blueprints/UI/Fonts/carolingia") },
	};

	bool PackageExists(IAssetRegistry& AssetRegistry, const TCHAR* PackagePath)
	{
		TArray<FAssetData> Assets;
		return AssetRegistry.GetAssetsByPackageName(FName(PackagePath), Assets, true, false) && !Assets.IsEmpty();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorUIAssetOrg01MigrationAuditTest, "Grimrock.Editor.UIAssetOrg01.MigrationAudit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorUIAssetOrg01MigrationAuditTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridEditorUIAssetOrg01Private;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	AddInfo(TEXT("[UI-ASSET-ORG01] Exactly one package location must exist for each asset. During a move, Fix Up Redirectors before validation."));

	for (const FUiAssetMove& Move : Moves)
	{
		const bool bSourceExists = PackageExists(AssetRegistry, Move.SourcePackage);
		const bool bTargetExists = PackageExists(AssetRegistry, Move.TargetPackage);
		const bool bExactlyOneLocation = bSourceExists != bTargetExists;

		TestTrue(
			*FString::Printf(TEXT("%s exists in exactly one migration location"), Move.Label),
			bExactlyOneLocation);

		const TCHAR* Location = bSourceExists && !bTargetExists
			? TEXT("SOURCE")
			: (!bSourceExists && bTargetExists ? TEXT("TARGET") : TEXT("INVALID"));

		AddInfo(FString::Printf(
			TEXT("[UI-ASSET-ORG01] Asset=%s SourceExists=%s TargetExists=%s Location=%s"),
			Move.Label,
			bSourceExists ? TEXT("true") : TEXT("false"),
			bTargetExists ? TEXT("true") : TEXT("false"),
			Location));
	}

	return true;
}

#endif
