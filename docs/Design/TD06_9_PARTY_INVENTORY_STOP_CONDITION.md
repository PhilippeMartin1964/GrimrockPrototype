# TD06.9 — PartyInventory final re-audit / cleanup / stop condition

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.8 — Item Definition Registry / Rehydration audit**  
Baseline GitHub : `0c4b8050bd5fc55bda50364a7b9953b6a44394b3`  
Statut : **IMPLÉMENTÉ — VALIDATION UE REQUISE**

## Objet

TD06.9 clôt le re-audit de `UGridPartyInventoryComponent` après les extractions Hotbar, Cursor et Equipment et après la décision TD06.8 de conserver Registry/Rehydration dans le cœur.

L'objectif n'est plus de réduire la taille du fichier pour elle-même. Il est de supprimer le code mort prouvé, de remettre la dernière méthode Diagnostics dans son unité dédiée, puis de vérifier si une extraction supplémentaire supprimerait encore un risque réel.

## Nettoyage de production

Les huit helpers historiques suivants étaient présents uniquement à leur propre définition après TD06.7/TD06.8 :

```text
GetItemTypeName
GetEquipmentSlotsText
GridInventoryCompatibilityDiagnosticsIsHandSlot
GridInventoryCompatibilityDiagnosticsIsExcludedPaperDollSlot
GridInventoryCompatibilityDiagnosticsIsNewPaperDollSlot
GridInventoryCompatibilityDiagnosticsLooksPotentiallyEquippable
GetEquipmentStatBonusText
GetDamageResistanceSetText
```

TD06.9 les supprime. Il s'agit de **108 lignes de code mort confirmé** dans le fichier principal.

## Dernière frontière Diagnostics

`GetEquipmentDiagnosticsForCharacter()` était encore implémentée dans `GridPartyInventoryComponent.cpp` alors que la responsabilité Diagnostics a été extraite par TD02.3.

TD06.9 déplace uniquement son implémentation vers :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentDiagnostics.cpp
```

Le header et la `UFUNCTION` restent inchangés. Le comportement est déjà caractérisé par :

```text
Grimrock.TechnicalDebt.TD02_3.PartyInventoryDiagnosticsContract
```

Le fichier Diagnostics réutilise ses helpers Unity-safe existants :

```text
GridPartyInventoryDiagnosticsGetEquipmentSlotName
GridPartyInventoryDiagnosticsForEachEquipmentItem
```

## Mesures finales TD06

```text
TD06.1 baseline principale          2 337 lignes
après TD06.3 Hotbar                ~2 117 lignes
après TD06.5 Cursor                 1 677 lignes
après TD06.7 Equipment              1 357 lignes
après TD06.9 cleanup/Diagnostics    1 228 lignes
```

Réduction du fichier principal pendant TD06 : **1 109 lignes**, soit environ **47,5 %**.

Distribution après TD06.9 :

```text
GridPartyInventoryComponent.cpp                 1 228 lignes
GridPartyInventoryComponentHotbar.cpp             237 lignes
GridPartyInventoryComponentCursorTransfer.cpp     513 lignes
GridPartyInventoryComponentEquipment.cpp          397 lignes
GridPartyInventoryComponentWorldTransfer.cpp      105 lignes
GridPartyInventoryComponentDiagnostics.cpp        488 lignes
GridPartyInventoryComponent.h                     294 lignes
```

Ces fichiers restent tous l'implémentation de la **même classe**. Aucun second propriétaire d'état n'a été créé.

## Cœur restant

Le fichier principal conserve maintenant essentiellement :

```text
Party lifecycle / reset / restore
character creation / selection / summary
inventory add / stack / remove / count / consume
Item Definition registry / rehydration / instance application
weight recalculation
ownership validation
default initialization
hotbar restore / sanitation / validation
equipment-count synchronization
```

Ces responsabilités sont fortement transversales à l'autorité `FGridPartyInventoryState`. Les séparer davantage aujourd'hui déplacerait surtout des appels entre `.cpp` sans isoler une nouvelle transaction autonome ni réduire un risque observé.

## Décision Registry / Rehydration

TD06.8 a caractérisé le registre transient et son rehydrate : atomicité, sources possédées, déduplication et exclusion des bindings Spell/Ability sont protégées.

**Décision : conserver Registry/Rehydration dans le fichier principal.**

Son extraction resterait techniquement possible mais n'enlèverait ni duplication d'autorité, ni dépendance problématique, ni contrat insuffisamment testable.

## Stop condition TD06

Après ce nettoyage, les frontières à forte cohésion qui justifiaient TD06 disposent toutes d'une unité dédiée :

- Hotbar ;
- Cursor Transfer ;
- Equipment Core ;
- Equipment World Transfer ;
- Diagnostics.

Le cœur restant est cohérent avec le rôle d'autorité/orchestrateur de `UGridPartyInventoryComponent`.

**Décision TD06.9 : arrêter ici la décomposition de `UGridPartyInventoryComponent`, sous réserve de la validation UE post-changement.**

Réouvrir uniquement en présence d'un signal concret :

- nouvelle responsabilité métier importante ;
- duplication d'état ou d'autorité ;
- régression récurrente sur une zone précise ;
- difficulté de test réelle ;
- dépendance de compilation problématique ;
- bloc de logique autonome dont l'extraction réduit clairement le risque.

Aucun objectif arbitraire de LOC ne justifie une TD06.10.

## Invariants préservés

TD06.9 ne modifie pas :

- `GridPartyInventoryComponent.h` ;
- `FGridPartyInventoryState` ;
- les `UFUNCTION` / Blueprint APIs ;
- SaveGame / migration ;
- les règles Inventory/Hotbar/Cursor/Equipment ;
- les `RuntimeObjectId` ;
- l'ownership ;
- Registry/Rehydration.

## Validation post-TD06.9 requise

Depuis la racine du projet :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_8"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TechnicalDebt.TD02_3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.CharacterCreation.CC5"
```

Critères :

```text
TD06_8 : 1 Success / 0 warning / 0 Failed
TD02_3 : 1 Success / 0 warning / 0 Failed
CC5    : 2 Success / 0 warning / 0 Failed
```

Après validation verte, TD06.9 devient **VALIDÉ**, `TD-ARCH-002` passe en **surveillance / stop condition atteinte**, et les documents d'architecture courants peuvent être rafraîchis avant retour à la roadmap fonctionnelle.
