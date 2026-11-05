#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCatalystAreaResolver.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

namespace
{
	FGridCombatModifierContext MakeSurfaceDamageContext(const FGridCombatSurfaceState& Surface)
	{
		return FGridCombatModifierResolver::MakeAttackContext(
			Surface.SourceActionId, Surface.SourceActionId, EGridCombatActionSourcePolicy::Universal, EGridCombatActionType::Ability,
			Surface.PeriodicDamageType, EGridPhysicalDamageSubtype::None);
	}
}


void UGridTurnManagerComponent::ApplyCatalystBurstDamage(
	const TArray<FGridCatalystCellOutcome>& Outcomes)
{
	if (!IsValid(RuntimeActor) || !IsValid(PartyPawn) || !GetWorld())
	{
		return;
	}
	UGridMonsterOccupancySubsystem* Occupancy = GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>();
	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent.Get();
	const FIntPoint PartyCell(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	TSet<FGuid> DamagedMonsters;
	bool bDamagedParty = false;
	for (const FGridCatalystCellOutcome& Outcome : Outcomes)
	{
		if (!Outcome.Reaction.bReacted || !Outcome.Reaction.bExplosive)
		{
			continue;
		}
		const int32 Raw = FMath::Clamp(FMath::RoundToInt(
			static_cast<float>(FMath::Max(0, RuntimeActor->CatalystPoisonBurstDamage)) *
			FMath::Max(0.f, 1.f + static_cast<float>(Outcome.Reaction.ExplosionDamagePercentModifier) / 100.f)),
			0, 10000);
		if (Raw == 0)
		{
			continue;
		}
		if (IsValid(Occupancy))
		{
			AGridMonsterActor* Monster = Occupancy->GetOccupantAtCell(Outcome.Cell);
			if (IsValid(Monster) && !Monster->IsDead() &&
				!DamagedMonsters.Contains(Monster->ResolvePersistenceId()))
			{
				DamagedMonsters.Add(Monster->ResolvePersistenceId());
				FGridAttackTargetStats TargetBefore;
				TargetBefore.CurrentHealth = Monster->CurrentHealth;
				TargetBefore.PhysicalArmor = Monster->CurrentPhysicalArmor;
				TargetBefore.MagicalArmor = Monster->CurrentMagicalArmor;
				TargetBefore.DamageMultiplier = IsValid(Monster->MonsterDefinition)
					? Monster->MonsterDefinition->GetDamageMultiplier(EGridDamageType::Fire, EGridPhysicalDamageSubtype::None)
					: 1.f;
				TArray<FGridCombatModifierProfile> TargetProfiles;
				if (FGridCombatModifierResolver::CollectStatusModifiers(Monster->StatusEffects, TargetProfiles))
				{
					FGridResolvedCombatModifiers Mods;
					FGridCombatSurfaceState ProxySurface;
					ProxySurface.SurfaceType = EGridCombatSurfaceType::Fire;
					ProxySurface.SourceActionId = TEXT("Action_Mage_SurfaceCatalyst_01");
					ProxySurface.PeriodicDamageType = EGridDamageType::Fire;
					FGridCombatModifierResolver::Resolve(
						TargetProfiles, MakeSurfaceDamageContext(ProxySurface), Mods);
					FGridCombatModifierResolver::ApplyIncomingAttackModifiers(
						TargetBefore, EGridDamageType::Fire, Mods);
				}
				const FGridAttackResult Damage = FGridCombatResolver::ResolveDirectDamage(
					TargetBefore, EGridDamageType::Fire, Raw);
				Monster->ApplyAttackResult(Damage);
				UE_LOG(LogGridTurnManager, Log,
					TEXT("[ARMEMENT01.17.3B] PoisonBurst Monster=%s Cell=(%d,%d) Raw=%d HP=%d->%d"),
					*GetNameSafe(Monster), Outcome.Cell.X, Outcome.Cell.Y,
					Raw, Damage.TargetHealthBefore, Damage.TargetHealthAfter);
			}
		}
		if (bDamagedParty || !IsValid(Inventory) || Outcome.Cell != PartyCell)
		{
			continue;
		}
		bDamagedParty = true;
		for (int32 CharacterIndex = 0; CharacterIndex < Inventory->PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
		{
			FGridCharacterInventoryState& Character =
				Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
			if (Character.Resources.CurrentHealth <= 0)
			{
				continue;
			}
			FGridAttackTargetStats TargetBefore;
			TargetBefore.CurrentHealth = Character.Resources.CurrentHealth;
			TargetBefore.PhysicalArmor = Inventory->GetEffectiveCurrentPhysicalArmor(CharacterIndex);
			TargetBefore.MagicalArmor = Inventory->GetEffectiveCurrentMagicalArmor(CharacterIndex);
			TargetBefore.DamageMultiplier = 1.f;
			TargetBefore.ResistancePercent = FGridCombatResolver::GetResistancePercent(
				Inventory->ComputeCharacterEquipmentResistances(CharacterIndex), EGridDamageType::Fire);
			TArray<FGridCombatModifierProfile> Profiles;
			if (FGridCombatModifierResolver::CollectCharacterModifiers(Character, Profiles))
			{
				FGridResolvedCombatModifiers Mods;
				FGridCombatSurfaceState ProxySurface;
				ProxySurface.SurfaceType = EGridCombatSurfaceType::Fire;
				ProxySurface.SourceActionId = TEXT("Action_Mage_SurfaceCatalyst_01");
				ProxySurface.PeriodicDamageType = EGridDamageType::Fire;
				FGridCombatModifierResolver::Resolve(Profiles, MakeSurfaceDamageContext(ProxySurface), Mods);
				FGridCombatModifierResolver::ApplyIncomingAttackModifiers(TargetBefore, EGridDamageType::Fire, Mods);
			}
			const FGridAttackResult Damage =
				FGridCombatResolver::ResolveDirectDamage(TargetBefore, EGridDamageType::Fire, Raw);
			if (Damage.bHit)
			{
				Inventory->ApplyPhysicalArmorDamage(
					CharacterIndex, Damage.PhysicalArmorDamage, &Damage.Trace.Damage.PhysicalArmorSources);
				Inventory->ApplyMagicalArmorDamage(CharacterIndex, Damage.MagicalArmorDamage);
				Character.Resources.CurrentHealth = FMath::Max(0, Character.Resources.CurrentHealth - Damage.HealthDamage);
			}
			Inventory->NotifyPartyInventoryChanged(CharacterIndex);
		}
	}
}

void UGridTurnManagerComponent::ResolveSurfaceDamageTick()
{
	if (!IsValid(RuntimeActor))
	{
		return;
	}

	TMap<FIntPoint, FGridCombatSurfaceState> Surfaces;
	RuntimeActor->GetCurrentCombatSurfaceSnapshot(Surfaces);

	UGridStatusEffectLifecycleSubsystem* StatusLifecycle =
		GetWorld() ? GetWorld()->GetSubsystem<UGridStatusEffectLifecycleSubsystem>() : nullptr;
	if (StatusLifecycle)
	{
		StatusLifecycle->BindToTurnManager(this);
	}

	UGridPartyInventoryComponent* Inventory =
		IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
	const FIntPoint PartyCell =
		IsValid(PartyPawn) ? FIntPoint(PartyPawn->CurrentCellX, PartyPawn->CurrentCellY) : FIntPoint(INDEX_NONE, INDEX_NONE);
	if (IsValid(Inventory))
	{
		if (const FGridCombatSurfaceState* Surface = Surfaces.Find(PartyCell))
		{
			for (int32 CharacterIndex = 0; CharacterIndex < Inventory->PartyInventoryState.ActiveCharacters.Num(); ++CharacterIndex)
			{
				FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
				if (Character.Resources.CurrentHealth <= 0)
				{
					continue;
				}

				FGridAttackTargetStats TargetBefore;
				TargetBefore.CurrentHealth = Character.Resources.CurrentHealth;
				// Physical armor includes remaining protection carried by equipped item instances.
				TargetBefore.PhysicalArmor = Inventory->GetEffectiveCurrentPhysicalArmor(CharacterIndex);
				TargetBefore.MagicalArmor = Inventory->GetEffectiveCurrentMagicalArmor(CharacterIndex);
				TargetBefore.DamageMultiplier = 1.0f;
				const FGridDamageResistanceSet Resistances = Inventory->ComputeCharacterEquipmentResistances(CharacterIndex);
				TargetBefore.ResistancePercent =
					FGridCombatResolver::GetResistancePercent(Resistances, Surface->PeriodicDamageType);

				TArray<FGridCombatModifierProfile> TargetProfiles;
				if (FGridCombatModifierResolver::CollectCharacterModifiers(Character, TargetProfiles))
				{
					FGridResolvedCombatModifiers TargetModifiers;
					FGridCombatModifierResolver::Resolve(TargetProfiles, MakeSurfaceDamageContext(*Surface), TargetModifiers);
					FGridCombatModifierResolver::ApplyIncomingAttackModifiers(TargetBefore, Surface->PeriodicDamageType, TargetModifiers);
				}

				FGridAttackResult Damage =
					FGridCombatResolver::ResolveDirectDamage(TargetBefore, Surface->PeriodicDamageType, Surface->PeriodicDamagePerRound);
				if (Damage.bHit)
				{
					// Match the canonical monster-attack damage commitment: deplete
					// equipped armor instances before the base physical pool.
					// Preserve the equipment-source trace for future feedback.
					Inventory->ApplyPhysicalArmorDamage(
						CharacterIndex, Damage.PhysicalArmorDamage, &Damage.Trace.Damage.PhysicalArmorSources);
					Inventory->ApplyMagicalArmorDamage(CharacterIndex, Damage.MagicalArmorDamage);
					Character.Resources.CurrentHealth = FMath::Max(0, Character.Resources.CurrentHealth - Damage.HealthDamage);
				}

				if (StatusLifecycle && !Surface->PeriodicStatusApplications.IsEmpty() && Character.Resources.CurrentHealth > 0)
				{
					StatusLifecycle->ApplyCombatStatusApplicationsToPartyCharacter(CharacterIndex, Surface->PeriodicStatusApplications,
						Surface->SourceCombatantId, TargetBefore, &Damage);
				}
				Inventory->NotifyPartyInventoryChanged(CharacterIndex);
				UE_LOG(LogGridTurnManager, Log,
					TEXT("[RPG03.6] SurfaceTick Target=Party Character=%d Cell=(%d,%d) Type=%s Raw=%d HP=%d->%d"),
					CharacterIndex, PartyCell.X, PartyCell.Y, *UEnum::GetValueAsString(Surface->SurfaceType),
					Surface->PeriodicDamagePerRound, Damage.TargetHealthBefore, Damage.TargetHealthAfter);
			}
		}
	}

	TArray<AGridMonsterActor*> Monsters;
	if (bCombatActive)
	{
		for (AGridMonsterActor* Monster : CombatMonsters)
		{
			if (IsValid(Monster)) Monsters.Add(Monster);
		}
	}
	else if (UGridMonsterOccupancySubsystem* Occupancy =
		GetWorld() ? GetWorld()->GetSubsystem<UGridMonsterOccupancySubsystem>() : nullptr)
	{
		for (const TPair<FIntPoint, FGridCombatSurfaceState>& Pair : Surfaces)
		{
			if (AGridMonsterActor* Monster = Occupancy->GetOccupantAtCell(Pair.Key))
			{
				Monsters.AddUnique(Monster);
			}
		}
	}

	for (AGridMonsterActor* Monster : Monsters)
	{
		if (!IsValid(Monster) || Monster->IsDead())
		{
			continue;
		}
		const FGridCombatSurfaceState* Surface = Surfaces.Find(Monster->CurrentCell);
		if (!Surface)
		{
			continue;
		}

		FGridAttackTargetStats TargetBefore;
		TargetBefore.CurrentHealth = Monster->CurrentHealth;
		TargetBefore.PhysicalArmor = Monster->CurrentPhysicalArmor;
		TargetBefore.MagicalArmor = Monster->CurrentMagicalArmor;
		TargetBefore.DamageMultiplier = IsValid(Monster->MonsterDefinition)
			? Monster->MonsterDefinition->GetDamageMultiplier(Surface->PeriodicDamageType, EGridPhysicalDamageSubtype::None)
			: 1.0f;

		TArray<FGridCombatModifierProfile> TargetProfiles;
		if (FGridCombatModifierResolver::CollectStatusModifiers(Monster->StatusEffects, TargetProfiles))
		{
			FGridResolvedCombatModifiers TargetModifiers;
			FGridCombatModifierResolver::Resolve(TargetProfiles, MakeSurfaceDamageContext(*Surface), TargetModifiers);
			FGridCombatModifierResolver::ApplyIncomingAttackModifiers(TargetBefore, Surface->PeriodicDamageType, TargetModifiers);
		}

		FGridAttackResult Damage =
			FGridCombatResolver::ResolveDirectDamage(TargetBefore, Surface->PeriodicDamageType, Surface->PeriodicDamagePerRound);
		Monster->ApplyAttackResult(Damage);
		if (StatusLifecycle && !Surface->PeriodicStatusApplications.IsEmpty() && !Monster->IsDead())
		{
			StatusLifecycle->ApplyCombatStatusApplicationsToMonster(
				Monster, Surface->PeriodicStatusApplications, Surface->SourceCombatantId, TargetBefore, &Damage);
		}
		UE_LOG(LogGridTurnManager, Log,
			TEXT("[RPG03.6] SurfaceTick Target=Monster Monster=%s Cell=(%d,%d) Type=%s Raw=%d HP=%d->%d"),
			*GetNameSafe(Monster), Monster->CurrentCell.X, Monster->CurrentCell.Y, *UEnum::GetValueAsString(Surface->SurfaceType),
			Surface->PeriodicDamagePerRound, Damage.TargetHealthBefore, Damage.TargetHealthAfter);
	}


}

void UGridTurnManagerComponent::ResolveExplorationSurfaceDamage(float ElapsedSeconds)
{
	if (bCombatActive || !IsValid(RuntimeActor) || !FMath::IsFinite(ElapsedSeconds) || ElapsedSeconds <= 0.f)
	{
		return;
	}
	ExplorationSurfaceTickElapsedSeconds += ElapsedSeconds;
	const float SecondsPerRound = FMath::Max(0.1f, RuntimeActor->OilExplorationSecondsPerRound);
	const int32 Ticks = FMath::Min(FMath::FloorToInt(ExplorationSurfaceTickElapsedSeconds / SecondsPerRound), 16);
	if (Ticks == 0)
	{
		return;
	}
	ExplorationSurfaceTickElapsedSeconds -= static_cast<float>(Ticks) * SecondsPerRound;
	for (int32 Index = 0; Index < Ticks; ++Index)
	{
		ResolveSurfaceDamageTick();
	}
}

void UGridTurnManagerComponent::ResolveCombatSurfaceRound()
{
	if (!bCombatActive || !IsValid(RuntimeActor))
	{
		return;
	}
	ExplorationSurfaceTickElapsedSeconds = 0.f;
	ResolveSurfaceDamageTick();

	TArray<FIntPoint> ExpiredCells;
	const int32 ExpiredCount = RuntimeActor->AdvanceCombatSurfaceRound(&ExpiredCells);
	if (ExpiredCount > 0)
	{
		UE_LOG(LogGridTurnManager, Log, TEXT("[RPG03.6] SurfaceRoundAdvanced Round=%d Expired=%d"), RoundNumber, ExpiredCount);
	}

	TArray<FIntPoint> ExpiredTrapCells;
	const int32 ExpiredTrapCount = RuntimeActor->AdvanceCombatTrapRound(&ExpiredTrapCells);
	if (ExpiredTrapCount > 0)
	{
		UE_LOG(LogGridTurnManager, Log, TEXT("[RPG03.9.2] TrapRoundAdvanced Round=%d Expired=%d"), RoundNumber, ExpiredTrapCount);
	}
}
