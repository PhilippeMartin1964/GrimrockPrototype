#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Tests/GridCanonicalRegistryFixture.h"

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
	EAutomationTestFlags::Editobool FRPG0396CProductionItemsTest::RunTest(const FString&)
{
    TArray<FName> Ids, B2;
    FRPGAlchemistAuthoring::GetB1ItemIds(Ids);
    FRPGAlchemistAuthoring::GetB2ItemIds(B2);
    Ids.Append(B2);
    TestEqual(TEXT("Production authoring owns eleven QuickItems"), Ids.Num(), 11);

    // These exact eleven bindings are the physical ItemSystem contract:
    // each historical DA_Item_* package is now irrelevant to execution.
    const TMap<FName, FName> ExpectedActions = {
        { TEXT("Item_Antidote"), TEXT("Action_Alchemist_Antidote") },
        { TEXT("Item_Bomb_Fire"), TEXT("Action_Alchemist_FireBomb") },
        { TEXT("Item_Bomb_Toxic"), TEXT("Action_Alchemist_ToxicBomb") },
        { TEXT("Item_DefensiveElixir_Fire"), TEXT("Action_Alchemist_DefensiveElixir_Fire") },
        { TEXT("Item_DefensiveElixir_Ice"), TEXT("Action_Alchemist_DefensiveElixir_Ice") },
        { TEXT("Item_DefensiveElixir_Lightning"), TEXT("Action_Alchemist_DefensiveElixir_Lightning") },
        { TEXT("Item_DefensiveElixir_Poison"), TEXT("Action_Alchemist_DefensiveElixir_Poison") },
        { TEXT("Item_Flask_Acid"), TEXT("Action_Alchemist_AcidFlask") },
        { TEXT("Item_Flask_CorrosiveCloud"), TEXT("Action_Alchemist_CorrosiveCloud") },
        { TEXT("Item_Flask_Oil"), TEXT("Action_Alchemist_OilSlick") },
        { TEXT("Item_Panacea"), TEXT("Action_Alchemist_Panacea") }
    };
    if (!TestEqual(TEXT("Eleven exact native QuickItem bindings"), ExpectedActions.Num(), 11))
        return false;

    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production canonical ItemSystem registered"), Fixture.bReady))
        return false;
    TSet<FName> Seen;
    int32 Validated = 0;
    for (const FName Id : Ids)
    {
        TestFalse(TEXT("Authoring QuickItem IDs are unique"), Seen.Contains(Id));
        Seen.Add(Id);
        const FName* ExpectedActionId = ExpectedActions.Find(Id);
        if (!TestNotNull(*FString::Printf(TEXT("%s has a canonical action contract"), *Id.ToString()),
            ExpectedActionId))
            continue;

        FGridResolvedItem Physical;
        if (!TestTrue(*FString::Printf(TEXT("Canonical %s resolves at non-ARM N0"), *Id.ToString()),
            Fixture.Resolve(Id, 0, Physical)))
            continue;
        TestFalse(TEXT("Alchemy consumable is not an ARM"), Physical.bIsArmament);
        TestTrue(TEXT("QuickItem has valid physical stack capacity"), Physical.MaxStack >= 1);
        TestTrue(TEXT("Exact action is bound to the physical ItemId"),
            Physical.AvailableActions.ContainsByPredicate(
                [ExpectedActionId](const FGridItemActionBinding& Binding)
                {
                    return Binding.ActionId == *ExpectedActionId;
                }));
        FGridCombatActionDefinition Action;
        if (!TestTrue(*FString::Printf(TEXT("Detailed %s combat action resolves"), *Id.ToString()),
            Fixture.Registry->ResolveDetailedNonArmAction(Id, 0, *ExpectedActionId, Action)))
            continue;
        TestTrue(TEXT("Resolved QuickItem action remains valid"), Action.IsValid());
        TestEqual(TEXT("ActionId comes from canonical registry"),
            Action.ActionId, *ExpectedActionId);
        TestEqual(TEXT("Alchemy QuickItem consumes one physical item"),
            Action.ResourceCosts.SourceItemQuantityCost, 1);
        ++Validated;
    }
    TestEqual(TEXT("All eleven native Alchemist items and detailed actions inspected"), Validated, 11);
    return !HasAnyErrors();
}

QuickItemActionIdOverride);
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
