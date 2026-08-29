# GEUI01 — Fondation de l’espace de travail dockable du Grid Editor

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI01 crée la fondation de docking à faible risque pour la refonte ergonomique du Grimrock Grid Editor.

Ce jalon ne supprime ni ne réécrit volontairement le Toolkit inline actuel. Son objectif est de démontrer que les panneaux Slate existants peuvent vivre dans des onglets Nomad indépendants de l’Unreal Editor avant la migration de l’interface plus large.

Le modèle d’édition faisant autorité reste inchangé :

~~~text
UGridDungeonAsset
    -> UGridLevelAsset
        -> Cells / Objects / Links / Lua / Quest references
~~~

Aucune représentation alternative du niveau n’est introduite.

## 2. Nouvelles fenêtres dockables

Quatre onglets Nomad sont enregistrés par GrimrockPrototypeEditor :

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
~~~

L’onglet existant reste inchangé :

~~~text
Grimrock Lua Scripts
~~~

Les cinq entrées sont exposées dans le menu Window de l’Unreal Editor et utilisent FGlobalTabmanager / SDockTab avec ETabRole::NomadTab.

Elles peuvent donc être dockées, détachées, placées en fenêtre flottante et fermées en utilisant le comportement natif des espaces de travail Unreal.

## 3. Composition privilégiant la réutilisation

GEUI01 réutilise volontairement les widgets Slate existants.

### Dungeon Levels

Héberge :

~~~text
SGridEditorOverviewMapPanel
~~~

La liste des niveaux et les actions de gestion des niveaux restent dans FGridLevelEdModeToolkit jusqu’à GEUI02. Une note de migration dans l’onglet rend cette frontière temporaire explicite.

### PlayTest & Validation

Héberge :

~~~text
SGridEditorValidationPanel
~~~

Les contrôles PlayTest existants restent dans FGridLevelEdModeToolkit jusqu’à GEUI03.

Les règles de validation ne sont pas dupliquées. Le panneau continue d’appeler le chemin faisant autorité existant GridEditorLuaService::ValidateCurrentLevelWithLua.

### Tools & Palette

Héberge le widget existant :

~~~text
SGridEditorToolPalettePanel
~~~

La sélection des outils et les mutations de palette ciblent toujours AGridLevelEditorActor et UGridObjectPaletteAsset exactement comme auparavant.

### Selected Object

Compose les widgets existants :

~~~text
SGridEditorObjectInspectorPanel
SGridEditorLinksPanel
~~~

Aucune logique de mutation des propriétés d’objet ou des connecteurs n’est copiée dans l’hôte de l’espace de travail.

## 4. Hôte de l’espace de travail

Nouveau widget :

~~~text
SGridEditorWorkspaceTab
~~~

Emplacement :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

Ses responsabilités sont volontairement limitées :

- trouver le AGridLevelEditorActor courant dans le monde éditeur ;
- composer le widget existant correspondant à l’onglet d’espace de travail demandé ;
- posséder l’état de présentation uniquement des panneaux Tool Palette et Validation pour cet onglet Nomad ;
- reconstruire l’onglet détaché lorsque le contexte léger de l’éditeur change.

Il ne possède aucune donnée de donjon, niveau, objet, connecteur ou runtime.

## 5. Synchronisation du contexte

L’ancien Toolkit inline reçoit des appels explicites de rafraîchissement depuis FGridLevelEdMode.

Un onglet Nomad détaché ne reçoit pas automatiquement ces appels ; GEUI01 observe donc un petit ensemble de valeurs de contexte éditeur existantes :

- acteur éditeur courant ;
- DungeonAsset / LevelAsset / ObjectPalette ;
- CurrentDungeonLevelId ;
- cellule et arête sélectionnées ;
- objet sélectionné ;
- outil actif ;
- entrée de palette sélectionnée ;
- nombres d’objets/liens ;
- sélection d’édition de route de patrouille de monstre.

Lorsque ce contexte change, seul l’hôte de l’espace de travail ouvert reconstruit son contenu Slate.

Il ne s’agit volontairement ni d’un nouveau subsystem, ni d’un bus d’événements, ni d’un nouveau modèle de données éditeur.

Un futur GEUI09 pourra remplacer cette observation prudente par des notifications plus ciblées après validation de la stabilité du nouvel espace de travail.

## 6. Note sur l’état de validation

L’onglet Nomad PlayTest & Validation possède son état de présentation courant (filtres de messages et derniers résultats affichés), tandis que l’ancien panneau Validation inline conserve son propre état de présentation courant.

Les deux exécutent la même logique de validation faisant autorité.

Cette duplication temporaire de l’état UI est acceptée pour GEUI01 parce que le Toolkit inline reste le fallback de compatibilité. GEUI03 décidera de l’état de présentation canonique unique lorsque PlayTest et Validation seront effectivement migrés.

## 7. Le Toolkit principal reste intact

GEUI01 ne supprime pas :

~~~text
DUNGEON LEVELS
PLAYTEST
TOOLS / PALETTE
OVERVIEW MAP
SELECTED OBJECT
CONNECTORS
VALIDATION
~~~

de FGridLevelEdModeToolkit.

C’est volontaire. Les nouvelles fenêtres sont introduites d’abord, validées sous UE5.5.4, puis les anciennes sections inline seront migrées responsabilité par responsabilité.

## 8. Fichiers modifiés

Nouveaux :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
docs/Design/GEUI01_DOCKABLE_GRID_EDITOR_WORKSPACE_FOUNDATION.md
~~~

Modifié :

~~~text
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp
~~~

Aucune source runtime, aucun fichier .uasset ou .umap n’est modifié.

## 9. Validation UE5.5.4 requise

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5
~~~

Confirmer au minimum que GrimrockPrototypeEditor compile correctement.

Validation visuelle dans l’Unreal Editor :

1. Ouvrir L_GrimrockEditor.
2. Activer Grimrock Grid Editor.
3. Ouvrir Window et vérifier exactement une entrée pour chacun des éléments suivants :
   - Dungeon Levels
   - PlayTest & Validation
   - Tools & Palette
   - Selected Object
   - Grimrock Lua Scripts
4. Ouvrir les quatre nouveaux onglets du Grid Editor.
5. Docker, détacher et placer en fenêtre flottante chaque onglet.
6. Sélectionner des cellules et des objets dans le viewport et vérifier que les panneaux détachés suivent la sélection.
7. Changer d’outil et d’entrée de palette depuis Tools & Palette.
8. Vérifier que les modifications de Selected Object mettent toujours à jour le même LevelAsset.
9. Créer/supprimer un connecteur depuis la fenêtre détachée Selected Object.
10. Lancer la validation depuis PlayTest & Validation.
11. Confirmer que le Toolkit inline d’origine fonctionne toujours sans changement.
12. Lancer une fois le PIE pour confirmer que les changements d’enregistrement du module éditeur n’ont pas affecté la préparation PIE.

## 10. Hors périmètre explicite

GEUI01 ne :

- retire pas les contrôles Dungeon Levels de FGridLevelEdModeToolkit ;
- retire pas les contrôles PlayTest de FGridLevelEdModeToolkit ;
- ne refond pas les catégories/recherche/favoris/éléments récents de la palette ;
- ne refond pas Selected Object en onglets internes finaux ;
- ne fusionne pas l’état de présentation Validation avec l’ancien panneau inline ;
- ne crée pas de subsystem Grid Editor ;
- ne déplace pas AGridLevelEditorActor ;
- ne déplace pas UGridLevelAsset ou UGridDungeonAsset ;
- ne crée pas de plugin ;
- ne modifie pas le comportement runtime ;
- ne modifie pas de fichiers .uasset ou .umap ;
- n’ouvre pas MON21.4.

## 11. Prochaines étapes de migration

Après validation UE5.5.4 :

~~~text
GEUI02 — Dungeon Levels Window
    extraire la liste/actions de niveaux et les composer avec Overview Map

GEUI03 — PlayTest & Validation Window
    migrer les contrôles PlayTest et établir la présentation canonique de Validation
~~~

Les anciennes sections inline ne devront être supprimées qu’après validation fonctionnelle de leur fenêtre dockable correspondante.
