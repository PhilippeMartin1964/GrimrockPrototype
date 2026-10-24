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
	TestEqual(TEXT("Both actual absorbers are shown in the shared grid"),
		UGridCombatTooltipWidget::CountAbsorbingSources(Hit.Trace.Damage), 2);
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
