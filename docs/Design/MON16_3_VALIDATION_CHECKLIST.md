# MON16.3 — Validation Checklist

## État

```text
Implémentation C++ : VALIDÉE
Documentation      : VALIDÉE
UE5.5.4             : nouveaux tests C++ chargés et exécutés
Automation MON16.3 : 11/11 SUCCESS
Régression MON16.2 : 10/10 SUCCESS
Régression MON16.1 :  7/7 SUCCESS
Régression MON15   : 42/42 SUCCESS
Régression MON14   : 19/19 SUCCESS
Campagne fournie   : 123/123 SUCCESS
Clôture            : OUI
```

Base : `91f5157b483f39e6ec6b6350ea6d16d95da3f476`.

Commit d'implémentation : `afd2ab1c3ade2d267ea7641eb4d61547e3dc9184`.

MON16.3 est **VALIDÉ ET CLOS** depuis le 16 août 2026.

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

## Validation UE5.5.4

Le log utilisateur du 16 août 2026 contient **123 tests terminés**, tous avec `Result={Success}`. Aucun `Result={Fail}` ni erreur Automation Controller n'est présent.

Les nouveaux tests C++ MON16.3 sont découverts et exécutés par l'éditeur UE5.5.4, ce qui confirme que le code MON16.3 correspondant est compilé et chargé dans cette session de validation.

## Automation MON16.3

- [x] `DefinitionValidation` — Success
- [x] `DirectDamagePipeline` — Success
- [x] `DamageTypeRouting` — Success
- [x] `StackScaling` — Success
- [x] `TurnLifecycleParty` — Success
- [x] `TurnLifecycleMonster` — Success
- [x] `LethalMonsterDot` — Success
- [x] `RoundLifecycle` — Success
- [x] `NonPeriodicIsolation` — Success
- [x] `MonsterDamageMultiplier` — Success
- [x] `NoParallelSystem` — Success

Bilan : **11/11 Success**.

## Contrôles runtime observés

- [x] Poison létal : `HP=2->0`
- [x] multiplicateur monstre : `Raw=6`, `Multiplier=0.500`, puis armure/HP
- [x] Poison `Rounds` : deux ticks visibles `10->8->6`
- [x] Burning monstre : armure magique consommée avant HP
- [x] Burning personnage : dégâts appliqués à la frontière du tour

## Régressions

```text
MON16.2    10/10 Success
MON16.1     7/7 Success
MON15      42/42 Success
MON14      19/19 Success
```

- [x] MON16.2 sans régression
- [x] MON16.1 sans régression
- [x] MON15 sans régression
- [x] MON14 sans régression

## Clôture

Critères satisfaits :

```text
[x] code MON16.3 chargé/exécuté sous UE5.5.4
[x] MON16.3 11/11 Success
[x] MON16.2 10/10 Success
[x] MON16.1 7/7 Success
[x] MON15 42/42 Success
[x] MON14 19/19 Success
[x] aucun échec dans les 123 tests terminés du log fourni
[x] documentation à jour dans docs/Design
```

**MON16.3 — VALIDÉ ET CLOS.**

Prochaine étape : `MON16.4 — Haste / Slow & InitiativeModifier`.
