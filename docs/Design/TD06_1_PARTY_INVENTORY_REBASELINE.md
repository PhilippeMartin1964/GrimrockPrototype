# TD06.1 — PartyInventory re-baseline et audit documentaire

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Baseline GitHub auditée : `51f9e300cfcc1039412bc8951ac7d64cdece73f0`  
Statut : **RÉALISÉ — TD06.2 PROCHAIN**

## Objet

TD05 a correctement atteint sa stop condition locale pour `AGridLevelRuntimeActor`, mais cette clôture a été interprétée trop largement dans le registre de dette. `TD-ARCH-002 — UGridPartyInventoryComponent` restait classé « surveillée / aucune tranche immédiate » malgré une concentration importante de responsabilités dans son fichier principal.

TD06.1 réouvre donc la campagne globale de réduction de dette, sans rouvrir TD05. Son but est de :

1. re-baseliner `UGridPartyInventoryComponent` ;
2. inventorier les responsabilités encore concentrées ;
3. vérifier ce que TD02.2/TD02.3 ont réellement extrait ;
4. repérer les résidus laissés par ces extractions ;
5. choisir une première frontière caractérisable ;
6. corriger la décision autoritaire du registre avant toute nouvelle modification C++.

Aucun changement de comportement, d’API Blueprint, de SaveGame ou d’asset n’est effectué dans TD06.1.

---

## 1. Re-baseline

À la baseline auditée :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponent.cpp
    2 337 lignes

Source/GrimrockPrototype/Public/Runtime/GridPartyInventoryComponent.h
      293 lignes

Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentDiagnostics.cpp
      465 lignes

Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentWorldTransfer.cpp
     ~104 lignes
```

La taille n’est pas utilisée comme seuil automatique de dette. Le signal pertinent est que le fichier principal contient plusieurs contrats transactionnels distincts qui peuvent être caractérisés indépendamment tout en gardant une autorité d’état unique.

---

## 2. Autorité à préserver

L’invariant architectural reste :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState
       -> ActiveCharacters
       -> ActiveEquipment
       -> CharacterPool
       -> CursorItem
       -> SelectedCharacterIndex
```

TD06 **ne doit pas** créer :

- un `UGridHotbarComponent` propriétaire de sa copie de raccourcis ;
- un `UGridEquipmentComponent` propriétaire d’une copie d’équipement ;
- un second registre d’items ;
- une duplication de `SelectedCharacterIndex` ;
- un état parallèle pour le curseur.

La stratégie autorisée est celle déjà utilisée par TD02.2/TD02.3 et TD05 : plusieurs unités `.cpp` implémentent les méthodes de **la même classe**.

---

## 3. Extractions déjà réalisées

### TD02.2 — Equipment World Transfer

Commit : `f2f63c37cda142daa3bcde01247d31add57f8570`

Deux transactions ont été déplacées dans :

```text
GridPartyInventoryComponentWorldTransfer.cpp
```

- `TryExtractOneEquippedItemForWorldTransfer()` ;
- `TryRestoreExtractedItemToEquipment()`.

Le déplacement a conservé l’état et l’API dans `UGridPartyInventoryComponent`.

### TD02.3 — Inventory Diagnostics

Commit : `7a9a97242536096882c6a646cd9adc37a6d8eb5c`

La présentation/log des diagnostics a été déplacée dans :

```text
GridPartyInventoryComponentDiagnostics.cpp
```

Cette unité possède maintenant ses propres helpers Unity-safe préfixés `GridPartyInventoryDiagnostics...`.

Ces deux extractions démontrent qu’un split par unité d’implémentation fonctionne sans fragmenter l’autorité.

---

## 4. Cartographie des responsabilités restantes

Le fichier principal conserve les blocs suivants.

### A. Party lifecycle / restore / création / sélection

Fonctions représentatives :

```text
InitializeDefaultPartyIfNeeded
ResetPartyForNewGame
RestorePartyInventoryState
CreateInitialCharacter
SetSelectedCharacterIndex
GetCharacterSummary
```

Ce bloc touche fortement l’autorité et la persistance. Il **ne constitue pas la première extraction recommandée**.

### B. Combat Hotbar

```text
GetCombatHotbarSlotCount
GetCharacterCombatHotbarBinding
SetCharacterCombatHotbarBinding
ClearCharacterCombatHotbarBinding
SetCharacterCombatHotbarBindingFromItem
MoveOrSwapCharacterCombatHotbarBinding
InitializeCombatHotbarDefaults
ValidateCombatHotbar
```

Helpers associés : sanitation, unicité des sources Equipment/QuickItem, présence de l’item source.

Frontière nette et déjà testée : **premier candidat TD06**.

### C. Inventory core

```text
CanAddItemToCharacterInventory
AddItemToCharacterInventory
RemoveItemFromCharacterInventoryByRuntimeId
RemoveFirstItemFromCharacterInventoryByDefinitionId
CountItemDefinitionInCharacterInventory
RemoveItemDefinitionFromCharacterInventory
```

Responsabilité transactionnelle importante ; à réauditer après Hotbar/Cursor/Equipment.

### D. Item Definition registry / rehydration

```text
RegisterItemDefinition
RehydrateOwnedItemDefinitions
FindItemDefinition
ApplyItemDefinitionToInstance
```

Frontière potentielle, mais elle est transversale à Inventory, Equipment et Hotbar. Elle est donc différée jusqu’à TD06.8.

### E. Equipment core

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

Candidat cohérent après la frontière Cursor.

### F. Cursor transfer

```text
SetCursorItem
ClearCursorItem
HasCursorItem
TryTakeInventorySlotToCursor
TryTakeInventorySlotQuantityToCursor
TryPlaceCursorItemInCharacterInventorySlot
TryMoveCharacterInventorySlot
TryPlaceCursorItemInCharacterInventory
TryEquipCursorItemToCharacterSlot
```

Cette zone réalise les transactions Inventory <-> Cursor <-> Equipment et constitue une seconde frontière forte après Hotbar.

### G. Weight / ownership / defaults

```text
RecalculateCharacterWeight
RecalculateAllWeights
ValidateInventoryOwnership
EnsureEquipmentCountMatchesActiveCharacters
InitializeCharacterDefaults
CalculateEquipmentWeight
```

Ces fonctions sont très proches des invariants centraux. Elles restent dans le cœur tant qu’une extraction n’apporte pas de bénéfice démontré.

---

## 5. Résidu TD02.3

L’audit du haut de `GridPartyInventoryComponent.cpp` montre encore des helpers dont le nom et la responsabilité appartiennent à l’ancien bloc Diagnostics, alors que `GridPartyInventoryComponentDiagnostics.cpp` possède maintenant les équivalents dédiés.

Exemples observés :

```text
GridInventoryCompatibilityDiagnosticsIsHandSlot
GridInventoryCompatibilityDiagnosticsIsExcludedPaperDollSlot
GridInventoryCompatibilityDiagnosticsIsNewPaperDollSlot
GridInventoryCompatibilityDiagnosticsLooksPotentiallyEquippable
GetItemTypeName
GetEquipmentSlotsText
GetEquipmentStatBonusText
GetDamageResistanceSetText
```

La recherche de symboles confirme notamment que les helpers `GridInventoryCompatibilityDiagnostics...` restent localisés dans le fichier principal, tandis que l’unité Diagnostics utilise les versions `GridPartyInventoryDiagnostics...`.

**Décision TD06.1 : ne pas supprimer ces helpers dans le commit d’audit.** La règle autoritaire impose de caractériser avant un changement de production. Leur suppression sera intégrée à une tranche C++ caractérisée, après confirmation finale de l’absence d’usage.

---

## 6. Première frontière : Hotbar

Hotbar est retenu avant Cursor/Equipment pour quatre raisons :

1. frontière métier nette ;
2. pas de changement d’état propriétaire ;
3. API publique déjà stable ;
4. couverture Automation existante issue de MON12.

Tests existants repérés :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON128HotbarTests.cpp
Source/GrimrockPrototype/Private/Tests/GridMonsterMON12CombatHudTests.cpp
```

Ils utilisent déjà les contrats de binding d’item, dont `SetCharacterCombatHotbarBindingFromItem()`.

### Contrats à figer dans TD06.2

TD06.2 doit couvrir explicitement au minimum :

```text
SlotCount == 10
Default bindings empty and indexed 0..9
Invalid character / slot -> false, no mutation
Equipment source -> unique RuntimeObjectId binding
QuickItem source -> unique ItemDefinition binding
Move to empty slot
Swap occupied slots
Clear binding without moving/consuming source item
Remove last QuickItem source -> binding sanitized/cleared
Restore -> missing/legacy hotbar initialized and invalid duplicates sanitized
ValidateCombatHotbar rejects malformed indices / duplicate sources
```

La caractérisation doit tester l’état observable, pas la localisation physique du code.

---

## 7. Cible TD06.3

Après baseline TD06.2 verte :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentHotbar.cpp
```

Contraintes :

- aucun nouvel état ;
- aucun nouveau composant propriétaire ;
- aucune modification de `FGridPartyInventoryState` ;
- aucun changement d’API Blueprint ;
- header public inchangé sauf nécessité démontrée ;
- helpers locaux préfixés `GridPartyInventoryHotbar...` ;
- mêmes valeurs de retour et mêmes notifications avant/après ;
- validation UE réelle après extraction.

---

## 8. Séquence TD06 retenue

```text
TD06.1   PartyInventory re-baseline / documentation audit      RÉALISÉ
TD06.2   Hotbar characterization                                PROCHAIN
TD06.3   Hotbar extraction                                      À FAIRE
TD06.4   Cursor Transfer characterization                       À FAIRE
TD06.5   Cursor Transfer extraction                             À FAIRE
TD06.6   Equipment Core characterization                        À FAIRE
TD06.7   Equipment Core extraction                              À FAIRE
TD06.8   Item Definition Registry / Rehydration audit           À FAIRE
TD06.9   final re-audit / stop condition                        À FAIRE
```

La séquence après TD06.3 reste révisable : chaque re-audit peut arrêter ou réordonner les extractions si la douleur réelle change.

---

## 9. Audit documentaire

Le registre autoritaire était incohérent avec le code sur un point majeur : `TD-ARCH-002` était marqué « surveillée / aucune tranche immédiate » sans re-baseline détaillée après TD02.2/TD02.3. TD06.1 corrige cette décision.

Plusieurs documents CURRENT/FOUNDATION restent également datés du début de TD05 et décrivent encore `AGridLevelRuntimeActor` à 3 359 lignes avec Diagnostics comme « prochaine frontière », alors que TD05.9 est clos. Les principaux documents concernés sont :

```text
docs/Architecture/ARCHITECTURE_INDEX.md
docs/Architecture/PROJECT_SYNTHESIS.md
docs/Architecture/COMBAT_MONSTER_AI_FOUNDATION.md
docs/Architecture/UI_GAME_FLOW_FOUNDATION.md
docs/Design/UI_ARCHITECTURE_CURRENT.md
```

Ils doivent être rafraîchis sur la **baseline fonctionnelle réellement publiée** afin de ne pas écraser ou anticiper des jalons MON21 présents localement mais absents du `master` GitHub au moment de TD06.1.

Le document `TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md` reste volontairement inchangé : il s’agit du snapshot historique TD05.1.

---

## 10. Validation TD06.1

TD06.1 est un audit documentaire et statique :

```text
Production C++ modifiée : NON
Header public modifié   : NON
Blueprint/assets modifiés: NON
SaveGame modifié        : NON
```

Aucune exécution Automation supplémentaire n’est requise pour prouver une absence de changement runtime.

La première validation UE obligatoire de TD06 intervient avec TD06.2 (nouvelle caractérisation Automation), puis TD06.3 (extraction de production).

---

## Conclusion

`TD-ARCH-002` ne doit plus rester une dette seulement « surveillée ». La concentration actuelle justifie une campagne ciblée, mais la stratégie reste conservatrice : caractériser puis déplacer des responsabilités cohérentes de **la même classe**, sans fragmenter `FGridPartyInventoryState`.

**Prochaine étape : TD06.2 — PartyInventory Hotbar characterization.**
