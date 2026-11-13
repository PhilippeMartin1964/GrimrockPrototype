#include "Runtime/Items/GridCanonicalItemCatalogues.h"

namespace
{
	bool AddError(TArray<FString>& Errors, const FString& Message)
	{
		Errors.Add(Message);
		return false;
	}

	bool IsArmId(FName Id)
	{
		const FString Value = Id.ToString();
		if (Value.Len() != 6 || !Value.StartsWith(TEXT("ARM")))
		{
			return false;
		}
		for (int32 Index = 3; Index < 6; ++Index)
		{
			if (Value[Index] < TCHAR('0') || Value[Index] > TCHAR('9'))
			{
				return false;
			}
		}
		const int32 Number = FCString::Atoi(*Value.Mid(3));
		return Number >= 1 && Number <= 82;
	}

	bool ValidateBindings(const TArray<FGridItemActionBinding>& Bindings,
		const UGridActionCatalogueAsset* Actions,
		const UGridActionPresentationCatalogueAsset* Presentations, const FString& Owner,
		TArray<FString>& Errors)
	{
		bool bValid = true;
		TSet<FName> Seen;
		for (const FGridItemActionBinding& Binding : Bindings)
		{
			if (Binding.ActionId.IsNone() || Binding.PresentationId.IsNone() ||
				Seen.Contains(Binding.ActionId))
			{
				bValid = AddError(Errors, FString::Printf(TEXT("%s: duplicate or incomplete action binding"),
					*Owner));
				continue;
			}
			Seen.Add(Binding.ActionId);
			if (!Actions->Find(Binding.ActionId) || !Presentations->Find(Binding.PresentationId))
			{
				bValid = AddError(Errors, FString::Printf(TEXT("%s: unknown action/presentation %s/%s"),
					*Owner, *Binding.ActionId.ToString(), *Binding.PresentationId.ToString()));
			}
		}
		return bValid;
	}
}

const FGridCanonicalAction* UGridActionCatalogueAsset::Find(FName ActionId) const
{
	return Actions.FindByPredicate([ActionId](const FGridCanonicalAction& Entry)
	{
		return Entry.ActionId == ActionId;
	});
}

bool UGridActionCatalogueAsset::Validate(TArray<FString>& Errors) const
{
	bool bValid = true;
	TSet<FName> Seen;
	for (const FGridCanonicalAction& Action : Actions)
	{
		if (Action.ActionId.IsNone() || Seen.Contains(Action.ActionId))
		{
			bValid = AddError(Errors, TEXT("Action catalogue: missing/duplicate ActionId"));
			continue;
		}
		Seen.Add(Action.ActionId);
		if (Action.CooldownRounds < 0 || Action.ManaCost < 0 ||
			Action.WeaponDamagePercent < 0 || Action.WeaponDamagePercent > 500)
		{
			bValid = AddError(Errors, FString::Printf(TEXT("Action %s: invalid cost/cooldown/scaling"),
				*Action.ActionId.ToString()));
		}
		for (const FGridCanonicalActionEffect& Effect : Action.Effects)
		{
			const bool bStatus = Effect.Kind == EGridCanonicalEffectKind::ApplyStatus ||
				Effect.Kind == EGridCanonicalEffectKind::RemoveStatus;
			if ((bStatus && Effect.StatusId.IsNone()) || (!bStatus && !Effect.StatusId.IsNone()))
			{
				bValid = AddError(Errors, FString::Printf(TEXT("Action %s: incorrect typed status effect"),
					*Action.ActionId.ToString()));
			}
		}
	}
	return bValid;
}

bool FGridCanonicalAudioVariants::IsValid() const
{
	if (!FMath::IsFinite(VolumeMultiplier) || VolumeMultiplier < 0.0f ||
		!FMath::IsFinite(PitchMin) || PitchMin <= 0.0f ||
		!FMath::IsFinite(PitchMax) || PitchMax < PitchMin)
	{
		return false;
	}
	for (const TSoftObjectPtr<USoundBase>& Sound : Sounds)
	{
		if (Sound.IsNull())
		{
			return false;
		}
	}
	return true;
}

bool FGridCanonicalVFXVariants::IsValid() const
{
	if (LocationOffset.ContainsNaN() || RotationOffset.ContainsNaN() ||
		Scale.ContainsNaN() || Scale.X <= 0.0f || Scale.Y <= 0.0f ||
		Scale.Z <= 0.0f)
	{
		return false;
	}
	for (const TSoftObjectPtr<UNiagaraSystem>& System : Systems)
	{
		if (System.IsNull())
		{
			return false;
		}
	}
	return true;
}

bool FGridCanonicalActionPresentation::IsValid() const
{
	if (PresentationId.IsNone() || !FMath::IsFinite(MotionDurationSeconds) ||
		MotionDurationSeconds < 0.0f || !FMath::IsFinite(ImpactDelaySeconds) ||
		ImpactDelaySeconds < 0.0f || !FMath::IsFinite(RecoveryDelaySeconds) ||
		RecoveryDelaySeconds < 0.0f || !FMath::IsFinite(FeedbackDurationSeconds) ||
		FeedbackDurationSeconds <= 0.0f || PeakLocationOffset.ContainsNaN() ||
		PeakRotationOffset.ContainsNaN())
	{
		return false;
	}
	return AttackAudio.IsValid() && ImpactHitAudio.IsValid() &&
		ImpactMissAudio.IsValid() && ImpactFleshAudio.IsValid() &&
		ImpactMetalAudio.IsValid() && ImpactWoodAudio.IsValid() &&
		ParryAudio.IsValid() && BlockAudio.IsValid() &&
		CriticalAudio.IsValid() && HeavyAudio.IsValid() &&
		AttackVFX.IsValid() && ImpactHitVFX.IsValid() &&
		ImpactMissVFX.IsValid() && ParryVFX.IsValid() && BlockVFX.IsValid();
}

const FGridCanonicalActionPresentation* UGridActionPresentationCatalogueAsset::Find(
	FName PresentationId) const
{
	return Presentations.FindByPredicate([PresentationId](const FGridCanonicalActionPresentation& Entry)
	{
		return Entry.PresentationId == PresentationId;
	});
}

bool UGridActionPresentationCatalogueAsset::Validate(TArray<FString>& Errors) const
{
	bool bValid = true;
	TSet<FName> Seen;
	for (const FGridCanonicalActionPresentation& Entry : Presentations)
	{
		if (Entry.PresentationId.IsNone() || Seen.Contains(Entry.PresentationId) ||
			!Entry.IsValid())
		{
			bValid = AddError(Errors, TEXT("Presentation catalogue: invalid settings, or missing/duplicate PresentationId"));
		}
		Seen.Add(Entry.PresentationId);
	}
	return bValid;
}

const FGridCanonicalWeaponVisual* UGridCanonicalWeaponVisualCatalogueAsset::FindForArchetype(
	FName ArchetypeId) const
{
	const FGridCanonicalWeaponVisualBinding* Binding = ArchetypeBindings.FindByPredicate(
		[ArchetypeId](const FGridCanonicalWeaponVisualBinding& Entry)
		{
			return Entry.ArchetypeId == ArchetypeId;
		});
	return Binding ? VisualProfiles.FindByPredicate(
		[Binding](const FGridCanonicalWeaponVisual& Entry)
		{
			return Entry.VisualId == Binding->VisualId;
		}) : nullptr;
}

bool UGridCanonicalWeaponVisualCatalogueAsset::Validate(TArray<FString>& Errors) const
{
	bool bValid = true;
	TSet<FName> SeenProfiles;
	for (const FGridCanonicalWeaponVisual& Profile : VisualProfiles)
	{
		if (Profile.VisualId.IsNone() || SeenProfiles.Contains(Profile.VisualId))
		{
			bValid = AddError(Errors, TEXT("Visual catalogue: missing/duplicate VisualId"));
		}
		SeenProfiles.Add(Profile.VisualId);
	}
	TSet<FName> SeenBindings;
	for (const FGridCanonicalWeaponVisualBinding& Binding : ArchetypeBindings)
	{
		if (!IsArmId(Binding.ArchetypeId) || SeenBindings.Contains(Binding.ArchetypeId) ||
			!SeenProfiles.Contains(Binding.VisualId))
		{
			bValid = AddError(Errors, FString::Printf(TEXT("Visual binding invalid: %s"),
				*Binding.ArchetypeId.ToString()));
		}
		SeenBindings.Add(Binding.ArchetypeId);
	}
	if (ActionIcons.Contains(EGridEquippedWeaponActionIconKind::None))
	{
		bValid = AddError(Errors, TEXT("Action icon must have a concrete equipped action kind"));
	}
	return bValid;
}

bool FGridCanonicalWeaponArchetype::TryResolve(int32 Level, FGridCanonicalResolvedWeapon& Out) const
{
	Out = FGridCanonicalResolvedWeapon();
	if (Level < 1 || Level > 20 || !IsArmId(ArchetypeId) ||
		!DiceCountGrowth.IsValid() || !DiceSidesGrowth.IsValid() || !FlatBonusGrowth.IsValid() ||
		!PhysicalArmorGrowth.IsValid() || !MagicalArmorGrowth.IsValid())
	{
		return false;
	}
	Out.Kind = Kind;
	Out.HandPolicy = HandPolicy;
	Out.DamageType = DamageType;
	Out.PhysicalSubtype = PhysicalSubtype;
	Out.ScalingAttribute = ScalingAttribute;
	Out.AccuracyBonus = AccuracyBonus;
	Out.ActionPointCost = ActionPointCost;
	Out.RangeCells = RangeCells;
	Out.PhysicalArmor = PhysicalArmorGrowth.Resolve(Level, PhysicalArmorN1);
	Out.MagicalArmor = MagicalArmorGrowth.Resolve(Level, MagicalArmorN1);
	Out.RequiredAmmoKindId = RequiredAmmoKindId;
	if (Out.PhysicalArmor < 0 || Out.MagicalArmor < 0 ||
		ActionPointCost < 0 || RangeCells < 0 ||
		(DamageType != EGridDamageType::Physical &&
			PhysicalSubtype != EGridPhysicalDamageSubtype::None))
	{
		return false;
	}
	const bool bOffensive = Kind == EGridCanonicalArmamentKind::Weapon;
	if (bOffensive != DamageDiceN1.bEnabled ||
		(!bOffensive && (DamageDiceN1.DiceCount != 0 || DamageDiceN1.DiceSides != 0 ||
			DamageDiceN1.FlatBonus != 0)) ||
		(bOffensive && (ActionPointCost == 0 || RangeCells == 0)) ||
		(!bOffensive && (ActionPointCost != 0 || RangeCells != 0 ||
			!RequiredAmmoKindId.IsNone() || Projectile.bEnabled)))
	{
		return false;
	}
	if (bOffensive)
	{
		Out.DamageDice.bEnabled = true;
		Out.DamageDice.DiceCount = DiceCountGrowth.Resolve(Level, DamageDiceN1.DiceCount);
		Out.DamageDice.DiceSides = DiceSidesGrowth.Resolve(Level, DamageDiceN1.DiceSides);
		Out.DamageDice.FlatBonus = FlatBonusGrowth.Resolve(Level, DamageDiceN1.FlatBonus);
		if (!Out.DamageDice.IsValid())
		{
			return false;
		}
	}
	else
	{
		// A shield/focus cannot acquire fake damage from unused dice-rule fields.
		Out.DamageDice.bEnabled = false;
		Out.DamageDice.DiceCount = 0;
		Out.DamageDice.DiceSides = 0;
		Out.DamageDice.FlatBonus = 0;
		if (DiceCountGrowth.StepAmount != 0 || DiceSidesGrowth.StepAmount != 0 ||
			FlatBonusGrowth.StepAmount != 0 ||
			!DiceCountGrowth.Exceptions.IsEmpty() || !DiceSidesGrowth.Exceptions.IsEmpty() ||
			!FlatBonusGrowth.Exceptions.IsEmpty())
		{
			return false;
		}
	}
	return true;
}

const FGridCanonicalWeaponArchetype* UGridCanonicalWeaponCatalogueAsset::Find(
	FName ArchetypeId) const
{
	return Archetypes.FindByPredicate([ArchetypeId](const FGridCanonicalWeaponArchetype& Entry)
	{
		return Entry.ArchetypeId == ArchetypeId;
	});
}

bool UGridCanonicalWeaponCatalogueAsset::Validate(TArray<FString>& Errors) const
{
	bool bValid = true;
	if (Archetypes.Num() != 82)
	{
		bValid = AddError(Errors, TEXT("Weapon catalogue: 82 ARM archetypes required"));
	}
	if (!ClassPreferences.IsEmpty())
	{
		if (ClassPreferences.Num() != 6)
		{
			bValid = AddError(Errors, TEXT("Weapon catalogue: six class preference rows required"));
		}
		TSet<FName> PreferenceIds;
		for (const FGridCanonicalClassPreference& Preference : ClassPreferences)
		{
			if (Preference.ClassId.IsNone() || PreferenceIds.Contains(Preference.ClassId))
			{
				bValid = AddError(Errors, TEXT("Weapon catalogue: duplicate/invalid class preference"));
			}
			PreferenceIds.Add(Preference.ClassId);
		}
	}
	int32 PassiveProfiles = 0;
	TSet<FName> Seen;
	for (const FGridCanonicalWeaponArchetype& Archetype : Archetypes)
	{
		if (!IsArmId(Archetype.ArchetypeId) || Seen.Contains(Archetype.ArchetypeId))
		{
			bValid = AddError(Errors, TEXT("Weapon catalogue: missing/invalid/duplicate ArchetypeId"));
			continue;
		}
		Seen.Add(Archetype.ArchetypeId);
		if (Archetype.Projectile.bEnabled &&
			(!FMath::IsFinite(Archetype.Projectile.InitialSpeed) ||
				Archetype.Projectile.InitialSpeed <= 0.0f ||
				!FMath::IsFinite(Archetype.Projectile.FlightLifetime) ||
				Archetype.Projectile.FlightLifetime <= 0.0f ||
				!FMath::IsFinite(Archetype.Projectile.Arc)))
		{
			bValid = AddError(Errors, TEXT("Weapon catalogue: invalid projectile policy"));
		}
		TSet<FName> SeenReactions;
		for (const FGridCanonicalReactionPolicy& Reaction : Archetype.Reactions)
		{
			if (Reaction.ReactionId.IsNone() || SeenReactions.Contains(Reaction.ReactionId) ||
				!FMath::IsFinite(Reaction.Chance) || Reaction.Chance < 0.0f ||
				Reaction.Chance > 1.0f || Reaction.Absorption < 0)
			{
				bValid = AddError(Errors, TEXT("Weapon catalogue: invalid/duplicate reaction"));
			}
			SeenReactions.Add(Reaction.ReactionId);
		}
		TSet<FName> SeenActions;
		for (const FGridItemActionBinding& Action : Archetype.Actions)
		{
			if (Action.ActionId.IsNone() || Action.PresentationId.IsNone() ||
				SeenActions.Contains(Action.ActionId))
			{
				bValid = AddError(Errors, TEXT("Weapon catalogue: duplicate/incomplete action binding"));
			}
			SeenActions.Add(Action.ActionId);
		}
		TSet<FName> SeenClasses;
		for (FName ClassId : Archetype.SecondaryClassIds)
		{
			if (ClassId.IsNone() || SeenClasses.Contains(ClassId) ||
				ClassId == Archetype.PrimaryClassId)
			{
				bValid = AddError(Errors, TEXT("Weapon catalogue: invalid/duplicate secondary class recommendation"));
			}
			SeenClasses.Add(ClassId);
		}
		if (!ClassPreferences.IsEmpty())
		{
			if (Archetype.PrimaryClassId.IsNone() ||
				!ClassPreferences.ContainsByPredicate([&Archetype](const FGridCanonicalClassPreference& Row)
				{
					return Row.ClassId == Archetype.PrimaryClassId;
				}))
			{
				bValid = AddError(Errors, TEXT("Weapon catalogue: missing/unknown primary class"));
			}
			for (FName ClassId : Archetype.SecondaryClassIds)
			{
				if (!ClassPreferences.ContainsByPredicate([ClassId](const FGridCanonicalClassPreference& Row)
				{
					return Row.ClassId == ClassId;
				}))
				{
					bValid = AddError(Errors, TEXT("Weapon catalogue: unknown secondary class"));
				}
			}
		}
		for (int32 Level = 1; Level <= 20; ++Level)
		{
			FGridCanonicalResolvedWeapon Weapon;
			if (!Archetype.TryResolve(Level, Weapon))
			{
				bValid = AddError(Errors, FString::Printf(TEXT("Weapon catalogue: invalid %s N%d"),
					*Archetype.ArchetypeId.ToString(), Level));
				break;
			}
			if (!Weapon.DamageDice.bEnabled)
			{
				++PassiveProfiles;
			}
		}
	}
	if (PassiveProfiles != 260)
	{
		bValid = AddError(Errors, FString::Printf(
			TEXT("Weapon catalogue: 260 passive profiles required; found %d"), PassiveProfiles));
	}
	return bValid;
}

bool UGridItemSystemAsset::Validate(TArray<FString>& Errors) const
{
	bool bValid = true;
	if (!WeaponCatalogue || !WeaponVisualCatalogue || !ActionCatalogue ||
		!ActionPresentationCatalogue)
	{
		return AddError(Errors, TEXT("Item system: four global catalogues are mandatory"));
	}
	bValid = WeaponCatalogue->Validate(Errors) && bValid;
	bValid = WeaponVisualCatalogue->Validate(Errors) && bValid;
	bValid = ActionCatalogue->Validate(Errors) && bValid;
	bValid = ActionPresentationCatalogue->Validate(Errors) && bValid;

	TSet<FName> Ids;
	TSet<FName> AmmoKinds;
	for (const UGridCanonicalItemDefinitionAsset* Item : ItemDefinitions)
	{
		if (!IsValid(Item) || !Item->HasValidIdentity() ||
			Ids.Contains(Item->ItemId))
		{
			bValid = AddError(Errors, TEXT("Item system: invalid or duplicate ItemId"));
			continue;
		}
		Ids.Add(Item->ItemId);
		if (const UGridAmmoDefinitionAsset* Ammo = Cast<UGridAmmoDefinitionAsset>(Item))
		{
			if (Ammo->AmmoKindId.IsNone() || Ammo->MaxStack < 1)
			{
				bValid = AddError(Errors, TEXT("Item system: invalid ammunition kind/stack"));
			}
			else if (AmmoKinds.Contains(Ammo->AmmoKindId))
			{
				bValid = AddError(Errors, TEXT("Item system: duplicate AmmoKindId"));
			}
			else
			{
				AmmoKinds.Add(Ammo->AmmoKindId);
			}
		}
		if (const UGridConsumableDefinitionAsset* Consumable = Cast<UGridConsumableDefinitionAsset>(Item))
		{
			if (Consumable->MaxStack < 1)
			{
				bValid = AddError(Errors, TEXT("Item system: invalid consumable stack size"));
			}
			bValid = ValidateBindings(Consumable->UseActions, ActionCatalogue,
				ActionPresentationCatalogue, Item->ItemId.ToString(), Errors) && bValid;
		}
		if (const UGridOtherEquipmentDefinitionAsset* Equipment =
			Cast<UGridOtherEquipmentDefinitionAsset>(Item))
		{
			TSet<EGridEquipmentSlot> Slots;
			for (EGridEquipmentSlot Slot : Equipment->EquipmentSlots)
			{
				if (Slot == EGridEquipmentSlot::None || Slots.Contains(Slot))
				{
					bValid = AddError(Errors, TEXT("Item system: invalid/duplicate equipment slot"));
				}
				Slots.Add(Slot);
			}
			if (Slots.IsEmpty())
			{
				bValid = AddError(Errors, TEXT("Item system: equipment needs a slot"));
			}
		}
		if (const UGridArmamentDefinitionAsset* Weapon = Cast<UGridArmamentDefinitionAsset>(Item))
		{
			if (!WeaponCatalogue->Find(Weapon->ArchetypeId) ||
				!WeaponVisualCatalogue->FindForArchetype(Weapon->ArchetypeId))
			{
				bValid = AddError(Errors, FString::Printf(
					TEXT("Item system: invalid armament references for %s"), *Item->ItemId.ToString()));
			}
		}
	}
	if (Ids.IsEmpty())
	{
		bValid = AddError(Errors, TEXT("Item system: no item definitions indexed"));
	}
	for (const FGridCanonicalWeaponArchetype& Weapon : WeaponCatalogue->Archetypes)
	{
		if (!WeaponVisualCatalogue->FindForArchetype(Weapon.ArchetypeId))
		{
			bValid = AddError(Errors, FString::Printf(
				TEXT("Item system: missing visual binding for %s"), *Weapon.ArchetypeId.ToString()));
		}
		if (!Weapon.RequiredAmmoKindId.IsNone() &&
			!AmmoKinds.Contains(Weapon.RequiredAmmoKindId))
		{
			bValid = AddError(Errors, FString::Printf(
				TEXT("Item system: unresolved ammo kind for %s"), *Weapon.ArchetypeId.ToString()));
		}
		bValid = ValidateBindings(Weapon.Actions, ActionCatalogue,
			ActionPresentationCatalogue, Weapon.ArchetypeId.ToString(), Errors) && bValid;
	}
	return bValid;
}
