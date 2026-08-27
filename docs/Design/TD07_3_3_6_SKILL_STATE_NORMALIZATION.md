# TD07.3.3.6 — Skill State Normalization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Caractérisation validée : `b3a23f4ec45d5b5f5d143a1248eade56bf74d7e5`  
Statut : **IMPLÉMENTÉ — NORMALIZATION 4/4 VALIDÉE — VALIDATION COMPLÈTE EN COURS**

## 1. Autorité cible

```text
FGridCharacterInventoryState::SkillRanks
    autorité durable unique
```

Chaque entrée reste :

```text
SkillId
Rank > 0
```

Le rank zéro reste représenté par l'absence d'entrée.

## 2. Suppression du miroir Save

Supprimés :

```text
FRPGSkillRankSaveState
FRPGCharacterSkillSaveState
UGrimrockPartySaveGame::CharacterSkillStates
CapturePartySkills()
ValidateSavedPartySkills()
RestorePartySkills()
```

Aucune copie CharacterId-keyed des Skills n'est désormais sérialisée à côté du personnage.

## 3. Validation canonique conservée

`FRPGSkillPersistence` subsiste uniquement comme frontière de validation du schéma courant :

```cpp
ValidatePartySkills(PartyState)
```

Elle vérifie :

```text
CharacterId valides et uniques Active + Pool
Skill state structurellement valide
SkillId canonique résolvable
Rank > 0
Rank <= Definition.MaxRank
```

La validation ne capture, ne restaure et ne mute aucun état.

## 4. Déterminisme

L'ancien snapshot triait les ranks par `SkillId`.

Le nouvel état durable maintient cet invariant à la source :

```cpp
FRPGSkillService::TrySetSkillRank()
    -> SkillRanks.Sort(SkillId)
```

La suppression du snapshot ne rend donc pas l'ordre d'écriture dépendant de l'ordre d'apprentissage.

## 5. Active / CharacterPool

Les ranks vivent dans `FGridCharacterInventoryState`.

Déplacer un personnage entre :

```text
ActiveCharacters
CharacterPool
```

déplace naturellement son état Skill avec lui ; aucune restauration par `CharacterId` n'est nécessaire.

## 6. Consumers

Les consumers restent inchangés :

```text
FRPGSkillService
FRPGSkillRuntimeService
FRPGSkillRequirementProjectionService
skill checks
action requirements
FGridSkillsPageService
recruitment
```

Tous lisent déjà directement `Character.SkillRanks`.

## 7. SaveGame v16

Le changement de layout ouvre :

```text
CurrentSaveVersion = 16
```

Contrat :

```text
SaveVersion == 16 -> validation/load
SaveVersion != 16 -> rejet
aucune migration
aucune réécriture
```

Au save et au load, les Skills sont validés directement depuis `PartyInventoryState`.

Aucun capture/restore Skill secondaire n'est exécuté.

## 8. Tests dédiés

```text
Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.SchemaAuthority
Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.DirectValidation
Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.DeterministicMutation
Grimrock.TechnicalDebt.TD07_3_3_6.Normalization.SaveSchemaVersion
```

Validation locale du 27 août 2026 :

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_3_6.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le build UE5.5.4 associé est vert.

Validation post-refactor du gate de caractérisation :

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_3_6.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Les garanties caractérisées avant normalisation restent donc valides avec `SkillRanks` comme autorité durable unique.

## 9. Régressions requises

Validation locale du 27 août 2026 — bloc Skills / MON20 :

```text
Grimrock.MON20.6.Skills
    24 succeeded / 0 warning / 0 failed / 0 not run

Grimrock.MON20.8
    24 succeeded / 0 warning / 0 failed / 0 not run

Grimrock.MON20.9.SkillPersistence
    7 succeeded / 0 warning / 0 failed / 0 not run

Grimrock.MON20.9.ActivePoolPersistence
    8 succeeded / 0 warning / 0 failed / 0 not run

Grimrock.MON20.9.RestoredConsumers
    8 succeeded / 0 warning / 0 failed / 0 not run
```

Le bloc de régressions Skills / requirements / UI et MON20.9 est vert. Restent les régressions transverses Save / Magic / Status puis le Shipping Win64.



```text
Grimrock.MON20.6.Skills
Grimrock.MON20.8
Grimrock.MON20.9.SkillPersistence
Grimrock.MON20.9.ActivePoolPersistence
Grimrock.MON20.9.RestoredConsumers
Grimrock.TechnicalDebt.TD07_3_2
Grimrock.TechnicalDebt.TD07_3_3_5.NormalizationB2
Grimrock.Magic.MON18.8
Grimrock.RPG.MON16.7
Grimrock.RPG.MON16.8
```

Puis Win64 Shipping.

## 10. Hors périmètre

```text
Spellbook
Status Effects
Pending Level Up notifications
Class / Race identity
DataAssets
Blueprints
maps
```

## 11. Stop condition

- [x] SkillRanks non-transient ;
- [x] SkillRanks autorité durable unique ;
- [x] structs de snapshot Skill supprimés ;
- [x] CharacterSkillStates supprimé ;
- [x] capture/restore Skill supprimé ;
- [x] validation canonique directe conservée ;
- [x] ordre déterministe maintenu à la mutation ;
- [x] SaveGame v16 exact-match ;
- [x] tests dédiés ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 après refactor ;
- [x] régressions MON20.6/20.8/20.9 vertes ;
- [ ] régressions Save/Magic/Status vertes ;
- [ ] Shipping Win64 vert.

Prochaine tranche après validation complète :

```text
TD07.3.3.7 — Normalize Spellbook
```
