# TD06.2 — PartyInventory Hotbar characterization

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.1 — PartyInventory re-baseline**  
Statut : **PRÊT À VALIDER LOCALEMENT**

## Objet

TD06.2 caractérise la frontière `Combat Hotbar 0–9` de `UGridPartyInventoryComponent` avant toute extraction de code de production.

Le but n'est pas de réécrire les tests MON12. La hotbar possède déjà une couverture fonctionnelle importante dans `GridMonsterMON128HotbarTests.cpp` et `GridMonsterMON12CombatHudTests.cpp`. TD06.2 ajoute un contrat transversal minimal et durable qui verrouille uniquement les comportements dont dépend TD06.3.

## Règle d'architecture

`UGridPartyInventoryComponent` reste l'unique façade et l'unique autorité de `FGridPartyInventoryState`.

TD06.3 pourra déplacer l'implémentation de la hotbar vers :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentHotbar.cpp
```

mais ne doit créer :

- aucun nouveau composant propriétaire de la hotbar ;
- aucun second stockage des raccourcis ;
- aucune API parallèle ;
- aucune dépendance UI dans l'autorité runtime.

## API publique caractérisée

Le test TD06.2 verrouille la présence réfléchie et `BlueprintCallable` des fonctions existantes :

```text
GetCombatHotbarSlotCount
GetCharacterCombatHotbarBinding
SetCharacterCombatHotbarBinding
ClearCharacterCombatHotbarBinding
SetCharacterCombatHotbarBindingFromItem
MoveOrSwapCharacterCombatHotbarBinding
```

Le header public ne doit donc pas changer pendant l'extraction TD06.3.

## Invariants caractérisés

### 1. Taille et initialisation

```text
FGridCombatHotbarBinding::SlotCount == 10
CombatHotbarSlots.Num() == 10
SlotIndex == position physique
nouveaux slots == vides + structurellement valides
```

### 2. Validation des indices

Les indices de personnage ou de slot invalides sont refusés sans mutation de la hotbar.

### 3. Normalisation

`SetCharacterCombatHotbarBinding()` impose le `SlotIndex` réel du slot cible. Le caller ne peut pas injecter un index persistant incohérent.

### 4. Isolation par personnage

Chaque `FGridCharacterInventoryState` possède sa propre hotbar. Une affectation sur le personnage A ne modifie pas le personnage B.

### 5. Unicité Equipment

Un même `PreferredSourceRuntimeId` de policy `Equipment` ne peut apparaître qu'une fois. Réaffecter la même source vers un autre slot déplace implicitement son identité en vidant l'ancien raccourci.

### 6. Unicité QuickItem

Une même `SourceDefinitionId` de policy `QuickItem` ne peut apparaître qu'une fois par personnage. Le raccourci est lié au type de consommable et non à l'identité runtime d'une pile.

### 7. Le raccourci ne possède pas l'item

Créer, déplacer ou effacer un raccourci ne consomme ni ne transfère l'item source. L'inventaire reste autoritaire sur les quantités.

### 8. Move / Swap atomique

```text
occupé -> occupé = swap
occupé -> vide    = move
```

Les deux bindings sont réindexés vers leur nouvelle position.

### 9. Sanitation au restore

Lors d'un restore :

- le premier binding Equipment dupliqué est conservé ;
- les duplicatas suivants sont effacés ;
- un QuickItem dont la définition n'est plus présente dans l'inventaire est effacé ;
- l'état restauré doit ensuite satisfaire `ValidateCombatHotbar()`.

### 10. Rejet atomique d'un état structurellement invalide

Une hotbar de taille différente de 10 est rejetée. Le composant conserve son état autoritaire précédent.

## Couverture MON12 réutilisée

La caractérisation TD06.2 complète, sans la remplacer, la couverture existante comprenant notamment :

```text
Grimrock.Monsters.MON12.8.1.DefaultHotbarIsEmpty
Grimrock.Monsters.MON12.8.1.PerCharacterBindings
Grimrock.Monsters.MON12.8.1.SaveMemoryRoundTrip
Grimrock.Monsters.MON12.8.1.LegacySaveGetsEmptyHotbar
Grimrock.Monsters.MON12.8.1.RejectInvalidHotbarAtomically
Grimrock.Monsters.MON12.8.2.MoveOrSwapBindings
Grimrock.Monsters.MON12.8.2.InventoryQuickItemBinding
Grimrock.Monsters.MON12.8.9.RepeatedEquipmentDropMovesBinding
Grimrock.Monsters.MON12.8.9.ConsumedQuickItemClearsBinding
Grimrock.Monsters.MON12.8.9.LegacyBindingsAreSanitized
```

## Nouveau test

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridTD062PartyInventoryHotbarTests.cpp
```

Filtre :

```text
Grimrock.TechnicalDebt.TD06_2.PartyInventoryHotbar.Contract
```

Le test ne dépend que de l'API publique du composant. Il n'appelle aucun helper privé de la hotbar. C'est intentionnel : le même test doit rester inchangé après TD06.3.

## Validation locale demandée

Depuis la racine du projet :

```powershell
.\Scripts\ValidateUE.ps1 -AutomationFilter "Grimrock.TechnicalDebt.TD06_2"
```

Puis, sans reconstruire :

```powershell
.\Scripts\ValidateUE.ps1 -SkipBuild -AutomationFilter "Grimrock.Monsters.MON12.8"
```

Critères TD06.2 :

```text
TD06_2 contract       : 1 Success / 0 Failed / 0 warning
MON12.8 regression    : 0 Failed
Editor build          : Success
```

Les warnings historiques éventuels d'autres suites ne doivent pas être confondus avec une régression TD06.2.

## Hors périmètre TD06.2

Aucun code de production n'est déplacé ici.

Ne pas traiter dans ce sous-jalon :

- Cursor Transfer ;
- Equipment Core ;
- Item Definition Registry / Rehydration ;
- diagnostics TD02.3 résiduels ;
- renommage UI ;
- changement de modèle de SaveGame.

## Étape suivante

Après validation locale verte :

**TD06.3 — extraire l'implémentation Hotbar vers `GridPartyInventoryComponentHotbar.cpp`, sans changer le header public ni le contrat TD06.2.**
