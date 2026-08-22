# MON17.8.4 — Generic Monster Death Animation

Statut : **CONTRAT C++ EXISTANT CONFIRMÉ + TESTS AJOUTÉS — asset Death GoblinThrower / validation UE5.5.4 requise**

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

Aucun gameplay ne doit dépendre d'un AnimNotify, d'un montage ou du Skeleton.

## 2. Audit du runtime actuel

`UGridMonsterDefinitionAsset` possède déjà :

```text
DeathMontage
DeathExpectedDuration
```

`DeathMontage` est explicitement presentation-only et optionnel.

`DeathExpectedDuration` :

- vaut `1.0 s` par défaut ;
- doit être fini et strictement supérieur à zéro ;
- sert de timeout / durée attendue de la présentation.

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

## 3. StartDeathPresentation existant

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

Aucun changement runtime n'est requis pour afficher une animation de chute.

## 4. Règle de maintien de la pose finale

Pour MON17.8.4, le maintien visuel du cadavre est authoré dans le montage :

```text
AM_GoblinThrower_Death
Enable Auto Blend Out = false
```

Ainsi le montage reste sur sa dernière pose au sol après la fin de la séquence.

Le timer `DeathExpectedDuration` reste indépendant : il marque la fin logique de la présentation et servira de point de départ au futur pipeline de dissolution MON17.8.5.

Ne pas :

```text
DestroyActor
réactiver l'Idle
rejouer la mort
faire dépendre loot / XP / MonsterDied de la fin du montage
```

## 5. Pipeline asset GoblinThrower

Réutiliser exactement le pipeline Mixamo validé pour `ThrowKnife` :

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

Le pipeline de référence reste celui de MON17.3.3.

## 6. Import / retarget UE5.5.4

### 6.1 Source Mixamo

Choisir une animation de mort adaptée à un humanoïde de petite taille : chute lisible, sans déplacement de gameplay volontaire.

Utiliser les **mêmes réglages d'export/import Mixamo que pour l'animation Throw validée** afin de conserver le même Skeleton source et éviter une nouvelle chaîne de retarget.

Après import, vérifier :

```text
Skeleton = skeleton Mixamo source déjà utilisé par Throw
Root Motion = disabled
```

### 6.2 Retarget

Dans `RTG_Mixamo_To_GoblinThrower` :

```text
Source       = IK_Mixamo
Target       = IK_GoblinThrower
RetargetPose = Goblin_Mixamo_TPose
```

Prévisualiser l'animation complète avant export.

Exporter sous :

```text
A_GoblinThrower_Death
```

dans le domaine Animation du GoblinThrower.

### 6.3 Vérification de la séquence

Ouvrir `A_GoblinThrower_Death` et vérifier :

- Skeleton = `SKEL_GoblinThrower` ;
- Root Motion désactivé ;
- le Gobelin tombe sans translation anormale hors de sa cellule ;
- aucune explosion de membres / twist de bras ;
- la dernière pose est utilisable comme cadavre ;
- noter la **Sequence Length exacte**.

## 7. Création du montage

Créer :

```text
AM_GoblinThrower_Death
```

à partir de `A_GoblinThrower_Death`.

Réglages :

```text
Slot                  = même Slot que le montage ThrowKnife validé
Enable Auto Blend Out = false
```

Ne pas ajouter d'AnimNotify de gameplay.

Un notify purement présentationnel n'est pas nécessaire à ce stade : le timer générique `DeathExpectedDuration` existe déjà.

## 8. Configuration DA_MON_GoblinThrower

Configurer :

```text
Death Montage          = AM_GoblinThrower_Death
Death Expected Duration = longueur réelle utile de la mort
```

La valeur exacte de `DeathExpectedDuration` doit être réglée après observation de `A_GoblinThrower_Death` / du montage.

Elle doit couvrir la chute jusqu'à la pose de cadavre stable, car MON17.8.5 utilisera ensuite ce point comme fin de la phase de chute.

## 9. Tests automatisés MON17.8.4

Fichier ajouté :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON178DeathPresentationTests.cpp
```

Nouveaux tests :

```text
Grimrock.Monsters.MON17.8.DeathDefinitionContract
Grimrock.Monsters.MON17.8.DeathPresentationApiContract
```

### DeathDefinitionContract

Protège :

- `DeathMontage` optionnel par défaut ;
- `DeathExpectedDuration = 1.0 s` par défaut ;
- définition valide sans montage ;
- rejet de `DeathExpectedDuration = 0`.

### DeathPresentationApiContract

Protège la présence du contrat générique :

```text
UGridMonsterDefinitionAsset.DeathMontage
UGridMonsterDefinitionAsset.DeathExpectedDuration
UGridMonsterDeathComponent.bDeathPresentationActive
CommitDeath
StartDeathPresentation
NotifyDeathPresentationComplete
RestoreCommittedDeathState
```

Ces tests ne valident pas un `.uasset` Gobelin tant que celui-ci n'a pas été créé localement.

## 10. Validation PIE demandée

Après création et assignation du montage :

### Mort fraîche GoblinThrower

Vérifier :

```text
1. le Gobelin peut mourir pendant Idle ;
2. le Gobelin peut mourir après déplacement ;
3. le Gobelin peut mourir après / autour d'un ThrowKnife ;
4. l'attaque en cours est annulée si nécessaire ;
5. la chute joue une seule fois ;
6. le Gobelin reste exactement dans sa cellule ;
7. la collision / occupation sont libérées immédiatement ;
8. le loot reste disponible ;
9. la dernière pose reste au sol ;
10. aucune pose Idle ne réapparaît après la chute.
```

### RatGiant

Vérifier une mort existante afin de s'assurer qu'aucune régression générique n'a été introduite.

## 11. Régressions automatisées à relancer

Après compilation :

```text
Grimrock.Monsters.MON17.8
Grimrock.Monsters.MON8
Grimrock.Monsters.MON10
Grimrock.Monsters.MON17.3.3
```

Les nouveaux tests C++ ne sont pas déclarés réussis avant retour UE5.5.4 local.

## 12. Frontière avec MON17.8.5

MON17.8.4 s'arrête lorsque :

```text
DeathMontage joue
Gobelin tombe correctement
pose finale reste affichée
DeathExpectedDuration est réglée
régressions passent
```

MON17.8.5 ajoutera ensuite le **Corpse Dissolve** générique :

```text
corpse hold
    -> delay
    -> Dynamic Material Instances
    -> DissolveAmount 0..1
    -> hide SkeletalMesh
```

sans détruire l'Actor mort ni modifier loot, XP ou persistance.
