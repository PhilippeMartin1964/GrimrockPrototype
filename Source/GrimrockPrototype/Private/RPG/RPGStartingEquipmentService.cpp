#include "RPG/RPGStartingEquipmentService.h"
#include "CoreGlobals.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridStartingKitStaticMetadataReadModel.h"
#include "GridStartingKitEquipmentPolicyReadModel.h"

bool FRPGStartingEquipmentService::TryBuildKit(
	const URPGClassAsset* ClassDefinition,
	FName KitId,
	FGridCharacterInventoryState& InOutCharacter,
	FGridCharacterEquipmentState& InOutEquipment,
	TArray<UGridItemDefinitionAsset*>& OutDefinitions,
	FString& OutError,
	const UGridItemSubsystem* CanonicalRegistry)
{
	OutDefinitions.Reset();
	OutError.Reset();
#if WITH_DEV_AUTOMATION_TESTS
	const bool bAllowLegacyFixture = !CanonicalRegistry && GIsAutomationTesting;
#else
	constexpr bool bAllowLegacyFixture = false;
#endif
	if (!bAllowLegacyFixture && (!IsValid(CanonicalRegistry) || !CanonicalRegistry->IsConfigured()))
	{
		OutError = TEXT("Starting kit requires the configured canonical ItemSystem.");
		return false;
	}
	if (KitId.IsNone())
	{
		return true; // Existing unconfigured class behaviour stays unchanged.
	}
	if (!IsValid(ClassDefinition) || !InOutCharacter.CharacterId.IsValid())
	{
		OutError = TEXT("Invalid class or character for starting equipment.");
		return false;
	}
	const FRPGStartingEquipmentKit* Kit = ClassDefinition->FindStartingEquipmentKit(KitId);
	if (!Kit || Kit->Items.IsEmpty())
	{
		OutError = FString::Printf(TEXT("Starting kit %s is not defined."), *KitId.ToString());
		return false;
	}

	// Stage everything locally. Partial grants must never leak on failure.
	FGridCharacterInventoryState Candidate = InOutCharacter;
	FGridCharacterEquipmentState CandidateEquipment = InOutEquipment;
	TArray<UGridItemDefinitionAsset*> CandidateDefinitions;
	TMap<FName, UGridItemDefinitionAsset*> DefinitionById;
	bool bTwoHandedHandReserved = false;

	for (const FRPGStartingEquipmentEntry& Entry : Kit->Items)
	{
		UGridItemDefinitionAsset* Definition = Entry.ItemDefinition.Get();
		const bool bExplicitCanonical = !Entry.CanonicalItemId.IsNone();
		const FName ItemId = bExplicitCanonical ? Entry.CanonicalItemId
			: (IsValid(Definition) ? Definition->ItemDefinitionId : NAME_None);
		const bool bCanonicalRegistered = CanonicalRegistry &&
			CanonicalRegistry->HasCanonicalItem(ItemId);
		if (!bAllowLegacyFixture && !bCanonicalRegistered)
		{
			OutError = TEXT("Starting kit item is not registered in the canonical ItemSystem.");
			return false;
		}
		if (ItemId.IsNone() ||
			Entry.ItemLevel < 0 || Entry.ItemLevel > 20 || Entry.Quantity < 1 ||
			(!IsValid(Definition) && !bExplicitCanonical) ||
			(bExplicitCanonical && (!bCanonicalRegistered ||
				(IsValid(Definition) && Definition->ItemDefinitionId != ItemId))))
		{
			OutError = TEXT("Starting kit contains an invalid item definition, canonical ID, level or quantity.");
			return false;
		}
		// All production kits have been migrated (5C). A registered
		// physical item now requires an explicit stable ID and level.
		// Never resurrect the old pointer-only N0/default-level rule.
		if (bCanonicalRegistered && !bExplicitCanonical)
		{
			OutError = TEXT("Registered starting kit item requires CanonicalItemId and an explicit physical level.");
			return false;
		}
		const bool bExplicitNative = bCanonicalRegistered;
		FGridResolvedItem NativeItem;
		if (bExplicitNative &&
			(!CanonicalRegistry->ResolveItem(ItemId, Entry.ItemLevel, NativeItem) ||
			 !NativeItem.IsValid() ||
			 !FMath::IsFinite(NativeItem.WeightKg) || NativeItem.WeightKg < 0.0f ||
			 (!NativeItem.bIsArmament && Entry.ItemLevel != 0)))
		{
			OutError = TEXT("Starting kit canonical item requires an explicit valid physical level.");
			return false; // No arbitrary N0 -> N1 promotion.
		}
		// The returned old-definition map is strictly outside the
		// registry. Matching obsolete pointers on registered entries
		// are checked for identity above and otherwise ignored.
		if (!bExplicitNative)
		{
			if (UGridItemDefinitionAsset** Previous = DefinitionById.Find(ItemId))
			{
				if (*Previous != Definition)
				{
					OutError = TEXT("Starting kit maps one item ID to multiple definitions.");
					return false;
				}
			}
			else
			{
				DefinitionById.Add(ItemId, Definition);
				CandidateDefinitions.Add(Definition);
			}
		}
		const int32 StackCapacity = bExplicitNative ? FMath::Max(1, NativeItem.MaxStack)
			: FMath::Max(1, Definition->MaxStackSize);
		const bool bStackable = bExplicitNative ? StackCapacity > 1 : Definition->bStackable;
		if ((!bStackable && Entry.Quantity != 1) ||
			(bStackable && Entry.Quantity > StackCapacity) ||
			(Entry.TargetSlot != EGridEquipmentSlot::None && Entry.Quantity != 1))
		{
			OutError = TEXT("Starting kit quantity is incompatible with the item or equipment slot.");
			return false;
		}

		int32 FixedLevel = 0;
		float FixedWeight = bExplicitNative ? NativeItem.WeightKg : Definition->Weight;
		if (bExplicitNative)
		{
			// Already validated the actual fixed level, weight and kind above.
			FixedLevel = Entry.ItemLevel;
		}
		else if (Definition->bUseWeaponCatalogue)
		{
			// Temporary path solely for identities absent from DA_ItemSystem.
			FixedLevel = Entry.ItemLevel == 0 ? Definition->DefaultWeaponItemLevel : Entry.ItemLevel;
			const FGridWeaponLevelProfile* Profile = nullptr;
			if (!Definition->TryGetWeaponCatalogueProfile(FixedLevel, Profile) || !Profile)
			{
				OutError = FString::Printf(TEXT("Starting kit item %s has no catalogue level %d."), *ItemId.ToString(), FixedLevel);
				return false;
			}
			FixedWeight = Profile->WeightKg;
		}
		else if (Entry.ItemLevel != 0)
		{
			OutError = FString::Printf(TEXT("Starting kit item %s is Legacy and has no levels."), *ItemId.ToString());
			return false;
		}

		const FGridStartingKitStaticMetadataReadView Metadata =
			FGridStartingKitStaticMetadataReadModel::Resolve(
				CanonicalRegistry, ItemId, FixedLevel,
				bExplicitNative ? FText::GetEmpty() : Definition->DisplayName, FixedWeight);
		if (!Metadata.bResolved)
		{
			OutError = FString::Printf(
				TEXT("Starting kit item %s cannot resolve canonical metadata at level %d."),
				*ItemId.ToString(), FixedLevel);
			return false;
		}

		EGridEquipmentSlot PhysicalSlot = Entry.TargetSlot;
		bool bEntryTwoHanded = false;
		if (PhysicalSlot != EGridEquipmentSlot::None)
		{
			// 06.3AO: the hand policy must agree with later equipment runtime
			// even if DA_Weapon_* has obsolete HandUsage/CompatibleSlots.
			const FGridStartingKitEquipmentPolicyReadView Hand = bExplicitNative
				? FGridStartingKitEquipmentPolicyReadModel::ResolveCanonical(
					CanonicalRegistry, ItemId, FixedLevel, PhysicalSlot)
				: FGridStartingKitEquipmentPolicyReadModel::Resolve(
					CanonicalRegistry, Definition, FixedLevel, PhysicalSlot);
			if (!Hand.bResolved || !CandidateEquipment.GetMutableSlot(Hand.PhysicalSlot))
			{
				OutError = FString::Printf(TEXT("Starting kit item %s is incompatible with slot %d."), *ItemId.ToString(), int32(PhysicalSlot));
				return false; // No partial kit commit.
			}
			bEntryTwoHanded = Hand.bTwoHanded;
			PhysicalSlot = Hand.PhysicalSlot;
			if ((bEntryTwoHanded && (CandidateEquipment.MainHand.IsValid() || CandidateEquipment.OffHand.IsValid())) ||
				(bTwoHandedHandReserved &&
					(PhysicalSlot == EGridEquipmentSlot::MainHand || PhysicalSlot == EGridEquipmentSlot::OffHand)))
			{
				OutError = TEXT("Starting kit hands conflict with a two-handed item.");
				return false;
			}
		}

		FGridItemInstance Item;
		Item.RuntimeObjectId = FGuid::NewGuid();
		Item.ItemDefinitionId = ItemId;
		Item.ItemLevel = FixedLevel;
		Item.Quantity = Entry.Quantity;
		Item.OwnerGuid = Candidate.CharacterId;
		Item.OwnerCharacterIndex = 0;
		Item.bLightsEnabled = bExplicitNative
			? NativeItem.LightEmitter.bDefaultEnabled
			: Definition->IsLightEnabledByDefault();

		if (PhysicalSlot == EGridEquipmentSlot::None)
		{
			FGridInventorySlot* Destination = Candidate.InventorySlots.FindByPredicate(
				[](const FGridInventorySlot& Slot) { return Slot.IsEmpty(); });
			if (!Destination)
			{
				OutError = TEXT("Not enough bag slots for the starting kit.");
				return false;
			}
			Item.OwnerType = EGridItemOwnerType::CharacterInventory;
			Item.EquipmentSlot = EGridEquipmentSlot::None;
			Destination->bOccupied = true;
			Destination->Item = Item;
		}
		else
		{
			FGridItemInstance* Equipped = CandidateEquipment.GetMutableSlot(PhysicalSlot);
			if (!Equipped || Equipped->IsValid())
			{
				OutError = TEXT("Two starting items target the same equipment slot.");
				return false;
			}
			Item.OwnerType = EGridItemOwnerType::EquipmentSlot;
			Item.EquipmentSlot = PhysicalSlot;
			*Equipped = Item;
			if (bEntryTwoHanded)
			{
				bTwoHandedHandReserved = true;
			}
		}
	}

	Candidate.StartingEquipmentKitId = KitId;
	InOutCharacter = MoveTemp(Candidate);
	InOutEquipment = MoveTemp(CandidateEquipment);
	OutDefinitions = MoveTemp(CandidateDefinitions);
	return true;
}
