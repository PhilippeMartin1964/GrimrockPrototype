# UI-INV01 — Selected Character Single Bag

Date : **20 septembre 2026**  
Statut : **AUTOMATION VALIDÉE — 20 septembre 2026 ; passe UMG/PIE visuelle encore à réaliser**

## Objectif

UI-INV01 fixe la règle de présentation de l'inventaire :

```text
un personnage sélectionné
    -> un seul sac affiché
```

L'interface ne doit pas empiler les six inventaires du groupe.

Le composant autoritaire reste `UGridPartyInventoryComponent`. Le widget ne copie aucun inventaire et ne possède aucun index parallèle.

## Existant réutilisé

Le code possédait déjà la bonne base :

- `GetInventorySlotCount()` lit le personnage sélectionné ;
- `GetInventoryItemAtSlot()` lit le personnage sélectionné ;
- `ResolveInventorySlotWidgetCount()` lit `Summary.MaxInventorySlots` du personnage sélectionné ;
- `RefreshRegisteredSlotWidgets()` remplit une seule grille ;
- `SelectCharacter()` change `SelectedCharacterIndex` puis rafraîchit le workspace.

UI-INV01 ne crée donc pas six grilles ni six ScrollBoxes.

## Correctif important

`RefreshInventory()` appelle maintenant :

```text
RefreshSelectedInventoryBagPresentation()
EnsureSelectedInventorySlotLayout()
RefreshRegisteredSlotWidgets()
```

`EnsureSelectedInventorySlotLayout()` réutilise `RebuildInventorySlotWidgets()`.

Cette étape est importante lorsque deux personnages ont des capacités différentes, par exemple :

```text
Ariadne : 24 slots
Borin   : 32 slots
```

La même grille est alors reconstruite uniquement si sa configuration doit changer. Le mécanisme existant de cache dans `RebuildInventorySlotWidgets()` évite une reconstruction inutile lorsque le nombre de slots, les colonnes et la classe de widget sont inchangés.

## Présentation du sac

Bindings optionnels ajoutés :

```text
Text_InventoryBagTitle
Text_InventoryBagSlotUsage
Text_InventoryBagWeight
ProgressBar_InventoryBagWeight
```

Ils affichent exclusivement le résumé du personnage sélectionné.

`Text_InventoryBagTitle` reçoit le nom du personnage. Le libellé fixe « Sac » reste du ressort de l'UMG afin de ne pas coder du texte décoratif dans le gameplay C++.

`Text_InventoryBagSlotUsage` :

```text
UsedInventorySlots / MaxInventorySlots
```

`Text_InventoryBagWeight` :

```text
CurrentWeight / MaxWeight
```

La ProgressBar utilise le même ratio de charge, clampé dans `[0..1]`.

## Contrat UMG cible

Dans `Panel_InventoryBag` :

```text
Panel_InventoryBag
└── VerticalBox_InventoryBag
    ├── Header
    │   ├── TextLabel_Bag             "Sac"
    │   ├── Text_InventoryBagTitle
    │   └── Button_CloseInventoryBag
    ├── futurs filtres / tri
    ├── InventorySlotsGridPanel       UNE SEULE grille
    └── Footer
        ├── Text_InventoryBagSlotUsage
        ├── Text_InventoryBagWeight
        └── ProgressBar_InventoryBagWeight
```

Il ne doit exister aucun :

```text
Bag_Character1
Bag_Character2
Bag_Character3
...
```

ni aucune ScrollBox dont la fonction est de parcourir les sacs du groupe.

Le changement de portrait à gauche remplace simplement le contenu de la même grille à droite.

## InventorySlotCountOverride

`InventorySlotCountOverride` reste disponible comme outil explicite de test/migration.

Valeur de production attendue :

```text
0
```

Avec `0`, le nombre de widgets suit la capacité du personnage sélectionné.

Une valeur positive force volontairement un nombre fixe de slots et court-circuite cette capacité. Elle ne doit pas être utilisée pour simuler plusieurs sacs.

## Hors périmètre

UI-INV01 ne réalise pas encore :

- transfert d'un item vers un autre personnage par drop sur portrait ;
- filtres ;
- tri ;
- rangement automatique ;
- comparaison d'équipement ;
- style final de surcharge.

Ces points appartiennent à UI-INV02, UI-FILTER01 et UI-WEIGHT01.

## Invariants

1. Une seule grille d'inventaire visible.
2. Cette grille lit uniquement `SelectedCharacterIndex`.
3. Le nombre de slots suit le personnage sélectionné.
4. Changer de personnage ne déplace aucun item.
5. Changer de personnage ne crée aucun nouvel inventaire.
6. Les slots existants, le Cursor item et le drag/drop sont réutilisés.
7. Aucun `.uasset` n'est modifié à l'aveugle.

## Automation

Filtre :

```text
Grimrock.UI.Inventory01
```

Tests :

```text
Grimrock.UI.Inventory01.SelectedBagProjection
Grimrock.UI.Inventory01.SingleBagAuthority
```

Ils vérifient :

- titre du sac synchronisé avec la sélection ;
- compteur de slots synchronisé ;
- capacité de grille synchronisée ;
- lecture d'un seul inventaire à la fois ;
- conservation de l'override explicite.

## Validation locale

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Inventory01"
```

Après la passe UMG, contrôler en PIE :

1. ouvrir l'inventaire ;
2. vérifier qu'un seul sac est visible ;
3. cliquer chaque portrait ;
4. vérifier que le même panneau change de nom et de contenu ;
5. vérifier que le nombre de slots s'adapte si les capacités diffèrent ;
6. vérifier que l'équipement suit le même personnage ;
7. vérifier que le Cursor item n'est pas perdu ;
8. vérifier que la barre UI-NAV01 reste visible ;
9. aucun `BindWidget` critique.


## Validation reçue

Validation locale du 20 septembre 2026 :

```text
Filter                 : Grimrock.UI.Inventory01
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le contrat C++ UI-INV01 est validé. La passe UMG/PIE reste nécessaire pour la validation visuelle complète.
