# GEUI02 — Fenêtre Dungeon Levels

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI02 transforme l’onglet Nomad `Dungeon Levels` de GEUI01 en premier espace de travail d’authoring complet.

La cible est une fenêtre unique combinant :

- l’identité du donjon et l’état du niveau courant ;
- la liste des niveaux du donjon et les actions de gestion des niveaux ;
- l’Overview Map 32x32 existante ;
- la navigation cellule/objet sélectionné déjà fournie par le widget Overview.

Aucun nouveau modèle de données de donjon ou de niveau n’est introduit.

## 2. Widget unique de gestion des niveaux

L’UI de gestion des niveaux vivait auparavant directement dans :

~~~text
FGridLevelEdModeToolkit::BuildDungeonLevelsPanel
~~~

GEUI02 extrait cette implémentation vers :

~~~text
SGridEditorDungeonLevelsPanel
~~~

Fichiers :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.cpp
~~~

Ce widget est désormais l’unique implémentation Slate du workflow courant de gestion des niveaux.

Il est réutilisé par :

~~~text
legacy inline Grid Editor Toolkit
GEUI02 Dungeon Levels Nomad tab
~~~

Cela préserve le fallback GEUI01 tout en évitant une duplication de logique.

## 3. Actions de niveau conservées

Le panneau extrait conserve le comportement existant :

- Load Default ;
- Reload Current ;
- Log Dungeon ;
- Log Transitions ;
- New Level ;
- sélection d’un niveau de donjon activé possédant un LevelAsset valide.

La `SWindow` transitoire `Create New Dungeon Level` est conservée.

Elle valide toujours :

- Level Id non vide ;
- Level Id unique ;
- position logique unique ;
- nom d’affichage de repli.

La création délègue toujours à :

~~~text
AGridLevelEditorActor::CreateAndAddDungeonLevel
~~~

Aucun contrat de création d’asset n’est dupliqué.

## 4. Nouvelle disposition de l’espace de travail Dungeon Levels

L’onglet Nomad utilise maintenant un splitter Slate horizontal :

~~~text
+------------------------------+------------------------------------------+
| DUNGEON / LEVELS             | OVERVIEW MAP                             |
|                              |                                          |
| Dungeon                      | 32x32 level overview                     |
| Default Level Id             |                                          |
| Current Level Id             | cell selection                           |
| Current LevelAsset           | object markers                           |
| Levels                       | selected-cell summary                    |
| Auto PIE Prepare             | objects at selected cell                 |
|                              |                                          |
| Load / Reload / Diagnostics  |                                          |
| New Level                    |                                          |
|                              |                                          |
| level list                   |                                          |
+------------------------------+------------------------------------------+
~~~

Proportion initiale du splitter :

~~~text
Level management : 36%
Overview Map      : 64%
~~~

### Ajustement ergonomique après validation

Après la première validation visuelle GEUI02, deux contraintes de mise en page ont été renforcées :

- l’Overview Map dérive maintenant sa largeur et sa hauteur demandées du même pas par cellule ;
- un niveau 32x32 demande donc une surface de carte carrée (ratio 1:1) ;
- les actions de gestion des niveaux sont affichées comme une pile verticale de boutons espacés au lieu d’une ligne horizontale compressée.

La règle 1:1 est implémentée dans `SGridEditorOverviewMapPanel`, de sorte que l’espace de travail dockable et l’Overview inline historique bénéficient de la même géométrie.

Une seconde correction après validation centre explicitement l’enfant grille dans son `SBox` carré. Cela empêche Slate d’étirer indépendamment le `SUniformGridPanel` sur l’axe X lorsque le panneau parent reçoit de l’espace horizontal supplémentaire. La surface de carte demandée reste carrée et le contenu réel de la grille conserve désormais la même échelle X/Y pendant le redimensionnement de la fenêtre.

Chaque côté possède sa propre zone de défilement, de sorte qu’une longue liste de niveaux ne pousse pas l’Overview Map vers le bas de la fenêtre.

## 5. Synchronisation

Les deux moitiés continuent d’opérer sur le `AGridLevelEditorActor` courant.

Lorsqu’un niveau est sélectionné ou créé :

1. l’acteur faisant autorité est mis à jour ;
2. le chemin existant d’application du niveau de donjon s’exécute ;
3. l’espace de travail GEUI se reconstruit ;
4. l’Overview Map suit donc le nouveau `LevelAsset` actif ;
5. les viewports éditeur sont redessinés.

L’observation de contexte GEUI01 existante reste inchangée.

## 6. Toolkit historique

L’ancienne section `DUNGEON LEVELS` est volontairement conservée pour ce jalon, mais elle ne contient plus d’implémentation séparée.

Elle instancie désormais :

~~~text
SGridEditorDungeonLevelsPanel
~~~

C’est le fallback de compatibilité promis par GEUI01.

L’ancienne section du Toolkit ne devra pas être supprimée avant que la migration complète de l’espace de travail atteigne GEUI06.

L’`OVERVIEW MAP` inline existante reste également présente pour la même raison.

## 7. Fichiers modifiés

Nouveaux :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorDungeonLevelsPanel.cpp
docs/Design/GEUI02_DUNGEON_LEVELS_WINDOW.md
~~~

Modifiés :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdModeToolkit.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

Aucune source runtime, aucun `.uasset` ou `.umap` n’est modifié.

## 8. Validation UE5.5.4 requise

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Validation visuelle :

1. Ouvrir `L_GrimrockEditor`.
2. Activer `Grimrock Grid Editor`.
3. Ouvrir `Window > Dungeon Levels`.
4. Confirmer que la partie gauche contient les métadonnées du donjon, les actions et la liste de niveaux.
5. Confirmer que la partie droite contient l’Overview Map.
6. Déplacer le splitter entre les deux panneaux.
7. Sélectionner chaque niveau activé existant et confirmer :
   - changement de Current Level Id ;
   - changement de Current LevelAsset ;
   - mise à jour de l’Overview Map vers ce niveau ;
   - suivi du niveau par la preview du viewport.
8. Tester `Load Default`.
9. Tester `Reload Current`.
10. Tester `New Level` jusqu’à l’ouverture de la boîte de dialogue ; la création d’un niveau jetable est facultative car elle modifie les assets.
11. Confirmer que l’ancienne section inline `DUNGEON LEVELS` expose toujours les mêmes actions.
12. Confirmer que l’ancienne `OVERVIEW MAP` inline fonctionne toujours.
13. Fermer et rouvrir l’onglet Nomad et confirmer que le niveau courant est conservé.

## 9. Hors périmètre explicite

GEUI02 ne :

- modifie pas UGridDungeonAsset ;
- modifie pas UGridLevelAsset ;
- modifie pas la sémantique de changement de niveau ;
- supprime pas l’ancienne section inline Dungeon Levels ;
- supprime pas l’ancienne section inline Overview Map ;
- ajoute pas suppression/réordonnancement/duplication de niveaux ;
- refond pas la topologie logique du donjon ;
- crée pas de subsystem ;
- crée pas de plugin ;
- modifie pas le comportement runtime ;
- modifie pas de fichiers .uasset ou .umap ;
- n’ouvre pas MON21.4.

## 10. Étape suivante

Après compilation et validation visuelle :

~~~text
GEUI03 — PlayTest & Validation Window
~~~

GEUI03 effectuera la même extraction privilégiant la réutilisation pour les contrôles PlayTest et les composera avec le panneau Validation existant.
