# UI-INVENTORY02 — Tri et feedback de projection du sac

Date : **22 septembre 2026**  
Statut : **UI-INVENTORY02.6 ACTIF — grille fixe, configuration centralisée, audit C++**

## Objectif

Compléter UI-FILTER01 sans modifier l'ordre physique ni l'autorité de l'inventaire.

Le tri est désormais choisi dans une liste déroulante à six options :

```text
Nom ordre croissant
Nom ordre décroissant
Type ordre croissant
Type ordre décroissant
Poids ordre croissant
Poids ordre décroissant
```

Le tri reste une projection de présentation : il ne déplace jamais physiquement les items.

## Invariant principal

Le tri ne déplace jamais un item dans `FGridCharacterInventoryState::InventorySlots`.

Exemple :

```text
item physique : slot 17
projection triée : première case visible
interaction : cible toujours le slot physique 17
```

Les `RuntimeObjectId`, drag/drop, clic, tooltip et menu contextuel continuent donc à utiliser l'identité physique réelle.

## Interaction filtres + tri

`EGridInventoryFilterCategory` et `EGridInventorySortMode` sont deux états de présentation transients.

- `Tous + Ordre` conserve la grille physique complète, cases vides comprises ;
- **la grille conserve toujours exactement la capacité du sac** (par exemple 40 cases) ;
- un filtre nommé projette les seuls items correspondants dans les premières cases, puis complète avec des cases vides ;
- un tri autre que `Ordre` projette les items triés dans les premières cases, puis complète avec des cases vides ;
- filtre + tri s'appliquent ensemble ;
- changer de personnage conserve le filtre et le tri tant que le widget existe.

## État vide

Nouveau binding optionnel :

```text
Text_InventoryEmptyState
```

Le C++ ne définit pas son texte ; il gère seulement sa visibilité.

Contrat UMG recommandé :

```text
Type        = Text Block
Name        = Text_InventoryEmptyState
Is Variable = Yes
Text        = Aucun objet à afficher
Visibility  = Collapsed
```

Il devient `HitTestInvisible` lorsque la projection contient zéro item.

## Contrôle de tri

`WBP_InventoryBag` expose uniquement :

```text
ComboBox_SortInventory
```

Le C++ peuple les six options et route nativement `OnSelectionChanged` vers `SetInventorySortMode()`.

`Button_SortInventory`, `Text_SortInventory` et `CycleInventorySortMode()` sont supprimés. Aucun Event Graph n'est requis.

## Modification UMG demandée

Dans :

```text
Content/GrimrockPrototype/Blueprints/UI/Inventory/WBP_InventoryBag
```

dans la zone de tri, remplacer l'ancien bouton par :

```text
ComboBox_SortInventory        Combo Box (String), Is Variable = Yes
```

Ne pas saisir manuellement les options : elles sont ajoutées par le C++.

Puis ajouter juste avant `Border_InventoryGridFrame` :

```text
Text_InventoryEmptyState      Text Block, Is Variable = Yes
Text = Aucun objet à afficher
Visibility = Collapsed
```

Ne créer aucun événement Blueprint.

## Tests

```text
Grimrock.UI.Inventory02.SortingProjection
Grimrock.UI.Inventory02.ControlsContract
```

Validation locale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Inventory02"
```

## Hors scope

Ce ticket ne réordonne pas réellement les objets du sac et ne crée pas de commande gameplay « ranger automatiquement ». Il s'agit strictement d'une projection UI.


## Validation finale — 22 septembre 2026

Validation locale fournie par l'utilisateur :

```text
Filter                 : Grimrock.UI.Inventory02
Succeeded              : 5
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le build Development Editor est validé et la validation PIE est confirmée pour :

- cycle Ordre / Nom / Type / Poids ;
- filtres combinés au tri ;
- compteur de projection ;
- état vide ;
- maintien des interactions sur les slots physiques réels.

Le WBP final a été poussé sur `master` dans :

```text
1e19e8d5183453f1fc4dd67880925250e122b897
UI-INVENTORY02 complete inventory bag presentation
```

`UI-INVENTORY02` est **CLOS**.


## UI-INVENTORY02.1 — correction de la grille à capacité fixe

La première version de UI-INVENTORY02 compactait la **grille elle-même** lorsque le tri ou un filtre était actif. Ce comportement est abandonné.

Contrat corrigé :

```text
Capacité du sac = 40
=> toujours 40 cellules visibles

Tous + Ordre
=> représentation physique complète

Tri Nom / Type / Poids
=> items projetés dans l'ordre demandé
=> cellules restantes vides jusqu'à 40

Filtre Équipement / Consommables / ...
=> items correspondants projetés en tête
=> cellules restantes vides jusqu'à 40
```

Les cellules vides créées par la projection utilisent `INDEX_NONE` et ne prétendent pas représenter un slot physique occupé/caché.

Interactions :

- un item visible conserve toujours son vrai `InventorySlotIndex` source ;
- tooltip, clic droit, drag source et actions continuent donc de viser l'objet physique réel ;
- un clic avec item au curseur sur une cellule vide virtuelle utilise l'insertion inventaire existante ;
- un drop depuis équipement/curseur vers une cellule vide virtuelle utilise la première capacité physique disponible ;
- un drag inventaire -> cellule vide virtuelle n'effectue pas de réorganisation physique implicite, car cette cellule n'a volontairement pas d'identité physique.

Test de régression ajouté :

```text
Grimrock.UI.Inventory02.FixedCapacity
```

La clôture précédente de UI-INVENTORY02 reste historiquement valide pour la version initiale ; UI-INVENTORY02.1 doit être revalidé par Automation + PIE avant nouvelle clôture.
