# TD07.3.3.6 — Skill State Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `157d7b5d75ddfeeb779101cb0dfd363c4b78d8b7`  
Statut : **CHARACTERIZATION À VALIDER**

## 1. Objet

TD07.3.3.6 doit déterminer l'autorité durable unique des ranks de Skills sans changer les règles MON20.6–MON20.9.

## 2. Autorité runtime actuelle

Tous les services gameplay travaillent directement sur :

```cpp
FGridCharacterInventoryState::SkillRanks
```

Exemples :

```text
FRPGSkillService
FRPGSkillRuntimeService
FRPGSkillRequirementProjectionService
FGridSkillsPageService
skill checks
action requirements
```

`TrySetSkillRank()` et `TryIncreaseSkillRank()` mutent directement le personnage.

Le sparse contract est :

```text
rank > 0
    -> une entrée SkillId + Rank

rank == 0
    -> aucune entrée
```

## 3. Problème de persistance actuel

`SkillRanks` est marqué :

```cpp
UPROPERTY(..., Transient)
TArray<FRPGSkillRank> SkillRanks;
```

MON20.9 contourne cette frontière via :

```text
FRPGSkillRankSaveState
FRPGCharacterSkillSaveState
FRPGSkillPersistence
UGrimrockPartySaveGame::CharacterSkillStates
```

Le Save contient donc une copie séparée de l'état métier.

## 4. Snapshot sparse actuel

`CapturePartySkills()` :

1. parcourt ActiveCharacters + CharacterPool ;
2. omet les personnages sans skill entraîné ;
3. valide SkillId / Rank / MaxRank via la définition canonique ;
4. trie les ranks par SkillId ;
5. trie les snapshots par CharacterId.

Ce déterminisme servait au format snapshot séparé et doit être caractérisé avant suppression.

## 5. Restore actuel

`RestorePartySkills()` :

1. valide l'intégralité du snapshot ;
2. clone `FGridPartyInventoryState` ;
3. efface tous les `SkillRanks` du clone ;
4. réapplique les snapshots via `TrySetSkillRank()` ;
5. commit le clone uniquement après succès complet.

Conséquence :

```text
personnage absent du snapshot sparse
    -> SkillRanks vidé
```

Une erreur de définition ou un rank > MaxRank laisse le state original intact.

## 6. Consumers après restore

Aucun cache Skill secondaire n'est nécessaire après restore.

Les consumers lisent immédiatement :

```text
Character.SkillRanks
```

Cela confirme que le snapshot Save n'est pas un read-model gameplay ; il sert uniquement à contourner le `Transient`.

## 7. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_6.Characterization
```

Tests :

```text
RuntimeAuthorityBoundary
SparseSnapshotContract
RestoreReplacementBoundary
SeparatePersistenceMirror
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 8. Direction de normalisation envisagée

Après validation du gate :

```text
FGridCharacterInventoryState::SkillRanks
    retirer Transient
    devenir autorité durable unique

FRPGSkillRankSaveState
    supprimer

FRPGCharacterSkillSaveState
    supprimer

FRPGSkillPersistence
    supprimer ou réduire aux seules validations encore réellement utiles

UGrimrockPartySaveGame::CharacterSkillStates
    supprimer
```

Le SaveGame devra ouvrir une nouvelle génération exact-match si le layout sérialisé change.

## 9. Invariants à préserver

La normalisation ne doit pas changer :

```text
sparse rank semantics
rank zéro = absence
MaxRank validation
duplicate SkillId rejection
atomic mutations
Active / CharacterPool behavior
skill checks
requirement projection
Skills page read model
recruitment preservation
```

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

## 11. Validation

À exécuter :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_6.Characterization"
```

Le build doit être exécuté : un nouveau fichier C++ de tests est ajouté.

## 12. Stop condition du gate

- [x] autorité runtime SkillRanks documentée ;
- [x] snapshot séparé documenté ;
- [x] sparse Active + Pool documenté ;
- [x] ordre déterministe documenté ;
- [x] restore de remplacement documenté ;
- [x] atomicité documentée ;
- [x] 4 tests de caractérisation ajoutés ;
- [ ] compilation UE5.5.4 verte ;
- [ ] 4/4 tests verts.

Aucune suppression de `Transient` ou de `CharacterSkillStates` ne commence avant ce gate vert.
