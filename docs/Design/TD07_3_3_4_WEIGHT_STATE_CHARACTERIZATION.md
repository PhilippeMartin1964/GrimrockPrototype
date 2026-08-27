# TD07.3.3.4 — Weight State Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `d831bf70fe5b596d599b5d88e74d287cf85f5853`  
Statut : **CHARACTERIZATION VALIDÉE — NORMALIZATION ACTIVE**

## 1. Objet

TD07.3.3.1 a identifié `CurrentWeight` et `MaxCarryWeight` comme des caches dérivés placés dans `FGridCharacterInventoryState`.

TD07.3.3.4 doit supprimer ces autorités secondaires, mais uniquement après caractérisation du comportement réel.

## 2. État courant

`FGridCharacterInventoryState` contient aujourd'hui :

```text
CurrentWeight
MaxCarryWeight
InventorySlots
```

et expose :

```cpp
bool IsOverloaded() const
{
    return CurrentWeight > MaxCarryWeight;
}
```

Ces valeurs sont recalculées par :

```cpp
UGridPartyInventoryComponent::RecalculateCharacterWeight()
```

qui :

1. additionne les poids de l'inventaire ;
2. ajoute le poids de l'équipement ;
3. recalcule `MaxCarryWeight` depuis `Attributes` ;
4. écrit les deux résultats dans le personnage.

## 3. Cache de capacité

`MaxCarryWeight` dépend de :

```text
Attributes.Strength * 5
```

mais n'est mis à jour que lorsqu'un appel explicite à `RecalculateCharacterWeight()` survient.

Conséquence caractérisée :

```text
Strength change
    -> Attributes change immédiatement
    -> MaxCarryWeight peut rester stale
    -> GetCharacterSummary() lit ce cache stale
```

## 4. Équipement

`GetCharacterSummary()` construit actuellement :

```text
BaseMaxWeight = CharacterState.MaxCarryWeight
MaxWeight     = BaseMaxWeight + EquipmentStatBonus.CarryWeightBonus
```

En parallèle :

```text
Equipment StrengthBonus
    -> Summary.Attributes.Strength
    -> ne recalcule pas BaseMaxWeight

Equipment CarryWeightBonus
    -> Summary.MaxWeight
    -> ne modifie pas Character.MaxCarryWeight
```

Exemple caractérisé :

```text
base Strength          10
base MaxCarryWeight    50
equipment STR bonus    +4
equipment carry bonus  +7

Summary Strength       14
Summary BaseMaxWeight  50
Summary MaxWeight      57

pas 77
```

Le projet ne convertit donc pas actuellement le `StrengthBonus` équipé en capacité de port supplémentaire.

## 5. Surcharge divergente

Deux calculs coexistent :

```text
Character.IsOverloaded()
    CurrentWeight > MaxCarryWeight

Summary.bOverloaded
    Summary.CurrentWeight > Summary.MaxWeight
```

Comme `Summary.MaxWeight` inclut `CarryWeightBonus` mais `Character.MaxCarryWeight` non, les deux résultats peuvent diverger.

Ce comportement doit être figé avant choix de la cible autoritaire.

## 6. CurrentWeight

`CurrentWeight` est un cache de :

```text
sum(inventory item total weights)
+ sum(equipment item total weights)
```

Le curseur global n'appartient pas à un personnage.

Lorsqu'un item passe de l'inventaire au curseur :

```text
character CurrentWeight
    -> diminue immédiatement après RecalculateCharacterWeight()
```

Le poids du curseur n'est donc pas attribué au personnage d'origine.

## 7. Restore

`RestorePartyInventoryState()` exécute :

```cpp
PartyInventoryState = MoveTemp(RestoredState);
RecalculateAllWeights();
```

Les valeurs sérialisées de `CurrentWeight` et `MaxCarryWeight` sont donc immédiatement remplacées après restore.

Cela confirme qu'elles sont reconstructibles et non des autorités nécessaires à la persistance.

## 8. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_4.Characterization
```

Tests :

```text
CachedCapacityContract
EquipmentCapacityProjectionContract
CursorWeightBoundary
RestoreRecalculatesCaches
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 9. Direction de normalisation envisagée

Après validation du gate :

```text
FGridCharacterInventoryState
    CurrentWeight       supprimer
    MaxCarryWeight      supprimer
    IsOverloaded()      supprimer ou déplacer hors de l'état durable
```

Projection cible :

```text
CurrentWeight
    calculé depuis InventorySlots + ActiveEquipment

BaseMaxWeight
    calculé depuis Attributes

MaxWeight
    règle finale à figer explicitement depuis la caractérisation équipement

bOverloaded
    calculé depuis CurrentWeight / MaxWeight
```

La suppression de deux propriétés sérialisées impliquera une nouvelle génération SaveGame exact-match.

## 10. Hors périmètre

Cette phase de caractérisation ne modifie pas :

```text
SaveGame v12
DerivedStats / Resources
Level / Experience
Skills
Spellbook
Status Effects
Class progression
DataAssets
Blueprints
maps
```

Les 41 findings TD07.3.1 restent inchangés.

## 11. Validation

À exécuter :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_4.Characterization"
```

Le build doit être exécuté : un nouveau fichier C++ de tests est ajouté.

## 12. Stop condition du gate

- [x] caches CurrentWeight / MaxCarryWeight documentés ;
- [x] stale capacity documentée ;
- [x] CarryWeightBonus documenté ;
- [x] StrengthBonus vs capacité documenté ;
- [x] divergence IsOverloaded / Summary documentée ;
- [x] frontière cursor documentée ;
- [x] reconstruction au restore documentée ;
- [x] 4 tests de caractérisation ajoutés ;
- [x] compilation UE5.5.4 verte ;
- [x] 4/4 tests verts.

Validation locale du 27 août 2026 :

```text
Filter                  : Grimrock.TechnicalDebt.TD07_3_3_4.Characterization
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code        : 0
```

Le gate est atteint. La phase B de normalisation peut commencer.
