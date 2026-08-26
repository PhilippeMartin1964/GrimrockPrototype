# TD06.5 — PartyInventory Cursor Transfer extraction

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.4 — PartyInventory Cursor Transfer characterization**  
Baseline GitHub : `324e88195aa34966a2d47f49f020c93afef8e3da`  
Commit : `776827223a584ed7287bba5c029c5c0a224f515f`  
Statut : **VALIDÉ**

## Objet

TD06.5 extrait la frontière transactionnelle Cursor de `UGridPartyInventoryComponent` vers :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentCursorTransfer.cpp
```

L'extraction répartit uniquement l'implémentation de la même classe. `UGridPartyInventoryComponent` et `FGridPartyInventoryState` restent l'unique façade et l'unique autorité d'état du groupe/inventaire.

## Périmètre déplacé

```text
TryTakeEquipmentSlotToCursor
TryTakeSelectedCharacterEquipmentSlotToCursor
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
```

Soit **17 méthodes**.

## Helpers locaux

Le nouveau fichier utilise des helpers locaux préfixés pour rester Unity-safe :

```text
GridPartyInventoryCursorTransferFindFreeInventorySlotIndex
GridPartyInventoryCursorTransferGetEquipmentSlotName
```

Aucune nouvelle autorité ni structure persistée n'est introduite.

## Mesure

Le fichier principal `GridPartyInventoryComponent.cpp` passe d'environ **2 116 lignes** avant TD06.5 à **1 677 lignes** après extraction.

La réduction de taille n'est pas un objectif en soi ; elle matérialise le retrait d'une frontière transactionnelle déjà caractérisée.

## Invariants préservés

TD06.5 ne modifie pas :

- `GridPartyInventoryComponent.h` ;
- les `UFUNCTION` / Blueprint APIs ;
- `FGridPartyInventoryState` ;
- la structure SaveGame ;
- les valeurs de retour ;
- les règles de split de pile ;
- les `RuntimeObjectId` ;
- les règles d'ownership ;
- les recalculs de poids / notifications déclenchés par les transactions ;
- la compatibilité Equipment.

## Baseline avant extraction

Validation réelle fournie le 26 août 2026 :

```text
Grimrock.TechnicalDebt.TD06_4
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0

Grimrock.CharacterCreation.CC0
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
```

## Validation post-extraction réelle

Validation locale fournie le **26 août 2026** après le commit TD06.5 :

```text
Filter                 : Grimrock.TechnicalDebt.TD06_4
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0

Filter                 : Grimrock.CharacterCreation.CC0
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

Commandes de référence :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_4"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.CharacterCreation.CC0"
```

La couverture caractérisée est identique avant et après extraction. TD06.5 est donc **VALIDÉ**.

## Suite

```text
TD06.6 — PartyInventory Equipment Core characterization
```

La caractérisation TD06.6 doit précéder toute extraction de l'Equipment Core et préserver la même autorité unique.
