# MON17.8.4 — Generic Monster Death Animation

Statut : **VALIDÉ EN PIE SUR GOBLINTHROWER — pipeline générique de mort conservé**

## 1. Objectif

Ajouter une animation de mort propre au `GoblinThrower` sans créer de logique de mort spécifique au Gobelin.

Le Gobelin sert de premier cas complet, mais le contrat reste générique pour tout le bestiaire :

```text
HP <= 0
    -> logique de mort générique immédiate
    -> DeathMontage optionnel
    -> présentation de la chute
    -> maintien du cadavre
    -> MON17.8.5 : dissolution générique
```

Aucun gameplay ne dépend d'un AnimNotify, d'un montage ou du Skeleton.

## 2. Runtime générique confirmé

`UGridMonsterDefinitionAsset` possède déjà :

```text
DeathMontage
DeathExpectedDuration
```

`DeathMontage` est optionnel et purement présentationnel.

`UGridMonsterDeathComponent::CommitDeath()` conserve l'ordre autoritaire suivant :

```text
Stop Idle Variations
Commit guard
Cancel Attack
Death Audio / VFX
Cancel Movement
Release Occupancy
Disable Collision
Generate / Place Loot
Award XP
Execute MonsterDied links
Broadcast OnMonsterDied
Notify Encounter
StartDeathPresentation
```

Le montage est donc lancé **après** le commit logique complet de la mort.

## 3. StartDeathPresentation

Le code générique :

```text
charge MonsterDefinition.DeathMontage
récupère l'AnimInstance
Montage_Play(DeathMontage)
si succès : bDeathPresentationActive = true
programme un timer DeathExpectedDuration
```

À expiration :

```text
NotifyDeathPresentationComplete()
    -> clear timer
    -> bDeathPresentationActive = false
```

Aucun changement runtime n'a été nécessaire pour afficher la chute.

## 4. Asset GoblinThrower validé

Animation retargetée :

```text
/Game/GrimrockPrototype/Monsters/GoblinThrower/Animation/A_GoblinThrower_Death
```

Caractéristiques observées sous UE5.5.4 :

```text
Sequence Length = 3.6333333 s
Root Motion     = disabled
```

Présentation :

- le Gobelin reçoit le coup fatal ;
- il s'effondre à la renverse vers l'arrière ;
- il reste spatialement dans sa case ;
- aucune translation de gameplay n'est produite par l'animation ;
- la pose finale est exploitable comme cadavre.

Pipeline de création :

```text
Animation Death Mixamo
        ↓
Skeleton source Mixamo déjà utilisé par Throw
        ↓
IK_Mixamo
        ↓
IK_GoblinThrower
        ↓
RTG_Mixamo_To_GoblinThrower
        ↓
Retarget Pose Goblin_Mixamo_TPose
        ↓
A_GoblinThrower_Death
        ↓
AM_GoblinThrower_Death
        ↓
DA_MON_GoblinThrower.DeathMontage
```

Ce pipeline réutilise exactement la chaîne validée auparavant pour `ThrowKnife`.

## 5. Montage de mort

Montage :

```text
AM_GoblinThrower_Death
```

Réglage essentiel validé :

```text
Enable Auto Blend Out = false
```

Le montage conserve donc la dernière pose au sol au lieu de revenir à Idle.

Le Slot reste celui déjà utilisé par le pipeline de montage du Gobelin ; aucun second système d'animation n'est introduit.

## 6. Configuration DA_MON_GoblinThrower

Configuration validée :

```text
Death Montage           = AM_GoblinThrower_Death
Death Expected Duration = 3.6333333 s
```

Une valeur d'affichage arrondie à environ `3.63 s` est acceptable dans l'éditeur, mais la référence observée de la séquence est `3.6333333 s`.

## 7. Validation PIE acquise

Les cinq critères de validation demandés ont été confirmés en PIE sous UE5.5.4 :

```text
1. le coup fatal déclenche immédiatement la chute ;
2. l'animation se déroule entièrement ;
3. le Gobelin reste dans sa case pendant toute la chute ;
4. arrivé en fin de séquence, il reste dans sa pose finale et ne revient pas debout ;
5. loot / combat / victoire continuent normalement sans attendre la fin de l'animation.
```

Résultat : **5/5 validés**.

Cela confirme la séparation recherchée :

```text
mort gameplay immédiate
!=
présentation de mort
```

La grille, l'occupation, le loot, l'XP et les événements restent indépendants de l'animation.

## 8. Tests automatisés MON17.8.4

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON178DeathPresentationTests.cpp
```

Tests :

```text
Grimrock.Monsters.MON17.8.DeathDefinitionContract
Grimrock.Monsters.MON17.8.DeathPresentationApiContract
```

Ils protègent le contrat générique `DeathMontage` / `DeathExpectedDuration` / `UGridMonsterDeathComponent`.

Ils ne remplacent pas la validation visuelle de l'asset binaire, désormais acquise pour le GoblinThrower.

## 9. Règle de maintien du cadavre

MON17.8.4 se termine volontairement avec le cadavre maintenu au sol :

```text
0.00 s      début chute
...
3.6333333   pose de cadavre stable
               ↓
        dernière frame conservée
```

Ne pas :

```text
DestroyActor
réactiver Idle
rejouer DeathMontage
faire dépendre loot / XP / MonsterDied de la fin du montage
```

## 10. Frontière avec MON17.8.5

MON17.8.4 est maintenant **validé côté présentation PIE GoblinThrower**.

MON17.8.5 ajoute le **Corpse Dissolve** générique :

```text
DeathExpectedDuration atteint
        ↓
corpse hold / delay
        ↓
Dynamic Material Instances
        ↓
DissolveAmount 0..1
        ↓
hide SkeletalMesh
```

Contraintes :

- pas de Tick permanent ;
- ne pas détruire l'Actor mort ;
- ne pas modifier loot, XP, occupation ou MonsterDied ;
- paramètres data-driven par MonsterDefinition ;
- matériaux vérifiés dans UE5.5.4 avant authoring du paramètre de dissolution ;
- restauration SaveGame traitée séparément dans MON17.8.6.
