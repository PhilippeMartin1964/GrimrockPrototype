# MON17.8.3 — Generic Monster Presentation State Integration

Statut : **CONTRAT ET TESTS AJOUTÉS — validation compilation / automation / PIE requise**

## 1. Objectif

MON17.8.3 formalise le fait que le GoblinThrower est uniquement le **cas pilote** d'un pipeline de présentation commun à tout le bestiaire.

Aucun Actor, composant ou branchement C++ spécifique au Gobelin ne doit être introduit.

Le même pont doit rester utilisable par :

```text
RatGiant
GoblinThrower
futurs monstres
```

via :

```text
AGridMonsterActor
    -> UGridMonsterAnimInstance
    -> AnimBP spécifique au Skeleton
    -> assets d'animation spécifiques à l'espèce
```

## 2. Conclusion d'audit

Le runtime possède déjà les états génériques requis :

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

`UGridMonsterAnimInstance` reflète déjà :

```text
MonsterState
bIsMoving
bIsTurning
bIsDead
MoveAlpha
TurnDirection
CurrentHealth
MaxHealth
CurrentCell
Facing
```

Aucune nouvelle propriété runtime n'est nécessaire pour MON17.8.3.

## 3. Règle fondamentale : état gameplay et locomotion visuelle sont orthogonaux

La locomotion ne doit pas être déduite uniquement de `MonsterState`.

Exemple :

```text
MonsterState = Pursuing
bIsMoving    = true
    -> Walk

MonsterState = Pursuing
bIsMoving    = false
    -> pose de base / Idle
```

Même principe pour `Repositioning`.

Le C++ décide du déplacement et expose `bIsMoving` / `MoveAlpha`. L'AnimBP choisit la pose correspondante.

## 4. Politique de présentation par état

| État / signal | Présentation actuelle | Politique MON17.8 |
|---|---|---|
| `Dormant` | pose de repos | réutilise Idle tant qu'aucun asset Dormant dédié n'existe |
| `Idle` | Idle | conserver |
| `Alert` | état gameplay + audio/VFX | réutilise Idle tant qu'aucun asset Alert dédié n'existe |
| `Pursuing` | locomotion lorsque `bIsMoving` | Walk piloté par `bIsMoving` / `MoveAlpha` |
| `Repositioning` | locomotion tactique | même contrat Walk générique |
| `Attacking` | montage d'attaque | montage via Slot au-dessus de la locomotion de base |
| `Hurt` | état + audio/VFX | aucun montage tant qu'un asset Hurt générique exploitable n'est pas fourni |
| `Dead` | pipeline DeathComponent | traité par MON17.8.4+ |
| `bIsTurning` / `TurnDirection` | rotation C++ autoritaire | aucun asset Turn obligatoire ; signal disponible pour un futur polish |

## 5. GoblinThrower

### Idle / Walk

Le Gobelin utilise désormais le contrat MON17.8.2 :

```text
Walk Sequence = A_GoblinThrower_Walk_Fwd
ExplicitTime  = 0.895 + MoveAlpha * 0.905
MoveDuration  = 1.00 s
Idle->Walk    = 0.20 s
Walk->Idle    = 0.20 s
```

### Pursuing / Repositioning

Aucune branche spécifique n'est requise :

```text
Pursuing / Repositioning
    + bIsMoving=true
        -> Walk
```

### Alert

Aucun asset Alert GoblinThrower n'est actuellement versionné.

Le Gobelin peut donc rester sur la pose Idle pendant l'état `Alert`, tandis que les systèmes audio/VFX existants portent la réaction de perception.

### Attack_ThrowKnife

Le contrat MON17.3.3 reste inchangé :

```text
A_GoblinThrower_ThrowKnife
AM_GoblinThrower_ThrowKnife
ProjectileSource
ExpectedDuration          = 2.20 s
ImpactTimeSeconds         = 1.00 s
ProjectileTravelDuration  = 0.20 s
LaunchDelay               = 0.80 s
```

Le Slot d'attaque ne doit pas être modifié par le polish de locomotion.

### Hurt

Aucun `HurtMontage` générique ni asset Hurt GoblinThrower n'existe actuellement.

MON17.8.3 ne crée donc aucune abstraction spéculative. L'état `Hurt`, le HurtAudio et le HurtVFX existants restent le contrat actif.

## 6. RatGiant

Le Rat Géant est le cas de non-régression représentatif.

Son AnimBP doit continuer à :

- dériver de `UGridMonsterAnimInstance` ;
- cibler un Skeleton compatible avec `SK_RatGiant` ;
- répondre à `bIsMoving` sans dépendance au Gobelin ;
- conserver son combat / Hurt / Death existants.

MON17.8.3 n'impose aucune modification binaire de son AnimBP.

## 7. Tests ajoutés

Nouveau fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON178PresentationTests.cpp
```

### 7.1 BestiaryPresentationBridge

```text
Grimrock.Monsters.MON17.8.BestiaryPresentationBridge
```

Charge directement :

```text
SK_RatGiant
ABP_MON_RatGiant
SK_GoblinThrower
ABP_MON_GoblinThrower
```

et vérifie pour les deux espèces :

- mesh chargeable ;
- AnimBP chargeable ;
- Skeleton présent ;
- AnimBP dérive de `UGridMonsterAnimInstance` ;
- `IAnimClassInterface` disponible ;
- Skeleton du mesh et Skeleton cible de l'AnimBP compatibles.

Cela corrige le trou identifié en MON17.8.1 où le test MON17.2 ne protégeait directement que le Rat Géant.

### 7.2 AnimationStateBridgeContract

```text
Grimrock.Monsters.MON17.8.AnimationStateBridgeContract
```

Vérifie que `UGridMonsterAnimInstance` continue d'exposer les propriétés génériques :

```text
MonsterState
bIsMoving
bIsTurning
bIsDead
MoveAlpha
TurnDirection
CurrentHealth
MaxHealth
CurrentCell
Facing
```

et que les huit états `EGridMonsterState` restent réfléchis.

## 8. Aucun changement runtime nécessaire

MON17.8.3 n'ajoute volontairement :

```text
aucun Actor spécifique
aucun Component spécifique
aucun switch MonsterId
aucun nouveau MonsterState
aucun nouveau Tick
aucun nouveau contrat Hurt spéculatif
```

Le pipeline existant suffit pour les états de présentation actuels.

## 9. Validation demandée sous UE5.5.4

Après récupération du commit :

1. compiler le projet ;
2. exécuter :

```text
Grimrock.Monsters.MON17.8
```

3. puis les régressions ciblées :

```text
Grimrock.Monsters.MON17.2
Grimrock.Monsters.MON17.3
Grimrock.Monsters.MON10
```

4. vérifier en PIE sur GoblinThrower :

```text
Idle
Walk une case
Walk plusieurs cases
Turn puis Walk
Alert -> Pursuing
Repositioning si déclenché par l'IA ranged
ThrowKnife
retour locomotion après ThrowKnife
Hurt sans mort
```

5. vérifier au minimum sur RatGiant :

```text
Idle
Walk
Pursuit / patrol
Attack
Hurt
```

Aucune compilation, automation ou régression PIE n'est déclarée réussie par cette documentation avant retour de l'environnement local UE5.5.4.

## 10. Suite

Une fois MON17.8.3 validé, la suite est :

```text
MON17.8.4 — Generic Monster Death Animation
```

Le GoblinThrower servira de premier cas pour importer/retargeter une animation de mort, mais l'exécution restera fondée sur le contrat générique existant :

```text
UGridMonsterDefinitionAsset.DeathMontage
DeathExpectedDuration
UGridMonsterDeathComponent
```

sans déplacer la logique de mort, de loot, d'XP ou de MonsterDied dans l'animation.
