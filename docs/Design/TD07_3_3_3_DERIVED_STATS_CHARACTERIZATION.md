# TD07.3.3.3 — Derived Stats / Mutable Resources Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `8f53a5ded926b369b83c72bdb2983caf2591d805`  
Statut : **CHARACTERIZATION À VALIDER**

## 1. Objet

TD07.3.3.1 a démontré que `FRPGDerivedStats` n'est pas un conteneur purement dérivé.

Il mélange aujourd'hui :

```text
calculable / projection
    MaxHealth
    MaxMana
    Initiative
    Accuracy
    Evasion

mutable / durable
    CurrentHealth
    CurrentMana
    PhysicalArmor courant
    MagicalArmor courant
```

Avant toute séparation de structure, TD07.3.3.3 fige le comportement réel des consommateurs existants.

## 2. Risque équipement

`UGridPartyInventoryComponent::GetCharacterSummary()` applique actuellement les bonus d'équipement comme une projection :

```text
Attributes
    + StrengthBonus
    + DexterityBonus
    + ConstitutionBonus
    + IntelligenceBonus
    + WisdomBonus
    + CharismaBonus

DerivedStats
    + MaxHealthBonus
    + MaxManaBonus
    + ArmorBonus -> PhysicalArmor
```

Mais cette projection ne recalcule pas automatiquement :

```text
Initiative
Accuracy
Evasion
```

depuis la DEX finale.

En parallèle, plusieurs consommateurs de combat ne lisent pas le résumé final.

## 3. Contrat entrant actuel

`GridMonsterCombatComponent.cpp` construit encore sa cible à partir de l'état stocké :

```cpp
Target.Evasion = Character.DerivedStats.Evasion;
Target.CurrentHealth = Character.DerivedStats.CurrentHealth;
Target.PhysicalArmor = Character.DerivedStats.PhysicalArmor;
Target.MagicalArmor = Character.DerivedStats.MagicalArmor;
```

et commit directement les dégâts dans ce même conteneur.

Conséquence caractérisée :

```text
ArmorBonus projeté dans le résumé
    !=
armure courante consommée par les attaques ennemies
```

Cette incohérence ne doit pas être corrigée implicitement pendant une simple normalisation de données.

## 4. Initiative actuelle

`GridTurnManagerInitiative.cpp` utilise :

```cpp
Entry.InitiativeBase = 10 + Character.DerivedStats.Initiative;
```

La DEX projetée avec bonus d'équipement est disponible via le résumé mais ne recalcule pas cette base.

## 5. MaxHealth / MaxMana

Le résumé applique les bonus de maximum puis clamp les ressources uniquement dans la projection.

L'état stocké n'est pas automatiquement augmenté lors de l'équipement.

Lors du retrait d'un bonus de maximum, le résumé peut donc clamp une ressource à la nouvelle capacité sans normaliser la valeur stockée.

Ce comportement est caractérisé avant décision de cible.

## 6. Factory mixte

`URPGCharacterRulesLibrary::CalculateDerivedStats()` calcule les maxima et projections, mais initialise aussi :

```text
CurrentHealth = MaxHealth
CurrentMana   = MaxMana
PhysicalArmor = BasePhysicalArmor
MagicalArmor  = BaseMagicalArmor
```

Cette factory mélange donc déjà calcul et état initial mutable.

## 7. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_3.Characterization
```

Tests :

```text
EquipmentProjectionContract
ResourceRemovalProjection
CombatConsumerBoundary
MixedFactoryContract
```

Attendu avant modification de production :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 8. Décision de méthode

Cette tranche suit deux phases séparées :

```text
Phase A — characterization gate
    tests uniquement
    aucun changement de modèle
    aucune évolution SaveGame

Phase B — normalization
    uniquement après gate vert
    séparer projection calculable et ressources mutables
    conserver explicitement la sémantique caractérisée
    toute évolution volontaire de gameplay doit être isolée et documentée
```

## 9. Cible structurelle envisagée

La cible exacte sera finalisée après validation, mais la direction est :

```text
FGridCharacterInventoryState
    Attributes                 autorité
    <calculated stats>         projection/reconstructible
    <mutable resources>        état durable

mutable resources
    CurrentHealth
    CurrentMana
    CurrentPhysicalArmor
    CurrentMagicalArmor
```

Les maxima et statistiques de combat calculables doivent pouvoir être reconstruits depuis les données autoritaires courantes.

## 10. Hors périmètre

Ce gate ne modifie pas :

```text
SaveGame v11
Level / Experience
CurrentWeight / MaxCarryWeight
Skills
Spellbook
Status Effects persistence
Class progression choices
Pending Level Up
DataAssets
Blueprints
maps
```

Les 41 findings TD07.3.1 restent inchangés.

## 11. Validation

À exécuter :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_3.Characterization"
```

Le build doit être exécuté : ce commit ajoute un nouveau fichier C++ de tests.

## 12. Stop condition du gate

- [x] risque DerivedStats mixte documenté ;
- [x] projection équipement documentée ;
- [x] consommateur combat entrant documenté ;
- [x] initiative documentée ;
- [x] comportement retrait de maxima documenté ;
- [x] 4 tests de caractérisation ajoutés ;
- [ ] compilation UE5.5.4 verte ;
- [ ] 4/4 tests verts.

Aucune séparation structurelle ne commence avant ce gate vert.
