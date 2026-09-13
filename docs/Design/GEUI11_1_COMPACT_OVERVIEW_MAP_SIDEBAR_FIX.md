# GEUI11.1 — Compact Overview Map Sidebar Fix

**Date :** 13 septembre 2026  
**Statut :** implémenté sur master — validation locale UE5.5.4 requise

## Objectif

Corriger l’Overview Map intégrée à la sidebar `DUNGEON EDITOR` afin que la grille 32x32 soit visible en entier sans réintroduire les détails avancés du panneau autonome `Dungeon Levels`.

## Cause

GEUI11 appliquait le clipping après le `SScaleBox` :

~~~text
SBox HeightOverride(340)
  -> ClipToBounds
     -> SScaleBox ScaleToFitX
        -> SGridEditorOverviewMapPanel
~~~

Le widget autoritatif `SGridEditorOverviewMapPanel` contient la map 32x32 puis une légende et les détails de cellule. La hauteur fixe de 340 px coupait donc la carte elle-même après son redimensionnement.

## Correction

GEUI11.1 inverse la responsabilité : le panneau autoritatif est d’abord limité à son carré natif de carte 32x32, soit 640x640 px, puis ce carré est redimensionné vers la largeur disponible de la sidebar.

~~~text
SScaleBox ScaleToFitX
  -> SBox 640x640 + ClipToBounds
     -> SGridEditorOverviewMapPanel
~~~

Conséquences :

- la map complète est toujours visible ;
- la largeur de la sidebar pilote automatiquement le facteur de réduction ;
- les détails `Selected Cell`, objets et légendes avancées situés sous la map ne participent plus au layout compact ;
- aucune logique de sélection, tooltip ou marqueur n’est dupliquée ;
- l’onglet autonome `Dungeon Levels` conserve le widget complet inchangé.

## Fichier modifié

~~~text
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
~~~

Aucun asset runtime, `.uasset` ou `.umap` n’est modifié.

## Validation

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Puis :

1. ouvrir `L_GrimrockEditor` ;
2. activer `Grimrock Grid Editor` ;
3. vérifier que l’Overview Map 32x32 est visible en entier dans la sidebar ;
4. redimensionner horizontalement la sidebar et confirmer que la map se réduit sans être coupée ;
5. cliquer sur des cellules proches des quatre bords de la map et confirmer que la sélection fonctionne ;
6. ouvrir l’onglet autonome `Dungeon Levels` et confirmer que sa présentation détaillée est inchangée.
