#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
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

void UGridTurnManagerComponent::ResolveCombatSurfaceRound()
{
	if (!bCombatActive || !IsValid(RuntimeActor))
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
				TargetBefore.PhysicalArmor = Character.Resources.CurrentPhysicalArmor;
				TargetBefore.MagicalArmor = Character.Resources.CurrentMagicalArmor;
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
					Character.Resources.CurrentPhysicalArmor =
						FMath::Max(0, Character.Resources.CurrentPhysicalArmor - Damage.PhysicalArmorDamage);
					Character.Resources.CurrentMagicalArmor =
						FMath::Max(0, Character.Resources.CurrentMagicalArmor - Damage.MagicalArmorDamage);
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

	const TArray<TObjectPtr<AGridMonsterActor>> Monsters = CombatMonsters;
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
