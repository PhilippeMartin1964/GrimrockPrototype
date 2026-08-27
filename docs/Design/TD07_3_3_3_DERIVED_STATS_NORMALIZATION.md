# TD07.3.3.3 — Normalize Derived Stats / Mutable Resources

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline de caractérisation validée : `12e2d5abc52bc9f085c8b9fef330bf7879f26efd`  
Statut : **VALIDÉ — STOP CONDITION ATTEINTE**

## 1. Objet

TD07.3.3.3 supprime le mélange entre statistiques reconstructibles et ressources mutables.

Avant :

```text
FRPGDerivedStats
    MaxHealth
    CurrentHealth
    MaxMana
    CurrentMana
    PhysicalArmor
    MagicalArmor
    Initiative
    Accuracy
    Evasion
```

Après :

```text
FRPGDerivedStats
    MaxHealth
    MaxMana
    Initiative
    Accuracy
    Evasion

FRPGCharacterResources
    CurrentHealth
    CurrentMana
    CurrentPhysicalArmor
    CurrentMagicalArmor
```

## 2. Autorités

`FGridCharacterInventoryState` porte désormais :

```text
Attributes       autorité durable
DerivedStats     projection calculée/reconstructible
Resources        état durable mutable
```

Les dégâts, soins, dépenses de mana et absorption d'armure mutent uniquement `Resources`.

## 3. Initialisation

`URPGCharacterRulesLibrary::CalculateDerivedStats()` ne crée plus d'état mutable.

Nouvelle frontière :

```cpp
FRPGCharacterResources InitializeCharacterResources(
    const FRPGDerivedStats& DerivedStats,
    const URPGClassAsset* ClassDefinition);
```

Un personnage frais démarre avec :

```text
CurrentHealth          = MaxHealth
CurrentMana            = MaxMana
CurrentPhysicalArmor   = BasePhysicalArmor
CurrentMagicalArmor    = BaseMagicalArmor
```

Création initiale, Story Companion et Custom Recruit utilisent cette frontière.

## 4. Level-Up

Le level-up calcule d'abord les nouveaux `DerivedStats`, puis reconstruit les ressources initiales et réapplique les déficits HP/Mana caractérisés.

Contrat conservé :

```text
HP : déficit absolu conservé
Mana : dépense absolue conservée
personnage mort : reste mort
armures : réinitialisées aux valeurs de classe
```

Cette dernière règle reproduit le comportement historique de MON15.3.

## 5. Équipement

Le comportement du gate de caractérisation est conservé.

`GetCharacterSummary()` :

```text
bonus DEX
    -> Attributes projetés
    -> pas de recalcul implicite Initiative/Accuracy/Evasion

MaxHealthBonus / MaxManaBonus
    -> maxima projetés

ArmorBonus
    -> Resources.CurrentPhysicalArmor projeté

retrait d'un bonus de maximum
    -> clamp dans le résumé
    -> état durable non normalisé automatiquement
```

Aucune modification d'équilibrage n'est introduite.

## 6. Combat

Les consommateurs qui mutaient auparavant :

```text
Character.DerivedStats.CurrentHealth
Character.DerivedStats.PhysicalArmor
Character.DerivedStats.MagicalArmor
```

utilisent désormais :

```text
Character.Resources.CurrentHealth
Character.Resources.CurrentPhysicalArmor
Character.Resources.CurrentMagicalArmor
```

L'initiative, Accuracy et Evasion restent dans `DerivedStats`.

## 7. Magic

MON18 n'utilise plus `FRPGDerivedStats` comme transporteur de mana.

Les transactions de coûts et le pipeline de hotbar prennent désormais `FRPGCharacterResources`.

Le résultat du hotbar transporte :

```text
CasterResources
CasterTurnState
```

et ne peut donc plus écraser une projection calculée en recopiant un conteneur mixte.

## 8. SaveGame v12

Le schéma sérialisé de `FGridCharacterInventoryState` change.

```text
CurrentSaveVersion = 12
```

Contrat :

```text
SaveVersion == 12
    -> validation/load

SaveVersion != 12
    -> rejet
    -> aucune migration
    -> aucune réécriture
```

La v11 est volontairement incompatible.

## 9. Tests dédiés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_3.Normalization
```

Tests :

```text
SchemaSeparation
ResourceInitialization
MagicResourceBoundary
SaveSchemaVersion
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

Le filtre de caractérisation doit également rester vert :

```text
Grimrock.TechnicalDebt.TD07_3_3_3.Characterization
4/4
```

## 10. Régressions requises

```text
Grimrock.RPG.MON15.2
Grimrock.RPG.MON15.3
Grimrock.RPG.MON16.3
Grimrock.RPG.MON16.7
Grimrock.RPG.MON16.8
Grimrock.Magic.MON18.3
Grimrock.Magic.MON18.4
Grimrock.Magic.MON18.5
Grimrock.Magic.MON18.6
Grimrock.Magic.MON18.9.2
Grimrock.UI.UI01.4.3e.2
Grimrock.Monsters.MON9
Grimrock.TechnicalDebt.TD07_3_2
```

Puis Win64 Shipping.

## 11. Hors périmètre

```text
CurrentWeight / MaxCarryWeight
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

- [x] `FRPGDerivedStats` ne contient plus de ressource mutable ;
- [x] `FRPGCharacterResources` introduit ;
- [x] création/recrutement initialise `Resources` ;
- [x] combat migre vers `Resources` ;
- [x] Magic migre vers `Resources` ;
- [x] level-up conserve la politique de déficit ;
- [x] SaveGame passe à v12 exact-match ;
- [x] v11 rejetée sans migration ;
- [x] tests dédiés ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 après refactor ;
- [x] régressions ciblées vertes ;
- [x] Shipping Win64 vert.

## 13. Validation de clôture — 27 août 2026

Validation locale fournie après le commit de normalisation :

```text
TD07.3.3.3 Normalization    4 success / 0 warning / 0 failed
TD07.3.3.3 Characterization 4 success / 0 warning / 0 failed

MON15.2                      1 success / 4 warning / 0 failed
MON15.3                      5 success / 1 warning / 0 failed
MON16.3                     10 success / 1 warning / 0 failed
MON16.7                     10 success / 0 warning / 0 failed
MON16.8                     10 success / 0 warning / 0 failed

Magic MON18.3                6 success / 0 warning / 0 failed
Magic MON18.4                8 success / 0 warning / 0 failed
Magic MON18.5                6 success / 0 warning / 0 failed
Magic MON18.6                7 success / 0 warning / 0 failed
Magic MON18.9.2              5 success / 0 warning / 0 failed
UI01.4.3e.2                  6 success / 0 warning / 0 failed

MON9                         9 success / 4 warning / 0 failed
TD07_3_2                     6 success / 0 warning / 0 failed

Win64 Shipping               COOK / PACKAGE VALIDATED
```

Tous les filtres ont terminé avec `Process exit code = 0` et aucun test Failed / Not run. Les warnings de quelques suites de régression restent non bloquants pour cette tranche : le harness les classe en validation réussie.

**TD07.3.3.3 est clos et validé.**

Prochaine tranche après validation complète :

```text
TD07.3.3.4 — Normalize Weight State
```


## Finalisation TD07.3.3.10

TD07.3.3.3 a séparé le contenu calculable de `DerivedStats` des ressources mutables. TD07.3.3.10 finalise la frontière de persistance :

```text
DerivedStats  -> Transient
Resources     -> durable
```

La projection `DerivedStats` est désormais reconstruite après chargement depuis `Attributes + ClassDefinition + Level`. Le schéma correspondant est v20 exact-match.
