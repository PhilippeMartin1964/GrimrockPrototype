# GEUI03 — Fenêtre PlayTest & Validation

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI03 complète le deuxième espace de travail dockable d’authoring en fusionnant les contrôles PlayTest existants avec le panneau Validation existant.

Le jalon reste uniquement de présentation :

- aucune règle de préparation PIE n’est modifiée ;
- aucune règle de validation n’est modifiée ;
- aucun chemin de validation Lua/Event -> Command n’est dupliqué ;
- aucune classe runtime ni aucun asset n’est modifié.

## 2. Widget PlayTest partagé

Les contrôles PlayTest vivaient auparavant directement dans :

~~~text
FGridLevelEdModeToolkit::BuildPlaytestPanel
~~~

GEUI03 les extrait vers :

~~~text
SGridEditorPlaytestPanel
~~~

Fichiers :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorPlaytestPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorPlaytestPanel.cpp
~~~

Le Toolkit inline historique et l’espace de travail dockable instancient tous deux ce même widget.

Il existe donc une seule implémentation Slate du comportement PlayTest.

## 3. Contrôles PlayTest conservés

Le panneau extrait conserve :

- Auto Prepare PIE ;
- Abort PIE On Error ;
- Current LevelAsset ;
- statut Start Cell / Facing / Valid ;
- avertissement de départ invalide ;
- Set Start From Selection ;
- Log PIE Readiness ;
- Debug Prepare PIE.

Les boutons d’action sont affichés sous forme de pile verticale espacée afin de rester lisibles dans un panneau docké étroit.

Les appels existants restent ceux faisant autorité :

~~~text
AGridLevelEditorActor::SetStartFromSelection
AGridLevelEditorActor::PreparePIETestFromStart
AGridLevelRuntimeActor::LogPIEReadinessDiagnostics
~~~

## 4. Espace de travail PlayTest & Validation

L’onglet Nomad utilise maintenant un splitter horizontal :

~~~text
+--------------------------------+------------------------------------------+
| PLAYTEST                       | VALIDATION                               |
|                                |                                          |
| Auto Prepare PIE               | Refresh Validation                       |
| Abort PIE On Error             | Copy Summary                             |
|                                |                                          |
| Current LevelAsset             | Errors / Warnings / Infos                |
| Start Cell / Facing / Valid    | severity filters                         |
|                                |                                          |
| Set Start From Selection       | validation message list                  |
| Log PIE Readiness              | Select/Focus object                      |
| Debug Prepare PIE              | Select cell                              |
+--------------------------------+------------------------------------------+
~~~

Proportion initiale du splitter :

~~~text
PlayTest   : 34%
Validation : 66%
~~~

Chaque côté possède sa propre zone de défilement.

## 5. Autorité de validation

Le panneau Validation dockable continue d’utiliser :

~~~text
SGridEditorValidationPanel
GridEditorLuaService::ValidateCurrentLevelWithLua
~~~

GEUI03 n’introduit pas de second validateur.

L’espace de travail dockable conserve son état de présentation (derniers messages affichés et filtres de sévérité) tant qu’il est ouvert. L’ancienne section Validation inline conserve son propre état de présentation de secours jusqu’à l’allègement du Toolkit monolithique dans GEUI06.

Il ne s’agit que d’une duplication de l’état UI ; la logique de validation et les données de niveau restent à source unique.

## 6. Toolkit historique

La section inline `PLAYTEST` existante reste visible comme fallback, mais délègue désormais à :

~~~text
SGridEditorPlaytestPanel
~~~

La section inline `VALIDATION` est également conservée jusqu’à GEUI06.

Cela reproduit la stratégie de migration GEUI02 et maintient un faible risque de rollback.

## 7. Fichiers modifiés

Nouveaux :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorPlaytestPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorPlaytestPanel.cpp
docs/Design/GEUI03_PLAYTEST_VALIDATION_WINDOW.md
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
3. Ouvrir `Window > PlayTest & Validation`.
4. Confirmer que PlayTest apparaît à gauche et Validation à droite.
5. Déplacer le splitter.
6. Basculer `Auto Prepare PIE`.
7. Basculer `Abort PIE On Error`.
8. Sélectionner une cellule valide et utiliser `Set Start From Selection`.
9. Confirmer que Start Cell / Facing / Valid se rafraîchit immédiatement.
10. Exécuter `Log PIE Readiness`.
11. Exécuter `Refresh Validation`.
12. Tester les filtres Errors / Warnings / Infos.
13. Utiliser au moins une action Select/Focus depuis un message de validation lorsqu’elle est disponible.
14. Confirmer que les sections inline historiques PLAYTEST et VALIDATION fonctionnent toujours.
15. Lancer une fois le PIE et confirmer que le comportement de préparation automatique reste inchangé.

## 9. Hors périmètre explicite

GEUI03 ne :

- ajoute pas de commande personnalisée Start PIE ;
- modifie pas les hooks PreBeginPIE ;
- modifie pas la sémantique bAutoPreparePIE ;
- modifie pas la sémantique bAbortPIEOnPreparationError ;
- modifie pas les règles de validation ;
- modifie pas la validation Lua ;
- supprime pas les panneaux inline historiques ;
- crée pas de subsystem ;
- crée pas de plugin ;
- modifie pas le comportement runtime ;
- modifie pas de fichiers .uasset ou .umap ;
- n’ouvre pas MON21.4.

## 10. Étape suivante

Après compilation et validation visuelle :

~~~text
GEUI04 — Tools & Palette Window
~~~

GEUI04 conservera le comportement existant des outils/palette mais préparera la surface d’authoring dédiée pour la recherche, le filtrage par catégorie puis, plus tard, Favorites / Recently Used.
