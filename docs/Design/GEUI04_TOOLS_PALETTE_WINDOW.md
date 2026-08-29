# GEUI04 — Fenêtre Tools & Palette

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI04 transforme l’onglet dockable `Tools & Palette` existant en surface d’authoring principale pour la sélection des outils et la découverte des objets.

Le jalon reste neutre vis-à-vis du modèle de données :

- aucun changement de schéma de `UGridObjectPaletteAsset` ;
- aucun changement de schéma de `UGridObjectArchetypeAsset` ;
- aucun nouvel enum de catégorie gameplay ;
- aucune persistance Favorites/Recent à ce stade ;
- aucun changement de comportement runtime.

## 2. Barre d’outils responsive

Les outils existants sont maintenant affichés au moyen d’une disposition Slate avec retour à la ligne plutôt que sur une ligne horizontale fixe.

Outils visibles :

~~~text
Select
Paint Cell
Paint Wall
Paint Object
Erase
Link
~~~

`Link` existait déjà dans `EGridEditorTool` et dans le code d’interaction éditeur ; GEUI04 l’expose simplement aux côtés des autres outils.

Lorsque la fenêtre se rétrécit, les tuiles d’outils passent à la ligne au lieu d’être rognées ou d’imposer une largeur horizontale excessive.

## 3. Recherche dans la palette

Lorsque `Paint Object` est actif, la section Palette expose maintenant un champ de recherche dynamique.

La recherche correspond aux données existantes :

- nom d’affichage effectif ;
- EntryId de palette ;
- catégorie effective ;
- ArchetypeId de l’archétype ;
- DisplayName de l’archétype ;
- Description de l’archétype.

La saisie met à jour uniquement la zone de résultats de la palette, de sorte que le champ de recherche conserve le focus au lieu de reconstruire l’espace de travail complet à chaque frappe.

Le texte de recherche est un état de présentation uniquement, stocké dans :

~~~text
FGridEditorToolPalettePanelState
~~~

Il ne rend pas un DataAsset dirty.

## 4. Filtres de catégorie

La Palette expose maintenant des boutons de catégorie générés à partir des catégories déjà présentes dans `UGridObjectPaletteAsset`.

Le premier bouton est :

~~~text
All
~~~

suivi des catégories effectives triées selon l’ordre éditeur existant :

~~~text
Doors
Mechanisms
Receptacles
Transitions
Items
Logic
Readable
Wall Decorations
Floor Decorations
Lights
Spawns
Uncategorized
...
~~~

Les catégories inconnues/personnalisées restent prises en charge et sont triées après les catégories éditeur connues.

La catégorie sélectionnée est également un état de présentation uniquement. GEUI04 ne crée pas de seconde taxonomie.

## 5. Résultats de palette responsives

L’ancienne disposition de résultats fixe sur cinq colonnes avec `SUniformGridPanel` est remplacée par des groupes `SWrapBox`.

Les tuiles d’objet reviennent ainsi à la ligne en fonction de la largeur réelle de la fenêtre dockée/flottante.

Les résultats restent regroupés par catégorie et affichent :

~~~text
Showing N of M palette entries
~~~

Lorsqu’aucune entrée ne correspond :

~~~text
No palette entries match the active filters.
~~~

## 6. Propriété de l’état

`FGridEditorToolPalettePanelState` contient maintenant :

~~~text
CachedIconBrushes
SearchText
SelectedCategory
~~~

Cet état appartient uniquement à l’UI éditeur.

L’espace de travail et le Toolkit inline historique conservent chacun leur propre état de présentation tant que les deux interfaces coexistent. Les données gameplay restent sous l’autorité de l’acteur/des assets existants.

Favorites et Recently Used sont volontairement repoussés au jalon ultérieur d’UX palette, où leur stratégie de persistance par utilisateur pourra être choisie explicitement.

## 7. Fichiers modifiés

Modifiés :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorToolPalettePanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorToolPalettePanel.cpp
~~~

Nouveau :

~~~text
docs/Design/GEUI04_TOOLS_PALETTE_WINDOW.md
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
3. Ouvrir `Window > Tools & Palette`.
4. Redimensionner la fenêtre étroite puis large ; confirmer que les tuiles d’outils reviennent proprement à la ligne.
5. Confirmer que l’outil `Link` est présent et active le mode Link existant.
6. Sélectionner `Paint Object`.
7. Confirmer que le champ de recherche et les boutons de catégorie apparaissent.
8. Rechercher par :
   - nom d’objet visible ;
   - fragment d’EntryId ;
   - nom de catégorie ;
   - id/nom d’archétype.
9. Confirmer que le nombre de résultats se met à jour en direct.
10. Sélectionner plusieurs filtres de catégorie puis revenir à `All`.
11. Redimensionner la fenêtre et confirmer que les tuiles d’objet reviennent à la ligne au lieu de se déformer.
12. Sélectionner une entrée de palette filtrée et confirmer que Paint Object utilise toujours le même archétype.
13. Confirmer que l’ancienne section inline Tools / Palette fonctionne toujours.

## 9. Hors périmètre explicite

GEUI04 ne :

- modifie pas les assets palette/archétype ;
- ajoute pas de nouvel enum de catégorie ;
- ajoute pas Favorites ;
- ajoute pas Recently Used ;
- ne persiste pas les préférences UI dans les assets gameplay ;
- ne virtualise pas les palettes très volumineuses ;
- ne déplace pas les assets d’icônes ;
- ne crée pas de plugin ;
- ne modifie pas le comportement runtime ;
- ne modifie pas de fichiers .uasset ou .umap ;
- n’ouvre pas MON21.4.

## 10. Étape suivante

Après compilation et validation visuelle :

~~~text
GEUI05 — Selected Object Workspace
~~~

GEUI05 consolidera l’inspecteur d’objet et les connecteurs dans l’espace de travail dédié à l’objet sélectionné, initialement avec une organisation Properties / Links à faible risque avant une subdivision plus poussée General / Placement / Behavior / Diagnostics.

## GEUI04.1 — Correction UX après validation

La revue visuelle a montré que deux choix de présentation GEUI04 n’étaient pas adaptés à l’espace de travail dédié.

### Tools

Le sélecteur d’outils ne repose plus sur un `SWrapBox` responsive.

Les six outils sont maintenant présentés dans une grille stable de **3 colonnes x 2 lignes** :

~~~text
Select        Paint Cell     Paint Wall
Paint Object  Erase          Link
~~~

Cela empêche un parent Slate étroit ou mesuré de manière ambiguë de réduire les outils d’authoring à une seule colonne verticale.

### Catégories de palette

Les filtres de catégorie sont maintenant présentés sous forme de **barre d’onglets** horizontale, et non comme boutons de filtre indépendants.

La ligne d’onglets commence par :

~~~text
All
~~~

puis les catégories effectives de la palette. L’onglet sélectionné utilise un traitement distinct du fond/texte et un soulignement visible. Si la barre d’onglets complète est plus large que la fenêtre dockée, elle défile horizontalement au lieu de revenir à la ligne sous forme de rangées de boutons.

La recherche reste au-dessus des onglets. Les tuiles de résultats d’objet restent responsives en dessous.

## GEUI04.2 — Outils compacts sur une seule ligne et grille de palette carrée

Une seconde revue visuelle sur la fenêtre dockable réelle UE5.5.4 a montré deux défauts de mise en page restants :

- les six Tools étaient encore répartis sur deux lignes ;
- les tuiles de résultats de palette étaient en pratique disposées sur une seule colonne verticale, avec beaucoup trop d’espace vide autour.

GEUI04.2 supprime les deux dispositions responsives ambiguës.

### Tools

Les outils sont maintenant hébergés dans un seul `SHorizontalBox` avec six slots `AutoWidth` :

~~~text
Select | Paint Cell | Paint Wall | Paint Object | Erase | Link
~~~

Il n’y a ni retour à la ligne ni seconde rangée.

### Résultats de palette

Les entrées de palette n’utilisent plus `SWrapBox`.

Chaque catégorie utilise un `SUniformGridPanel` compact avec jusqu’à **8 colonnes par ligne**. Les slots de grille utilisent un alignement explicite gauche/haut afin que les boutons carrés ne puissent pas s’étirer avec la largeur disponible du panneau.

Chaque tuile de palette est un carré strict de **96 x 96** avec :

- padding de contenu du bouton réduit à 3 px ;
- zone d’icône de 44 px ;
- libellé compact centré en 8 pt ;
- espacement de 2 px entre les cellules de grille.

Lorsqu’un onglet de catégorie précis est sélectionné, son titre de catégorie n’est pas répété au-dessus de la grille. Les en-têtes de catégorie sont conservés uniquement dans l’onglet `All`, où ils séparent les groupes.

Ce jalon ne modifie que la présentation ; la sélection de palette et l’application de l’archétype restent inchangées.

## GEUI04.3 — Restaurer la sémantique de Select et masquer l’outil Link

Les tests visuels après GEUI04 ont révélé deux régressions de comportement/présentation.

### Select n’est plus un outil de peinture continue

Historiquement, le chemin d’entrée éditeur routait tous les outils de clic gauche, y compris `Select`, via le cache de peinture continue :

~~~text
CommitHoveredSelection
ShouldApplyPaintForCurrentSelection
ApplyPaint
~~~

Ce modèle convient à Paint Cell / Paint Wall / Paint Object / Erase, mais pas à la sélection d’objet.

GEUI04.3 donne à `Select` un chemin de clic explicite dans `FGridLevelEdMode::InputKey` :

1. mettre à jour le hover grille/objet depuis la souris ;
2. valider la cellule/arête survolée ;
3. exécuter le `AGridLevelEditorActor::ApplyPrimaryToolAction` existant ;
4. rafraîchir l’état de sélection observé ;
5. ne pas entrer dans le mode de peinture continue/capture souris.

L’implémentation de sélection faisant autorité reste :

~~~text
AGridLevelEditorActor::SelectHoveredObject
AGridLevelEditorActor::SelectObjectAtSelection
AGridLevelEditorActor::SelectObjectById
~~~

Aucun modèle de données de sélection n’est modifié.

### Link est de nouveau masqué

`EGridEditorTool::Link` et son comportement interne restent dans le code éditeur parce que les connecteurs utilisent toujours cette infrastructure.

Cependant, GEUI04.3 retire `Link` de la barre Tools visible. GEUI04 l’avait exposé involontairement lors de l’énumération des outils éditeur disponibles.

La barre d’authoring visible redevient :

~~~text
Select | Paint Cell | Paint Wall | Paint Object | Erase
~~~
