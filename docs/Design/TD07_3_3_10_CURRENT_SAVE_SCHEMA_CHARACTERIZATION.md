# TD07.3.3.10 — Current Save Schema / Closure Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `9fcfca344daa9c1a9007d56212a9be87972792f1`  
Statut : **CHARACTERIZATION VALIDÉE — NORMALIZATION IMPLÉMENTÉE / À VALIDER**

## 1. Objet

TD07.3.3.10 est la tranche finale de Character State Normalization.

Elle doit :

```text
auditer le schéma Save courant
confirmer la suppression des snapshots annexes personnage
identifier tout dernier état calculable encore durable
ouvrir le dernier schéma prototype si nécessaire
exécuter les régressions finales
clore TD07.3.3
```

## 2. Résultat de l'audit Save v19

`UGrimrockPartySaveGame` contient désormais :

```text
SaveVersion
PartyInventoryState
DungeonRuntimeState
CurrentDungeonLevelId
PartyCellX
PartyCellY
PartyFacing
```

Il ne contient plus :

```text
ClassProgressionStates
CharacterSkillStates
CharacterSpellbookStates
CharacterStatusEffectStates
PendingLevelUpNotifications
```

L'état durable propre aux personnages voyage donc uniquement dans :

```text
PartyInventoryState
    ActiveCharacters[]
    CharacterPool[]
```

## 3. Autorités déjà normalisées

```text
Attributes                         durable
Experience                         durable
Level                              transient projection
Resources                          durable mutable
SelectedClassProgressionChoiceIds durable
SkillRanks                         durable
KnownSpellIds                      durable
StatusEffects                      durable
LastAcknowledgedLevel              durable
InventorySlots                     durable
CombatHotbarSlots                  durable
```

Supprimés :

```text
Strength
bRPGAttributesInitialized
CurrentWeight
MaxCarryWeight
```

## 4. Dernier finding — DerivedStats

Le dernier reliquat structurel est :

```text
FGridCharacterInventoryState::DerivedStats
```

TD07.3.3.3 l'a correctement réduit à une projection calculable :

```text
MaxHealth
MaxMana
Initiative
Accuracy
Evasion
```

et a isolé l'état mutable dans `Resources`.

Mais le champ lui-même reste actuellement **non transient**.

Conséquence :

```text
Attributes + ClassDefinition + Level
    peuvent recalculer DerivedStats

mais

DerivedStats
    est encore sérialisé dans PartyInventoryState
```

Cela contredit la politique TD07.3 :

```text
Derived = recalculé
Derived != autorité durable
```

## 5. Gap de validation

`UGrimrockPartySaveGame::ValidateCurrentState()` valide actuellement :

```text
Experience / Level
LastAcknowledgedLevel
class progression choices
Skills
Spellbook
Status Effects
ownership / dungeon state
```

mais ne vérifie pas que :

```text
Character.DerivedStats
    ==
CalculateDerivedStats(
    Character.Attributes,
    Character.ClassDefinition,
    Character.Level)
```

Un cache DerivedStats incohérent peut donc être accepté par le schéma v19.

## 6. Cible après gate

La Characterization étant verte, TD07.3.3.10 normalise :

```text
DerivedStats
    -> Transient
    -> reconstruit après désérialisation

Resources
    -> reste durable
```

Pipeline cible :

```text
Load v20
    deserialize durable state
    rebuild Level from Experience
    resolve ClassDefinition
    rebuild DerivedStats from Attributes + Class + Level
    validate durable state
    rehydrate transient caches
    rebuild class progression runtime projection
```

## 7. Schéma attendu

Cette modification change le layout sérialisé de `FGridCharacterInventoryState`.

Cible :

```text
CurrentSaveVersion = 20
v19 et antérieures -> rejet
aucune migration
```

## 8. Frontière TD07.3.4 conservée

Ne sont pas nettoyés ici :

```text
ClassId + ClassDefinition + ClassDisplayName
RaceId + RaceDisplayName
PortraitVariantId + Portrait
ClassId + ClassIcon
```

Ils restent explicitement dans :

```text
TD07.3.4 — Authoring Identity Normalization
```

Cette tranche ne doit pas élargir son scope à l'authoring identity.

## 9. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_10.Characterization
```

Tests :

```text
SaveEnvelope
AuthorityFlags
DerivedStatsPersistenceGap
DerivedStatsValidationGap
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 10. Stop condition du gate

- [x] enveloppe Save v19 auditée ;
- [x] cinq snapshots annexes confirmés absents ;
- [x] autorités personnage finales cartographiées ;
- [x] read models exclus de l'autorité durable ;
- [x] frontière TD07.3.4 conservée ;
- [x] dernier reliquat DerivedStats identifié ;
- [x] gap de validation caractérisé ;
- [x] cible v20 documentée ;
- [x] 4 tests ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] 4/4 tests verts.


## 11. Validation locale

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_3_10.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-231038
```

Le gate est atteint. La normalisation finale peut commencer.
