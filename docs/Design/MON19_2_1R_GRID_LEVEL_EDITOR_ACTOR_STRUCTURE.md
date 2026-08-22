# MON19.2.1R — Décomposition structurelle de `GridLevelEditorActor`

**Date :** 22 août 2026  
**Statut :** implémentation structurelle — validation UE5.5.4 à effectuer par compilation et tests

## Objectif

`GridLevelEditorActor.cpp` avait dépassé 5 000 lignes et regroupait plusieurs responsabilités distinctes : cycle de vie de l’acteur éditeur, gestion du donjon et du preview, édition de la grille et des objets, manipulation des liens, validation du niveau et interaction avec le viewport.

MON19.2.1R réduit ce risque de maintenance sans modifier le comportement fonctionnel ni le contrat public de `AGridLevelEditorActor`.

## Principe retenu

`AGridLevelEditorActor` reste une seule classe et `GridLevelEditorActor.cpp` reste l’unique unité de traduction compilée par UnrealBuildTool.

Le fichier principal ne contient plus que l’assemblage ordonné de quatre responsabilités privées :

1. **Core / Dungeon** : helpers historiques, cycle de vie de l’acteur, diagnostics, `DungeonAsset`, création et changement de niveau, synchronisation du preview et préparation PIE ;
2. **Editing / Objects / Links** : grille, murs, placement et édition des objets, sélection, coordonnées, manipulation des liens et propriétés d’objets ;
3. **Validation** : `ValidateCurrentLevel()` et toute la validation actuelle du niveau, des objets, monstres, transitions, liens et conditions ;
4. **Interaction / Viewport** : effacement contextuel, hover objet, peinture cellule, rebuild géométrique et grille de coordonnées.

Les deux blocs les plus volumineux sont eux-mêmes composés de fragments `.inl` privés inclus dans leur ordre historique. Ces fragments ne constituent pas de nouvelles entités métier et ne sont jamais compilés séparément.

## Raisons de conserver une seule unité de traduction

Cette première étape est volontairement conservatrice :

- les helpers du namespace anonyme conservent exactement leur visibilité ;
- les blocs `WITH_EDITOR` et `WITH_EDITORONLY_DATA` conservent leur contexte ;
- les définitions de méthodes restent dans le même ordre logique ;
- aucun nouveau `UObject`, `UActorComponent` ou subsystem n’est introduit ;
- aucun changement de sérialisation n’est nécessaire ;
- le risque de régression est nettement plus faible qu’une extraction immédiate en plusieurs services autonomes.

## Hors périmètre

MON19.2.1R ne modifie pas :

- `GridLevelEditorActor.h` ;
- `UGridLevelAsset` ;
- le runtime de gameplay ;
- la logique Event → Command ;
- l’identité actuelle de `CreateLink()` / `RemoveExactLink()` ;
- l’interface CONNECTORS ;
- les `.uasset` et `.umap`.

Les corrections fonctionnelles des connecteurs restent réservées à MON19.2.1B.

## Structure obtenue

```text
Source/GrimrockPrototypeEditor/Private/EditorTools/
├── GridLevelEditorActor.cpp
└── GridLevelEditorActorParts/
    ├── GridLevelEditorActor_CoreDungeon.inl
    ├── GridLevelEditorActor_EditingObjectsLinks.inl
    ├── GridLevelEditorActor_Validation.inl
    ├── GridLevelEditorActor_InteractionViewport.inl
    ├── CoreDungeon/
    │   └── GridLevelEditorActor_CoreDungeon_*.inl
    └── EditingObjectsLinks/
        └── GridLevelEditorActor_EditingObjectsLinks_*.inl
```

## Vérifications structurelles effectuées

Avant publication, le contenu d’origine a été partitionné sans réécriture fonctionnelle. Les contrôles statiques ont confirmé :

- 5 029 lignes sources couvertes par les quatre responsabilités ;
- 109 implémentations de méthodes `AGridLevelEditorActor::` conservées ;
- blocs préprocesseur équilibrés ;
- accolades et parenthèses équilibrées ;
- aucun fichier runtime ou asset binaire modifié.

Ces contrôles ne remplacent pas une compilation Unreal Engine.

## Validation demandée après compilation

Après compilation UE5.5.4, relancer au minimum :

```text
Grimrock.MON19.2.Editor.ConditionalLinkIdentity
Grimrock.MON19.2.Editor.LinkPolicyMatrix
```

Ces deux tests étaient verts avant MON19.2.1R et constituent le filet de non-régression immédiat avant MON19.2.1B.

## Suite architecturale

Cette décomposition physique prépare les extractions métier futures sans les imposer maintenant. Les candidats naturels restent :

- `GridEditorLinkService` pour les mutations exactes de liens ;
- `GridLevelValidationService` pour rendre la validation indépendante de l’acteur éditeur.

Ces extractions ne doivent être réalisées que lorsqu’elles apportent un contrat testable clair et sans refactor massif.
