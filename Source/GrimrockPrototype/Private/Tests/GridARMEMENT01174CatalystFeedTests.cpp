#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/GridEventFeedTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCatalystLaunchFeed,
	"Grimrock.RPG.ARMEMENT01.17.4A.CatalystLaunchFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCatalystLaunchFeed::RunTest(const FString&)
{
	const FText Hero = FText::FromString(TEXT("Elias"));
	const FGridHudEventEntry Launch = FGridCombatEventFeedFormatter::MakeCatalystLaunch(Hero, 9);
	TestEqual(TEXT("Combat category"), Launch.Category, EGridHudEventCategory::Combat);
	TestEqual(TEXT("Stable throw event"), Launch.EventCode, FName(TEXT("Combat.Surface.CatalystZone")));
	TestEqual(TEXT("Visible for the initiating player"), Launch.Visibility, EGridEventVisibility::Always);
	TestTrue(TEXT("Named character and affected cells"), Launch.Message.ToString().Contains(TEXT("Elias")) &&
		Launch.Message.ToString().Contains(TEXT("9")));

	FGridEventFeedBuffer Feed;
	FGridHudEventEntry Published;
	TestTrue(TEXT("One accepted throw entry"), Feed.TryPublish(Launch, true, &Published));
	TestEqual(TEXT("Only one logical entry"), Feed.GetEntries().Num(), 1);
	TestEqual(TEXT("Sequence follows normal combat feed"), Published.SequenceId, int64(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCatalystSurfaceDamageFeed,
	"Grimrock.RPG.ARMEMENT01.17.4A.SurfaceDamageFeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCatalystSurfaceDamageFeed::RunTest(const FString&)
{
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	Target.MagicalArmor = 0;
	const FGridAttackResult FireDamage = FGridCombatResolver::ResolveDirectDamage(
		Target, EGridDamageType::Fire, 6);
	FGridHudEventEntry Tick;
	TestTrue(TEXT("Effective periodic fire is reported"), FGridCombatEventFeedFormatter::BuildSurfaceDamage(
		FText::FromString(TEXT("Rat géant")), FireDamage, TEXT("Action_Mage_SurfaceCatalyst_01"), false, Tick));
	TestEqual(TEXT("Periodic code"), Tick.EventCode, FName(TEXT("Combat.Surface.FireDamage")));
	TestEqual(TEXT("Fire semantic"), Tick.Semantic, EGridHudEventSemantic::Fire);
	TestTrue(TEXT("Name preserved"), Tick.Header.ToString().Contains(TEXT("Rat géant")));
	TestTrue(TEXT("Correct HP delta"), Tick.Message.ToString().Contains(TEXT("20")) &&
		Tick.Message.ToString().Contains(TEXT("14")));

	FGridHudEventEntry Burst;
	TestTrue(TEXT("Immediate poison burst is distinct"), FGridCombatEventFeedFormatter::BuildSurfaceDamage(
		FText::FromString(TEXT("Rat géant")), FireDamage, TEXT("Action_Mage_SurfaceCatalyst_01"), true, Burst));
	TestEqual(TEXT("Burst code"), Burst.EventCode, FName(TEXT("Combat.Surface.CatalystBurstDamage")));

	Target.MagicalArmor = 20;
	const FGridAttackResult Absorbed = FGridCombatResolver::ResolveDirectDamage(
		Target, EGridDamageType::Fire, 6);
	FGridHudEventEntry Armor;
	TestTrue(TEXT("Absorbed fire still reaches feed"), FGridCombatEventFeedFormatter::BuildSurfaceDamage(
		FText::FromString(TEXT("Rat géant")), Absorbed, NAME_None, false, Armor));
	TestTrue(TEXT("Armor noted, zero fictitious HP damage"), Armor.Message.ToString().Contains(TEXT("0")) &&
		Armor.Details.Num() > 0);

	Target.ResistancePercent = 100;
	const FGridAttackResult Resisted = FGridCombatResolver::ResolveDirectDamage(
		Target, EGridDamageType::Fire, 6);
	FGridHudEventEntry Ignored;
	TestFalse(TEXT("Fully resisted fire must not invent damage"), FGridCombatEventFeedFormatter::BuildSurfaceDamage(
		FText::FromString(TEXT("Rat géant")), Resisted, NAME_None, false, Ignored));

	FGridEventFeedBuffer Feed;
	TestTrue(TEXT("Burst in shared event history"), Feed.TryPublish(Burst, true));
	TestTrue(TEXT("Tick in shared event history"), Feed.TryPublish(Tick, true));
	TestEqual(TEXT("Two distinct resolved effects"), Feed.GetEntries().Num(), 2);
	return true;
}
#endif
