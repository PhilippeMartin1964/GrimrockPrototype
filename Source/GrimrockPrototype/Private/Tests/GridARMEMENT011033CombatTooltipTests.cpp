#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridEventFeedWidget.h"
#include "UI/GridCombatTooltipWidget.h"
#include "UI/UILocalizationText.h"

namespace ARMEMENT011033
{
	UGridItemDefinitionAsset* Shield()
	{
		return LoadObject<UGridItemDefinitionAsset>(nullptr,
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
	}

	UGridPartyInventoryComponent* EquipShield(int32 BaseArmor)
	{
		UGridItemDefinitionAsset* Definition = Shield();
		if (!Definition) return nullptr;
		UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
		Inventory->InitializeDefaultPartyIfNeeded();
		if (!Inventory->IsValidCharacterIndex(0) || !Inventory->RegisterItemDefinition(Definition)) return nullptr;
		Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor = BaseArmor;
		Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth = 20;
		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = Definition->ItemDefinitionId;
		Item.Quantity = 1;
		if (!Inventory->ApplyItemDefinitionToInstance(Item) || !Inventory->AddItemToCharacterInventory(0, Item)) return nullptr;
		const TArray<FGridInventorySlot>& Slots = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
		int32 Slot = INDEX_NONE;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (!Slots[Index].IsEmpty() && Slots[Index].Item.RuntimeObjectId == Item.RuntimeObjectId)
			{
				Slot = Index;
				break;
			}
		}
		if (Slot == INDEX_NONE || !Inventory->EquipItemFromInventorySlot(0, Slot, EGridEquipmentSlot::OffHand)) return nullptr;
		return Inventory;
	}

	FGridHudEventEntry MakeTooltipEntry(const FGridAttackResult& Hit)
	{
		FGridHudEventEntry Entry;
		Entry.bHasCombatTooltipTrace = true;
		Entry.CombatAttacker = FText::FromString(TEXT("Rat géant"));
		Entry.CombatDefender = FText::FromString(TEXT("Elias"));
		Entry.CombatTooltipTrace = Hit.Trace;
		Entry.CombatTooltipTrace.bValid = true;
		return Entry;
	}

	FGridAttackResult Apply(UGridPartyInventoryComponent* Inventory, int32 Damage)
	{
		FGridAttackTargetStats Target;
		Target.CurrentHealth = Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
		Target.PhysicalArmor = Inventory->GetEffectiveCurrentPhysicalArmor(0);
		FGridAttackResult Hit = FGridCombatResolver::ResolveDirectDamage(
			Target, EGridDamageType::Physical, Damage, EGridPhysicalDamageSubtype::Bludgeoning);
		if (Hit.bHit)
		{
			Inventory->ApplyPhysicalArmorDamage(0, Hit.PhysicalArmorDamage, &Hit.Trace.Damage.PhysicalArmorSources);
			Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth -= Hit.HealthDamage;
		}
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT011033ImpactSnapshot,
	"Grimrock.RPG.ARMEMENT01.10.3.3.RealShieldImpactSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT011033ImpactSnapshot::RunTest(const FString&)
{
	// The authored CSV alone is insufficient: all UI keys must exist in the
	// actual ST_UI asset before the Combat Tooltip can be considered functional.
	const TCHAR* RequiredLocalizationKeys[] = {
		TEXT("Combat.Tooltip.Title"),
		TEXT("Combat.Tooltip.Section.Attack"),
		TEXT("Combat.Tooltip.Section.Absorption"),
		TEXT("Combat.Tooltip.Section.Consequences"),
		TEXT("Combat.Tooltip.Label.Attacker"),
		TEXT("Combat.Tooltip.Label.Defender"),
		TEXT("Combat.Tooltip.Label.Attack"),
		TEXT("Combat.Tooltip.Label.Defense"),
		TEXT("Combat.Tooltip.Label.Damage"),
		TEXT("Combat.Tooltip.Label.Health"),
		TEXT("Combat.Tooltip.Column.Source"),
		TEXT("Combat.Tooltip.Column.Before"),
		TEXT("Combat.Tooltip.Column.Absorbed"),
		TEXT("Combat.Tooltip.Column.After"),
		TEXT("Combat.Tooltip.Armor.Character"),
		TEXT("Combat.Tooltip.Armor.DepletedSuffix"),
		TEXT("Combat.Tooltip.Armor.DetailFormat"),
		TEXT("Combat.Tooltip.Fallback.Armor"),
		TEXT("Combat.Tooltip.Status.AlreadyDepletedNamed"),
		TEXT("Combat.Tooltip.Status.DepletedNamed"),
		TEXT("Combat.Tooltip.Value.AttackRoll"),
		TEXT("Combat.Tooltip.Value.DefenseRoll"),
		TEXT("Combat.Tooltip.Value.DamageResolved"),
		TEXT("Combat.Tooltip.Value.HealthAfterAttack"),
		TEXT("Combat.Tooltip.Value.NoDamage"),
		TEXT("Combat.Tooltip.Value.NoHealthChange"),
		TEXT("Combat.Feed.Compact.AttackDefense"),
		TEXT("Combat.Feed.Compact.ArmorAbsorbed"),
		TEXT("Combat.Feed.Compact.DamageLine"),
		TEXT("Combat.Feed.Compact.HealthShort"),
		TEXT("Combat.Feed.Compact.HealthTarget"),
		TEXT("Combat.Feed.Compact.HealthTransition"),
		TEXT("Combat.Feed.Compact.ResistanceStage"),
		TEXT("Combat.Tooltip.Attack.PositiveValue"),
		TEXT("Combat.Tooltip.Attack.Dice"),
		TEXT("Combat.Tooltip.Attack.SourceAccuracy"),
		TEXT("Combat.Tooltip.Attack.ActionAccuracy"),
		TEXT("Combat.Tooltip.Attack.Total"),
		TEXT("Combat.Tooltip.Attack.DirectDamage"),
		TEXT("Combat.Tooltip.Attack.NoRoll"),
		TEXT("Combat.Tooltip.Defense.Base"),
		TEXT("Combat.Tooltip.Defense.Evasion"),
		TEXT("Combat.Tooltip.Defense.Total"),
		TEXT("Combat.Tooltip.Defense.DirectDamage"),
		TEXT("Combat.Tooltip.Defense.NoRoll"),
		TEXT("Combat.Tooltip.Outcome.Miss"),
		TEXT("Combat.Tooltip.Outcome.Hit"),
		TEXT("Combat.Tooltip.Outcome.NaturalMiss"),
		TEXT("Combat.Tooltip.Outcome.NaturalHit"),
		TEXT("Combat.Tooltip.Outcome.AlwaysHit"),
		TEXT("Combat.Tooltip.Outcome.CriticalHit")
	};
	bool bAllKeysMaterialized = true;
	for (const TCHAR* Key : RequiredLocalizationKeys)
	{
		if (FUILocalizationText::Get(Key).IsEmpty())
		{
			AddError(FString::Printf(TEXT("ST_UI is missing a Combat Tooltip localization key: %s"), Key));
			bAllKeysMaterialized = false;
		}
	}
	if (!bAllKeysMaterialized) return false;

	UGridItemDefinitionAsset* Definition = ARMEMENT011033::Shield();
	if (!TestNotNull(TEXT("Real N1 round shield"), Definition)) return false;
	TestEqual(TEXT("Canonical shield physical armor"), Definition->EquipmentStatBonus.ArmorBonus, 5);
	UGridPartyInventoryComponent* Inventory = ARMEMENT011033::EquipShield(0);
	if (!TestNotNull(TEXT("Equipped real shield"), Inventory)) return false;
	const FGridAttackResult First = ARMEMENT011033::Apply(Inventory, 4);
	TestEqual(TEXT("Four absorbed points"), First.PhysicalArmorDamage, 4);
	TestEqual(TEXT("Visible armor rows match the physical protection consumed"),
		UGridCombatTooltipWidget::CountAbsorbingSources(First.Trace.Damage), 1);
	TestEqual(TEXT("One source recorded"), First.Trace.Damage.PhysicalArmorSources.Num(), 1);
	if (!First.Trace.Damage.PhysicalArmorSources.IsValidIndex(0)) return false;
	const FGridArmorAbsorptionSourceTrace& Source = First.Trace.Damage.PhysicalArmorSources[0];
	TestEqual(TEXT("Correct item definition"), Source.ItemDefinitionId, Definition->ItemDefinitionId);
	TestTrue(TEXT("Equipment is identified"), Source.bEquipment);
	TestEqual(TEXT("Before at impact"), Source.Before, 5);
	TestEqual(TEXT("Absorbed at impact"), Source.Absorbed, 4);
	TestEqual(TEXT("After at impact"), Source.After, 1);
	TestFalse(TEXT("Not yet depleted"), Source.bExhaustedByHit);

	FGridCombatLogEntry Log;
	Log.Type = EGridCombatLogEntryType::AttackHit;
	Log.SourceDisplayName = FText::FromString(TEXT("Rat géant"));
	Log.TargetDisplayName = FText::FromString(TEXT("Elias"));
	Log.AttackResult = First;
	// Direct damage supplies a valid damage trace; synthetic attack test requires
	// the same valid trace marker as a normal resolved hit.
	Log.AttackResult.Trace.bValid = true;
	FGridHudEventEntry Feed;
	if (!TestTrue(TEXT("Formatter creates actual Event Feed entry"),
		FGridCombatEventFeedFormatter::BuildEntry(Log, Feed))) return false;
	TestTrue(TEXT("Structured hover enabled"), Feed.bHasCombatTooltipTrace);
	const FGridCombatTooltipViewData TooltipData = UGridCombatTooltipWidget::BuildViewData(Feed);
	TestTrue(TEXT("Pure read model is valid"), TooltipData.bValid);
	TestTrue(TEXT("Tooltip exposes its raw combat trace"), TooltipData.Trace.bValid);
	TestTrue(TEXT("Positive absorption displays the entire section"), TooltipData.bShowAbsorption);
	TestEqual(TEXT("One absorbed source passed to native row presenter"), TooltipData.VisibleArmorRows.Num(), 1);
	TestEqual(TEXT("Raw source value remains 5"), TooltipData.VisibleArmorRows[0].Before, 5);
	TestEqual(TEXT("Raw absorbed value remains 4"), TooltipData.VisibleArmorRows[0].Absorbed, 4);
	TestEqual(TEXT("Raw after value remains 1"), TooltipData.VisibleArmorRows[0].After, 1);
	TestTrue(TEXT("Before protection depletion there is no exhaustion notice"),
		TooltipData.NewlyExhaustedSources.IsEmpty());
	const FGridCombatTooltipDisplayTexts BeforeText =
		UGridCombatTooltipWidget::BuildDisplayTexts(TooltipData);
	TestTrue(TEXT("C++ prepares a localized attack string"), !BeforeText.Attack.IsEmpty());
	TestTrue(TEXT("C++ prepares a localized defense string"), !BeforeText.Defense.IsEmpty());
	TestTrue(TEXT("C++ prepares a localized damage string"), !BeforeText.Damage.IsEmpty());
	TestTrue(TEXT("C++ prepares a localized health string"), !BeforeText.Health.IsEmpty());
	TestFalse(TEXT("No warning before protection depletion"), BeforeText.bShowStatus);
	// Compatibility projection remains tested; the native presenter now formats the localized dice text.
	FGridHudEventEntry SyntheticDiceEntry = Feed;
	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.bUsed = true;
	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.Kind = EGridCombatRollTraceKind::Range;
	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.Minimum = 1;
	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.Maximum = 20;
	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.FinalValue = 16;
	const auto DiceView = UGridCombatTooltipWidget::BuildViewData(SyntheticDiceEntry);
	TestTrue(TEXT("Resolved range roll is represented as dice"), DiceView.AttackDice.bHasDice);
	TestEqual(TEXT("Dice count"), DiceView.AttackDice.Count, 1);
	TestEqual(TEXT("Dice sides"), DiceView.AttackDice.Sides, 20);
	TestEqual(TEXT("Rolled numeric value"), DiceView.AttackDice.Rolled, 16);
	TestEqual(TEXT("Numeric offset"), DiceView.AttackDice.Offset, 0);

	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.Minimum = 3;
	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.Maximum = 6;
	SyntheticDiceEntry.CombatTooltipTrace.Attack.Roll.FinalValue = 5;
	const auto OffsetDiceView = UGridCombatTooltipWidget::BuildViewData(SyntheticDiceEntry);
	TestEqual(TEXT("Virtual die width from non-1 range"), OffsetDiceView.AttackDice.Sides, 4);
	TestEqual(TEXT("Virtual die result from non-1 range"), OffsetDiceView.AttackDice.Rolled, 3);
	TestEqual(TEXT("Virtual die flat modifier"), OffsetDiceView.AttackDice.Offset, 2);
	SyntheticDiceEntry.CombatTooltipTrace.Attack.FinalValue = 18;
	SyntheticDiceEntry.CombatTooltipTrace.Defense.BaseValue = 10;
	SyntheticDiceEntry.CombatTooltipTrace.Defense.EvasionModifier = 1;
	SyntheticDiceEntry.CombatTooltipTrace.Defense.FinalValue = 11;
	const FGridCombatTooltipDisplayTexts FormattedDice =
		UGridCombatTooltipWidget::BuildDisplayTexts(
			UGridCombatTooltipWidget::BuildViewData(SyntheticDiceEntry));
	TestTrue(TEXT("C++ renders range roll including signed offset"),
		FormattedDice.Attack.ToString().Contains(TEXT("1d4")));
	TestTrue(TEXT("C++ includes final attack total instead of raw die total"),
		FormattedDice.Attack.ToString().Contains(TEXT("18")));
	TestTrue(TEXT("Defense is formatted in C++, not Blueprint"),
		FormattedDice.Defense.ToString().Contains(TEXT("11")));
	TestEqual(TEXT("Immutable source snapshot travels with the feed"),
		Feed.CombatTooltipTrace.Damage.PhysicalArmorSources[0].Absorbed, 4);
	TestTrue(TEXT("Text fallback states source explicitly"),
		UGridEventFeedWidget::BuildEntryTooltipText(Feed).ToString().Contains(Definition->DisplayName.ToString()));
	TestFalse(TEXT("Main feed remains compact"),
		UGridEventFeedWidget::BuildEntryText(Feed).ToString().Contains(TEXT("protection epuisee")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT011033DepletionSnapshot,
	"Grimrock.RPG.ARMEMENT01.10.3.3.DepletionAndHistoricalImmutability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT011033DepletionSnapshot::RunTest(const FString&)
{
	UGridPartyInventoryComponent* Inventory = ARMEMENT011033::EquipShield(0);
	if (!TestNotNull(TEXT("Equipped shield"), Inventory)) return false;
	const FGridAttackResult First = ARMEMENT011033::Apply(Inventory, 4);
	const FGridAttackResult Second = ARMEMENT011033::Apply(Inventory, 5);
	const FGridAttackResult Third = ARMEMENT011033::Apply(Inventory, 3);
	TestEqual(TEXT("First hit has one visible absorption row"),
		UGridCombatTooltipWidget::CountAbsorbingSources(First.Trace.Damage), 1);
	TestEqual(TEXT("Hit that exhausts the shield still has a visible row"),
		UGridCombatTooltipWidget::CountAbsorbingSources(Second.Trace.Damage), 1);
	TestEqual(TEXT("Third hit with shield at zero hides all absorption rows"),
		UGridCombatTooltipWidget::CountAbsorbingSources(Third.Trace.Damage), 0);

	const FGridCombatTooltipViewData FirstView = UGridCombatTooltipWidget::BuildViewData(
		ARMEMENT011033::MakeTooltipEntry(First));
	const FGridCombatTooltipViewData SecondView = UGridCombatTooltipWidget::BuildViewData(
		ARMEMENT011033::MakeTooltipEntry(Second));
	const FGridCombatTooltipViewData ThirdView = UGridCombatTooltipWidget::BuildViewData(
		ARMEMENT011033::MakeTooltipEntry(Third));
	TestTrue(TEXT("Historical first hit stays visible"), FirstView.bShowAbsorption);
	TestEqual(TEXT("First view keeps its five points before"), FirstView.VisibleArmorRows[0].Before, 5);
	TestTrue(TEXT("Last point consumed still yields visible row"), SecondView.bShowAbsorption);
	TestEqual(TEXT("Second view has one exhausted source"), SecondView.NewlyExhaustedSources.Num(), 1);
	TestFalse(TEXT("Third hit hides the entire absorption section"), ThirdView.bShowAbsorption);
	TestTrue(TEXT("Third hit has no visual armor rows"), ThirdView.VisibleArmorRows.IsEmpty());
	TestTrue(TEXT("Third hit signals already depleted equipment"), ThirdView.AlreadyDepletedSources.Num() == 1);
	TestTrue(TEXT("Third hit does not repeat new depletion"), ThirdView.NewlyExhaustedSources.IsEmpty());
	const FGridCombatTooltipDisplayTexts SecondText =
		UGridCombatTooltipWidget::BuildDisplayTexts(SecondView);
	const FGridCombatTooltipDisplayTexts ThirdText =
		UGridCombatTooltipWidget::BuildDisplayTexts(ThirdView);
	TestTrue(TEXT("C++ creates new depletion notice"), SecondText.bShowStatus);
	TestTrue(TEXT("Depletion notice names real equipped shield"),
		SecondText.Status.ToString().Contains(ARMEMENT011033::Shield()->DisplayName.ToString()));
	TestTrue(TEXT("C++ creates already-depleted notice"), ThirdText.bShowStatus);
	TestFalse(TEXT("Different status messages for new and past depletion"),
		SecondText.Status.EqualTo(ThirdText.Status));
	FGridCombatDamageTrace NoArmorDamage;
	NoArmorDamage.bResolved = true;
	TestEqual(TEXT("Empty physical protection snapshot hides absorption section"),
		UGridCombatTooltipWidget::CountAbsorbingSources(NoArmorDamage), 0);
	if (!TestTrue(TEXT("Every physical hit carries its one equipment source"),
		First.Trace.Damage.PhysicalArmorSources.Num() == 1 &&
		Second.Trace.Damage.PhysicalArmorSources.Num() == 1 &&
		Third.Trace.Damage.PhysicalArmorSources.Num() == 1)) return false;
	const auto& A = First.Trace.Damage.PhysicalArmorSources[0];
	const auto& B = Second.Trace.Damage.PhysicalArmorSources[0];
	const auto& C = Third.Trace.Damage.PhysicalArmorSources[0];
	TestEqual(TEXT("Old hit remains five to one"), A.Before, 5);
	TestEqual(TEXT("Old hit remains one"), A.After, 1);
	TestEqual(TEXT("Shield loses the final point"), B.Absorbed, 1);
	TestEqual(TEXT("Second hit ends at zero"), B.After, 0);
	TestTrue(TEXT("Depletion flagged on transition only"), B.bExhaustedByHit);
	TestEqual(TEXT("Third hit starts at zero"), C.Before, 0);
	TestEqual(TEXT("Third hit absorbs zero"), C.Absorbed, 0);
	TestFalse(TEXT("Third hit does not repeat depletion alert"), C.bExhaustedByHit);
	TestEqual(TEXT("No shield regeneration after three hits"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT011033MultipleSources,
	"Grimrock.RPG.ARMEMENT01.10.3.3.EquipmentAndBaseArmorBreakdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT011033MultipleSources::RunTest(const FString&)
{
	UGridPartyInventoryComponent* Inventory = ARMEMENT011033::EquipShield(2);
	if (!TestNotNull(TEXT("Shield plus two base armor"), Inventory)) return false;
	const FGridAttackResult Hit = ARMEMENT011033::Apply(Inventory, 6);
	const auto& Rows = Hit.Trace.Damage.PhysicalArmorSources;
	TestEqual(TEXT("Exactly two physical armor sources"), Rows.Num(), 2);
	TestEqual(TEXT("Both actual absorbers are shown in the read model"),
		UGridCombatTooltipWidget::CountAbsorbingSources(Hit.Trace.Damage), 2);
	const FGridCombatTooltipViewData MultipleView = UGridCombatTooltipWidget::BuildViewData(
		ARMEMENT011033::MakeTooltipEntry(Hit));
	TestTrue(TEXT("Multiple sources show absorption section"), MultipleView.bShowAbsorption);
	TestEqual(TEXT("Two sources supplied to native WBP-row instantiation"), MultipleView.VisibleArmorRows.Num(), 2);
	if (Rows.Num() != 2) return false;
	TestTrue(TEXT("First source is the equipped shield"), Rows[0].bEquipment);
	TestEqual(TEXT("Shield absorbs its full five"), Rows[0].Absorbed, 5);
	TestTrue(TEXT("Shield depleted on this hit"), Rows[0].bExhaustedByHit);
	TestFalse(TEXT("Second source is character base reserve"), Rows[1].bEquipment);
	TestEqual(TEXT("Base reserve before hit"), Rows[1].Before, 2);
	TestEqual(TEXT("Base reserve absorbs the remainder"), Rows[1].Absorbed, 1);
	TestEqual(TEXT("Base reserve after hit"), Rows[1].After, 1);
	TestEqual(TEXT("Snapshot reconciles with resolver"),
		Rows[0].Absorbed + Rows[1].Absorbed, Hit.PhysicalArmorDamage);
	TestEqual(TEXT("Final armor remains exactly one"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 1);
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT011033AttackBreakdown,
	"Grimrock.RPG.ARMEMENT01.10.3.3.TransparentAttackTotal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT011033AttackBreakdown::RunTest(const FString&)
{
	// Reproduces the PIE screenshot: the natural d20 roll is 3, but the
	// defender sees an attack total of 5 because source accuracy adds 2.
	FGridCombatTooltipViewData View;
	View.bValid = true;
	FGridCombatAttackTrace& Attack = View.Trace.Attack;
	Attack.Roll.bUsed = true;
	Attack.Roll.Kind = EGridCombatRollTraceKind::Die;
	Attack.Roll.DiceCount = 1;
	Attack.Roll.DiceSides = 20;
	Attack.Roll.Minimum = 1;
	Attack.Roll.Maximum = 20;
	Attack.Roll.FinalValue = 3;
	Attack.Roll.IndividualRolls = {3};
	Attack.SourceAccuracy = 2;
	Attack.AttackAccuracyBonus = 0;
	Attack.FinalValue = 5;

	const FText Terms = FGridCombatEventFeedFormatter::FormatAttackTerms(Attack);
	TestEqual(TEXT("The displayed expression includes source accuracy rather than jumping 3 to 5"),
		Terms.ToString(), FString(TEXT("1d20 (3) + 2")));
	const FGridCombatTooltipDisplayTexts Display =
		UGridCombatTooltipWidget::BuildDisplayTexts(View);
	TestTrue(TEXT("Combat tooltip labels source accuracy instead of repeating the compact feed"),
		Display.Attack.ToString().Contains(TEXT("Précision de l'attaquant")) &&
		Display.Attack.ToString().Contains(TEXT("+2")));
	TestFalse(TEXT("Combat tooltip is not the compact attack formula"),
		Display.Attack.ToString().Contains(TEXT("1d20 (3) + 2 ;")));
	TestTrue(TEXT("Combat tooltip still contains the resolved attack total"),
		Display.Attack.ToString().Contains(TEXT("Total de l'attaque : 5")));

	// The two accuracy contributions must stay distinct (including penalties),
	// not be silently merged into an unexplained delta.
	Attack.SourceAccuracy = 4;
	Attack.AttackAccuracyBonus = -2;
	Attack.FinalValue = 5;
	TestEqual(TEXT("Positive accuracy and negative action modifier both visible"),
		FGridCombatEventFeedFormatter::FormatAttackTerms(Attack).ToString(),
		FString(TEXT("1d20 (3) + 4 - 2")));

	// NdM+K must be shown once and remain independent from source accuracy.
	Attack.Roll.DiceFlatBonus = 2;
	Attack.Roll.FinalValue = 5;
	Attack.SourceAccuracy = -1;
	Attack.AttackAccuracyBonus = 3;
	Attack.FinalValue = 7;
	TestEqual(TEXT("Roll flat bonus and signed accuracy terms are displayed exactly once"),
		FGridCombatEventFeedFormatter::FormatAttackTerms(Attack).ToString(),
		FString(TEXT("1d20 (3) + 2 - 1 + 3")));
	TestTrue(TEXT("Attack display includes the mechanically resolved total"),
		UGridCombatTooltipWidget::BuildDisplayTexts(View).Attack.ToString().Contains(TEXT("7")));

	Attack.Roll.DiceFlatBonus = 0;
	Attack.Roll.FinalValue = 3;
	Attack.SourceAccuracy = 0;
	Attack.AttackAccuracyBonus = 0;
	Attack.FinalValue = 3;
	TestEqual(TEXT("No invented +0 in an unmodified attack"),
		FGridCombatEventFeedFormatter::FormatAttackTerms(Attack).ToString(),
		FString(TEXT("1d20 (3)")));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT0110337LabeledComponents,
	"Grimrock.RPG.ARMEMENT01.10.3.3.7.LabeledAccuracyAndMissExplanation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT0110337LabeledComponents::RunTest(const FString&)
{
	FGridAttackSourceStats Source;
	Source.Accuracy = 1;
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	Target.Evasion = 1;
	FGridAttackDefinition Action;
	Action.MinDamage = 1;
	Action.MaxDamage = 4;
	Action.AccuracyBonus = 1;

	// Actual authoritative resolver, not a hand-crafted tooltip trace.
	const FGridAttackResult Miss =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Action, 5, 2);
	TestFalse(TEXT("Natural 5 plus two modifiers misses defense 11"), Miss.bHit);
	TestEqual(TEXT("Natural roll recorded"), Miss.Trace.Attack.Roll.FinalValue, 5);
	TestEqual(TEXT("Attacker accuracy recorded"), Miss.Trace.Attack.SourceAccuracy, 1);
	TestEqual(TEXT("Action accuracy recorded"), Miss.Trace.Attack.AttackAccuracyBonus, 1);
	TestEqual(TEXT("Final hit test recorded"), Miss.Trace.Attack.FinalValue, 7);

	const FGridCombatTooltipDisplayTexts MissText =
		UGridCombatTooltipWidget::BuildDisplayTexts(
			UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(Miss)));
	const FString AttackBreakdown = MissText.Attack.ToString();
	const FString DefenseBreakdown = MissText.Defense.ToString();
	TestTrue(TEXT("Natural dice is labeled explicitly"), AttackBreakdown.Contains(TEXT("Jet de dé"))
		&& AttackBreakdown.Contains(TEXT("1d20 (5)")));
	TestTrue(TEXT("The first +1 is labeled attacker accuracy"),
		AttackBreakdown.Contains(TEXT("Précision de l'attaquant : +1")));
	TestTrue(TEXT("The second +1 is labeled action accuracy, not assumed weapon accuracy"),
		AttackBreakdown.Contains(TEXT("Modificateur de l'attaque : +1")));
	TestTrue(TEXT("Resolved total is distinctly labeled"),
		AttackBreakdown.Contains(TEXT("Total de l'attaque : 7")));
	TestFalse(TEXT("Detailed tooltip does not repeat the compact +1 +1 expression"),
		AttackBreakdown.Contains(TEXT("1d20 (5) + 1 + 1")));
	TestTrue(TEXT("Defense clearly identifies base and evasion"),
		DefenseBreakdown.Contains(TEXT("Défense de base : 10"))
		&& DefenseBreakdown.Contains(TEXT("Esquive du défenseur : +1"))
		&& DefenseBreakdown.Contains(TEXT("Défense totale : 11")));
	TestTrue(TEXT("Miss is explained by total lower than defense"),
		DefenseBreakdown.Contains(TEXT("ÉCHEC"))
		&& DefenseBreakdown.Contains(TEXT("7 < 11")));
	TestFalse(TEXT("Miss produces no damage"),
		Miss.Trace.Damage.bResolved);

	// No cosmetic zero-value rows, and display both positive and negative modifiers.
	Source.Accuracy = 4;
	Action.AccuracyBonus = -2;
	const FGridAttackResult Modified =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Action, 5, 2);
	const FString ModifierBreakdown = UGridCombatTooltipWidget::BuildDisplayTexts(
		UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(Modified)))
			.Attack.ToString();
	TestTrue(TEXT("Source accuracy +4 remains identifiable"),
		ModifierBreakdown.Contains(TEXT("Précision de l'attaquant : +4")));
	TestTrue(TEXT("Negative action modifier remains identifiable"),
		ModifierBreakdown.Contains(TEXT("Modificateur de l'attaque"))
		&& ModifierBreakdown.Contains(TEXT("2")));
	TestEqual(TEXT("Mechanical attack total remains authoritative"), Modified.Trace.Attack.FinalValue, 7);

	Source.Accuracy = 0;
	Action.AccuracyBonus = 0;
	const FGridAttackResult Bare =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Action, 5, 2);
	const FString BareText = UGridCombatTooltipWidget::BuildDisplayTexts(
		UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(Bare)))
			.Attack.ToString();
	TestFalse(TEXT("Zero attacker accuracy does not pollute tooltip"),
		BareText.Contains(TEXT("Précision de l'attaquant")));
	TestFalse(TEXT("Zero action accuracy does not pollute tooltip"),
		BareText.Contains(TEXT("Modificateur de l'attaque")));

	const FGridAttackResult Hit =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Action, 13, 2);
	TestTrue(TEXT("Hit mechanically resolves"), Hit.bHit);
	const FString HitOutcome = UGridCombatTooltipWidget::BuildDisplayTexts(
		UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(Hit)))
			.Defense.ToString();
	TestTrue(TEXT("Successful numerical comparison is explained"), HitOutcome.Contains(TEXT("TOUCHÉ"))
		&& HitOutcome.Contains(TEXT("13")) && HitOutcome.Contains(TEXT("11")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT0110337ExceptionalOutcomes,
	"Grimrock.RPG.ARMEMENT01.10.3.3.7.AutomaticHitMissCriticalAndDirectDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT0110337ExceptionalOutcomes::RunTest(const FString&)
{
	FGridAttackSourceStats Source;
	Source.Accuracy = 30;
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	Target.Evasion = 1;
	FGridAttackDefinition Action;
	Action.MinDamage = 1;
	Action.MaxDamage = 4;

	const FGridAttackResult NaturalMiss =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Action, 1, 2);
	TestFalse(TEXT("Natural 1 loses even with huge accuracy"), NaturalMiss.bHit);
	const FString NaturalMissText = UGridCombatTooltipWidget::BuildDisplayTexts(
		UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(NaturalMiss)))
			.Defense.ToString();
	TestTrue(TEXT("Natural 1 is described as an automatic miss"),
		NaturalMissText.Contains(TEXT("ÉCHEC automatique"))
		&& NaturalMissText.Contains(TEXT("1 naturel")));

	Source.Accuracy = -30;
	Source.CriticalChancePercent = 5;
	const FGridAttackResult NaturalHit =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Action, 20, 2);
	TestTrue(TEXT("Natural 20 hits despite low total"), NaturalHit.bHit);
	TestTrue(TEXT("Critical chance 5 percent accepts natural 20"), NaturalHit.bCriticalHit);
	const FString CriticalText = UGridCombatTooltipWidget::BuildDisplayTexts(
		UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(NaturalHit)))
			.Defense.ToString();
	TestTrue(TEXT("Natural 20 explicitly overrides numeric comparison"),
		CriticalText.Contains(TEXT("TOUCHÉ automatiquement")));
	TestTrue(TEXT("Critical hit is explained by natural roll and threshold"),
		CriticalText.Contains(TEXT("Coup critique"))
		&& CriticalText.Contains(TEXT("seuil 20")));

	Action.bAlwaysHits = true;
	const FGridAttackResult GuaranteedHit =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Action, 1, 2);
	TestTrue(TEXT("Guaranteed hit takes priority over natural 1"), GuaranteedHit.bHit);
	const FString GuaranteedText = UGridCombatTooltipWidget::BuildDisplayTexts(
		UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(GuaranteedHit)))
			.Defense.ToString();
	TestTrue(TEXT("Guaranteed hit explicitly identified"),
		GuaranteedText.Contains(TEXT("toucher garanti")));

	const FGridAttackResult Direct =
		FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Physical, 3);
	TestTrue(TEXT("Direct damage resolves without an attack roll"), Direct.bHit);
	const FGridCombatTooltipDisplayTexts DirectText = UGridCombatTooltipWidget::BuildDisplayTexts(
		UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(Direct)));
	TestTrue(TEXT("Direct damage does not invent a natural d20"),
		DirectText.Attack.ToString().Contains(TEXT("aucun jet d'attaque")));
	TestTrue(TEXT("Direct damage does not invent a defense comparison"),
		DirectText.Defense.ToString().Contains(TEXT("Sans jet de défense")));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT0110338DesignerSeparatedLines,
	"Grimrock.RPG.ARMEMENT01.10.3.3.8.IndependentDesignerTexts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT0110338DesignerSeparatedLines::RunTest(const FString&)
{
	FGridAttackSourceStats Source;
	Source.Accuracy = 1;
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	Target.Evasion = 1;
	FGridAttackDefinition Attack;
	Attack.MinDamage = 1;
	Attack.MaxDamage = 4;
	Attack.AccuracyBonus = 1;

	const FGridAttackResult Hit =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 11, 2);
	const FGridCombatTooltipViewData Data = UGridCombatTooltipWidget::BuildViewData(
		ARMEMENT011033::MakeTooltipEntry(Hit));
	const FGridCombatTooltipDetailTexts Fields =
		UGridCombatTooltipWidget::BuildDetailTexts(Data);

	TestTrue(TEXT("Dice is a separately localizable and stylable field"),
		Fields.AttackDice.ToString().Contains(TEXT("1d20 (11)")));
	TestTrue(TEXT("Attacker precision can be styled without restyling dice"),
		Fields.AttackSourceAccuracy.ToString().Contains(TEXT("+1")));
	TestTrue(TEXT("Action accuracy can be styled independently"),
		Fields.AttackActionAccuracy.ToString().Contains(TEXT("+1")));
	TestTrue(TEXT("Attack total is its own stylable text"),
		Fields.AttackTotal.ToString().Contains(TEXT("13")));
	TestTrue(TEXT("Defense base is its own stylable text"),
		Fields.DefenseBase.ToString().Contains(TEXT("10")));
	TestTrue(TEXT("Defender evasion is its own stylable text"),
		Fields.DefenseEvasion.ToString().Contains(TEXT("+1")));
	TestTrue(TEXT("Defense total is its own stylable text"),
		Fields.DefenseTotal.ToString().Contains(TEXT("11")));
	TestTrue(TEXT("Hit decision is its own stylable text"),
		Fields.AttackOutcome.ToString().Contains(TEXT("TOUCHÉ")));
	TestTrue(TEXT("Non-critical hits have no critical line"),
		Fields.AttackCritical.IsEmpty());

	// No structural or localizable text changes may affect legacy WBP output.
	const FGridCombatTooltipDisplayTexts Legacy =
		UGridCombatTooltipWidget::BuildDisplayTexts(Data);
	TestTrue(TEXT("Legacy multiline fallback still explains both bonuses"),
		Legacy.Attack.ToString().Contains(Fields.AttackSourceAccuracy.ToString())
		&& Legacy.Attack.ToString().Contains(Fields.AttackActionAccuracy.ToString()));
	TestTrue(TEXT("Legacy multiline fallback still includes the same outcome"),
		Legacy.Defense.ToString().Contains(Fields.AttackOutcome.ToString()));

	Source.Accuracy = 0;
	Attack.AccuracyBonus = 0;
	Target.Evasion = 0;
	const FGridAttackResult Bare =
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 5, 2);
	const FGridCombatTooltipDetailTexts BareFields =
		UGridCombatTooltipWidget::BuildDetailTexts(
			UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(Bare)));
	TestTrue(TEXT("Zero-valued accuracy rows collapse without blank noise"),
		BareFields.AttackSourceAccuracy.IsEmpty() && BareFields.AttackActionAccuracy.IsEmpty());
	TestTrue(TEXT("Zero-valued evasion row collapses without blank noise"),
		BareFields.DefenseEvasion.IsEmpty());
	TestTrue(TEXT("Remaining attack and defense totals stay visible"),
		!BareFields.AttackTotal.IsEmpty() && !BareFields.DefenseTotal.IsEmpty());

	const FGridAttackResult Direct =
		FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Physical, 3);
	const FGridCombatTooltipDetailTexts DirectFields =
		UGridCombatTooltipWidget::BuildDetailTexts(
			UGridCombatTooltipWidget::BuildViewData(ARMEMENT011033::MakeTooltipEntry(Direct)));
	TestTrue(TEXT("Direct damage uses only dedicated no-roll explanation fields"),
		DirectFields.AttackDice.ToString().Contains(TEXT("aucun jet"))
		&& DirectFields.DefenseBase.ToString().Contains(TEXT("Sans jet")));
	TestTrue(TEXT("Direct damage does not create fictional totals or a hit test"),
		DirectFields.AttackTotal.IsEmpty() && DirectFields.DefenseTotal.IsEmpty()
		&& DirectFields.AttackOutcome.IsEmpty());

	const FGridCombatTooltipDetailTexts Empty = UGridCombatTooltipWidget::BuildDetailTexts(
		FGridCombatTooltipViewData{});
	TestTrue(TEXT("Invalid snapshots do not fabricate UI lines"),
		Empty.AttackDice.IsEmpty() && Empty.AttackOutcome.IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
