#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395CProductionClassTest,
	"Grimrock.RPG.RPG03.9.5C.ProductionClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395CProductionClassTest::RunTest(const FString&)
{
	URPGClassAsset* Priest = LoadObject<URPGClassAsset>(nullptr, FRPGPriestAuthoring::PriestAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Priest loads"), Priest))
	{
		return false;
	}
	TestEqual(TEXT("Production Priest ClassId is Priest"), Priest->ClassId, FName(TEXT("Priest")));
	TestTrue(TEXT("Production Priest is structurally valid"), Priest->IsValidDefinition());
	TestEqual(TEXT("Production Priest has fifteen Choice records"), Priest->ProgressionChoices.Num(), 15);
	TestEqual(TEXT("Production Priest has fourteen active actions"), Priest->CombatActions.Num(), 14);

	for (const FName ChoiceId : {
		FName(TEXT("Talent_Priest_Restoration_Miracle")),
		FName(TEXT("Talent_Priest_Protection_DivineBastion")),
		FName(TEXT("Talent_Priest_Exorcism_MajorExorcism")) })
	{
		TestNotNull(*FString::Printf(TEXT("Production Priest contains terminal talent %s"), *ChoiceId.ToString()),
			Priest->FindProgressionChoice(ChoiceId));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395CProductionStatusesTest,
	"Grimrock.RPG.RPG03.9.5C.ProductionStatuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395CProductionStatusesTest::RunTest(const FString&)
{
	TArray<FName> StatusIds;
	FRPGPriestAuthoring::GetRequiredPriestStatusIds(StatusIds);
	TestEqual(TEXT("Priest materializes seven owned statuses"), StatusIds.Num(), 7);

	for (const FName EffectId : StatusIds)
	{
		const FString Path = FRPGPriestAuthoring::GetStatusObjectPath(EffectId);
		UGridStatusEffectDefinitionAsset* Status = LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Path);
		TestTrue(*FString::Printf(TEXT("Production %s loads and is structurally valid"), *EffectId.ToString()),
			IsValid(Status) && Status->EffectId == EffectId && Status->IsValidDefinition());
	}

	UGridStatusEffectDefinitionAsset* Sanctuary =
		LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *FRPGPriestAuthoring::GetStatusObjectPath(TEXT("Status_Sanctuary")));
	TestTrue(TEXT("Production Sanctuary keeps targetability + own-damage break contract"),
		IsValid(Sanctuary) && Sanctuary->Control.bBlockDirectHostileTargeting &&
		Sanctuary->bExpireAtOwnerNextActivation && Sanctuary->CombatReactions.Num() == 1 &&
		Sanctuary->CombatReactions[0].bRequireOwnerAsEventSource &&
		Sanctuary->CombatReactions[0].bRequireAppliedDamage &&
		Sanctuary->CombatReactions[0].bConsumeOwningStatus);

	UGridStatusEffectDefinitionAsset* Banished =
		LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *FRPGPriestAuthoring::GetStatusObjectPath(TEXT("Status_Banished")));
	TestTrue(TEXT("Production Banished keeps one-Turn skip activation contract"),
		IsValid(Banished) && Banished->DurationUnit == EGridStatusEffectDurationUnit::Turns &&
		Banished->DefaultDuration == 1 && Banished->Control.bSkipActivation);
	return true;
}

#endif
