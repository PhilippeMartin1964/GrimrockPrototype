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

## Correction GEUI11.1

GEUI11.1 inverse la responsabilité : le panneau autoritatif est d’abord limité à son carré natif de carte 32x32, soit 640x640 px, puis ce carré est redimensionné vers la largeur disponible de la sidebar.

~~~text
SScaleBox ScaleToFitX
  -> SBox 640x640 + ClipToBounds
     -> SGridEditorOverviewMapPanel
~~~

Conséquences :

- la map complète est toujours visible ;
- la largeur de la sidebar pilote automatiquement le facteur de réduction ;
- aucune logique de sélection, tooltip ou marqueur n’est dupliquée ;
- l’onglet autonome `Dungeon Levels` conserve le widget complet inchangé.

## GEUI11.2 — Légende, contraste et nettoyage

Le retour d’usage après GEUI11.1 a montré trois points à corriger :

1. la légende de la carte était masquée par le clipping 640x640 ;
2. le fond de la carte et les cases vides manquaient de contraste ;
3. `SGridEditorOverviewMapPanel::HasObjectAtCell()` n’était référencé nulle part et constituait du code mort.

GEUI11.2 étend donc la zone compacte à 640x664 afin d’inclure la légende autoritative existante tout en continuant à exclure la section détaillée `Selected Cell` située plus bas.

Le conteneur compact reçoit également un fond gris foncé dédié autour de la map. Les cellules vides gardent leur noir actuel, ce qui augmente nettement la séparation visuelle entre la grille, ses espacements et le cadre.

Le helper mort `HasObjectAtCell()` est supprimé de la déclaration et de l’implémentation.

Aucune seconde légende ni logique de carte n’est créée dans le Toolkit.

## Fichiers modifiés

~~~text
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorOverviewMapPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorOverviewMapPanel.cpp
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
4. vérifier que la légende `Selected / Selected Object` apparaît sous la map ;
5. vérifier que les cases vides se détachent clairement du fond/cadre de la grille ;
6. redimensionner horizontalement la sidebar et confirmer que map + légende se réduisent ensemble sans être coupées ;
7. cliquer sur des cellules proches des quatre bords de la map et confirmer que la sélection fonctionne ;
8. ouvrir l’onglet autonome `Dungeon Levels` et confirmer que sa présentation détaillée reste inchangée.
