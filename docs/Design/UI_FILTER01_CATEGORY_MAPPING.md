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


## UI-FILTER01.3.1 — Routage natif des clics

La validation PIE de UI-FILTER01.3 a montré que l'état initial fonctionnait (`Tous` désactivé) mais que les clics Blueprint des six autres boutons ne modifiaient pas la projection.

Le routage des sept boutons est donc déplacé dans `UGridInventoryBagWidget::NativeConstruct()` avec `AddUniqueDynamic`.

Chaque handler natif appelle uniquement :

```text
SetInventoryFilterCategory(<catégorie>)
```

Les bindings sont retirés dans `NativeDestruct()`.

Conséquence : `WBP_InventoryBag` n'a plus besoin d'aucun Event Graph pour les filtres. Le Blueprint conserve uniquement les widgets et leur style.


## Validation PIE reçue — UI-FILTER01.3.1 — 22 septembre 2026

Validation utilisateur confirmée en PIE : le routage natif des sept boutons de filtre est fonctionnel. Les événements `OnClicked` Blueprint ne sont plus nécessaires pour le filtrage.

Cette validation est une validation fonctionnelle PIE ; aucun nouveau résultat Automation distinct n'est ajouté ici au-delà de la validation UI-FILTER01.3 déjà enregistrée.

## UI-FILTER01.3.2 — Sélection visuelle sans état Disabled

Date : **23 septembre 2026**  
Statut : **IMPLÉMENTÉ — validation locale requise**

La règle historique de UI-FILTER01.3 `Selected -> Disabled` est remplacée.

Le bouton correspondant au filtre courant reste désormais **Enabled**. La sélection est un état de présentation distinct de l'état d'interaction :

- les sept boutons restent cliquables ;
- `Hovered` et `Pressed` continuent donc à fonctionner normalement ;
- Slate n'applique plus son rendu grisé de widget désactivé au filtre sélectionné ;
- la sélection persistante est représentée par le multiplicateur `BackgroundColor` du `UButton`.

Deux propriétés sont exposées sur `WBP_InventoryBag` :

```text
UnselectedFilterBackgroundColor
SelectedFilterBackgroundColor
```

Valeurs C++ par défaut :

```text
Unselected = blanc
Selected   = rouge chaud (1.00, 0.35, 0.25, 1.00)
```

Ces valeurs restent éditables dans le Blueprint. Elles multiplient le brush de l'état courant, ce qui permet de conserver une seule texture par catégorie tout en gardant les styles `Normal`, `Hovered` et `Pressed`.

Le clic sur la catégorie déjà sélectionnée reste sans effet fonctionnel : `SetInventoryFilterCategory()` détecte déjà que la catégorie n'a pas changé et retourne sans reconstruire la projection.

### Note sur Pressed Padding

`Pressed Padding` déplace le **contenu enfant** du `Button`. Il ne déplace pas le brush `Pressed` utilisé comme fond du `Button Style`. Si l'icône complète est directement utilisée comme brush du bouton, le changement de teinte fonctionne mais le padding ne déplace pas cette image.

### Test ajouté

```text
Grimrock.UI.Filter01.BagSelectionPresentation
```

Il vérifie que :

- tous les boutons restent Enabled ;
- la catégorie sélectionnée reçoit `SelectedFilterBackgroundColor` ;
- les autres reçoivent `UnselectedFilterBackgroundColor` ;
- la sélection visuelle se déplace correctement lorsqu'une autre catégorie est choisie.

Validation locale à exécuter :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Filter01"
```

Ne pas considérer UI-FILTER01.3.2 comme validé tant que la sortie locale UE5 n'a pas été fournie.

## UI-FILTER01.3.3 — Autorité visuelle rendue entièrement à UMG

Date : **23 septembre 2026**  
Statut : **IMPLÉMENTÉ — validation locale requise**

UI-FILTER01.3.3 remplace explicitement les règles visuelles introduites par UI-FILTER01.3 et UI-FILTER01.3.2.

Le C++ des boutons de filtre ne doit plus modifier **aucun paramètre visuel ou interactif du `UButton`**. En particulier, `UGridInventoryBagWidget` ne fait plus :

- aucun `SetIsEnabled(...)` pour représenter la sélection ;
- aucun `SetBackgroundColor(...)` ;
- aucune couleur `Selected` / `Unselected` exposée par le C++ ;
- aucun rafraîchissement de style ou de teinte lors du changement de filtre ;
- aucune substitution de l'état `Disabled` pour simuler un état sélectionné.

L'autorité du C++ est désormais limitée au **routage fonctionnel** des clics :

```text
Button_FilterAll          -> SetInventoryFilterCategory(All)
Button_FilterEquipment    -> SetInventoryFilterCategory(Equipment)
Button_FilterConsumables  -> SetInventoryFilterCategory(Consumables)
Button_FilterMagic        -> SetInventoryFilterCategory(Magic)
Button_FilterIngredients  -> SetInventoryFilterCategory(Ingredients)
Button_FilterBooksAndKeys -> SetInventoryFilterCategory(BooksAndKeys)
Button_FilterMisc         -> SetInventoryFilterCategory(Misc)
```

Toute l'apparence des boutons appartient à `WBP_InventoryBag` et à chaque `Button Style` UMG :

```text
Normal
Hovered
Pressed
Disabled
Normal Padding
Pressed Padding
Background Color
Color and Opacity
Brush Tint
Draw As
Image / Resource Object
```

Le C++ ne réécrit plus ces valeurs au runtime. Le designer peut donc décider librement des couleurs, textures, tints, paddings et autres paramètres directement dans le widget `Button`.

Le test `Grimrock.UI.Filter01.BagSelectionPresentation`, devenu contraire à ce contrat, est supprimé. `Grimrock.UI.Filter01.BagControlsContract` continue de vérifier uniquement l'existence des sept boutons et de leurs handlers natifs.

Validation locale à exécuter :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Filter01"
```

Ne pas considérer UI-FILTER01.3.3 comme validé tant que la sortie locale UE5 n'a pas été fournie.

## UI-FILTER01.3.4 — Cadre de sélection natif, style du Button intact

Date : **23 septembre 2026**  
Statut : **IMPLÉMENTÉ — validation locale requise**

Le filtre courant reçoit désormais un cadre de surbrillance persistant créé par `UGridInventoryBagWidget`.

Le contrat visuel reste volontairement minimal :

- le C++ ne modifie toujours pas `BackgroundColor`, `ColorAndOpacity`, les brushes `Normal/Hovered/Pressed/Disabled`, les tints ni les paddings du `Button` ;
- le bouton reste `Enabled` ;
- le C++ ajoute uniquement un `UImage` transparent avec contour, superposé autour du bouton sélectionné ;
- le cadre est `HitTestInvisible` et ne gêne donc pas les clics ;
- lorsque le filtre change, seule la visibilité des sept cadres est mise à jour.

Paramètres éditables dans `WBP_InventoryBag > Class Defaults > Inventory | Filter | Selection` :

```text
SelectedFilterFrameColor
SelectedFilterFrameThickness
```

Valeurs par défaut :

```text
Color     = (1.00, 0.65, 0.10, 1.00)
Thickness = 3.0
```

Le cadre est construit avec un `FSlateRoundedBoxBrush` à remplissage transparent et contour coloré. Les boutons restent propriétaires de leur style UMG.

Test de contrat étendu :

```text
Grimrock.UI.Filter01.BagControlsContract
```

Validation locale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Filter01"
```

Une validation PIE est également requise pour confirmer visuellement le cadre sur les sept boutons.

