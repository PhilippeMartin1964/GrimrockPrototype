# GEUI07 — UX de la palette

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI07 ajoute des fonctions de productivité à l’espace de travail dédié `Tools & Palette` sans modifier les assets gameplay ni le modèle de données de la palette.

La palette existante fournit déjà :

- recherche ;
- onglets de catégorie ;
- tuiles compactes 96x96 ;
- sélection d’objet stable.

GEUI07 ajoute :

~~~text
Favorites
Recent
~~~

comme vues de premier niveau de la palette.

## 2. Organisation des onglets

La barre d’onglets de la palette est maintenant ordonnée ainsi :

~~~text
All | Favorites | Recent | Doors | Mechanisms | Receptacles | ...
~~~

`Favorites` et `Recent` sont des vues de présentation, pas de nouvelles catégories d’objets.

Les catégories effectives existantes restent celles faisant autorité et continuent de provenir de `UGridObjectPaletteAsset` / des données d’archétype.

La recherche s’applique à chaque vue :

- All ;
- Favorites ;
- Recent ;
- une catégorie précise.

## 3. Favorites

Chaque tuile compacte de palette expose maintenant une petite étoile dans son coin supérieur droit :

~~~text
☆  not favorite
★  favorite
~~~

Cliquer sur l’étoile :

1. bascule l’EntryId dans l’ensemble des favoris de l’utilisateur ;
2. persiste l’ensemble mis à jour ;
3. rafraîchit uniquement la zone de résultats de la palette ;
4. ne sélectionne ni ne place l’objet.

L’onglet `Favorites` n’affiche que les entrées marquées d’une étoile.

S’il est vide, il affiche :

~~~text
No favorites yet. Use the star on a palette tile to add one.
~~~

Les favoris sont indexés par l’`EntryId` de palette existant.

Aucun indicateur de favori n’est écrit dans un DataAsset.

## 4. Recently Used

Une entrée de palette devient récente lorsque l’utilisateur choisit réellement sa tuile d’objet.

GEUI07 conserve au maximum :

~~~text
16
~~~

EntryIds récents.

Règles :

- l’entrée choisie le plus récemment apparaît en premier ;
- choisir de nouveau une entrée déjà récente la replace en première position ;
- les doublons sont supprimés ;
- les anciennes entrées tombent de la fin de la liste au-delà de 16.

L’onglet `Recent` conserve cet ordre de récence au lieu de regrouper les entrées par catégorie.

S’il est vide :

~~~text
No recently used entries yet.
~~~

## 5. Persistance par utilisateur

Favorites et Recent constituent un état propre à l’utilisateur de l’éditeur.

Ils sont persistés via :

~~~text
GEditorPerProjectIni
~~~

sous :

~~~text
[Grimrock.GridEditor.Palette]
Favorites=...
Recent=...
~~~

C’est volontairement :

- par projet ;
- par utilisateur éditeur ;
- hors `.uasset` ;
- hors `.umap` ;
- hors `UGridObjectPaletteAsset` ;
- hors `UGridLevelAsset`.

Modifier les favoris ne rend donc jamais le contenu gameplay dirty.

## 6. État de la palette

`FGridEditorToolPalettePanelState` possède maintenant :

~~~text
CachedIconBrushes
SearchText
SelectedView
SelectedCategory
FavoriteEntryIds
RecentEntryIds
bUserStateLoaded
~~~

Modes de vue :

~~~text
All
Favorites
Recent
Category
~~~

Le champ catégorie n’a de sens que pour `Category`.

Cela évite d’encoder des pseudo-catégories telles que Favorites ou Recent sous forme de fausses catégories `FName`.

## 7. Disposition des résultats

GEUI07 conserve la grille compacte de GEUI04.2 :

- tuiles strictement 96x96 ;
- jusqu’à 8 colonnes par ligne ;
- espacement de grille de 2 px ;
- aucun `SWrapBox` ;
- cellules alignées gauche/haut.

La présentation varie selon la vue :

- `All` : entrées regroupées sous des en-têtes de catégorie ;
- `Category` : une grille compacte plate, sans titre de catégorie redondant ;
- `Favorites` : une grille compacte plate ;
- `Recent` : une grille compacte plate ordonnée par récence.

## 8. Fichiers modifiés

Modifiés :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorToolPalettePanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorToolPalettePanel.cpp
~~~

Nouveau :

~~~text
docs/Design/GEUI07_PALETTE_UX.md
~~~

Aucune source runtime, aucun `.uasset` ni `.umap` n’est modifié.

## 9. Validation UE5.5.4 requise

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Validation visuelle :

1. Ouvrir `Window > Tools & Palette`.
2. Sélectionner `Paint Object`.
3. Confirmer que l’ordre des onglets commence par :
   - All
   - Favorites
   - Recent
4. Confirmer que chaque tuile d’objet reste en 96x96 et montre une petite étoile en haut à droite.
5. Marquer plusieurs entrées comme favorites.
6. Ouvrir Favorites et confirmer que seules ces entrées apparaissent.
7. Retirer une étoile pendant que Favorites est actif et confirmer que l’entrée disparaît immédiatement.
8. Choisir plusieurs tuiles d’objet dans un ordre connu.
9. Ouvrir Recent et confirmer l’ordre du plus récent au plus ancien.
10. Choisir de nouveau un ancien objet récent et confirmer qu’il remonte en première position.
11. Rechercher dans Favorites et Recent.
12. Fermer puis rouvrir la fenêtre Tools & Palette ; confirmer que Favorites et Recent sont conservés.
13. Redémarrer l’Unreal Editor et confirmer qu’ils sont toujours conservés.
14. Confirmer que sélectionner une entrée active toujours Paint Object et utilise le même archétype.

## 10. Hors périmètre explicite

GEUI07 ne :

- modifie pas les DataAssets palette/archétype ;
- ajoute pas de métadonnée favorite au contenu gameplay ;
- ne synchronise pas les favoris entre machines/utilisateurs ;
- ajoute pas de réordonnancement par glisser-déposer ;
- ajoute pas de catégories définies par l’utilisateur ;
- ajoute pas d’édition de l’asset palette ;
- modifie pas la sémantique de placement ;
- modifie pas le comportement runtime ;
- modifie pas de `.uasset` ou `.umap` ;
- n’ouvre pas MON21.4.

## 11. Étape suivante

Après compilation et validation visuelle :

~~~text
GEUI08 — Validation UX
~~~

GEUI08 pourra se concentrer sur la lisibilité de la validation, le filtrage et la navigation à l’intérieur de l’espace de travail dédié PlayTest & Validation.

## GEUI07.1 — Icône de palette agrandie dans une tuile fixe

L’ajustement visuel après validation conserve la tuile de palette strictement à :

~~~text
96 x 96
~~~

tout en augmentant l’icône d’objet de :

~~~text
44 x 44
~~~

à :

~~~text
60 x 60
~~~

La taille de la tuile, l’étoile Favorites en overlay, le libellé et l’espacement de grille restent inchangés. Cela améliore la reconnaissance visuelle des objets sans réduire la densité de la palette.
