# WORLDOBJ-MIG03.4B — Migration de l’authoring visuel

Statut : implémenté, validation UE locale à effectuer.

## Objectif

Après la coupure runtime de WORLDOBJ-MIG03.4A, `StaticPart` et `MovingParts` sont les seules données visuelles consommées par les objets runtime. Cette étape aligne les fonctions de provisioning du Grid Editor sur ce contrat afin qu’un asset créé ou réparé par l’éditeur dispose immédiatement de la présentation cible.

Il ne s’agit pas de réintroduire un fallback runtime : le runtime ne relit pas les anciens champs.

## Escaliers

`EnsureStairsTransitionArchetypes()` complète désormais chaque archétype avec :

- `PlacementSurface = Floor` ;
- `DefaultLocalPosition = (0,0,0)` ;
- `StaticPart.Mesh` = mesh d’escalier ;
- `StaticPart.LocalTransform = Identity` ;
- `MovingParts` vide ;
- recalcul de la projection de placement transitoire.

## Fosse / trappe

`EnsurePitTrapdoorArchetype()` complète désormais l’archétype avec :

- `PlacementSurface = Floor` ;
- `DefaultLocalPosition = (0,0,0)` ;
- `StaticPart.Mesh = SM_Pit_Stone_01` ;
- `StaticPart.LocalTransform = Identity`.

Une composition cible avec un seul `MovingPart` est invalide : elle est remise à zéro.

Si l’asset possède encore une paire complète `PitLeftLeafMesh` / `PitRightLeafMesh` et aucun `MovingParts`, l’éditeur réalise une migration de données ponctuelle vers le contrat cible :

- `MovingParts.Part0` = volet gauche ; rotation autour de Y ; pivot gauche ; angle négatif ;
- `MovingParts.Part1` = volet droit ; rotation autour de Y ; pivot droit ; angle positif ;
- durée reprise depuis `DefaultBehavior.PitAnimation.MoveDuration`.

Cette copie est exclusivement une étape de conversion des `.uasset`. Aucune lecture runtime des anciens champs n’est rétablie.

Une fosse sans paire de volets reste valide comme fosse statique ouverte.

## Étape suivante

WORLDOBJ-MIG03.4C doit terminer l’alignement de l’éditeur et des tests, puis supprimer physiquement du schéma :

- `PreviewMesh` ;
- `FixedMesh` ;
- `MovingMesh` ;
- `PitLeftLeafMesh` ;
- `PitRightLeafMesh`.
