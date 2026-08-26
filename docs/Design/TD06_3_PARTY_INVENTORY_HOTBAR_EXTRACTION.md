# TD06.3 — PartyInventory Hotbar extraction

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.2 — PartyInventory Hotbar characterization**  
Commit : `9f11fa8da3c61840a62850a85ab2cc5174d7215d`  
Statut : **VALIDÉ**

## Objet

TD06.3 extrait du fichier principal de `UGridPartyInventoryComponent` les opérations publiques de la hotbar caractérisées par TD06.2, sans changer l'API publique, le header, la structure persistante ni l'autorité de l'état.

## Extraction

Nouveau fichier :

```text
Source/GrimrockPrototype/Private/Runtime/GridPartyInventoryComponentHotbar.cpp
```

Opérations déplacées :

```text
GetCombatHotbarSlotCount
GetCharacterCombatHotbarBinding
SetCharacterCombatHotbarBinding
ClearCharacterCombatHotbarBinding
SetCharacterCombatHotbarBindingFromItem
MoveOrSwapCharacterCombatHotbarBinding
```

Le fichier principal conserve volontairement les routines privées de restore/migration :

```text
InitializeCombatHotbarDefaults
ValidateCombatHotbar
SanitizeCombatHotbarBindings
ClearQuickItemHotbarBindings
```

Elles restent liées au lifecycle, à la restauration et à la consommation d'inventaire. Les déplacer dans cette tranche aurait élargi le périmètre au-delà du contrat caractérisé.

## Invariants

- `UGridPartyInventoryComponent` reste l'unique façade et autorité de `FGridPartyInventoryState`.
- Aucun nouveau composant Hotbar n'est créé.
- Aucun champ, `UFUNCTION`, Blueprint binding ou SaveGame n'est modifié.
- Les dix slots restent persistés dans `FGridCharacterInventoryState::CombatHotbarSlots`.
- Les opérations déplacées continuent d'appeler les services existants du même composant.
- Le helper local du nouveau `.cpp` est préfixé `GridPartyInventoryHotbar...` pour rester Unity-safe.

## Validation

Baseline TD06.2 avant extraction :

```text
Grimrock.TechnicalDebt.TD06_2
    1 succeeded
    0 warnings
    0 failed

Grimrock.Monsters.MON12.8
    26 succeeded
    0 warnings
    0 failed
```

Validation réelle post-extraction fournie le 26 août 2026 :

```text
Grimrock.TechnicalDebt.TD06_2
    Succeeded              : 1
    Succeeded with warnings: 0
    Failed                 : 0
    Not run                : 0

Grimrock.Monsters.MON12.8
    Succeeded              : 26
    Succeeded with warnings: 0
    Failed                 : 0
    Not run                : 0
```

La couverture caractérisée est donc identique avant et après extraction. TD06.3 est **VALIDÉ**.

## Suite

La frontière suivante est :

```text
TD06.4 — PartyInventory Cursor Transfer characterization
```
