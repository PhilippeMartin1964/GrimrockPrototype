#pragma once

#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridQuickItemUseReadModel.h"
#include "GridPhysicalThrowPreflightReadModel.h"

/**
 * ARM-CLEAN06.3AW: context/read/hotbar capabilities of an actual item instance.
 * The configured registry is the sole authority for the definition.
 * Instance-authored readable content remains independent of static data
 * for a valid physical canonical item.
 */
struct FGridItemInteractionReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	bool bCanRead = false;
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
	static FGridItemInteractionReadView Project(const FGridItemInstance& Item,
		const FGridResolvedItem* Resolved, const FGridQuickItemUseReadView& Use)
	{
		FGridItemInteractionReadView Out;
		Out.bRegisteredIdentity = Use.bRegisteredIdentity;
		if (!Resolved)
		{
			return Out;
		}
		const FGridResolvedItem& Canonical = *Resolved;
		const bool bHasInstanceText = !Item.ReadTextOverride.IsEmpty() ||
			(IsValid(Item.ReadableContentAsset) && !Item.ReadableContentAsset->BodyText.IsEmpty());
		Out.bResolved = true;
		Out.DisplayName = Canonical.DisplayName;
		Out.Description = Canonical.Description;
		Out.ItemIcon = Canonical.ItemIcon;
		Out.DefaultReadText = Canonical.ReadText;
		Out.bCanRead = bHasInstanceText || Canonical.bReadable || !Out.DefaultReadText.IsEmpty();
		Out.bPhysicalThrowHotbar = FGridPhysicalThrowPreflightReadModel::CanBind(Canonical, Item);
		// Only a true direct self-heal may appear as the inline "Consume"
            // action. Status cleansing uses the combat hotbar executor.
		Out.bCanConsumeItem = Use.bResolved &&
			Use.Action.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
			Use.Action.EffectProfile.IsValid();
		Out.bCanAddToHotbar = Out.bPhysicalThrowHotbar || Use.bResolved;
		return Out;
	}

	static FGridItemInteractionReadView Resolve(
		const UGridItemSubsystem* Registry, const FGridItemInstance& Item)
	{
		FGridResolvedItem Resolved;
		const bool bResolved = Registry && Registry->ResolvePhysicalItem(Item, Resolved);
		const FGridResolvedItem* Definition = bResolved ? &Resolved : nullptr;
		const bool bRegistered = bResolved || (Registry && Registry->ContainsItem(Item.ItemDefinitionId));
		return Project(Item, Definition, FGridQuickItemUseReadModel::Project(Registry, Definition, bRegistered));
	}
};
