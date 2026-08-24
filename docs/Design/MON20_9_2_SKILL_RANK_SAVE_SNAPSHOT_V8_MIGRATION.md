# MON20.9.2 — Skill Rank Save Snapshot + v8 Migration

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — 8/8 SUCCESS**

## 1. Objectif

Rendre persistants les `SkillRanks` introduits en MON20.6 sans transformer `FGridCharacterInventoryState` en format de sauvegarde implicite et sans dupliquer la persistance des talents MON15/MON20.7.

Le runtime reste :

```text
FGridCharacterInventoryState::SkillRanks
    TArray<FRPGSkillRank>
    Transient
```

La frontière durable devient :

```text
FRPGCharacterSkillSaveState
    CharacterId
    SkillRanks[]
```

## 2. Nouveau contrat de snapshot

Deux structures SaveGame sont ajoutées :

```text
FRPGSkillRankSaveState
    SkillId
    Rank

FRPGCharacterSkillSaveState
    CharacterId
    SkillRanks[]
```

`UGrimrockPartySaveGame` possède maintenant :

```text
CharacterSkillStates : TArray<FRPGCharacterSkillSaveState>
```

Les snapshots sont sparse : un personnage sans compétence entraînée n'est pas écrit.

Les `RequirementIds` de MON20.8 ne sont jamais sauvegardés. Ils restent entièrement dérivés du rang restauré et de la définition `URPGSkillAsset` canonique.

## 3. `FRPGSkillPersistence`

Le nouveau service pur fournit :

```text
CapturePartySkills(...)
ValidateSavedPartySkills(...)
RestorePartySkills(...)
ResolveDefinitionBySkillId(...)
```

Chaque opération possède également une variante avec resolver injecté pour les tests déterministes.

### Capture

La capture :

- inspecte `ActiveCharacters` puis `CharacterPool` ;
- exige des `CharacterId` valides et uniques dans l'ensemble du groupe ;
- valide la structure sparse via `FRPGSkillService::ValidateSkillState()` ;
- résout chaque définition canonique ;
- vérifie `1 <= Rank <= MaxRank` ;
- omet les personnages sans Skill ;
- trie les rangs par `SkillId` ;
- trie les personnages par `CharacterId` ;
- ne modifie la sortie qu'après validation complète.

### Validation

Un snapshot persistant est rejeté si :

```text
CharacterId invalide / dupliqué / orphelin
snapshot personnage vide
SkillId vide / dupliqué
Rank <= 0
Rank > Definition.MaxRank
définition canonique absente ou invalide
```

### Restore

Le restore travaille sur une copie candidate de `FGridPartyInventoryState` :

```text
copy party
    -> clear SkillRanks actifs + réserve
    -> resolve CharacterId
    -> resolve RPGSkill:<SkillId>
    -> FRPGSkillService::TrySetSkillRank()
    -> commit candidate seulement si tout réussit
```

Échec = aucune mutation partielle.

## 4. SaveGame v8

Le contrat devient :

```text
CurrentSaveVersion = 8
MinimumCompatibleSaveVersion = 1
```

À la sauvegarde, `UGrimrockPartySaveGame::Serialize()` capture désormais les Skills avant `ValidateCurrentSave()`.

Au chargement :

```text
PrepareLoadedSave()
RestoreStatusEffectState()
Restore class progression
Restore Skill snapshots
Restore pending Level Up state
```

Le restore Skill est donc terminé avant les futurs recalculs MON20.8 de requirements, actions et page Compétences.

## 5. Migration v7 -> v8

Une sauvegarde v7 ne pouvait pas contenir de rang Skill durable puisque `SkillRanks` était `Transient`.

La migration applique donc :

```text
validate progression / spellbook / level variables v7
clear any in-memory transient SkillRanks
CharacterSkillStates = []
SaveVersion = 8
ValidateCurrentSave(v8)
```

Aucun rang n'est inventé.

Les chemins v1-v6 initialisent eux aussi explicitement `CharacterSkillStates=[]` et suppriment tout éventuel `SkillRanks` transient avant leur validation finale v8.

## 6. Validation v8

`FRPGSaveMigrationService::ValidateCurrentSave()` valide maintenant également :

```text
FRPGSkillPersistence::ValidateSavedPartySkills(
    PartyInventoryState,
    CharacterSkillStates)
```

Cette validation couvre personnages actifs et réserve.

## 7. Tests Automation

Filtre :

```text
Grimrock.MON20.9.SkillPersistence
```

Tests :

```text
CaptureActivePoolSparse
CaptureDeterministicOrder
CaptureInvalidCharacterAtomic
CaptureInvalidSkillAtomic
RestoreRoundTrip
RestoreByCharacterId
RestoreInvalidSnapshotAtomic
V7ToV8Migration
```

Ils couvrent notamment :

- actifs + réserve ;
- snapshot sparse ;
- ordre déterministe ;
- `CharacterId` invalide/dupliqué ;
- `SkillId` dupliqué ;
- rang supérieur à `MaxRank` ;
- définition canonique absente ;
- restore atomique ;
- rattachement par `CharacterId` indépendant de l'ordre ;
- migration v7 -> v8 sans rang inventé ;
- `CurrentSaveVersion == 8`.

## 8. Validation UE5.5.4

Campagne exécutée le **24 août 2026** :

```text
Grimrock.MON20.9.SkillPersistence
8 / 8 Success
0 Fail
0 Error
```

Tests confirmés `Success` :

```text
CaptureActivePoolSparse
CaptureDeterministicOrder
CaptureInvalidCharacterAtomic
CaptureInvalidSkillAtomic
RestoreByCharacterId
RestoreInvalidSnapshotAtomic
RestoreRoundTrip
V7ToV8Migration
```

MON20.9.2 est donc **VALIDÉ UE5.5.4**.

## 9. Hors scope

MON20.9.2 n'ajoute pas :

- de nouvel asset Skill de production ;
- de nouvelle persistance Talent ;
- de persistance des `RequirementIds` ;
- de monnaie de points de compétence ;
- de modification `.uasset` / `.umap` ;
- de changement visuel de la page Compétences.

Les régressions actifs/réserve et consommateurs restaurés sont traitées dans MON20.9.3 et MON20.9.4.
