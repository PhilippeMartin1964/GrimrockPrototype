#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace RPG0310
{
	struct FClassSpec
	{
		FName ClassId;
		const TCHAR* ObjectPath = nullptr;
	};

	const FClassSpec ClassSpecs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior") },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue") },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger") },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage") },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest") },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist") }
	};

	const TCHAR* AlchemistItemPaths[] = {
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Antidote.DA_Item_Antidote"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Bomb_Fire.DA_Item_Bomb_Fire"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Bomb_Toxic.DA_Item_Bomb_Toxic"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Fire.DA_Item_DefensiveElixir_Fire"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Ice.DA_Item_DefensiveElixir_Ice"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Lightning.DA_Item_DefensiveElixir_Lightning"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DefensiveElixir_Poison.DA_Item_DefensiveElixir_Poison"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Acid.DA_Item_Flask_Acid"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_CorrosiveCloud.DA_Item_Flask_CorrosiveCloud"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"),
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Panacea.DA_Item_Panacea")
	};

	TArray<URPGClassAsset*> LoadProductionClasses(FAutomationTestBase& Test)
	{
		TArray<URPGClassAsset*> Result;
		for (const FClassSpec& Spec : ClassSpecs)
		{
			URPGClassAsset* Asset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
			Test.TestNotNull(*FString::Printf(TEXT("Production class %s loads"), *Spec.ClassId.ToString()), Asset);
			if (Asset)
			{
				Test.TestEqual(*FString::Printf(TEXT("%s keeps its ClassId"), *Spec.ClassId.ToString()), Asset->ClassId, Spec.ClassId);
				Test.TestTrue(*FString::Printf(TEXT("%s remains structurally valid"), *Spec.ClassId.ToString()), Asset->IsValidDefinition());
				Result.Add(Asset);
			}
		}
		return Result;
	}

	int32 CountConceptualChoices(const URPGClassAsset& Asset, int32 RequiredLevel = INDEX_NONE)
	{
		int32 Ungrouped = 0;
		TSet<FName> ExclusiveGroups;
		for (const FRPGClassProgressionChoiceDefinition& Choice : Asset.ProgressionChoices)
		{
			if (RequiredLevel != INDEX_NONE && Choice.MinimumLevel != RequiredLevel)
			{
				continue;
			}
			if (Choice.ExclusiveChoiceGroupId.IsNone())
			{
				++Ungrouped;
			}
			else
			{
				ExclusiveGroups.Add(Choice.ExclusiveChoiceGroupId);
			}
		}
		return Ungrouped + ExclusiveGroups.Num();
	}

	void GatherResolvableRequirements(const URPGClassAsset& Asset, TSet<FName>& OutRequirements)
	{
		OutRequirements.Reset();
		OutRequirements.Add(Asset.ClassId);
		for (const FRPGClassProgressionLevelGrant& Grant : Asset.ProgressionLevelGrants)
		{
			OutRequirements.Append(Grant.GrantedRequirementIds);
		}
		for (const FRPGClassProgressionChoiceDefinition& Choice : Asset.ProgressionChoices)
		{
			OutRequirements.Add(Choice.ChoiceId);
			OutRequirements.Append(Choice.GrantedRequirementIds);
		}
	}

	bool HasSaneActionEconomy(const FGridCombatActionDefinition& Action)
	{
		return Action.IsValid() &&
			Action.ActionPointCost >= 1 && Action.ActionPointCost <= 4 &&
			Action.ResourceCosts.ManaCost >= 0 && Action.ResourceCosts.ManaCost <= 15 &&
			Action.RangeCells >= 0 && Action.RangeCells <= 6 &&
			Action.CooldownRounds >= 0 && Action.CooldownRounds <= 5 &&
			Action.AreaRadiusCells >= 0 && Action.AreaRadiusCells <= 2;
	}

	void GatherAppliedStatusIds(const FGridCombatActionDefinition& Action, TSet<FName>& OutIds)
	{
		for (const FGridCombatStatusApplicationProfile& Status : Action.StatusApplications)
		{
			if (!Status.StatusEffectId.IsNone())
			{
				OutIds.Add(Status.StatusEffectId);
			}
		}
		for (const FGridCombatSurfaceEffectProfile& Surface : Action.SurfaceEffects)
		{
			for (const FGridCombatStatusApplicationProfile& Status : Surface.PeriodicStatusApplications)
			{
				if (!Status.StatusEffectId.IsNone())
				{
					OutIds.Add(Status.StatusEffectId);
				}
			}
		}
	}

	bool ValidateStatusAsset(FAutomationTestBase& Test, FName EffectId)
	{
		const FString Path = FString::Printf(
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_%s.DA_%s"),
			*EffectId.ToString(), *EffectId.ToString());
		UGridStatusEffectDefinitionAsset* Status = LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Path);
		return Test.TestTrue(*FString::Printf(TEXT("Applied status %s resolves to a valid production asset"), *EffectId.ToString()),
			IsValid(Status) && Status->EffectId == EffectId && Status->IsValidDefinition());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0310ProductionClassSetTest,
	"Grimrock.RPG.RPG03.10.Balance.ProductionClassSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0310ProductionClassSetTest::RunTest(const FString&)
{
	TArray<URPGClassAsset*> Classes = RPG0310::LoadProductionClasses(*this);
	TestEqual(TEXT("Exactly six production RPG classes load"), Classes.Num(), 6);

	TSet<FName> SeenClassIds;
	int32 TotalConceptualTalents = 0;
	for (const URPGClassAsset* ClassAsset : Classes)
	{
		if (!ClassAsset)
		{
			continue;
		}
		TestFalse(TEXT("ClassId is globally unique"), SeenClassIds.Contains(ClassAsset->ClassId));
		SeenClassIds.Add(ClassAsset->ClassId);
		const int32 ConceptualCount = RPG0310::CountConceptualChoices(*ClassAsset);
		TestEqual(*FString::Printf(TEXT("%s exposes fifteen conceptual talents"), *ClassAsset->ClassId.ToString()), ConceptualCount, 15);
		TotalConceptualTalents += ConceptualCount;
	}
	TestEqual(TEXT("The six classes expose exactly ninety conceptual talents"), TotalConceptualTalents, 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0310TierEconomyTest,
	"Grimrock.RPG.RPG03.10.Balance.TierEconomy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0310TierEconomyTest::RunTest(const FString&)
{
	const TArray<int32> Tiers = { 2, 6, 10, 14, 18 };
	const TSet<int32> TierSet(Tiers);
	for (const URPGClassAsset* ClassAsset : RPG0310::LoadProductionClasses(*this))
	{
		if (!ClassAsset)
		{
			continue;
		}

		int32 PositiveGrantCount = 0;
		int32 TotalChoicePoints = 0;
		TSet<int32> PositiveGrantLevels;
		for (const FRPGClassProgressionLevelGrant& Grant : ClassAsset->ProgressionLevelGrants)
		{
			if (Grant.ChoicePointsGranted <= 0)
			{
				continue;
			}
			++PositiveGrantCount;
			TotalChoicePoints += Grant.ChoicePointsGranted;
			PositiveGrantLevels.Add(Grant.Level);
			TestEqual(*FString::Printf(TEXT("%s grants one point at each talent tier"), *ClassAsset->ClassId.ToString()),
				Grant.ChoicePointsGranted, 1);
			TestTrue(*FString::Printf(TEXT("%s only grants talent points on canonical tiers"), *ClassAsset->ClassId.ToString()),
				TierSet.Contains(Grant.Level));
		}
		TestEqual(*FString::Printf(TEXT("%s has five positive choice-point grants"), *ClassAsset->ClassId.ToString()), PositiveGrantCount, 5);
		TestEqual(*FString::Printf(TEXT("%s grants five total talent points"), *ClassAsset->ClassId.ToString()), TotalChoicePoints, 5);
		TestEqual(*FString::Printf(TEXT("%s covers all five canonical tier levels"), *ClassAsset->ClassId.ToString()), PositiveGrantLevels.Num(), 5);

		for (const int32 Tier : Tiers)
		{
			TestEqual(
				*FString::Printf(TEXT("%s exposes three conceptual choices at level %d"), *ClassAsset->ClassId.ToString(), Tier),
				RPG0310::CountConceptualChoices(*ClassAsset, Tier), 3);
		}
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset->ProgressionChoices)
		{
			TestTrue(*FString::Printf(TEXT("%s choice %s sits on a canonical tier"), *ClassAsset->ClassId.ToString(), *Choice.ChoiceId.ToString()),
				TierSet.Contains(Choice.MinimumLevel));
			TestEqual(*FString::Printf(TEXT("%s choice %s costs one point"), *ClassAsset->ClassId.ToString(), *Choice.ChoiceId.ToString()),
				Choice.PointCost, 1);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0310RequirementClosureTest,
	"Grimrock.RPG.RPG03.10.Coherence.RequirementClosure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0310RequirementClosureTest::RunTest(const FString&)
{
	TSet<FName> GlobalActionIds;
	for (const URPGClassAsset* ClassAsset : RPG0310::LoadProductionClasses(*this))
	{
		if (!ClassAsset)
		{
			continue;
		}
		TSet<FName> Requirements;
		RPG0310::GatherResolvableRequirements(*ClassAsset, Requirements);
		for (const FGridCombatActionDefinition& Action : ClassAsset->CombatActions)
		{
			TestFalse(*FString::Printf(TEXT("Class ActionId %s is globally unique"), *Action.ActionId.ToString()),
				GlobalActionIds.Contains(Action.ActionId));
			GlobalActionIds.Add(Action.ActionId);
			for (const FName Requirement : Action.Requirements)
			{
				TestTrue(*FString::Printf(TEXT("%s action %s requirement %s resolves in its class progression"),
					*ClassAsset->ClassId.ToString(), *Action.ActionId.ToString(), *Requirement.ToString()),
					Requirements.Contains(Requirement));
			}
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0310ActionEconomyTest,
	"Grimrock.RPG.RPG03.10.Balance.ActionEconomy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0310ActionEconomyTest::RunTest(const FString&)
{
	int32 CheckedActions = 0;
	for (const URPGClassAsset* ClassAsset : RPG0310::LoadProductionClasses(*this))
	{
		if (!ClassAsset)
		{
			continue;
		}
		for (const FGridCombatActionDefinition& Action : ClassAsset->CombatActions)
		{
			TestTrue(*FString::Printf(TEXT("%s action %s stays inside RPG03 economy bounds"),
				*ClassAsset->ClassId.ToString(), *Action.ActionId.ToString()), RPG0310::HasSaneActionEconomy(Action));
			++CheckedActions;
		}
	}

	int32 CheckedQuickItems = 0;
	for (const TCHAR* Path : RPG0310::AlchemistItemPaths)
	{
		UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(nullptr, Path);
		if (!TestNotNull(*FString::Printf(TEXT("Alchemist QuickItem loads: %s"), Path), Item))
		{
			continue;
		}
		FGridCombatActionDefinition Action;
		if (!TestTrue(*FString::Printf(TEXT("%s builds a QuickItem action"), *Item->ItemDefinitionId.ToString()),
			Item->BuildQuickItemCombatActionDefinition(Action)))
		{
			continue;
		}
		TestTrue(*FString::Printf(TEXT("%s stays inside RPG03 economy bounds"), *Action.ActionId.ToString()),
			RPG0310::HasSaneActionEconomy(Action));
		TestEqual(TEXT("Materialized Alchemist QuickItems consume exactly one source item"),
			Action.ResourceCosts.SourceItemQuantityCost, 1);
		++CheckedQuickItems;
	}
	TestTrue(TEXT("Global balance test inspects at least one class action"), CheckedActions > 0);
	TestEqual(TEXT("Global balance test inspects all eleven Alchemist QuickItems"), CheckedQuickItems, 11);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0310AppliedStatusClosureTest,
	"Grimrock.RPG.RPG03.10.Coherence.AppliedStatusClosure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0310AppliedStatusClosureTest::RunTest(const FString&)
{
	TSet<FName> AppliedStatusIds;
	for (const URPGClassAsset* ClassAsset : RPG0310::LoadProductionClasses(*this))
	{
		if (!ClassAsset)
		{
			continue;
		}
		for (const FGridCombatActionDefinition& Action : ClassAsset->CombatActions)
		{
			RPG0310::GatherAppliedStatusIds(Action, AppliedStatusIds);
		}
	}
	for (const TCHAR* Path : RPG0310::AlchemistItemPaths)
	{
		if (UGridItemDefinitionAsset* Item = LoadObject<UGridItemDefinitionAsset>(nullptr, Path))
		{
			FGridCombatActionDefinition Action;
			if (Item->BuildQuickItemCombatActionDefinition(Action))
			{
				RPG0310::GatherAppliedStatusIds(Action, AppliedStatusIds);
			}
		}
	}

	TestTrue(TEXT("At least one RPG03 action applies a status"), !AppliedStatusIds.IsEmpty());
	for (const FName EffectId : AppliedStatusIds)
	{
		RPG0310::ValidateStatusAsset(*this, EffectId);
	}
	return true;
}

#endif
