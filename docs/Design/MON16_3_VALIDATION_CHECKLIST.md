# MON16.3 — Validation Checklist

## État

```text
Implémentation C++ : préparée
Documentation      : préparée
Compilation UE5    : EN ATTENTE
Automation MON16.3 : EN ATTENTE
Régressions        : EN ATTENTE
Clôture            : NON
```

Base : `91f5157b483f39e6ec6b6350ea6d16d95da3f476`.

## Architecture

- [x] réutilisation de `FGridCombatResolver`
- [x] aucun second système de résistance
- [x] `EGridDamageType` existant réutilisé
- [x] `FGridDamageResistanceSet` existant réutilisé
- [x] `MonsterDefinition->GetDamageMultiplier()` réutilisé
- [x] `AGridMonsterActor::ApplyAttackResult()` réutilisé
- [x] aucun hard-code Poison/Bleeding/Burning dans le moteur périodique
- [x] profil périodique data-driven sur la définition
- [x] référence de définition runtime `Transient`
- [x] aucune persistance ajoutée

## Règles de dégâts

- [x] aucun jet d'attaque
- [x] aucune Evasion
- [x] aucun critique
- [x] multiplier existant avant résistance
- [x] résistance en pourcentage existante
- [x] Physical -> PhysicalArmor -> Health
- [x] non-Physical -> MagicalArmor -> Health
- [x] scaling `DamagePerStack * StackCount`
- [x] `Potency` n'augmente pas le dégât
- [x] saturation du raw damage à `MAX_int32`

## Lifecycle

- [x] DoT résolu avant `AdvanceDuration()`
- [x] dernier tick exécuté avant expiration
- [x] `Turns` seulement à la frontière du combattant concerné
- [x] `Rounds` seulement aux frontières de round
- [x] Round 1 reste une baseline sans tick
- [x] `Permanent` DoT rejeté en MON16.3
- [x] mort arrête les DoT suivants de la même cible
- [x] mort de monstre passe par le pipeline existant
- [x] party wipe de round conserve la précédence Defeat

## Hors périmètre respecté

- [x] aucune chance d'application de statut
- [x] aucune attaque/spell modifiée pour appliquer Poison/Bleeding/Burning
- [x] aucune immunité de statut parallèle
- [x] aucun dispel/cure
- [x] aucun Haste/Slow effectif
- [x] aucun Stun/Silence/Immobilize effectif
- [x] aucun HUD/icône/WBP
- [x] aucun `.uasset`/`.umap`
- [x] aucune sauvegarde/restauration
- [x] aucun nouveau VFX/audio de statut

## Compilation UE5.5.4

Attendu : 0 erreur C++, UHT ou link.

- [ ] compilation / chargement confirmé par log utilisateur

## Automation ciblée

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16.3
```

- [ ] `DefinitionValidation` — Success
- [ ] `DirectDamagePipeline` — Success
- [ ] `DamageTypeRouting` — Success
- [ ] `StackScaling` — Success
- [ ] `TurnLifecycleParty` — Success
- [ ] `TurnLifecycleMonster` — Success
- [ ] `LethalMonsterDot` — Success
- [ ] `RoundLifecycle` — Success
- [ ] `NonPeriodicIsolation` — Success
- [ ] `MonsterDamageMultiplier` — Success
- [ ] `NoParallelSystem` — Success

Attendu : **11/11 Success**.

## Régressions minimales

Après MON16.3 vert :

```text
Automation RunTests Grimrock.RPG.MON16.2
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

- [ ] MON16.2 : 10/10 Success
- [ ] MON16.1 : 7/7 Success
- [ ] MON15 : 42/42 Success
- [ ] MON14 : 19/19 Success

## Clôture

MON16.3 pourra être marqué **VALIDÉ ET CLOS** après compilation/chargement UE5.5.4, 11/11 MON16.3 et régressions appropriées sans échec sur les logs utilisateur.

Prochaine étape : `MON16.4 — Haste / Slow & InitiativeModifier`.
