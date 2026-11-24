#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace GridTD07352Normalization
{
	bool LoadProjectFile(const TCHAR* RelativePath, FString& OutText)
	{
		return FFileHelper::LoadFileToString(OutText, *FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352SchemaAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.SchemaAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352SchemaAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* ItemClass = UGridNonArmamentDefinitionAsset::StaticClass();
	TestNotNull(TEXT("Item definition class exists"), ItemClass);
	if (!ItemClass)
	{
		return false;
	}

	TestNull(TEXT("bProvidesAttack property is physically removed"), ItemClass->FindPropertyByName(TEXT("bProvidesAttack")));
	TestNull(TEXT("item-level OffensiveProfile property is physically removed"), ItemClass->FindPropertyByName(TEXT("OffensiveProfile")));

	const FProperty* CombatActions = ItemClass->FindPropertyByName(TEXT("ActionBindings"));
	TestNotNull(TEXT("ActionBindings own item action identity"), CombatActions);
	TestTrue(TEXT("ActionBindings are durable authoring data"), CombatActions && !CombatActions->HasAnyPropertyFlags(CPF_Transient));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352CombatActionsOnlyBehaviorTest, "Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.CombatActionsOnlyBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352CombatActionsOnlyBehaviorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridTD07352Normalization;

	FGridCanonicalRegistryFixture Fixture;
	if (!TestTrue(TEXT("Configured item system"), Fixture.bReady)) return false;
	FGridCombatActionDefinition Action;
	const FName Bomb(TEXT("Item_Bomb_Fire"));
	FGridResolvedItem Item;
	if (!TestTrue(TEXT("Bomb has exactly one authored action binding"),
			Fixture.Resolve(Bomb, 0, Item) && Item.AvailableActions.Num() == 1)) return false;
	const FName ActionId = Item.AvailableActions[0].ActionId;
	TestFalse(TEXT("A passive item cannot borrow the action"),
		Fixture.Registry->ResolveDetailedNonArmAction(TEXT("Stone"), 0, ActionId, Action));
	TestTrue(TEXT("The authored source can resolve its action"),
		Fixture.Registry->ResolveDetailedNonArmAction(Bomb, 0, ActionId, Action));
	TestEqual(TEXT("QuickItem consumes one source item"), Action.ResourceCosts.SourceItemQuantityCost, 1);
	TestEqual(TEXT("Configured attack stays bound to the same identity"), Action.ActionId, ActionId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352ShurikenAssetAuthorityTest, "Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.ShurikenAssetAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTD07352ShurikenAssetAuthorityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    // Production shuriken is now the unique N0 non-armament identity.
    // No historical DA_Weapon_Shuriken is needed to exercise throw policy.
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem"));
    if (!TestNotNull(TEXT("Canonical root loads"), Root))
        return false;
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Canonical item registry validates"),
        Registry->RegisterItemSystem(Root, Errors)))
        return false;
    FGridResolvedItem Shuriken;
    if (!TestTrue(TEXT("Real N0 Shuriken resolves"),
        Registry->ResolveItem(TEXT("Shuriken"), 0, Shuriken)))
        return false;
    TestTrue(TEXT("Resolved Shuriken is valid"), Shuriken.IsValid());
    TestFalse(TEXT("Physical Shuriken is not one of the 22 ARM weapons"), Shuriken.bIsArmament);
    TestEqual(TEXT("Shuriken keeps canonical N0"), Shuriken.ItemLevel, 0);
    TestEqual(TEXT("Shuriken keeps stable ItemId"), Shuriken.ItemId, FName(TEXT("Shuriken")));
    TestTrue(TEXT("Shuriken physical throw is authored canonically"),
        Shuriken.Tags.Contains(FName(TEXT("Throwable"))) &&
        Shuriken.GenericThrow.bCombatRecoverable &&
        FMath::IsFinite(Shuriken.GenericThrow.InitialSpeed) &&
        Shuriken.GenericThrow.InitialSpeed > KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Shuriken world mesh comes from the canonical visual source"),
        Shuriken.WorldMesh.LoadSynchronous() != nullptr);
    FGridResolvedItem WrongLevel;
    TestFalse(TEXT("N1 Shuriken cannot borrow an ARM level"),
        Registry->ResolveItem(TEXT("Shuriken"), 1, WrongLevel));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTD07352RuntimeFallbackRemovalTest, "Grimrock.TechnicalDebt.TD07_3_5_2.Normalization.RuntimeFallbackRemoval",
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
	TestTrue(TEXT("Catalog header loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatActionCatalog.h"), CatalogHeader));
	TestTrue(TEXT("Catalog source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatActionCatalog.cpp"), CatalogSource));
	TestTrue(TEXT("Player catalog source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActionCatalog.cpp"), PlayerCatalogSource));
	TestTrue(TEXT("Player actions source loads"),
		LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/Combat/GridTurnManagerPlayerActions.cpp"), PlayerActionsSource));
	TestTrue(
		TEXT("Hotbar source loads"), LoadProjectFile(TEXT("Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentHotbar.cpp"), HotbarSource));

	TestFalse(TEXT("Legacy item attack flag declaration is gone"), ItemHeader.Contains(TEXT("bool bProvidesAttack =")));
	TestFalse(TEXT("Legacy item-level profile declaration is gone"), ItemHeader.Contains(TEXT("FGridOffensiveEquipmentProfile OffensiveProfile;")));
	TestFalse(TEXT("Legacy item helper is gone"), ItemHeader.Contains(TEXT("HasValidOffensiveProfile")));
	TestFalse(TEXT("Legacy item adapter declaration is gone"), CatalogHeader.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Legacy item adapter implementation is gone"), CatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Player catalog legacy adapter fallback is gone"), PlayerCatalogSource.Contains(TEXT("MakeLegacyEquipmentAttackDefinition")));
	TestFalse(TEXT("Player attack resolver no longer reads item-level profiles"), PlayerActionsSource.Contains(TEXT("Definition->OffensiveProfile")));
	TestFalse(TEXT("Hotbar no longer reads item-level profiles"), HotbarSource.Contains(TEXT("Definition->OffensiveProfile")));
	TestFalse(TEXT("Item runtime no longer contains HasValidOffensiveProfile"), ItemSource.Contains(TEXT("HasValidOffensiveProfile")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
