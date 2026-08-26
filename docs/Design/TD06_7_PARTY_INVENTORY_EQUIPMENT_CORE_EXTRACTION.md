# TD06.7 — PartyInventory Equipment Core extraction

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.6 — PartyInventory Equipment Core characterization**  
Baseline GitHub : `c3142eefa07d62e24523ea42b9a6f495b0b999ad`  
Statut : **IMPLÉMENTÉ — VALIDATION UE REQUISE**

## Objet

TD06.7 extrait la frontière **Equipment Core** de `UGridPartyInventoryComponent` vers :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentEquipment.cpp
```

Il s'agit uniquement d'une répartition de l'implémentation de la même classe. `UGridPartyInventoryComponent` et `FGridPartyInventoryState` restent l'unique façade et l'unique autorité d'état.

## Méthodes déplacées

```text
CanEquipItemToSlot
EquipItemFromInventorySlot
UnequipItemToInventory
GetEquippedItem
IsEquipmentSlotOccupied
TryConsumeEquippedItemQuantityForCombatAction
ComputeCharacterEquipmentStatBonus
ComputeCharacterEquipmentResistances
```

Soit **8 méthodes** caractérisées par TD06.6.

## Helpers locaux Unity-safe

Le nouveau fichier possède ses helpers locaux préfixés :

```text
GridPartyInventoryEquipmentIsSupportedSlot
GridPartyInventoryEquipmentIsHandSlot
GridPartyInventoryEquipmentGetSlotName
GridPartyInventoryEquipmentFindFreeInventorySlotIndex
GridPartyInventoryEquipmentAddStatBonus
GridPartyInventoryEquipmentForEachItem
```

Les helpers historiques suivants deviennent réellement sans référence dans le fichier principal et sont supprimés dans la même tranche :

```text
IsSupportedEquipmentSlot
IsHandEquipmentSlot
FindFreeInventorySlotIndex
AddEquipmentStatBonus
```

En revanche, `GetEquipmentSlotName` et `ForEachEquipmentItem` restent volontairement dans le fichier principal, car ils servent encore au registry/rehydration, aux diagnostics, au poids et à la validation d'ownership.

## Mesure

```text
GridPartyInventoryComponent.cpp
    avant TD06.7 : 1 677 lignes
    après TD06.7 : 1 357 lignes

GridPartyInventoryComponentEquipment.cpp
    nouveau       : 397 lignes
```

La réduction de **320 lignes** du cœur n'est pas un objectif arbitraire : elle correspond à une frontière métier caractérisée et aux helpers devenus morts après son déplacement.

## Invariants préservés

TD06.7 ne modifie pas :

- `GridPartyInventoryComponent.h` ;
- les `UFUNCTION` / Blueprint APIs ;
- `FGridPartyInventoryState` ;
- le SaveGame ;
- la compatibilité `CompatibleEquipmentSlots` ;
- le fallback historique de compatibilité sans Item Definition ;
- les swaps atomiques Inventory <-> Equipment ;
- les `RuntimeObjectId` ;
- l'ownership ;
- les recalculs de poids / notifications ;
- la consommation combat par définition + runtime ID ;
- l'agrégation Stats / Resistances.

Les transactions Cursor et Equipment World Transfer restent dans leurs fichiers dédiés existants.

## Baseline avant extraction

Validation réelle fournie le 26 août 2026 :

```text
Grimrock.TechnicalDebt.TD06_6
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0

Grimrock.TechnicalDebt.TD06_4
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## Validation post-extraction requise

Depuis la racine du projet :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_6"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TechnicalDebt.TD06_4"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.CharacterCreation.CC5"
```

Critères :

```text
TD06_6 : 1 Success / 0 warning / 0 Failed
TD06_4 : 1 Success / 0 warning / 0 Failed
CC5    : 2 Success / 0 Failed
```

TD06.7 ne devient **VALIDÉ** qu'après ces exécutions réelles sous UE5.5.4.

## Suite

Si la validation reste verte :

```text
TD06.8 — Item Definition Registry / Rehydration audit
```
