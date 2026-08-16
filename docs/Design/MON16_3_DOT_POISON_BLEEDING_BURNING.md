# MON16.3 — DoT Poison / Bleeding / Burning

## Statut

**Implémenté — validation UE5 en attente.**

Base :

```text
91f5157b483f39e6ec6b6350ea6d16d95da3f476
Close MON16.2 status effect lifecycle
```

MON16.3 donne aux effets de statut un premier comportement gameplay réel : des dégâts périodiques data-driven déclenchés par les frontières `Turns` / `Rounds` de MON16.2.

## Périmètre

Implémenté : profil de dégâts périodiques générique, résolution déterministe sans jet d'attaque, réutilisation des résistances/multiplicateurs/pools d'armure existants, scaling par stacks, tick avant décrément/expiration, personnages et monstres, mort de monstre par DoT via le pipeline existant.

Hors périmètre : application automatique de Poison/Bleeding/Burning par une attaque ou un sort, chance d'infliger un statut, immunités de statut distinctes, dispel/cure, Haste/Slow, Stun/Silence/Immobilize, HUD/icônes/WBP, sauvegarde/restauration, nouveaux VFX/audio dédiés aux statuts.

## 1. Architecture réutilisée

MON16.3 ne crée pas de second système de dégâts.

Le calcul passe par :

```text
FGridStatusEffectPeriodicDamageResolver
        |
        v
FGridCombatResolver::ResolveDirectDamage()
        |
        +-- DamageMultiplier existant
        +-- ResistancePercent existant
        +-- PhysicalArmor / MagicalArmor existants
        +-- Health existant
```

`ResolveDirectDamage()` est un chemin garanti du resolver de combat : aucun d20, aucune Evasion, aucun critique. Le reste de la résolution est partagé avec une attaque normale.

## 2. Profil data-driven

`UGridStatusEffectDefinitionAsset` reçoit :

```cpp
FGridStatusEffectPeriodicDamageProfile PeriodicDamage;
```

avec :

```text
DamageType
DamagePerStack
```

`DamagePerStack = 0` signifie que le statut n'inflige aucun dégât périodique.

Aucun code de production ne teste `EffectId == Poison`, `Bleeding` ou `Burning`. Ces noms correspondent à des configurations de données du même moteur.

Exemples de configuration future :

```text
Poison
  DamageType     = Poison
  DamagePerStack = <valeur de design>

Bleeding
  DamageType     = Physical
  DamagePerStack = <valeur de design>

Burning
  DamageType     = Fire
  DamagePerStack = <valeur de design>
```

MON16.3 ne crée volontairement aucun `.uasset` pour ces exemples.

## 3. Autorité runtime

`FGridStatusEffectRuntimeState` garde `EffectId` comme identité stable et reçoit un pointeur `Transient` vers sa définition statique :

```cpp
TObjectPtr<UGridStatusEffectDefinitionAsset> DefinitionAsset;
```

Ce pointeur sert uniquement au runtime pour retrouver le profil périodique sans hard-code ni Asset Manager lookup à chaque frontière.

Il n'est pas une identité de source et ne fait pas partie du contrat de persistance MON16.7. `SourceId` conserve son rôle existant.

## 4. Sémantique du tick

Pour un effet `Turns` :

```text
fin de l'activation de la cible
    -> résolution du DoT
    -> décrément RemainingDuration
    -> expiration éventuelle
```

Pour un effet `Rounds` :

```text
Round 1
    -> baseline uniquement

frontière Round N -> N+1
    -> résolution du DoT
    -> décrément RemainingDuration
    -> expiration éventuelle
```

Ainsi, une durée de 2 produit exactement deux ticks aux deux frontières logiques correspondantes. Le dernier tick est appliqué **avant** la suppression de l'effet.

Les effets `Permanent` avec dégâts périodiques sont rejetés en MON16.3 car aucun événement périodique permanent distinct n'est défini.

## 5. Scaling par stacks

Le dégât brut est :

```text
RawDamage = DamagePerStack * StackCount
```

avec saturation à `MAX_int32`.

`Potency` n'intervient jamais dans le calcul des dégâts. Elle reste exclusivement le critère de précédence de `ReplaceIfStronger` défini par MON16.2.

## 6. Résistances et armures

MON16.3 réutilise `EGridDamageType` et `FGridDamageResistanceSet`.

Personnages :

```text
ComputeCharacterEquipmentResistances()
        -> FGridCombatResolver::GetResistancePercent()
```

Monstres :

```text
MonsterDefinition->GetDamageMultiplier()
```

Le routage des pools reste celui du resolver existant :

```text
Physical -> PhysicalArmor -> Health
non-Physical -> MagicalArmor -> Health
```

Donc, avec les configurations ci-dessus :

```text
Bleeding -> PhysicalArmor
Burning  -> MagicalArmor
Poison   -> MagicalArmor
```

Aucun contournement spécial d'armure n'est introduit dans MON16.3.

## 7. Mort et ordre déterministe

Les effets sont déjà stockés par ordre déterministe d'`EffectId`. Ils sont résolus dans cet ordre.

Si une cible meurt pendant ses DoT, les effets périodiques suivants ne lui infligent plus de dégâts.

Pour un monstre, le résultat est appliqué avec `AGridMonsterActor::ApplyAttackResult()`. Un DoT létal passe donc par `MarkDead()` / `DeathComponent` et le pipeline de mort existant.

À une frontière globale de round, les personnages sont résolus avant les monstres. Si toute la party est tuée par les DoT de round, MON16.3 appelle la défaite avant de résoudre les monstres. Cette règle conserve la précédence déjà utilisée par `BeginNextCombatantTurn()` : défaite de la party avant test de victoire.

## 8. Intégration au lifecycle MON16.2

`UGridStatusEffectLifecycleSubsystem` reste l'autorité temporelle. MON16.3 lui ajoute seulement la phase de résolution périodique juste avant chaque `AdvanceDuration()`.

Il n'y a toujours :

- aucun tick frame ;
- aucun timer en secondes ;
- aucune horloge parallèle ;
- aucune dépendance Widget/UMG.

## 9. Fichiers

Modifiés :

```text
Source/GrimrockPrototype/Public/Runtime/Combat/GridCombatResolver.h
Source/GrimrockPrototype/Private/Runtime/Combat/GridCombatResolver.cpp
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectTypes.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectTypes.cpp
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectDefinitionAsset.cpp
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp
```

Ajoutés :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectPeriodicDamageResolver.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON163StatusEffectPeriodicDamageTests.cpp
docs/Design/MON16_3_DOT_POISON_BLEEDING_BURNING.md
docs/Design/MON16_3_VALIDATION_CHECKLIST.md
```

Aucun `.uasset`, `.umap` ou WBP.

## 10. Automation

Namespace :

```text
Grimrock.RPG.MON16.3
```

Tests :

```text
DefinitionValidation
DirectDamagePipeline
DamageTypeRouting
StackScaling
TurnLifecycleParty
TurnLifecycleMonster
LethalMonsterDot
RoundLifecycle
NonPeriodicIsolation
MonsterDamageMultiplier
NoParallelSystem
```

Attendu : **11/11 Success**.

Régressions minimales après succès ciblé :

```text
Automation RunTests Grimrock.RPG.MON16.2
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

MON16.3 ne sera déclaré **VALIDÉ ET CLOS** qu'après chargement/compilation UE5.5.4 et succès réel des tests sur logs utilisateur.

Prochaine étape après clôture : **MON16.4 — Haste / Slow & InitiativeModifier**.
