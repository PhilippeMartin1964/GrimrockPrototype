# TD06.4 — PartyInventory Cursor Transfer characterization

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.3 — PartyInventory Hotbar extraction**  
Baseline GitHub : `ccc6bd6e2c95592dd402be2f5c90579ed6b59959`  
Statut : **PRÊT À VALIDER LOCALEMENT**

## Objet

TD06.4 caractérise la frontière transactionnelle `Inventory <-> Cursor <-> Equipment` de `UGridPartyInventoryComponent` avant toute extraction de code de production.

Le but est de figer les comportements observables dont dépend TD06.5, sans créer une nouvelle autorité d'état et sans tester la localisation physique de l'implémentation.

## Autorité préservée

`UGridPartyInventoryComponent` reste l'unique façade et l'unique autorité de `FGridPartyInventoryState`.

La caractérisation ne crée :

- aucun nouveau composant ;
- aucun nouvel état de curseur ;
- aucune copie d'inventaire ;
- aucune structure SaveGame ;
- aucune modification de Blueprint API.

## Contrat Automation

Nouveau test :

```text
Source/GrimrockPrototype/Private/Tests/GridTD064PartyInventoryCursorTransferTests.cpp
```

Filtre :

```text
Grimrock.TechnicalDebt.TD06_4.PartyInventoryCursorTransfer.Contract
```

Le test utilise uniquement l'API publique de `UGridPartyInventoryComponent` et l'état autoritaire observable.

## Contrats caractérisés

### API publique

Les fonctions Blueprint suivantes doivent rester réfléchies et `BlueprintCallable` :

```text
SetCursorItem
ClearCursorItem
HasCursorItem
TryTakeInventorySlotToCursor
TryTakeInventorySlotQuantityToCursor
TryPlaceCursorItemInCharacterInventorySlot
TryMoveCharacterInventorySlot
TryPlaceCursorItemInCharacterInventory
TryPlaceCursorItemInSelectedCharacterInventory
TryClearCursorToSelectedCharacterInventory
TryDropCursorItem
CanEquipCursorItemToCharacterSlot
TryEquipCursorItemToCharacterSlot
TryEquipCursorItemToSelectedCharacterSlot
TryTakeEquipmentSlotToCursor
TryTakeSelectedCharacterEquipmentSlotToCursor
```

`GetCursorItem()` reste une API C++ non réfléchie et son contrat est vérifié directement.

### Inventory -> Cursor

Le test verrouille :

- transfert complet d'un slot ;
- conservation du `RuntimeObjectId` pour un transfert complet ;
- split d'une pile stackable ;
- quantité restante dans la pile source ;
- création d'un nouveau `RuntimeObjectId` pour la portion séparée ;
- normalisation de l'ownership vers `Cursor`.

### Cursor -> Inventory

Le test verrouille :

- placement dans un slot vide ;
- conservation de l'identité runtime ;
- retour vers le premier slot libre ;
- swap atomique avec un slot occupé ;
- normalisation des ownerships des deux objets.

### Inventory slot move / swap

`TryMoveCharacterInventorySlot()` est conservé dans cette frontière car il appartient au même contrat de transaction de l'UI d'inventaire.

Le test verrouille :

- move vers slot vide ;
- swap entre deux slots occupés ;
- conservation des `RuntimeObjectId`.

### Cursor <-> Equipment

Le test verrouille :

- compatibilité Cursor -> équipement ;
- équipement vers slot vide ;
- Equipment -> Cursor ;
- swap Cursor / équipement occupé ;
- conservation de l'identité runtime ;
- `OwnerType`, `OwnerCharacterIndex` et `EquipmentSlot` normalisés.

### Ownership exclusif

Après chaque transaction structurante, le test appelle :

```text
ValidateInventoryOwnership()
```

Le contrat attendu reste : une même identité runtime ne peut avoir qu'un propriétaire autoritaire à la fois.

## Cas volontairement non exécuté

`TryDropCursorItem()` est actuellement une API réfléchie mais son chemin de production retourne `false` avec un warning `NotImplemented`.

TD06.4 verrouille son existence dans l'API mais ne l'appelle pas dans l'Automation, afin de préserver un résultat cible sans warning. Cette caractérisation n'entérine pas le comportement fonctionnel futur de drop au sol.

## Validation requise

Après récupération du commit TD06.4 :

```powershell
.\Scripts\ValidateUE.ps1 -AutomationFilter "Grimrock.TechnicalDebt.TD06_4"
```

Résultat attendu :

```text
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

TD06.4 n'est **VALIDÉ** qu'après exécution réelle sous UE5.5.4.

## Cible TD06.5

Après validation verte, la cible proposée est :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentCursorTransfer.cpp
```

Périmètre candidat :

```text
SetCursorItem
ClearCursorItem
HasCursorItem
GetCursorItem
TryTakeInventorySlotToCursor
TryTakeInventorySlotQuantityToCursor
TryPlaceCursorItemInCharacterInventorySlot
TryMoveCharacterInventorySlot
TryPlaceCursorItemInCharacterInventory
TryPlaceCursorItemInSelectedCharacterInventory
TryClearCursorToSelectedCharacterInventory
TryDropCursorItem
CanEquipCursorItemToCharacterSlot
TryEquipCursorItemToCharacterSlot
TryEquipCursorItemToSelectedCharacterSlot
TryTakeEquipmentSlotToCursor
TryTakeSelectedCharacterEquipmentSlotToCursor
```

Contraintes TD06.5 :

- aucun changement du header public sauf nécessité démontrée ;
- aucune nouvelle autorité ;
- mêmes valeurs de retour ;
- mêmes notifications et recalculs de poids ;
- mêmes identités runtime avant/après transaction ;
- helpers locaux préfixés `GridPartyInventoryCursorTransfer...` ;
- validation TD06.4 identique avant et après extraction ;
- revalidation d'au moins `Grimrock.CharacterCreation.CC0` pour couvrir l'ancien contrat d'ownership exclusif.

## Stop condition locale

Si l'extraction TD06.5 impose de dupliquer des règles Equipment ou Ownership au lieu de réutiliser les méthodes de la même classe, le périmètre doit être réduit plutôt qu'élargi.
