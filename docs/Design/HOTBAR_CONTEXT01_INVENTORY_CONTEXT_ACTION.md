# HOTBAR-CONTEXT01 — ajout d'un item d'inventaire à la barre d'action

## Contrat UX

Le clic droit sur un item d'inventaire compatible avec la barre d'action propose :

```text
Ajouter à la barre d'action
```

Cette action ne déplace pas l'item et ne le consomme pas. Elle crée uniquement un binding dans la barre personnelle du personnage sélectionné.

## Compatibilité

L'option est affichée uniquement si l'item peut déjà être représenté par l'autorité hotbar existante :

- `BuildInventoryCombatActionDefinition()` fournit une action `QuickItem` ; ou
- `IsPhysicallyThrowable()` est vrai.

Une clé, une gemme ou un autre item sans action d'inventaire et sans lancer physique n'affiche pas cette option.

## Affectation

L'exécution réutilise exclusivement :

```cpp
UGridPartyInventoryComponent::SetCharacterCombatHotbarBindingFromItem(...)
```

Le premier slot configurable libre est utilisé. Le slot `PrimaryAttack` reste protégé et n'est jamais remplacé. Aucun slot occupé n'est écrasé automatiquement.

Si la même définition `QuickItem` est déjà présente dans la barre, l'option reste visible mais désactivée avec la raison :

```text
Déjà dans la barre d'action.
```

Si tous les slots configurables sont occupés, l'option reste visible mais désactivée avec la raison :

```text
Barre d'action pleine.
```

L'état est reconstruit au moment du clic via `ExecuteInventoryContextActionByIndex()`, de sorte qu'un changement intervenu après l'ouverture du menu ne puisse pas provoquer d'écrasement.

## Interaction avec HOTBAR-STACK01

Une pile reste liée à la barre tant qu'au moins un exemplaire de sa définition demeure dans l'inventaire. L'ajout d'une pile `Pierre x4` produit donc un raccourci `x4`, puis `x3`, `x2`, `x1` au fil des utilisations. Le binding est retiré seulement à l'épuisement du stock.

## Blueprint

Aucune modification de `.uasset` n'est nécessaire. `WBP_ItemActionMenu` construit déjà ses boutons à partir de `FGridItemContextAction` et exécute l'action par son index.

## Validation automatisée

Filtre :

```text
Grimrock.Hotbar.HOTBAR_CONTEXT01
```

Il couvre :

- apparition et activation de l'option pour une pierre lançable ;
- création du binding sans consommation du stock ;
- désactivation lorsque la définition est déjà assignée ;
- désactivation lorsque la barre est pleine ;
- absence de l'option pour un item incompatible.
