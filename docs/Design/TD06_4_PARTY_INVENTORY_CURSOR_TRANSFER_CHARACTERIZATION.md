# TD06.4 — PartyInventory Cursor Transfer characterization

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.3 — PartyInventory Hotbar extraction**  
Baseline GitHub : `324e88195aa34966a2d47f49f020c93afef8e3da`  
Statut : **VALIDÉ**

## Objet

TD06.4 caractérise la frontière transactionnelle `Inventory <-> Cursor <-> Equipment` de `UGridPartyInventoryComponent` avant toute extraction de code de production.

`UGridPartyInventoryComponent` reste l'unique façade et l'unique autorité de `FGridPartyInventoryState`.

## Contrat Automation

```text
Source/GrimrockPrototype/Private/Tests/GridTD064PartyInventoryCursorTransferTests.cpp
Grimrock.TechnicalDebt.TD06_4.PartyInventoryCursorTransfer.Contract
```

Le contrat couvre notamment :

- Inventory -> Cursor, transfert complet et split de pile ;
- conservation ou création contrôlée des `RuntimeObjectId` ;
- Cursor -> Inventory et swap avec un slot occupé ;
- move/swap entre slots d'inventaire ;
- Cursor <-> Equipment ;
- normalisation de l'ownership ;
- `ValidateInventoryOwnership()` après les transactions structurantes ;
- maintien de l'API publique Blueprint existante.

## Validation réelle

Validation locale fournie le **26 août 2026** :

```text
Filter                 : Grimrock.TechnicalDebt.TD06_4
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

Baseline complémentaire avant extraction TD06.5 :

```text
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

## Décision

La caractérisation est verte. La frontière est donc autorisée pour **TD06.5 — extraction Cursor Transfer**, sans modification du header public, du SaveGame, de la Blueprint API ni de l'autorité d'état.
