#include "Runtime/GridLevelRuntimeActor.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/GridEventFeedPublisher.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridDungeonTrapDamage, Log, All);

namespace GridDungeonTrapDamage
{
    /** Equivalent to the already validated RPG03.6 periodic surface context.
     * Hazards are guaranteed direct damage, not a fabricated attack action. */
    FGridCombatModifierContext MakeIncomingContext(const FGridDungeonTrapDefinitionParams& Trap)
    {
        return FGridCombatModifierResolver::MakeAttackContext(
            FName(TEXT("Dungeon.Trap.Activated")), FName(TEXT("Dungeon.Trap")),
            EGridCombatActionSourcePolicy::Universal, EGridCombatActionType::Ability,
            Trap.DamageType, Trap.PhysicalSubtype);
    }

    FText LocalizedDamageType(EGridDamageType Type, EGridPhysicalDamageSubtype Subtype)
    {
        switch (Type)
        {
            case EGridDamageType::Physical:
                switch (Subtype)
                {
                    case EGridPhysicalDamageSubtype::Slashing:
                        return NSLOCTEXT("TRAP01", "DamageSlashing", "tranchants");
                    case EGridPhysicalDamageSubtype::Piercing:
                        return NSLOCTEXT("TRAP01", "DamagePiercing", "perçants");
                    case EGridPhysicalDamageSubtype::Bludgeoning:
                        return NSLOCTEXT("TRAP01", "DamageBludgeoning", "contondants");
                    default:
                        return NSLOCTEXT("TRAP01", "DamagePhysical", "physiques");
                }
            case EGridDamageType::Fire: return NSLOCTEXT("TRAP01", "DamageFire", "de feu");
            case EGridDamageType::Ice: return NSLOCTEXT("TRAP01", "DamageIce", "de givre");
            case EGridDamageType::Lightning: return NSLOCTEXT("TRAP01", "DamageLightning", "de foudre");
            case EGridDamageType::Poison: return NSLOCTEXT("TRAP01", "DamagePoison", "de poison");
            case EGridDamageType::Holy: return NSLOCTEXT("TRAP01", "DamageHoly", "sacrés");
            case EGridDamageType::Necrotic: return NSLOCTEXT("TRAP01", "DamageNecrotic", "nécrotiques");
            case EGridDamageType::Arcane: return NSLOCTEXT("TRAP01", "DamageArcane", "arcaniques");
            default: return NSLOCTEXT("TRAP01", "DamageUnknown", "inconnus");
        }
    }

    /** Returns the selected living member for a trap intentionally not
     * configured as party-wide; stable first living fallback after death. */
    int32 ChooseSingleVictim(const UGridPartyInventoryComponent& Inventory)
    {
        const TArray<FGridCharacterInventoryState>& Characters =
            Inventory.PartyInventoryState.ActiveCharacters;
        const int32 Selected = Inventory.GetSelectedCharacterIndex();
        if (Characters.IsValidIndex(Selected) &&
            Characters[Selected].Resources.CurrentHealth > 0)
        {
            return Selected;
        }
        for (int32 Index = 0; Index < Characters.Num(); ++Index)
        {
            if (Characters[Index].Resources.CurrentHealth > 0)
            {
                return Index;
            }
        }
        return INDEX_NONE;
    }
}

int32 AGridLevelRuntimeActor::TriggerDungeonTrapsOnPartyEnterCell(
    AGrimrockPartyPawn* PartyPawn, int32 CellX, int32 CellY,
    FRandomStream& RandomStream)
{
    if (!IsValid(LevelAsset) || !LevelAsset->IsValidCoord(CellX, CellY) ||
        !IsValid(PartyPawn) || PartyPawn->LevelRuntimeActor != this ||
        PartyPawn->CurrentCellX != CellX || PartyPawn->CurrentCellY != CellY ||
        !IsValid(PartyPawn->PartyInventoryComponent))
    {
        return 0;
    }

    UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
    const TArray<FGridCharacterInventoryState>& Characters =
        Inventory->PartyInventoryState.ActiveCharacters;
    if (!Characters.ContainsByPredicate([](const FGridCharacterInventoryState& Character)
        { return Character.Resources.CurrentHealth > 0; }))
    {
        return 0;
    }

    // Copy only candidate placements before executing puzzle links: a link
    // can alter the world or travel to another level, invalidating iterators.
    UGridLevelAsset* EnteredLevel = LevelAsset.Get();
    TArray<FGridWorldObjectInstance> Candidates;
    for (const FGridWorldObjectInstance& Object : LevelAsset->WorldObjectInstances)
    {
        if (Object.Type == EGridLevelObjectType::Trigger &&
            Object.CellX == CellX && Object.CellY == CellY &&
            Object.InstanceId.IsValid())
        {
            Candidates.Add(Object);
        }
    }

    int32 Fired = 0;
    for (const FGridWorldObjectInstance& Object : Candidates)
    {
        if (LevelAsset.Get() != EnteredLevel || PartyPawn->LevelRuntimeActor != this ||
            PartyPawn->CurrentCellX != CellX || PartyPawn->CurrentCellY != CellY)
        {
            break; // A previous puzzle link changed the active level/cell.
        }
        const UGridWorldObjectDefinitionAsset* Definition =
            FindWorldObjectDefinition(Object.WorldObjectDefinitionId);
        if (!IsValid(Definition) ||
            Definition->SupportedType != EGridLevelObjectType::Trigger ||
            !Definition->DungeonTrap.bIsDungeonTrap ||
            !Definition->DungeonTrap.IsValidContract())
        {
            continue;
        }
        const FGridDungeonTrapDefinitionParams& Params = Definition->DungeonTrap;
        if (Params.ActivationPolicy != EGridDungeonTrapActivationPolicy::OnEnterCell)
        {
            continue; // OnCrossBoundary and OnInteract belong to later milestones.
        }

        // A previous trap in this same cell may have incapacitated everybody.
        // Do not consume a second hazard when there is no living victim left.
        if (!Inventory->PartyInventoryState.ActiveCharacters.ContainsByPredicate(
            [](const FGridCharacterInventoryState& Character)
            {
                return Character.Resources.CurrentHealth > 0;
            }))
        {
            break;
        }

        const EGridDungeonTrapState CurrentState = GetDungeonTrapState(Object.InstanceId);
        if (CurrentState == EGridDungeonTrapState::Unknown ||
            CurrentState == EGridDungeonTrapState::Disarmed ||
            CurrentState == EGridDungeonTrapState::Disabled ||
            (Params.bOneShot && CurrentState == EGridDungeonTrapState::Triggered))
        {
            continue;
        }

        // Build a complete transaction before any RNG/state changes.
        // A broken/empty party cannot consume the trap.
        const int32 SingleVictim = Params.bAffectsEntireParty
            ? INDEX_NONE : GridDungeonTrapDamage::ChooseSingleVictim(*Inventory);
        if (!Params.bAffectsEntireParty && SingleVictim == INDEX_NONE)
        {
            continue;
        }

        // One shared roll for this trap instance, never separate damage rolls
        // per character. Individual resistances and armor are applied below.
        const int32 RawDamage = Params.MinimumDamage == Params.MaximumDamage
            ? Params.MinimumDamage
            : RandomStream.RandRange(Params.MinimumDamage, Params.MaximumDamage);

        if (!TryRecordDungeonTrapTriggered(Object.InstanceId, PartyPawn))
        {
            continue; // No damage without a committed trigger state.
        }

        ++Fired;
        const FGridCombatModifierContext IncomingContext =
            GridDungeonTrapDamage::MakeIncomingContext(Params);
        for (int32 CharacterIndex = 0;
             CharacterIndex < Inventory->PartyInventoryState.ActiveCharacters.Num();
             ++CharacterIndex)
        {
            FGridCharacterInventoryState& Character =
                Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex];
            if (Character.Resources.CurrentHealth <= 0 ||
                (!Params.bAffectsEntireParty && CharacterIndex != SingleVictim))
            {
                continue;
            }

            FGridAttackTargetStats Target;
            Target.CurrentHealth = Character.Resources.CurrentHealth;
            Target.PhysicalArmor = Inventory->GetEffectiveCurrentPhysicalArmor(CharacterIndex);
            Target.MagicalArmor = Inventory->GetEffectiveCurrentMagicalArmor(CharacterIndex);
            Target.DamageMultiplier = 1.0f;
            Target.ResistancePercent = FGridCombatResolver::GetResistancePercent(
                Inventory->ComputeCharacterEquipmentResistances(CharacterIndex),
                Params.DamageType);

            // Reuse the same defensive modifier authority as combat surfaces:
            // class progression and status effects can protect against traps.
            TArray<FGridCombatModifierProfile> DefensiveProfiles;
            if (FGridCombatModifierResolver::CollectCharacterModifiers(
                Character, DefensiveProfiles))
            {
                FGridResolvedCombatModifiers DefensiveModifiers;
                FGridCombatModifierResolver::Resolve(
                    DefensiveProfiles, IncomingContext, DefensiveModifiers);
                FGridCombatModifierResolver::ApplyIncomingAttackModifiers(
                    Target, Params.DamageType, DefensiveModifiers);
            }

            FGridAttackResult Result =
                FGridCombatResolver::ResolveDirectDamage(
                    Target, Params.DamageType, RawDamage, Params.PhysicalSubtype);
            if (Result.bHit)
            {
                // The canonical inventory API depletes equipped armor before
                // base armor pools, preserving equipment-source provenance.
                Inventory->ApplyPhysicalArmorDamage(
                    CharacterIndex, Result.PhysicalArmorDamage,
                    &Result.Trace.Damage.PhysicalArmorSources);
                Inventory->ApplyMagicalArmorDamage(
                    CharacterIndex, Result.MagicalArmorDamage);
                Character.Resources.CurrentHealth = FMath::Max(
                    0, Character.Resources.CurrentHealth - Result.HealthDamage);
            }
            Inventory->NotifyPartyInventoryChanged(CharacterIndex);

            const FText Name = Character.DisplayName.IsEmpty()
                ? FText::Format(NSLOCTEXT("TRAP01", "TrapCharacterFallback",
                    "Personnage {0}"), FText::AsNumber(CharacterIndex + 1))
                : Character.DisplayName;
            const FText DamageLabel = GridDungeonTrapDamage::LocalizedDamageType(
                Params.DamageType, Params.PhysicalSubtype);
            const FText Feedback = FText::Format(
                NSLOCTEXT("TRAP01", "TrapDamageMember",
                    "{0} : {1} dégâts ({2}) aux PV, {3} à l'armure physique, {4} à l'armure magique."),
                Name, FText::AsNumber(Result.HealthDamage), DamageLabel,
                FText::AsNumber(Result.PhysicalArmorDamage),
                FText::AsNumber(Result.MagicalArmorDamage));

            GridEventFeedPublisher::PublishDungeonEvent(
                this, FName(TEXT("Dungeon.Trap.Damage")), Feedback,
                EGridHudEventSemantic::Mechanism, EGridEventVisibility::Always,
                true, GridEventFeedPublisher::SourceIdFromGuid(Object.InstanceId));

            UE_LOG(LogGridDungeonTrapDamage, Log,
                TEXT("[TRAP01.3] Instance=%s Member=%d Type=%s Subtype=%s Raw=%d Resistance=%d ArmorP=%d ArmorM=%d HP=%d->%d"),
                *Object.InstanceId.ToString(), CharacterIndex,
                *UEnum::GetValueAsString(Params.DamageType),
                *UEnum::GetValueAsString(Params.PhysicalSubtype),
                RawDamage, Result.ResistancePercent,
                Result.PhysicalArmorDamage, Result.MagicalArmorDamage,
                Result.TargetHealthBefore, Result.TargetHealthAfter);
        }

        // A successful, committed activation can now drive the authored puzzle
        // links. Only one-shot traps suppress future activations; non-one-shot
        // traps may fire again on another genuine cell entry.
        ExecuteLinksFromRuntimeObject(Object.InstanceId, EGridObjectEvent::Activated);
    }

    return Fired;
}
