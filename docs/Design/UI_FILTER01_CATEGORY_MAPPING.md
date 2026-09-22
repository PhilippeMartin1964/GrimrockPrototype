# UI-FILTER01.1 — Inventory Filter Category Mapping

Date : **22 septembre 2026**  
Statut : **VALIDÉ — 22 septembre 2026**

## Objectif

Définir les catégories visibles de l'Inventaire sans modifier la taxonomie gameplay existante.

L'autorité reste `EGridItemType` dans `UGridItemDefinitionAsset`. Le filtre est uniquement une projection UI.

## Catégories visibles

```text
Tous
Équipement
Consommables
Magie
Ingrédients
Livres et clés
Divers
```

## Mapping canonique

| Catégorie UI | EGridItemType |
|---|---|
| Tous | tous les types |
| Équipement | Torch, Weapon, Shield, Armor, Jewelry |
| Consommables | Potion, Food |
| Magie | Scroll, Gem |
| Ingrédients | Component |
| Livres et clés | Key, Book |
| Divers | Quest, Misc, None |

## Justification des cas ambigus

- `Torch -> Équipement` : la torche est un objet utilitaire équipable en main dans le runtime actuel.
- `Gem -> Magie` : une gemme est un objet fantasy/spécial distinct d'un composant d'artisanat. Elle ne doit pas être assimilée à `Component` sans règle de crafting explicite.
- `Component -> Ingrédients` : c'est le type actuellement adapté aux composants de crafting/loot, par exemple les dents de rat.
- `Quest -> Divers` : un objet de quête peut être de nature quelconque ; il n'est pas nécessairement un livre ou une clé.
- `None -> Divers` : fallback de présentation. Un item valide mais mal typé reste retrouvable dans une catégorie nommée au lieu de disparaître.

## Contrat C++

`EGridInventoryFilterCategory` est un enum de présentation `BlueprintType`.

`ResolveGridInventoryFilterCategory(EGridItemType)` réalise le mapping.

`DoesGridItemTypeMatchInventoryFilter(EGridItemType, EGridInventoryFilterCategory)` applique le filtre, avec `All` comme bypass.

Ces helpers ne modifient jamais l'item et ne deviennent pas une autorité gameplay.

## Hors périmètre de UI-FILTER01.1

Ce ticket ne réalise pas encore :

- boutons UMG ;
- filtrage effectif de la grille ;
- conservation du filtre lors d'un changement de personnage ;
- tri par nom, poids, type ou autre critère ;
- rangement automatique ;
- déplacement physique des items dans les slots.

Ces comportements appartiennent aux étapes suivantes de UI-FILTER01.

## Validation

Filtre :

```text
Grimrock.UI.Filter01
```

Test initial :

```text
Grimrock.UI.Filter01.CategoryMapping
```

Il couvre les quinze valeurs actuelles de `EGridItemType`, le bypass `Tous` et plusieurs rejets inter-catégories.


## Validation reçue — 22 septembre 2026

```text
Filter                 : Grimrock.UI.Filter01
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

UI-FILTER01.1 est validé. Le mapping catégories UI -> `EGridItemType` est désormais le contrat de référence pour UI-FILTER01.2.


## UI-FILTER01.2 — Application du filtre à la grille

Le filtre devient un état de présentation transient de `UGridInventoryWidget`, initialisé à `All`.

API Blueprint :

```text
SetInventoryFilterCategory(Category)
GetInventoryFilterCategory()
```

La grille filtrée est une **projection compacte** des slots physiques correspondants :

- `All` conserve la grille physique complète, y compris les slots vides ;
- une catégorie nommée n'affiche que les slots occupés dont le `EGridItemType` correspond ;
- les résultats conservent l'ordre croissant des indices physiques ;
- chaque widget filtré conserve l'indice du slot physique source pour les clics, drag/drop et actions contextuelles ;
- changer de filtre ne déplace, ne trie et ne réécrit aucun item ;
- un item dont la définition n'est pas résolue utilise le fallback `None -> Divers` ;
- zéro résultat produit une grille filtrée vide, sans faux slot inventé.

Le filtre est conservé lors d'un changement de personnage tant que la même instance `WBP_InventoryBag` reste ouverte. Il n'est pas persisté dans la sauvegarde.

Le cache de génération compare aussi la liste des indices physiques visibles. Ainsi, si les items changent de slots mais que le nombre de résultats reste identique, la projection est tout de même reconstruite avec les bons indices sources.

Le filtrage n'implémente encore aucun tri ni rangement automatique.

Test ajouté :

```text
Grimrock.UI.Filter01.GridProjection
```


## Validation reçue — UI-FILTER01.2 — 22 septembre 2026

```text
Filter                 : Grimrock.UI.Filter01
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

UI-FILTER01.2 est validé côté C++/Automation. Le filtrage réel de la grille et la conservation des indices physiques sources sont couverts. La passe UMG de branchement des boutons reste à réaliser.


## UI-FILTER01.3 — Contrôles UMG et état sélectionné

`WBP_InventoryBag` possède les sept boutons de filtre suivants :

```text
Button_FilterAll
Button_FilterEquipment
Button_FilterConsumables
Button_FilterMagic
Button_FilterIngredients
Button_FilterBooksAndKeys
Button_FilterMisc
```

Leur clic continue d'appeler `SetInventoryFilterCategory` depuis le Blueprint, avec une seule commande par bouton.

Le C++ prend désormais en charge automatiquement l'état sélectionné : le bouton correspondant au filtre courant est désactivé et les six autres restent activés. L'apparence du bouton sélectionné provient donc du **Disabled style du Button UMG** ; aucune couleur n'est dupliquée dans le Graph ni codée en C++.

Cette règle est rafraîchie à la construction de `WBP_InventoryBag` et après tout changement de filtre, y compris si le filtre est changé par une autre voie plus tard.

Test ajouté :

```text
Grimrock.UI.Filter01.BagControlsContract
```


## Validation reçue — UI-FILTER01.3 — 22 septembre 2026

```text
Filter                 : Grimrock.UI.Filter01
Succeeded              : 3
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

UI-FILTER01.3 est validé côté C++/Automation. Le contrat natif des sept boutons et le feedback de sélection piloté par le filtre courant sont couverts. La validation visuelle PIE du style Disabled reste à confirmer dans l'asset UMG.
