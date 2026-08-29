# GEUI06 — Toolkit principal allégé

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI06 termine la première migration des espaces de travail du Grid Editor en supprimant les grands panneaux d’authoring encore dupliqués à l’intérieur du Toolkit du mode éditeur.

Le Toolkit principal n’est plus l’endroit où les panneaux d’authoring de niveau sont embarqués.

Il agit désormais comme un dashboard compact contenant :

- le titre Dungeon Editor ;
- les badges du contexte éditeur courant ;
- les options d’affichage des connecteurs dans le viewport ;
- les boutons d’ouverture des espaces de travail.

## 2. Sections d’authoring inline supprimées

Les anciennes sections inline suivantes sont supprimées du Toolkit principal :

~~~text
DUNGEON LEVELS
PLAYTEST
TOOLS / PALETTE
OVERVIEW MAP
SELECTED OBJECT
CONNECTORS
VALIDATION
~~~

Leurs remplaçants dockables faisant autorité sont :

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
~~~

L’authoring Lua reste dans :

~~~text
Grimrock Lua Scripts
~~~

Aucune fonctionnalité n’est supprimée ; seule la présentation dupliquée disparaît.

## 3. Lanceur d’espaces de travail

Le Toolkit expose désormais cinq boutons d’ouverture explicites :

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
Grimrock Lua Scripts
~~~

Chaque bouton invoque l’onglet Nomad déjà enregistré via `FGlobalTabmanager`.

Si un espace de travail est déjà ouvert, Unreal remet cet onglet au premier plan au lieu de construire une seconde implémentation indépendante de l’authoring.

## 4. Identifiants partagés des onglets d’espace de travail

GEUI06 ajoute :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorWorkspaceTabs.h
~~~

Ce header centralise les identifiants canoniques d’onglets utilisés à la fois par :

- `FGrimrockPrototypeEditorModule` pour l’enregistrement/les entrées de menu ;
- `FGridLevelEdModeToolkit` pour les boutons d’ouverture du dashboard.

Cela supprime la duplication des noms/chaînes entre l’enregistrement du module et le lanceur.

Les identifiants canoniques restent inchangés :

~~~text
GrimrockGridDungeonLevels
GrimrockGridPlaytestValidation
GrimrockGridToolsPalette
GrimrockGridSelectedObject
GrimrockLuaEditor
~~~

## 5. État du dashboard principal

Le dashboard ne conserve qu’un contexte léger.

Badges d’état :

~~~text
Tool
Cell
Edge/Facing
Object
Level
~~~

L’ancien badge Validation est supprimé car son état de présentation appartenait à l’ancien widget Validation inline et deviendrait obsolète une fois ce widget disparu.

L’état de validation vit désormais dans l’espace de travail dédié `PlayTest & Validation`.

## 6. Paramètres d’affichage conservés

Ces options limitées au viewport restent dans le dashboard principal :

~~~text
Show Outgoing Connectors
Show Incoming Connectors
Show Connector Labels
~~~

Elles restent des paramètres globaux d’affichage utiles et n’appartiennent pas exclusivement à la page d’authoring Selected Object / Connectors.

Leur comportement est inchangé.

## 7. Nettoyage du Toolkit

GEUI06 supprime l’état de présentation propre au Toolkit qui n’est plus nécessaire :

~~~text
FGridEditorPanelExpansionState
ToolPaletteState
ValidationState
BuildCollapsiblePanelSection
TogglePanelExpansion
ExpandValidationIfMessagesNeedAttention
GetValidationStatusText
~~~

Le nom public de méthode `RefreshPalette()` est volontairement conservé car `FGridLevelEdMode` l’utilise déjà comme point d’entrée de rafraîchissement du Toolkit. Son implémentation rafraîchit maintenant le dashboard compact au lieu de reconstruire une palette inline.

Le nom affiché du Toolkit passe de :

~~~text
Grimrock Grid Palette
~~~

à :

~~~text
Grimrock Grid Editor
~~~

## 8. Fichiers modifiés

Nouveaux :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorWorkspaceTabs.h
docs/Design/GEUI06_SLIM_MAIN_TOOLKIT.md
~~~

Modifiés :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdModeToolkit.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp
~~~

Aucune source runtime, aucun `.uasset` ni `.umap` n’est modifié.

## 9. Validation UE5.5.4 requise

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Validation visuelle :

1. Ouvrir `L_GrimrockEditor`.
2. Activer `Grimrock Grid Editor`.
3. Confirmer que le Toolkit inline ne contient plus les sept anciens panneaux d’authoring.
4. Confirmer que le Toolkit contient :
   - le titre DUNGEON EDITOR ;
   - les badges Tool / Cell / Edge-Facing / Object / Level ;
   - les trois options d’affichage des connecteurs ;
   - le lanceur WORKSPACE.
5. Ouvrir chaque bouton du lanceur et confirmer que l’onglet Nomad attendu apparaît.
6. Cliquer sur un lanceur correspondant à une fenêtre déjà ouverte et confirmer qu’Unreal la remet au premier plan/la réutilise.
7. Changer la cellule/l’objet/l’outil sélectionné et confirmer que les badges du dashboard se rafraîchissent.
8. Basculer outgoing/incoming/labels et confirmer que le comportement du viewport est inchangé.
9. Confirmer que les entrées du menu `Window` ouvrent toujours les cinq espaces de travail.
10. Exécuter un court smoke test PIE.

## 10. Hors périmètre explicite

GEUI06 ne :

- supprime aucune capacité d’authoring ;
- supprime aucun widget partagé sous-jacent ;
- modifie pas les données niveau/runtime ;
- modifie pas la logique des connecteurs ;
- modifie pas la logique de validation ;
- modifie pas la préparation PIE ;
- ne persiste pas le layout de l’espace de travail ;
- ne crée pas de plugin ;
- ne modifie pas de `.uasset` ou `.umap` ;
- n’ouvre pas MON21.4.

## 11. Étape suivante

Après compilation et validation visuelle, la première phase de migration des espaces de travail est terminée.

Le prochain élément de la roadmap est :

~~~text
GEUI07 — Palette UX
~~~

GEUI07 peut alors se concentrer uniquement sur les fonctions de productivité avancées de la palette telles que Favorites, Recently Used et l’organisation orientée utilisateur, sans subir les contraintes de l’ancien Toolkit monolithique.

## GEUI06.1 — Limiter les fenêtres d’espace de travail au mode Grid Editor

Les onglets Nomad d’espace de travail sont maintenant explicitement liés au cycle de vie de :

~~~text
EM_GrimrockGridLevelEdMode
~~~

Comportement :

- tant que Grimrock Grid Editor est actif, les onglets d’espace de travail peuvent être ouverts normalement ;
- lorsque l’on quitte le mode, tous les onglets Grimrock vivants reçoivent une demande de fermeture ;
- tant que le mode est inactif, leurs spawners Nomad refusent leur création via `FCanSpawnTab` ;
- le menu Window d’Unreal ne peut donc pas faire apparaître ces fenêtres d’authoring en dehors du mode Grid Editor.

Les onglets concernés sont :

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
Grimrock Lua Scripts
~~~

`GridEditorWorkspaceTabs::All()` centralise cette liste afin que le comportement de fermeture et les identifiants canoniques ne puissent pas diverger.

Il s’agit uniquement d’un comportement de cycle de vie éditeur et cela n’affecte pas les données de niveau/runtime.

## GEUI06.2 — Restaurer les onglets ouverts lors du retour dans Grid Editor

GEUI06.1 masquait correctement les fenêtres du Grid Editor en dehors du mode éditeur, mais ne mémorisait pas quelles fenêtres d’espace de travail étaient ouvertes lorsque l’utilisateur quittait le mode.

GEUI06.2 ajoute un comportement de restauration de session.

### Comportement à la sortie

Avant de fermer les onglets d’espace de travail, `FGridLevelEdMode::Exit()` enregistre maintenant l’ensemble exact des valeurs `TabId` des espaces de travail Grimrock encore vivants.

Seuls les onglets réellement ouverts sont enregistrés.

Si un utilisateur ferme manuellement un espace de travail avant de quitter Grid Editor, cet espace de travail n’est pas restauré ensuite.

### Comportement à l’entrée

Après que `FGridLevelEdMode::Enter()` a activé le mode et initialisé son Toolkit, chaque onglet d’espace de travail mémorisé est de nouveau invoqué via :

~~~text
FGlobalTabmanager::TryInvokeTab
~~~

Comme les mêmes TabIds stables sont réutilisés, le layout de docking d’Unreal peut restaurer chaque onglet dans sa pile de docking / position de fenêtre flottante précédente au lieu de créer une surface d’authoring sans rapport.

Le workflow attendu est donc :

~~~text
Grid Editor actif
  -> Dungeon Levels + Tools & Palette ouverts et positionnés

Quitter Grid Editor
  -> ces fenêtres disparaissent

Revenir dans Grid Editor
  -> Dungeon Levels + Tools & Palette se rouvrent automatiquement
     à leurs emplacements de docking Unreal mémorisés
~~~

Cette liste de restauration est un état de présentation éditeur limité à la session. Elle ne touche ni les assets gameplay ni les données de niveau.

## GEUI06.3 — Restaurer la géométrie des fenêtres flottantes

GEUI06.2 mémorisait quels onglets d’espace de travail étaient ouverts, mais un onglet Nomad flottant pouvait encore être recréé à la position centrée par défaut d’Unreal.

GEUI06.3 mémorise le rectangle de la fenêtre flottante avant la fermeture de l’onglet.

Pour chaque onglet d’espace de travail vivant lors de la sortie du Grid Editor :

1. trouver la `SWindow` propriétaire via `FSlateApplication::FindWidgetWindow` ;
2. la comparer à `FGlobalTabmanager::GetRootWindow()` ;
3. si ce n’est pas la fenêtre racine de l’éditeur, la traiter comme une fenêtre flottante d’espace de travail ;
4. stocker son `FSlateRect` obtenu par `GetRectInScreen()` ;
5. fermer l’onglet comme auparavant.

Lors du retour dans Grid Editor :

1. restaurer le même TabId avec `TryInvokeTab` ;
2. trouver sa nouvelle `SWindow` flottante ;
3. réappliquer le rectangle sauvegardé via `SWindow::ReshapeWindow`.

Les onglets dockés dans la fenêtre principale d’Unreal Editor ne sont pas redimensionnés ; ce code ne peut donc pas déplacer ou redimensionner accidentellement la fenêtre racine du Level Editor.

Résultat attendu : une fenêtre `PlayTest & Validation` flottante revient exactement à la même position écran et avec la même taille qu’avant la sortie du Grid Editor.
