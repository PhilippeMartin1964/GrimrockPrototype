#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridQuickItemUseReadModel.h"

/**
 * ARM-CLEAN06.3AW: context/read/hotbar capabilities of an actual item instance.
 * A configured registry is the sole authority. Historical capabilities
 * are available only to isolated registry-less Automation fixtures.
 * Instance-authored readable content remains independent of static data
 * for a valid physical canonical item.
 */
struct FGridItemInteractionReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	bool bCanRead = false;
	bool bCanConsumeLegacy = false;
	bool bCanConsumeItem = false;
	bool bCanAddToHotbar = false;
	bool bPhysicalThrowHotbar = false;
	FText DisplayName;
	FText Description;
	FText DefaultReadText;
	TSoftObjectPtr<UTexture2D> ItemIcon;
};

struct FGridItemInteractionReadModel
{
	static FGridItemInteractionReadView Resolve(
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* OldDefinition,
		const FGridItemInstance& Item)
	{
		FGridItemInteractionReadView Out;
		Out.bRegisteredIdentity = Registry && Registry->ContainsItem(Item.ItemDefinitionId);
		if (!Item.IsValid())
		{
			return Out;
		}
		const bool bHasInstanceText = !Item.ReadTextOverride.IsEmpty() ||
			(IsValid(Item.ReadableContentAsset) && !Item.ReadableContentAsset->BodyText.IsEmpty());

		if (Out.bRegisteredIdentity)
		{
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical) ||
				(!Canonical.bIsArmament && Item.ItemLevel != 0))
			{
				return Out; // Known invalid ARM N0/N21 never borrows old capabilities.
			}
			Out.bResolved = true;
			Out.DisplayName = Canonical.DisplayName;
			Out.Description = Canonical.Description;
			Out.ItemIcon = Canonical.ItemIcon;
			Out.DefaultReadText = Canonical.ReadText;
			Out.bCanRead = bHasInstanceText || Canonical.bReadable || !Out.DefaultReadText.IsEmpty();
			Out.bPhysicalThrowHotbar = Canonical.bIsArmament &&
				Canonical.Weapon.Projectile.bEnabled &&
				Canonical.Weapon.HandPolicy != EGridHandPolicy::TwoHanded &&
				Canonical.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand) &&
				FMath::IsFinite(Canonical.WeightKg) && Canonical.WeightKg >= 0.0f &&
				FMath::IsFinite(Canonical.Weapon.Projectile.InitialSpeed) &&
				Canonical.Weapon.Projectile.InitialSpeed > KINDA_SMALL_NUMBER;
			const FGridQuickItemUseReadView Use =
				FGridQuickItemUseReadModel::Resolve(Registry, Item);
			// Only a true direct self-heal may appear as the inline "Consume"
            // action. Status cleansing uses the combat hotbar executor.
            Out.bCanConsumeItem = Use.bResolved &&
                Use.Action.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
                Use.Action.EffectProfile.IsValid();
			Out.bCanAddToHotbar = Out.bPhysicalThrowHotbar || Use.bResolved;
			// No registered item ever executes an OLD QuickItem action.
			return Out;
		}

		// No production identity may obtain an interaction from historical
		// data, including readable text on an unknown instance.
#if WITH_DEV_AUTOMATION_TESTS
		if (Registry || !GIsAutomationTesting) return Out;
		const bool bLegacy = IsValid(OldDefinition) &&
			OldDefinition->ItemDefinitionId == Item.ItemDefinitionId;
		Out.bResolved = bLegacy;
		Out.bCanRead = bHasInstanceText ||
			(bLegacy && (OldDefinition->ItemType == EGridItemType::Book ||
				OldDefinition->ItemType == EGridItemType::Scroll ||
				OldDefinition->ItemTags.Contains(TEXT("Readable")) ||
				OldDefinition->ItemTags.Contains(TEXT("Lisible")) ||
				!OldDefinition->ReadText.IsEmpty()));
		if (bLegacy)
		{
			Out.DisplayName = OldDefinition->DisplayName;
			Out.Description = OldDefinition->Description;
			Out.DefaultReadText = OldDefinition->ReadText;
			Out.ItemIcon = OldDefinition->Icon;
			Out.bCanConsumeLegacy = OldDefinition->ItemType == EGridItemType::Potion ||
				OldDefinition->ItemType == EGridItemType::Food;
			FGridCombatActionDefinition LegacyAction;
			Out.bCanAddToHotbar =
				OldDefinition->BuildInventoryCombatActionDefinition(LegacyAction) ||
				OldDefinition->IsPhysicallyThrowable();
		}
#endif // WITH_DEV_AUTOMATION_TESTS
		return Out;
	}
};
