# UI-INVENTORY02 — Tri et feedback de projection du sac

Date : **22 septembre 2026**  
Statut : **C++ PRÊT — UMG/Automation À VALIDER**

## Objectif

Compléter UI-FILTER01 sans modifier l'ordre physique ni l'autorité de l'inventaire.

Le sac propose désormais quatre tris de présentation :

```text
Ordre -> ordre/cases physiques
Nom   -> nom affiché
Type  -> EGridItemType puis nom
Poids -> poids total de la pile puis nom
```

Tous les tris sont ascendants et déterministes.

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
- un filtre nommé compacte les seuls items correspondants ;
- un tri autre que `Ordre` compacte les items occupés puis les ordonne ;
- filtre + tri s'appliquent ensemble ;
- changer de personnage conserve le filtre et le tri tant que le widget existe.

## Compteur

Le binding existant `Text_InventoryBagSlotUsage` devient plus informatif :

```text
3 affichés / 17 utilisés / 40 cases
```

Il distingue donc la projection visible de l'occupation physique réelle.

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

`WBP_InventoryBag` expose :

```text
Button_SortInventory
└── Text_SortInventory
```

Le clic est routé nativement :

```text
Ordre -> Nom -> Type -> Poids -> Ordre
```

Le texte est mis à jour par le C++ :

```text
Tri : Ordre
Tri : Nom
Tri : Type
Tri : Poids
```

Aucun Event Graph n'est requis.

## Modification UMG demandée

Dans :

```text
Content/GrimrockPrototype/Blueprints/UI/Inventory/WBP_InventoryBag
```

ajouter dans `WrapBox_InventoryFilters`, après les sept filtres :

```text
Button_SortInventory          Button, Is Variable = Yes
└── Text_SortInventory        Text Block, Is Variable = Yes
```

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
Grimrock.UI.Inventory02.SortCycle
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
