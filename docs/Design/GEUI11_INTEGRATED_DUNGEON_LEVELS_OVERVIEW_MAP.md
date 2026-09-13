# GEUI11 — Integrated Dungeon Levels & Overview Map

**Date :** 13 septembre 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle locale requises

## 1. Objectif

Réintégrer la navigation `Dungeon Levels` dans la barre latérale principale `DUNGEON EDITOR`, car le choix du niveau est le premier geste d'authoring effectué à l'activation du Grid Editor.

La carte `OVERVIEW MAP` du niveau courant est replacée immédiatement sous la navigation des niveaux afin que le contexte de travail principal soit disponible sans ouvrir une fenêtre supplémentaire.

## 2. Principe d'architecture

GEUI11 ne recrée aucune logique de niveau ni de carte.

Le Toolkit principal réutilise directement les widgets autoritatifs existants :

~~~text
SGridEditorDungeonLevelsPanel
SGridEditorOverviewMapPanel
~~~

Ainsi :

- le changement de niveau continue à passer par le workflow existant de `AGridLevelEditorActor` ;
- la création d'un niveau continue à utiliser le dialogue existant ;
- les clics dans la carte continuent à appeler `SelectCellFromOverview` ;
- les marqueurs, tooltips et couleurs de la map restent identiques ;
- l'onglet Nomad `Dungeon Levels` complet reste disponible comme vue détaillée.

## 3. Nouvelle hiérarchie de la sidebar

Le Toolkit principal devient :

~~~text
DUNGEON EDITOR
  Status / Connector display

DUNGEON LEVELS
  Dungeon metadata
  Current/default level
  Level actions
  Clickable level list

OVERVIEW MAP
  32x32 current-level map

WORKSPACE
  PlayTest & Validation
  Tools & Palette
  Selected Object
  Grimrock Lua Scripts
~~~

Le bouton `Dungeon Levels` est retiré du lanceur `WORKSPACE`, puisque cette fonction est maintenant directement disponible dans le panneau principal.

## 4. Overview Map compacte

La map autoritative est conçue autour d'une grille 32x32 d'environ 640x640 px.

Pour la sidebar, GEUI11 l'insère dans un `SScaleBox` configuré avec :

~~~text
EStretch::ScaleToFitX
EStretchDirection::DownOnly
~~~

La carte est donc réduite uniquement si la largeur disponible l'exige, sans agrandissement artificiel.

Un viewport de 340 px de hauteur avec clipping conserve la carte elle-même visible dans la sidebar et masque les détails avancés qui se trouvent sous la map dans le widget complet. Ces détails restent disponibles dans l'onglet autonome `Dungeon Levels`.

## 5. Fichiers modifiés

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdModeToolkit.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
~~~

Nouveau :

~~~text
docs/Design/GEUI11_INTEGRATED_DUNGEON_LEVELS_OVERVIEW_MAP.md
~~~

Aucune donnée runtime, aucun `.uasset` et aucune `.umap` ne sont modifiés.

## 6. Validation UE5.5.4

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Validation visuelle :

1. Ouvrir `L_GrimrockEditor`.
2. Activer `Grimrock Grid Editor`.
3. Vérifier que `DUNGEON LEVELS` apparaît directement sous le header `DUNGEON EDITOR`.
4. Vérifier que la liste des niveaux est immédiatement accessible sans ouvrir de fenêtre supplémentaire.
5. Cliquer sur plusieurs niveaux et confirmer que `Current Level Id`, le viewport et la map suivent le niveau sélectionné.
6. Vérifier que `New Level`, `Reload Current` et `Load Default` fonctionnent toujours depuis la sidebar.
7. Vérifier que `OVERVIEW MAP` apparaît sous la navigation des niveaux et tient dans la largeur de la sidebar.
8. Cliquer sur plusieurs cellules de la map et confirmer que la sélection du Grid Editor suit correctement.
9. Vérifier que le launcher `WORKSPACE` ne contient plus de bouton `Dungeon Levels`.
10. Vérifier que l'onglet autonome `Dungeon Levels` reste ouvrable depuis le menu Window et garde sa présentation détaillée.
11. Effectuer un court smoke test PIE.

## 7. Intention UX

`Dungeon Levels` et `Overview Map` ne sont plus considérés comme des workspaces spécialisés : ils constituent le contexte permanent de l'édition d'un donjon.

Les autres panneaux (`Tools & Palette`, `Selected Object`, `PlayTest & Validation`, Lua) restent des surfaces spécialisées ouvertes à la demande.
