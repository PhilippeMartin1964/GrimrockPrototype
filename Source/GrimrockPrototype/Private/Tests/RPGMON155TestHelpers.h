#pragma once

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "RPG/RPGLevelUpService.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/RPGLevelUpWidget.h"

namespace
{
	struct FMON155RuntimeStateGuard
	{
		FMON155RuntimeStateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}

		~FMON155RuntimeStateGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}
	};

	FGridCombatActionDefinition MakeMON155ChoiceAction()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Ability_MON155_ChoiceA");
		Action.DisplayName = FText::FromString(TEXT("MON15.5 Choice A"));
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		Action.EffectProfile.RestoreHealth = 1;
		Action.Requirements.Add(TEXT("Choice_A"));
		return Action;
	}

	URPGClassAsset* MakeMON155Class(UObject* Outer)
	{
		URPGClassAsset* ClassDefinition = NewObject<URPGClassAsset>(Outer);
		ClassDefinition->ClassId = TEXT("MON155_Fighter");
		ClassDefinition->DisplayName = FText::FromString(TEXT("MON15.5 Fighter"));
		ClassDefinition->BaseAttributes = FRPGAttributes{ 12, 12, 12, 10, 10, 10 };
		ClassDefinition->HealthAtLevelOne = 20;
		ClassDefinition->HealthPerLevel = 5;
		ClassDefinition->ManaAtLevelOne = 5;
		ClassDefinition->ManaPerLevel = 2;
		ClassDefinition->BasePhysicalArmor = 2;
		ClassDefinition->BaseMagicalArmor = 1;

		FRPGClassProgressionLevelGrant Level2;
		Level2.Level = 2;
		Level2.ChoicePointsGranted = 1;
		Level2.GrantedRequirementIds.Add(TEXT("Feature_Level2"));
		ClassDefinition->ProgressionLevelGrants.Add(Level2);

		FRPGClassProgressionLevelGrant Level3;
		Level3.Level = 3;
		Level3.ChoicePointsGranted = 1;
		ClassDefinition->ProgressionLevelGrants.Add(Level3);

		FRPGClassProgressionLevelGrant Level4;
		Level4.Level = 4;
		Level4.ChoicePointsGranted = 2;
		Level4.GrantedRequirementIds.Add(TEXT("Feature_Level4"));
		ClassDefinition->ProgressionLevelGrants.Add(Level4);

		FRPGClassProgressionChoiceDefinition ChoiceA;
		ChoiceA.ChoiceId = TEXT("Choice_A");
		ChoiceA.DisplayName = FText::FromString(TEXT("Choice A"));
		ChoiceA.MinimumLevel = 2;
		ChoiceA.PointCost = 1;
		ChoiceA.GrantedRequirementIds.Add(TEXT("Feature_A"));
		ClassDefinition->ProgressionChoices.Add(ChoiceA);

		FRPGClassProgressionChoiceDefinition ChoiceB;
		ChoiceB.ChoiceId = TEXT("Choice_B");
		ChoiceB.DisplayName = FText::FromString(TEXT("Choice B"));
		ChoiceB.MinimumLevel = 3;
		ChoiceB.PointCost = 1;
		ChoiceB.PrerequisiteChoiceIds.Add(TEXT("Choice_A"));
		ChoiceB.GrantedRequirementIds.Add(TEXT("Feature_B"));
		ClassDefinition->ProgressionChoices.Add(ChoiceB);

		FRPGClassProgressionChoiceDefinition ChoiceC;
		ChoiceC.ChoiceId = TEXT("Choice_C");
		ChoiceC.DisplayName = FText::FromString(TEXT("Choice C"));
		ChoiceC.MinimumLevel = 4;
		ChoiceC.PointCost = 2;
		ChoiceC.PrerequisiteChoiceIds.Add(TEXT("Choice_B"));
		ChoiceC.GrantedRequirementIds.Add(TEXT("Feature_C"));
		ClassDefinition->ProgressionChoices.Add(ChoiceC);

		FRPGClassProgressionChoiceDefinition Expensive;
		Expensive.ChoiceId = TEXT("Choice_Expensive");
		Expensive.DisplayName = FText::FromString(TEXT("Expensive"));
		Expensive.MinimumLevel = 2;
		Expensive.PointCost = 3;
		ClassDefinition->ProgressionChoices.Add(Expensive);

		ClassDefinition->CombatActions.Add(MakeMON155ChoiceAction());
		return ClassDefinition;
	}

	FGridCharacterInventoryState MakeMON155Character(URPGClassAsset* ClassDefinition, int32 Level, int32 Experience, const TCHAR* DisplayName = TEXT("Elias"))
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(DisplayName);
		Character.ClassId = ClassDefinition->ClassId;
		Character.ClassDisplayName = ClassDefinition->DisplayName;
		Character.ClassDefinition = ClassDefinition;
		Character.Level = Level;
		Character.Experience = Experience;
		Character.Attributes = ClassDefinition->BaseAttributes;
		Character.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, ClassDefinition, Level);
		Character.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Character.DerivedStats, ClassDefinition);
		return Character;
	}

	UGridPartyInventoryComponent* MakeMON155Inventory(int32 CharacterLevel, int32 CharacterExperience, URPGClassAsset*& OutClassDefinition)
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		OutClassDefinition = MakeMON155Class(Component);
		Component->PartyInventoryState.ActiveCharacters.Add(MakeMON155Character(OutClassDefinition, CharacterLevel, CharacterExperience));
		Component->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Component;
	}

	TSet<FName> GetMON155RuntimeRequirements(UGridPartyInventoryComponent* Component, int32 CharacterIndex)
	{
		TSet<FName> Requirements;
		if (!Component || !Component->PartyInventoryState.ActiveCharacters.IsValidIndex(CharacterIndex))
		{
			return Requirements;
		}

		FRPGClassProgressionTransactionService::RefreshCharacterProjection(Component, CharacterIndex);
		const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[CharacterIndex];
		FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(Character.CharacterId, Requirements);
		return Requirements;
	}

	FGridAvailableCombatAction BuildMON155ChoiceActionAvailability(UGridPartyInventoryComponent* Component, URPGClassAsset* ClassDefinition)
	{
		const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
		FGridCombatActionCatalogContext Context;
		Context.CharacterIndex = 0;
		Context.CharacterId = Character.CharacterId;
		Context.bCombatActive = true;
		Context.bCharacterDefeated = false;
		Context.bActiveCombatant = true;
		Context.bPartyBusy = false;
		Context.bEnableClassActionExecutors = true;
		Context.RemainingActionPoints = 4;
		Context.CurrentHealth = 5;
		Context.MaximumHealth = 10;
		Context.CurrentMana = 10;
		Context.MaximumMana = 10;
		Context.SatisfiedRequirements.Add(ClassDefinition->ClassId);

		FGridCombatActionContribution Contribution;
		Contribution.Definition = ClassDefinition->CombatActions[0];
		Contribution.SourceDefinitionId = ClassDefinition->ClassId;
		Contribution.AvailableSourceQuantity = 1;

		TArray<FGridAvailableCombatAction> Actions;
		FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
		return Actions.Num() == 1 ? Actions[0] : FGridAvailableCombatAction();
	}
}
