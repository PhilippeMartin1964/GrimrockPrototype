# TD06.3 — PartyInventory Hotbar extraction

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.2 — PartyInventory Hotbar characterization**  
Statut : **PRÊT À VALIDER LOCALEMENT**

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

## Validation requise

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

Après extraction, relancer :

```powershell
.\Scripts\ValidateUE.ps1 -AutomationFilter "Grimrock.TechnicalDebt.TD06_2"
.\Scripts\ValidateUE.ps1 -SkipBuild -AutomationFilter "Grimrock.Monsters.MON12.8"
```

TD06.3 n'est considéré **VALIDÉ** qu'après reproduction de ces résultats sous UE5.5.4.

## Suite

Après validation, mettre à jour le registre autoritaire puis ouvrir :

```text
TD06.4 — PartyInventory Cursor Transfer characterization
```
