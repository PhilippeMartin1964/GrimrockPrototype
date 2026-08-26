# TD06.6 — PartyInventory Equipment Core characterization

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.5 — PartyInventory Cursor Transfer extraction**  
Baseline GitHub : `776827223a584ed7287bba5c029c5c0a224f515f`  
Statut : **PRÊT À VALIDER LOCALEMENT**

## Objet

TD06.6 caractérise la frontière **Equipment Core** de `UGridPartyInventoryComponent` avant toute extraction de code de production.

Le sous-jalon ne déplace aucune implémentation runtime. Il ajoute uniquement un contrat Automation ciblé et enregistre les invariants que TD06.7 devra conserver.

`UGridPartyInventoryComponent` et `FGridPartyInventoryState` restent l'unique façade et l'unique autorité d'état du groupe/inventaire.

## Périmètre caractérisé

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

Les méthodes Cursor déjà extraites par TD06.5 et les transactions Equipment <-> World déjà isolées dans `GridPartyInventoryComponentWorldTransfer.cpp` restent hors de ce périmètre.

## Nouveau contrat Automation

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridTD066PartyInventoryEquipmentCoreTests.cpp
```

Filtre :

```text
Grimrock.TechnicalDebt.TD06_6.PartyInventoryEquipmentCore.Contract
```

Le test utilise l'API publique du composant, plus l'API C++ existante de consommation combat. Aucun helper privé Equipment n'est appelé.

## Invariants verrouillés

### 1. API publique Blueprint

Les fonctions publiques réfléchies suivantes doivent rester présentes et `BlueprintCallable` :

```text
CanEquipItemToSlot
EquipItemFromInventorySlot
UnequipItemToInventory
GetEquippedItem
IsEquipmentSlotOccupied
ComputeCharacterEquipmentStatBonus
ComputeCharacterEquipmentResistances
```

TD06.7 ne doit pas nécessiter de modification du header public pour cette extraction.

### 2. Compatibilité par Item Definition

Lorsqu'une `UGridItemDefinitionAsset` est enregistrée, `CanEquipItemToSlot()` respecte `CompatibleEquipmentSlots`.

Un slot déclaré compatible est accepté ; un slot non déclaré est refusé.

### 3. Fallback historique sans Item Definition

Le comportement existant est explicitement caractérisé :

```text
item runtime valide + définition absente + slot supporté -> true
item runtime valide + définition absente + None          -> false
```

Ce fallback n'est pas déclaré souhaitable à long terme ; TD06.6 le protège uniquement contre une modification accidentelle pendant l'extraction structurelle TD06.7. Toute suppression future doit être un changement fonctionnel séparé et explicite.

### 4. Equip vers slot vide

`EquipItemFromInventorySlot()` :

- retire l'item du slot d'inventaire source ;
- conserve son `RuntimeObjectId` ;
- normalise `OwnerType`, `OwnerGuid`, `OwnerCharacterIndex` et `EquipmentSlot` ;
- rend le slot Equipment occupé ;
- conserve l'ownership exclusif.

### 5. Rejet incompatible atomique

Une tentative d'équipement vers un slot incompatible :

- retourne `false` ;
- laisse le slot inventaire source inchangé ;
- laisse l'équipement déjà présent inchangé.

### 6. Swap atomique Inventory -> Equipment occupé

Équiper un nouvel item sur un slot déjà occupé échange les deux objets atomiquement :

```text
nouvel item -> Equipment
ancien item -> slot inventaire source du nouvel item
```

Les deux identités runtime sont conservées et les owners sont normalisés.

### 7. Unequip vers le premier slot libre

`UnequipItemToInventory()` :

- déplace l'item vers le premier slot d'inventaire libre ;
- conserve son identité runtime ;
- normalise l'ownership vers `CharacterInventory` ;
- vide le slot Equipment.

### 8. Inventaire plein

Si aucun slot d'inventaire n'est libre, `UnequipItemToInventory()` échoue sans mutation : l'item reste équipé avec la même identité runtime.

### 9. Lecture Equipment

`GetEquippedItem()` et `IsEquipmentSlotOccupied()` restent cohérents avant/après les transactions et refusent les index de personnage invalides.

### 10. Consommation combat par identité

`TryConsumeEquippedItemQuantityForCombatAction()` :

- n'accepte que MainHand/OffHand ;
- vérifie `ItemDefinitionId` ;
- vérifie le `RuntimeObjectId` lorsqu'il est fourni ;
- refuse quantité nulle, négative ou supérieure à la pile ;
- décrémente exactement une consommation partielle sans changer l'identité runtime ;
- vide le slot lorsque toute la quantité restante est consommée.

### 11. Agrégation Stats / Resistances

`ComputeCharacterEquipmentStatBonus()` et `ComputeCharacterEquipmentResistances()` somment les bonus des définitions actuellement équipées.

Le contrat vérifie aussi qu'un swap d'équipement modifie immédiatement l'agrégat : les bonus de l'ancien item disparaissent et ceux du nouvel item apparaissent.

### 12. Ownership unique

`ValidateInventoryOwnership()` doit rester vert après :

- equip ;
- swap ;
- unequip ;
- consommation combat ;
- échec d'une unequip avec inventaire plein.

## Hors périmètre TD06.6

Aucun code de production n'est déplacé dans ce sous-jalon.

Ne pas traiter ici :

- Cursor Transfer ;
- Equipment World Transfer ;
- Item Definition Registry / Rehydration ;
- Inventory add/remove générique ;
- lifecycle / restore ;
- diagnostics historiques TD02.3 ;
- SaveGame ;
- Blueprint assets.

## Cible TD06.7

Après validation réelle du contrat, l'implémentation Equipment Core pourra être déplacée vers un `.cpp` dédié de la même classe, recommandé :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentEquipment.cpp
```

Aucun nouveau composant propriétaire et aucune copie de `FGridPartyInventoryState` ne doivent être créés.

Les helpers nécessaires au nouveau `.cpp` devront être locaux et préfixés `GridPartyInventoryEquipment...` afin de rester Unity-safe.

## Validation locale demandée

Depuis la racine du projet :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_6"
```

Puis, sans reconstruire, régression Cursor/Inventory immédiate :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TechnicalDebt.TD06_4"
```

Critères :

```text
TD06_6 contract : 1 Success / 0 Failed / 0 warning
TD06_4 contract : 1 Success / 0 Failed / 0 warning
Editor build    : Success
```

TD06.6 ne devient **VALIDÉ** qu'après exécution réelle sous UE5.5.4.

## Étape suivante

Après validation verte :

**TD06.7 — extraire l'Equipment Core vers `GridPartyInventoryComponentEquipment.cpp`, sans changer le header public ni le contrat TD06.6.**
