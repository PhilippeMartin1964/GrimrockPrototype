# WORLDOBJ-MIG03.4B — Migration de l’authoring visuel

Statut : **validé — étape historique terminée**.

Ce document décrit spécifiquement la tranche `WORLDOBJ-MIG03.4B`. La feuille de route consolidée `WORLDOBJ-MIG00` → `WORLDOBJ-MIG10` et le modèle de données cible sont désormais décrits dans :

`docs/Architecture/WORLDOBJ_MIGRATION_ROADMAP_AND_TARGET_DATA_MODEL.md`

La référence architecturale cible reste :

`docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`

## Objectif historique de MIG03.4B

Après la coupure runtime de WORLDOBJ-MIG03.4A, `StaticPart` et `MovingParts` sont devenus les données visuelles autoritaires consommées par les objets runtime. Cette étape a aligné les fonctions de provisioning du Grid Editor sur ce contrat afin qu’un asset créé ou réparé par l’éditeur dispose immédiatement de la présentation cible.

Il ne s’agissait pas de réintroduire un fallback runtime : le runtime ne devait plus considérer les anciens champs comme source de vérité.

## Escaliers

`EnsureStairsTransitionArchetypes()` a été aligné sur :

- `PlacementSurface = Floor` ;
- `DefaultLocalPosition = (0,0,0)` ;
- `StaticPart.Mesh` = mesh d’escalier ;
- `StaticPart.LocalTransform = Identity` ;
- `MovingParts` vide ;
- recalcul de la projection de placement transitoire.

## Fosse / trappe

`EnsurePitTrapdoorArchetype()` a été aligné sur :

- `PlacementSurface = Floor` ;
- `DefaultLocalPosition = (0,0,0)` ;
- `StaticPart.Mesh = SM_Pit_Stone_01` ;
- `StaticPart.LocalTransform = Identity`.

Une composition cible avec un seul `MovingPart` est invalide pour une trappe à deux volets.

Pendant la migration des assets, une paire historique complète `PitLeftLeafMesh` / `PitRightLeafMesh` pouvait être convertie ponctuellement vers :

- `MovingParts.Part0` = volet gauche ; rotation autour de Y ; pivot gauche ; angle négatif ;
- `MovingParts.Part1` = volet droit ; rotation autour de Y ; pivot droit ; angle positif ;
- durée reprise depuis la configuration existante.

Cette copie était exclusivement un mécanisme de conversion de données d’authoring, jamais un contrat runtime à conserver.

Une fosse sans paire de volets reste valide comme fosse statique ouverte.

## Suite réalisée après MIG03.4B

Les étapes suivantes ont depuis été implémentées et validées progressivement :

```text
MIG03.4C
└── retrait du schéma d’authoring visuel legacy

MIG03.4D
├── détachement des sémantiques visuelles des anciens champs
├── migration de la validation
├── détachement des tests
├── migration du provisioning éditeur
├── suppression des helpers de provisioning morts
└── migration de l’inspecteur vers StaticPart / MovingParts
```

La dernière tranche validée de cette série est :

```text
a013f1f30f91c61d67fef4b1b6d5d854a46bc3ce
WORLDOBJ-MIG03.4D move inspector to visual composition
```

## Legacy encore présent après cette étape

Même si MIG03.4B est terminé, les symboles suivants existent encore temporairement comme ponts C++ `Transient` dans l’état courant du projet et doivent être supprimés avant la clôture complète de MIG03/MIG09 :

```text
PreviewMesh
FixedMesh
MovingMesh
PitLeftLeafMesh
PitRightLeafMesh
```

Ils ne font plus partie du modèle cible. Le contrat cible reste exclusivement :

```text
Visual
├── StaticPart
└── MovingParts
    ├── Part0
    └── Part1
```

Le suivi de leur suppression et des jalons `MIG04` à `MIG10` appartient désormais au document de roadmap consolidé.