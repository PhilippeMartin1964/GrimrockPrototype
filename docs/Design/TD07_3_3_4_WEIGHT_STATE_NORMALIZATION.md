# TD07.3.3.4 — Normalize Weight State

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline de caractérisation validée : `48c3f8e230c7bd5f0ef73fac45cf4e58d685f112`  
Statut : **VALIDÉ — STOP CONDITION ATTEINTE**

## 1. Objet

TD07.3.3.4 supprime les deux caches de poids du personnage durable :

```text
CurrentWeight
MaxCarryWeight
```

et retire également le helper `FGridCharacterInventoryState::IsOverloaded()`.

La charge devient une projection pure de l'état autoritaire inventaire / équipement / attributs.

## 2. Modèle cible

`FGridCharacterInventoryState` ne porte plus aucune donnée de poids.

La projection est centralisée dans :

```cpp
UGridPartyInventoryComponent::GetCharacterSummary()
```

avec :

```text
CurrentWeight
    = somme InventorySlots
    + somme ActiveEquipment

BaseMaxWeight
    = CalculateMaxCarryWeight(Character.Attributes)

MaxWeight
    = max(0, BaseMaxWeight + EquipmentStatBonus.CarryWeightBonus)

bOverloaded
    = CurrentWeight > MaxWeight
```

## 3. Politique d'équipement conservée

La caractérisation a montré :

```text
Equipment StrengthBonus
    -> modifie Summary.Attributes.Strength
    -> ne modifie pas BaseMaxWeight

Equipment CarryWeightBonus
    -> modifie MaxWeight
```

TD07.3.3.4 conserve volontairement cette règle.

Exemple :

```text
base Strength          10
base capacity          50
equipment STR bonus    +4
equipment carry bonus  +7

projected Strength     14
BaseMaxWeight          50
MaxWeight              57
```

Il n'y a pas de recalcul caché vers 77.

## 4. Suppression des API de recalcul

Les API :

```text
RecalculateCharacterWeight
RecalculateAllWeights
```

sont supprimées.

Elles existaient pour écrire les caches retirés.

Les transactions qui mutent inventaire ou équipement appellent désormais uniquement :

```cpp
NotifyPartyInventoryChanged(CharacterIndex)
```

ou la notification globale appropriée.

L'UI relit ensuite `GetCharacterSummary()`, qui reconstruit la charge courante.

## 5. Restore

`RestorePartyInventoryState()` n'a plus besoin de réécrire des caches après restauration.

Le snapshot contient seulement les sources de vérité :

```text
Attributes
InventorySlots
ActiveEquipment
```

Le premier `GetCharacterSummary()` reconstruit directement les poids.

## 6. Recrutement et création

Story Companion, Custom Recruit et Party Recruitment ne fabriquent plus de valeurs de poids persistées.

Le poids d'un personnage recruté est disponible immédiatement via son résumé, sans étape de normalisation secondaire.

## 7. Diagnostics et UI

Les diagnostics de groupe utilisent maintenant `GetCharacterSummary()`.

Les surfaces UI continuaient déjà de consommer :

```text
Summary.CurrentWeight
Summary.MaxWeight
Summary.bOverloaded
```

Le contrat UI reste donc stable malgré la suppression de l'état durable.

## 8. SaveGame v13

La suppression de deux propriétés sérialisées modifie le schéma de `FGridCharacterInventoryState`.

```text
CurrentSaveVersion = 13
```

Contrat :

```text
SaveVersion == 13
    -> validation/load

SaveVersion != 13
    -> rejet
    -> aucune migration
    -> aucune réécriture
```

La v12 est volontairement incompatible.

## 9. Tests dédiés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_4.Normalization
```

Tests :

```text
SchemaSeparation
LiveProjection
EquipmentPolicy
SaveSchemaVersion
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

Le filtre de caractérisation doit rester vert :

```text
Grimrock.TechnicalDebt.TD07_3_3_4.Characterization
4/4
```

## 10. Régressions requises

```text
Grimrock.CharacterCreation.CC0
Grimrock.CharacterCreation.CC1
Grimrock.CharacterCreation.CC2
Grimrock.CharacterCreation.CC6
Grimrock.RPG.MON15.3
Grimrock.MON20.2.Recruitment
Grimrock.TechnicalDebt.TD02_3
Grimrock.TechnicalDebt.TD06_6
Grimrock.TechnicalDebt.TD07_3_2
Grimrock.TechnicalDebt.TD07_3_3_2
Grimrock.TechnicalDebt.TD07_3_3_3
Grimrock.Monsters.MON9
Grimrock.RPG.MON16.7
Grimrock.RPG.MON16.8
```

Puis Win64 Shipping.

## 11. Hors périmètre

```text
Level / Experience
Skills
Spellbook ownership
Status Effects persistence model
Class progression choices
Pending Level Up notifications
DataAssets
Blueprints
maps
```

Les 41 findings TD07.3.1 restent inchangés.

## 12. Stop condition

- [x] `CurrentWeight` retiré du state durable ;
- [x] `MaxCarryWeight` retiré du state durable ;
- [x] `IsOverloaded()` retiré du state durable ;
- [x] anciennes API Recalculate retirées ;
- [x] poids courant calculé depuis inventaire + équipement ;
- [x] capacité calculée depuis Attributes ;
- [x] `CarryWeightBonus` conservé comme bonus final ;
- [x] politique StrengthBonus conservée ;
- [x] recrutement/création sans cache ;
- [x] diagnostics basés sur le résumé ;
- [x] SaveGame passe à v13 exact-match ;
- [x] v12 rejetée sans migration ;
- [x] tests dédiés ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 après refactor ;
- [x] régressions ciblées vertes ;
- [x] Shipping Win64 vert.

## 13. Validation de clôture — 27 août 2026

Validation locale fournie après le commit de normalisation :

```text
TD07.3.3.4 Normalization     4 success / 0 warning / 0 failed
TD07.3.3.4 Characterization  4 success / 0 warning / 0 failed

CC0                            4 success / 0 warning / 0 failed
CC1                            3 success / 0 warning / 0 failed
CC2                            3 success / 0 warning / 0 failed
CC6                            6 success / 0 warning / 0 failed

MON15.3                        5 success / 1 warning / 0 failed
MON20.2 Recruitment            6 success / 0 warning / 0 failed

TD02.3                         1 success / 0 warning / 0 failed
TD06.6                         1 success / 0 warning / 0 failed
TD07.3.2                       6 success / 0 warning / 0 failed
TD07.3.3.2                     3 success / 0 warning / 0 failed
TD07.3.3.3                     8 success / 0 warning / 0 failed

MON9                           9 success / 4 warning / 0 failed
MON16.7                       10 success / 0 warning / 0 failed
MON16.8                       10 success / 0 warning / 0 failed

Win64 Shipping                 COOK / PACKAGE VALIDATED
```

Tous les filtres ont terminé avec `Process exit code = 0` et aucun test Failed / Not run. Les warnings de MON15.3 et MON9 restent non bloquants pour cette tranche : le harness les classe en validation réussie.

**TD07.3.3.4 est clos et validé.**

Prochaine tranche après validation complète :

```text
TD07.3.3.5 — Normalize XP / Level / Class Progression
```
