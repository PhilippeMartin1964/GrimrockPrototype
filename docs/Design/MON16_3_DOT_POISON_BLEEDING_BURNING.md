# MON16.3 — DoT Poison / Bleeding / Burning

## Statut

**VALIDÉ ET CLOS — 16 août 2026.**

Base :

```text
91f5157b483f39e6ec6b6350ea6d16d95da3f476
Close MON16.2 status effect lifecycle
```

Commit logique d'implémentation :

```text
afd2ab1c3ade2d267ea7641eb4d61547e3dc9184
Add MON16.3 periodic status damage
```

MON16.3 donne aux effets de statut un premier comportement gameplay réel : des dégâts périodiques data-driven déclenchés par les frontières `Turns` / `Rounds` de MON16.2.

## Périmètre

Implémenté : profil de dégâts périodiques générique, résolution déterministe sans jet d'attaque, réutilisation des résistances/multiplicateurs/pools d'armure existants, scaling par stacks, tick avant décrément/expiration, personnages et monstres, mort de monstre par DoT via le pipeline existant.

Hors périmètre : application automatique de Poison/Bleeding/Burning par une attaque ou un sort, chance d'infliger un statut, immunités de statut distinctes, dispel/cure, Haste/Slow, Stun/Silence/Immobilize, HUD/icônes/WBP, sauvegarde/restauration, nouveaux VFX/audio dédiés aux statuts.

## 1. Architecture réutilisée

MON16.3 ne crée pas de second système de dégâts.

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

avec `DamageType` et `DamagePerStack`. `DamagePerStack = 0` signifie que le statut n'inflige aucun dégât périodique.

Aucun code de production ne teste `EffectId == Poison`, `Bleeding` ou `Burning`. Ces noms correspondent à des configurations de données du même moteur.

Configuration de référence :

```text
Poison   -> DamageType=Poison
Bleeding -> DamageType=Physical
Burning  -> DamageType=Fire
```

MON16.3 ne crée volontairement aucun `.uasset` pour ces exemples.

## 3. Autorité runtime

`FGridStatusEffectRuntimeState` garde `EffectId` comme identité stable et reçoit un pointeur `Transient` vers sa définition statique :

```cpp
TObjectPtr<UGridStatusEffectDefinitionAsset> DefinitionAsset;
```

Ce pointeur sert uniquement au runtime pour retrouver le profil périodique sans hard-code ni Asset Manager lookup à chaque frontière. Il n'est pas une identité de source et ne fait pas partie du contrat de persistance MON16.7. `SourceId` conserve son rôle existant.

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

Une durée de 2 produit donc exactement deux ticks. Le dernier tick est appliqué **avant** la suppression de l'effet. Les effets `Permanent` avec dégâts périodiques sont rejetés en MON16.3.

## 5. Scaling par stacks

```text
RawDamage = DamagePerStack * StackCount
```

avec saturation à `MAX_int32`. `Potency` n'intervient jamais dans le calcul des dégâts ; elle reste exclusivement le critère de précédence de `ReplaceIfStronger` défini par MON16.2.

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

Routage conservé :

```text
Physical     -> PhysicalArmor -> Health
non-Physical -> MagicalArmor  -> Health
```

Donc :

```text
Bleeding -> PhysicalArmor
Burning  -> MagicalArmor
Poison   -> MagicalArmor
```

Aucun contournement spécial d'armure n'est introduit.

## 7. Mort et ordre déterministe

Les effets sont résolus dans l'ordre déterministe d'`EffectId`. Si une cible meurt pendant ses DoT, les effets périodiques suivants ne lui infligent plus de dégâts.

Pour un monstre, le résultat passe par `AGridMonsterActor::ApplyAttackResult()`, puis par le pipeline de mort existant. À une frontière globale de round, les personnages sont résolus avant les monstres ; si toute la party est tuée par les DoT de round, la défaite est résolue avant les monstres afin de conserver la précédence du TurnManager.

## 8. Intégration MON16.2

`UGridStatusEffectLifecycleSubsystem` reste l'autorité temporelle. MON16.3 lui ajoute seulement la résolution périodique juste avant chaque `AdvanceDuration()`.

Il n'y a toujours aucun tick frame, aucun timer en secondes, aucune horloge parallèle et aucune dépendance Widget/UMG.

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

## 10. Validation UE5.5.4 — 16 août 2026

Le log utilisateur contient **123 tests terminés**, tous en `Result={Success}` et aucun échec de l'Automation Controller.

### MON16.3 ciblé

```text
DamageTypeRouting          Success
DefinitionValidation       Success
DirectDamagePipeline       Success
LethalMonsterDot           Success
MonsterDamageMultiplier    Success
NonPeriodicIsolation       Success
NoParallelSystem           Success
RoundLifecycle             Success
StackScaling               Success
TurnLifecycleMonster       Success
TurnLifecycleParty         Success
```

Bilan : **11/11 Success**.

Les traces runtime confirment notamment :

```text
Poison létal : HP=2->0
Poison multiplicateur 0.5 : Raw=6, MagicalArmor=1, Health=2, HP=10->8
Poison round : HP=10->8->6
Burning monstre : Raw=3, MagicalArmor=2, Health=1, HP=5->4
Burning party : Raw=3, Health=3, HP=10->7
```

### Régressions demandées

```text
MON16.2    10/10 Success
MON16.1     7/7 Success
MON15      42/42 Success
MON14      19/19 Success
```

Les nouveaux tests C++ MON16.3 sont découverts, chargés et exécutés par UE5.5.4 ; le code MON16.3 correspondant est donc compilé et chargé dans l'éditeur utilisé pour la campagne.

## 11. Contrat gelé à la clôture

MON16.3 est **VALIDÉ ET CLOS** avec les règles suivantes :

- DoT entièrement data-driven ;
- pas de hard-code Poison/Bleeding/Burning ;
- pas de jet d'attaque, Evasion ou critique pour un tick ;
- résistances, multiplicateurs et pools existants réutilisés ;
- `DamagePerStack * StackCount` ;
- tick avant décrément et expiration ;
- Round 1 baseline ;
- mort via pipeline existant ;
- aucun système de dégâts/résistance parallèle ;
- aucune dépendance UI ;
- aucune persistance avant MON16.7.

Prochaine étape : **MON16.4 — Haste / Slow & InitiativeModifier**.
