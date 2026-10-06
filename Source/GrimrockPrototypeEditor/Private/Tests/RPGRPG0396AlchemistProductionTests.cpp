#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396CProductionClassTest,
	"Grimrock.RPG.RPG03.9.6C.ProductionClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396CProductionClassTest::RunTest(const FString&)
{
	URPGClassAsset* Alchemist =
		LoadObject<URPGClassAsset>(nullptr, FRPGAlchemistAuthoring::AlchemistAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Alchemist loads"), Alchemist))
	{
		return false;
	}
	TestEqual(TEXT("Production Alchemist ClassId"), Alchemist->ClassId, FName(TEXT("Alchemist")));
	TestTrue(TEXT("Production Alchemist is structurally valid"), Alchemist->IsValidDefinition());
	TestEqual(TEXT("Production Alchemist has fifteen Choice records"), Alchemist->ProgressionChoices.Num(), 15);
	TestEqual(TEXT("Catalyst is the only direct class action"), Alchemist->CombatActions.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396CProductionItemsTest,
	"Grimrock.RPG.RPG03.9.6C.ProductionItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396CProductionItemsTest::RunTest(const FString&)
{
	TArray<FName> Ids;
	TArray<FName> B2;
	FRPGAlchemistAuthoring::GetB1ItemIds(Ids);
	FRPGAlchemistAuthoring::GetB2ItemIds(B2);
	Ids.Append(B2);
	TestEqual(TEXT("Production authoring owns eleven QuickItems"), Ids.Num(), 11);

	for (const FName Id : Ids)
	{
		const FString Path = FRPGAlchemistAuthoring::GetItemObjectPath(Id);
		UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
		if (!TestTrue(*FString::Printf(TEXT("Production %s loads valid"), *Id.ToString()),
			IsValid(Item) && Item->ItemDefinitionId == Id && Item->IsValidDefinition()))
		{
			continue;
		}
		FGridCombatActionDefinition Action;
		TestTrue(*FString::Printf(TEXT("Production %s builds its QuickItem action"), *Id.ToString()),
			Item->BuildQuickItemCombatActionDefinition(Action));
		TestEqual(TEXT("Production QuickItem preserves canonical action override"),
			Action.ActionId, Item->QuickItemActionIdOverride);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396CProductionStatusesTest,
	"Grimrock.RPG.RPG03.9.6C.ProductionStatuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396CProductionStatusesTest::RunTest(const FString&)
{
	TArray<FName> Ids;
	TArray<FName> B2;
	FRPGAlchemistAuthoring::GetB1StatusIds(Ids);
	FRPGAlchemistAuthoring::GetB2StatusIds(B2);
	Ids.Append(B2);
	TestEqual(TEXT("Production authoring owns six Alchemist statuses"), Ids.Num(), 6);

	for (const FName Id : Ids)
	{
		const FString Path = FRPGAlchemistAuthoring::GetStatusObjectPath(Id);
		UGridStatusEffectDefinitionAsset* Status =
			LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Path);
		TestTrue(*FString::Printf(TEXT("Production %s loads valid"), *Id.ToString()),
			IsValid(Status) && Status->EffectId == Id && Status->IsValidDefinition());
	}

	const FString BurningPath = FRPGAlchemistAuthoring::GetStatusObjectPath(TEXT("Status_Burning"));
	UGridStatusEffectDefinitionAsset* Burning =
		LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *BurningPath);
	TestTrue(TEXT("Alchemist reuses existing shared Burning status"),
		IsValid(Burning) && Burning->EffectId == TEXT("Status_Burning") && Burning->IsValidDefinition());
	return true;
}

#endif
