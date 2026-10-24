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
		TEXT("Combat.Feed.Compact.ResistanceStage")
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
	TestEqual(TEXT("One visible row in authored Blueprint layout"), TooltipData.VisibleArmorRows.Num(), 1);
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
	// Numeric dice projection is data only; WBP owns the localized notation.
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
	TestEqual(TEXT("Two sources flow to the authored WBP"), MultipleView.VisibleArmorRows.Num(), 2);
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

#endif // WITH_DEV_AUTOMATION_TESTS
