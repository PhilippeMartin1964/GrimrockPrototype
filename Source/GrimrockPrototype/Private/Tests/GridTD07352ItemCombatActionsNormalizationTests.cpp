#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace GridTD07352Normalization
{
	const TCHAR* ShurikenAssetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken.DA_Weapon_Shuriken");

	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	FGridCombatActionDefinition MakeAttackAction(FName ActionId, int32 RangeCells = 1)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromName(ActionId);
		Action.ActionType = RangeCells > 1 ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = 2;
		Action.RangeCells = RangeCells;
		Action.PresentationProfileId = ActionId;
		Action.OffensiveProfile.AttackId = ActionId;
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Action.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 1;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 3;
		Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::Strength;
		Action.OffensiveProfile.RangeCells = RangeCells;
		return Action;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352SchemaAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ItemClass = UGridItemDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Item definition class exists"), ItemClass);
	if (!ItemClass)
	{
		return false;
	}

	TestNull(TEXT("bProvidesAttack property is physically removed"),
		ItemClass->FindPropertyByName(TEXT("bProvidesAttack")));
	TestNull(TEXT("item-level OffensiveProfile property is physically removed"),
		ItemClass->FindPropertyByName(TEXT("OffensiveProfile")));

	const FProperty* CombatActions = ItemClass->FindPropertyByName(TEXT("CombatActions"));
	TestNotNull(TEXT("CombatActions remains the item combat authoring property"), CombatActions);
	TestTrue(TEXT("CombatActions remains durable authoring data"),
		CombatActions && !CombatActions->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352CombatActionsOnlyBehaviorTest,
	"Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.CombatActionsOnlyBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352CombatActionsOnlyBehaviorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07352Normalization;

	UGridItemDefinitionAsset* Weapon = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	Weapon->ItemDefinitionId = TEXT("TD07352_Sword");
	Weapon->DisplayName = FText::FromString(TEXT("Sword"));
	Weapon->ItemType = EGridItemType::Weapon;
	Weapon->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);

	TestTrue(TEXT("A non-combat item definition can remain structurally valid"), Weapon->IsValidDefinition());
	TestFalse(TEXT("An empty CombatActions array grants no attack"),
		Weapon->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));

	Weapon->CombatActions.Add(MakeAttackAction(TEXT("Attack_TD07352_Sword")));
	TestTrue(TEXT("A current CombatAction grants the main-hand attack"),
		Weapon->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Current CombatActions keep the item definition valid"), Weapon->IsValidDefinition());

	Weapon->bThrowable = true;
	FGridCombatActionDefinition InventoryAction;
	TestTrue(TEXT("Throwable inventory action is built from the configured CombatAction"),
		Weapon->BuildInventoryCombatActionDefinition(InventoryAction));
	TestEqual(TEXT("Inventory throw keeps the configured attack profile"),
		InventoryAction.OffensiveProfile.AttackId, FName(TEXT("Attack_TD07352_Sword")));
	TestEqual(TEXT("Inventory throw consumes one source item"),
		InventoryAction.ResourceCosts.SourceItemQuantityCost, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352ShurikenAssetAuthorityTest,
	"Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.ShurikenAssetAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352ShurikenAssetAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07352Normalization;

	UGridItemDefinitionAsset* Shuriken = LoadObject<UGridItemDefinitionAsset>(nullptr, ShurikenAssetPath);
	TestNotNull(TEXT("Repaired DA_Weapon_Shuriken loads"), Shuriken);
	if (!Shuriken)
	{
		return false;
	}

	TestEqual(TEXT("Shuriken business id remains stable"), Shuriken->ItemDefinitionId, FName(TEXT("Shuriken")));
	TestTrue(TEXT("Shuriken remains throwable"), Shuriken->bThrowable);
	TestTrue(TEXT("Shuriken remains a MainHand item"),
		Shuriken->CompatibleEquipmentSlots.Contains(EGridEquipmentSlot::MainHand));
	TestTrue(TEXT("Repaired Shuriken is valid"), Shuriken->IsValidDefinition());
	TestTrue(TEXT("Repaired Shuriken provides its attack from CombatActions"),
		Shuriken->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));

	const FGridCombatActionDefinition* Attack = Shuriken->CombatActions.FindByPredicate(
		[](const FGridCombatActionDefinition& Candidate)
		{
			return Candidate.ActionId == FName(TEXT("Attack_Shuriken"));
		});
	TestNotNull(TEXT("Attack_Shuriken exists in CombatActions"), Attack);
	if (!Attack)
	{
		return false;
	}

	TestEqual(TEXT("Shuriken action is ranged"), Attack->ActionType, EGridCombatActionType::RangedAttack);
	TestEqual(TEXT("Shuriken action uses Equipment source"), Attack->SourcePolicy, EGridCombatActionSourcePolicy::Equipment);
	TestEqual(TEXT("Shuriken action targets first axial target"), Attack->TargetingPolicy, EGridCombatTargetingPolicy::FirstAxialTarget);
	TestEqual(TEXT("Shuriken range remains three cells"), Attack->RangeCells, 3);
	TestEqual(TEXT("Shuriken damage remains Physical"), Attack->OffensiveProfile.AttackDefinition.DamageType, EGridDamageType::Physical);
	TestEqual(TEXT("Shuriken subtype remains Piercing"),
		Attack->OffensiveProfile.AttackDefinition.PhysicalSubtype, EGridPhysicalDamageSubtype::Piercing);
	TestEqual(TEXT("Shuriken minimum damage remains one"), Attack->OffensiveProfile.AttackDefinition.MinDamage, 1);
	TestEqual(TEXT("Shuriken maximum damage remains four"), Attack->OffensiveProfile.AttackDefinition.MaxDamage, 4);
	TestEqual(TEXT("Shuriken scaling remains Dexterity"),
		Attack->OffensiveProfile.DamageScalingAttribute, EGridAttackScalingAttribute::Dexterity);
	TestEqual(TEXT("Shuriken profile range remains three"), Attack->OffensiveProfile.RangeCells, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352RuntimeFallbackRemovalTest,
	"Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.RuntimeFallbackRemoval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352RuntimeFallbackRemovalTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07352Normalization;

	FString ItemHeader;
	FString ItemSource;
	FString CatalogHeader;
	FString CatalogSource;
	FString PlayerCatalogSource;
	FString PlayerActionsSource;
	FString HotbarSource;

	TestTrue(TEXT("Item header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/GridItemDefinitionAsset.h"), ItemHeader));
	TestTrue(TEXT("Item source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridItemDefinitionAsset.cpp"), ItemSource));
	TestTrue(TEXT("Catalog header loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatActionCatalog.h"), CatalogHeader));
	TestTrue(TEXT("Catalog source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatActionCatalog.cpp"), CatalogSource));
	TestTrue(TEXT("Player catalog source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp"), PlayerCatalogSource));
	TestTrue(TEXT("Player actions source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActions.cpp"), PlayerActionsSource));
	TestTrue(TEXT("Hotbar source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentHotbar.cpp"), HotbarSource));

	TestFalse(TEXT("Legacy item attack flag declaration is gone"),
		ItemHeader.Contains(TEXT("bool bProvidesAttack =")));
	TestFalse(TEXT("Legacy item-level profile declaration is gone"),
		ItemHeader.Contains(TEXT("FGridOffensiveEquipmentProfile OffensiveProfile;")));
	TestFalse(TEXT("Legacy item helper is gone"),
		ItemHeader.Contains(TEXT("HasValidOffensiveProfile")));
	TestFalse(TEXT("Legacy item adapter declaration is gone"),
		CatalogHeader.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Legacy item adapter implementation is gone"),
		CatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Player catalog legacy adapter fallback is gone"),
		PlayerCatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Player attack resolver no longer reads item-level profiles"),
		PlayerActionsSource.Contains(TEXT("Definition->OffensiveProfile")));
	TestFalse(TEXT("Hotbar no longer reads item-level profiles"),
		HotbarSource.Contains(TEXT("Definition->OffensiveProfile")));
	TestFalse(TEXT("Item runtime no longer contains HasValidOffensiveProfile"),
		ItemSource.Contains(TEXT("HasValidOffensiveProfile")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
