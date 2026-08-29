# GEUI09 — Nettoyage du rafraîchissement et de l’état

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI01 a introduit une observation légère via `Tick()` afin que les onglets Nomad détachés puissent suivre le contexte du Grid Editor sans ajouter de système d’événements parallèle.

C’était volontairement temporaire.

À GEUI08, l’ensemble des espaces de travail était devenu suffisamment stable pour affiner la politique d’observation.

GEUI09 fait deux choses :

1. limiter le contexte observé à l’espace de travail qui en a réellement besoin ;
2. préserver l’état UI de la session éditeur lors de la destruction/recréation des espaces de travail.

GEUI09 n’introduit volontairement **pas** de bus d’événements global éditeur ni de subsystem.

## 2. Problème avant GEUI09

Chaque `SGridEditorWorkspaceTab` observait le même contexte large :

~~~text
DungeonAsset
LevelAsset
ObjectPalette
CurrentDungeonLevelId
SelectedPaletteEntryId
SelectedObjectId
SelectedCell
SelectedEdge
ActiveTool
ObjectCount
LinkCount
Patrol edit state
Patrol waypoint
~~~

Toute différence reconstruisait le contenu complet de **chaque espace de travail ouvert**.

Exemples de travail inutile :

- sélectionner un autre objet reconstruisait Tools & Palette ;
- sélectionner une autre cellule reconstruisait PlayTest & Validation ;
- changer l’outil de peinture actif pouvait reconstruire des fenêtres sans rapport ;
- les changements du nombre de connecteurs pouvaient reconstruire des fenêtres n’affichant jamais de connecteurs.

Cela augmentait également le risque de détruire un état d’interaction Slate transitoire.

## 3. Observation spécifique à chaque espace de travail

GEUI09 conserve un polling léger du contexte, mais n’évalue que les champs pertinents pour chaque espace de travail.

### Dungeon Levels

Observés :

~~~text
DungeonAsset
LevelAsset
CurrentDungeonLevelId
SelectedCell
SelectedEdge
ObjectCount
~~~

Cela garde synchronisés la liste des niveaux, la sélection de l’overview et la géométrie courante.

### PlayTest & Validation

Observés :

~~~text
DungeonAsset
LevelAsset
CurrentDungeonLevelId
~~~

La sélection d’objet/cellule ne reconstruit plus l’interface de recherche/filtrage de Validation.

Les actions à l’intérieur de PlayTest et Validation demandent déjà leur propre rafraîchissement lorsqu’il est nécessaire.

### Tools & Palette

Observés :

~~~text
ObjectPalette
SelectedPaletteEntryId
ActiveTool
~~~

La sélection cellule/objet dans le viewport ne détruit plus et ne recrée plus l’espace de travail de palette.

### Selected Object

Observés :

~~~text
DungeonAsset
LevelAsset
CurrentDungeonLevelId
ObjectPalette
SelectedObjectId
SelectedCell
SelectedEdge
ObjectCount
LinkCount
Patrol edit state
Selected patrol waypoint
~~~

Cet espace de travail reste volontairement le plus sensible à la sélection.

## 4. Nettoyage du polling de l’acteur

Le weak pointer vers l’acteur éditeur observé est réutilisé tant qu’il reste valide.

`FindEditorActor()` n’est donc plus nécessaire simplement pour redécouvrir le même acteur lors de chaque contrôle de contexte.

Un indicateur dédié :

~~~text
bObservedHasEditorActor
~~~

permet aussi à l’espace de travail de détecter proprement la disparition/réapparition de l’acteur au lieu de laisser un contenu détaché obsolète lorsque l’acteur éditeur est détruit.

## 5. État UI de session

GEUI09 ajoute un état de présentation de session éditeur unique, possédé dans l’implémentation du module éditeur.

Il préserve :

~~~text
ToolPaletteState
ValidationState
SelectedObjectPage
ValidationLevelAsset
~~~

Conséquences :

- l’état recherche/vue/favoris/récents de la Palette survit à la fermeture puis réouverture de Tools & Palette dans la même session UE ;
- les filtres/recherche/résultats de Validation survivent à une destruction temporaire de l’espace de travail ;
- Properties vs Connectors survit à la recréation de l’onglet Selected Object ;
- la sortie/rentrée du mode ne réinitialise plus ces choix de présentation.

Il ne s’agit **pas de données gameplay** et cet état n’est pas stocké dans des `.uasset` ou `.umap`.

Favorites/Recent conservent leur persistance de configuration par utilisateur déjà introduite avec GEUI07.

## 6. Sécurité de Validation lors d’un changement de niveau

Les messages de validation appartiennent à un niveau donné.

Par conséquent, lorsque l’espace de travail PlayTest & Validation détecte un `UGridLevelAsset` différent de celui associé à l’état de validation de session, GEUI09 efface :

~~~text
ValidationMessages
bValidationHasRun
~~~

Les préférences de recherche et de filtres de sévérité de l’utilisateur restent intactes.

Cela empêche des erreurs obsolètes du niveau précédent d’apparaître comme si elles appartenaient au nouveau niveau sélectionné.

## 7. Pas encore de bus d’événements global

GEUI09 n’ajoute volontairement pas :

~~~text
UGridEditorSubsystem
FGridEditorEventBus
global multicast delegates
new UObject notification models
~~~

L’éditeur courant possède un seul acteur d’authoring et un petit nombre d’espaces de travail dockables.

Une architecture globale d’événements ajouterait actuellement davantage de complexité de cycle de vie que de valeur.

Si un futur authoring de niveau côté joueur/runtime exige un découplage plus important, ce sujet relève des travaux ultérieurs de préparation plugin/runtime.

## 8. Fichiers modifiés

Modifiés :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

Nouveau :

~~~text
docs/Design/GEUI09_REFRESH_STATE_CLEANUP.md
~~~

Aucune source runtime, aucun `.uasset` ni `.umap` n’est modifié.

## 9. Validation UE5.5.4 requise

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Validation visuelle/comportementale :

1. Ouvrir les quatre espaces de travail du Grid Editor.
2. Dans Validation, saisir une recherche de plusieurs caractères.
3. Changer l’objet/la cellule sélectionné(e) dans le viewport.
4. Confirmer que le texte de recherche Validation reste intact et que l’interaction ciblée n’est pas réinitialisée de manière inattendue.
5. Dans Tools & Palette, choisir une catégorie/recherche.
6. Changer la sélection cellule/objet du viewport.
7. Confirmer que l’état UI de la palette reste inchangé.
8. Passer Selected Object sur Connectors.
9. Quitter Grid Editor puis y revenir.
10. Confirmer que Selected Object revient sur Connectors.
11. Confirmer que Tools & Palette conserve sa vue/recherche de session.
12. Confirmer que Validation conserve son état recherche/filtres.
13. Passer à un autre niveau du donjon.
14. Confirmer que les anciens messages de validation sont effacés et que Validation indique qu’aucune validation n’a encore été exécutée pour le nouveau niveau.
15. Revenir au niveau précédent et relancer la validation.
16. Confirmer qu’aucun comportement runtime/PIE n’a changé.

## 10. Hors périmètre explicite

GEUI09 ne :

- modifie pas les données d’authoring ;
- modifie pas le comportement runtime ;
- modifie pas la sémantique des connecteurs ;
- modifie pas les règles de validation ;
- ajoute pas de subsystem éditeur global ;
- ne persiste pas chaque détail UI entre des lancements UE distincts ;
- modifie pas de `.uasset` ou `.umap` ;
- n’ouvre pas MON21.4.

## 11. Étape suivante

Après validation :

~~~text
GEUI10 — Plugin / Player Level Editor readiness audit
~~~

GEUI10 doit être un jalon d’audit/documentation architectural, et non un refactor. Il classera le code courant du Grid Editor entre editor-only, réutilisable, compatible runtime et candidat à une future extraction, avant de revenir à l’authoring de contenu réel de donjon.
