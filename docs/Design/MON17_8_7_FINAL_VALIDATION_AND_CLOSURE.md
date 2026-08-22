# MON17.8.7 — Final Validation & Closure

Statut : **MON17.8 — VALIDÉ ET CLOS**

Date de clôture : **22 août 2026**

## 1. Objet

MON17.8 avait pour objectif de finaliser le polish d'animation et de mort du Goblin Thrower tout en renforçant le contrat de présentation générique des monstres.

La règle d'architecture est restée constante :

```text
gameplay / grille / combat = autorité C++ et data
animation / audio / VFX / matériaux = présentation
```

Aucun Actor, composant ou pipeline spécifique au Gobelin n'a été introduit côté gameplay.

## 2. Résultat final par sous-étape

### MON17.8.1 — Audit / Animation Contract

- audit de l'architecture existante ;
- confirmation de `AGridMonsterActor` + composants génériques comme pipeline unique ;
- identification des trous Walk / Death / Dissolve / Persistence ;
- refus d'introduire une abstraction Goblin-specific parallèle.

### MON17.8.2 — Generic Walk Synchronization

- synchronisation de `MoveAlpha` sur l'alpha spatial effectivement utilisé par la translation EaseInOut ;
- conservation du mouvement grid-authoritative ;
- Root Motion désactivé ;
- Goblin Thrower réglé avec `A_GoblinThrower_Walk_Fwd` ;
- cycle exploité : `0.895 .. 1.800 s`, durée `0.905 s` ;
- Sequence Evaluator piloté par :

```text
ExplicitTime = 0.895 + MoveAlpha * 0.905
```

- `DA_MON_GoblinThrower.MoveDuration = 1.0 s` ;
- locomotion validée visuellement en PIE.

### MON17.8.3 — Generic Monster Presentation Contract

Contrat final orthogonal :

```text
MonsterState = gameplay
bIsMoving / MoveAlpha = locomotion
bIsTurning / TurnDirection = rotation
AttackMontage = overlay combat existant
```

Les états suivants restent génériques :

```text
Dormant
Idle
Alert
Pursuing
Attacking
Repositioning
Hurt
Dead
```

Aucune animation Hurt spécifique n'a été inventée en l'absence d'un asset réel.

### MON17.8.4 — Generic Death Animation Contract

Le Goblin Thrower utilise :

```text
A_GoblinThrower_Death
AM_GoblinThrower_Death
DeathExpectedDuration = 3.6333333 s
Enable Auto Blend Out = false
```

Le gameplay de mort reste commité avant toute présentation :

```text
HP=0
-> Dead
-> attaque annulée
-> mouvement/réservation annulés
-> occupation libérée
-> collision désactivée
-> loot / XP / links / encounter
-> DeathMontage présentationnel
```

La chute et le maintien final au sol ont été validés en PIE.

### MON17.8.5 — Generic Corpse Dissolve

Extension data-driven générique :

```text
bEnableDeathDissolve
DeathDissolveDelay
DeathDissolveDuration
DeathDissolveParameterName
```

Configuration Goblin Thrower validée :

```text
bEnableDeathDissolve       = true
DeathDissolveDelay         = 2.0 s
DeathDissolveDuration      = 1.5 s
DeathDissolveParameterName = DissolveAmount
```

Pipeline :

```text
DeathMontage
-> final pose
-> corpse hold
-> Dynamic Material Instances
-> DissolveAmount 0 -> 1
-> SkeletalMesh hidden
```

Le matériau générique `MF_MonsterDeathDissolveMask` utilise un Noise borné :

```text
Noise -> Clamp(0.01, 0.99) -> threshold
```

Cela garantit :

```text
DissolveAmount=0 -> aucune perforation parasite
DissolveAmount=1 -> disparition complète
```

Le dissolve progressif du Gobelin a été validé en PIE.

### MON17.8.6 — Save / Restore

Contrat canonique :

```text
mort fraîche
  -> DeathMontage
  -> corpse hold
  -> dissolve
  -> mesh hidden

mort restaurée
  -> Dead immédiatement
  -> aucune occupation
  -> aucune collision
  -> aucun DeathMontage
  -> aucun DeathAudio / DeathVFX
  -> aucun dissolve rejoué
  -> mesh hidden immédiatement
  -> Actor runtime conservé
```

Aucun état de progression du dissolve n'est sérialisé.

Les trois scénarios PIE Save/Load ont été validés :

1. sauvegarde après dissolution complète ;
2. sauvegarde pendant la chute / hold / dissolve ;
3. plusieurs Gobelins morts avant sauvegarde.

Dans les trois cas : aucun replay, aucune duplication de loot/XP/événement, aucune occupation fantôme.

## 3. Validation automatisée finale

Suites exécutées localement sous UE5.5.4 :

```text
Grimrock.Monsters.MON17.8      8/8 SUCCESS
Grimrock.Monsters.MON9        13/13 SUCCESS
Grimrock.Monsters.MON8         7/7 SUCCESS
Grimrock.Monsters.MON10       37/37 SUCCESS
Grimrock.Monsters.MON17.3.3    1/1 SUCCESS
```

Total :

```text
66 / 66 SUCCESS
```

Les nouveaux contrats de restore confirment notamment :

```text
RestoreDead ... PresentationHidden=true
RestoreAlive ... State=Idle HP=7 Enabled=true
```

## 4. Validation PIE finale — Goblin Thrower

Validé :

- Idle ;
- déplacement case par case ;
- Walk synchronisé au déplacement ;
- ThrowKnife inchangé ;
- chute de mort immédiate ;
- montage joué entièrement ;
- maintien final dans la cellule ;
- corpse hold ;
- dissolve progressif ;
- disparition finale ;
- aucune perforation de matériau lorsqu'il est vivant ;
- Save/Load mort sans replay de présentation.

## 5. Non-régression Rat Giant

Le Rat Giant existant a été validé en PIE après MON17.8 :

- apparition normale ;
- Idle normal ;
- déplacement grille normal ;
- attaque normale ;
- mort normale ;
- aucun dissolve parasite lorsqu'il est vivant ;
- aucune modification visuelle inattendue issue des matériaux du Goblin Thrower ;
- restauration d'un Rat mort compatible avec le nouveau contrat générique de persistance.

Le Rat Giant actuel reste volontairement sans dissolve lorsque `bEnableDeathDissolve=false`.

### Évolution future hors MON17.8

La définition actuelle du Rat Giant est appelée à être remplacée ultérieurement par un nouveau modèle `.fbx` acquis sur CGTrader.

Cette future refonte du Rat Giant est **hors périmètre de MON17.8**. Elle devra être traitée comme un chantier dédié de remplacement/retargeting/configuration de bestiaire afin de ne pas altérer rétroactivement la validation du contrat générique MON17.8.

## 6. Contrat générique final MON17.8

```text
UGridMonsterDefinitionAsset
        |
        +-- MoveDuration
        +-- DeathMontage
        +-- DeathExpectedDuration
        +-- bEnableDeathDissolve
        +-- DeathDissolveDelay
        +-- DeathDissolveDuration
        +-- DeathDissolveParameterName
        |
        v
AGridMonsterActor
        |
        +--> UGridMonsterMovementComponent
        |       grid-authoritative movement
        |       MoveAlpha = visual spatial progression
        |
        +--> UGridMonsterAnimInstance
        |       mirrors state/signals only
        |
        +--> UGridMonsterCombatComponent
        |       existing generic attacks / montages
        |
        +--> UGridMonsterDeathComponent
                gameplay death commitment
                -> DeathMontage
                -> corpse hold
                -> optional dissolve
                -> hidden mesh
                -> canonical dead restore
```

Principes garantis :

```text
pas de Root Motion autoritaire
pas de Goblin-specific gameplay code
pas de Tick permanent pour le dissolve
pas de replay de mort au chargement
pas de sérialisation des états visuels transitoires
pas de duplication loot / XP / events
```

## 7. Conclusion

**MON17.8 — Goblin Thrower Animation & Death Polish est VALIDÉ ET CLOS.**

Le Goblin Thrower a servi de pilote, mais le résultat livré est un contrat de présentation générique réutilisable pour les futurs monstres du bestiaire.

Les futurs chantiers d'assets — notamment le remplacement du Rat Giant — pourront réutiliser ce contrat sans introduire de logique spécifique supplémentaire.
