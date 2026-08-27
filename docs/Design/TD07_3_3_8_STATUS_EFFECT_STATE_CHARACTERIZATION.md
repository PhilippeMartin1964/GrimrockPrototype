# TD07.3.3.8 — Status Effect State Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `be869676c208a7fa7484d00bde3c90d4392389b5`  
Statut : **CHARACTERIZATION VALIDÉE — NORMALIZATION ACTIVE**

## 1. Objet

TD07.3.3.8 normalise la persistance des Status Effects des personnages.

La donnée métier est déjà portée par :

```text
FGridCharacterInventoryState::StatusEffects
    -> FGridStatusEffectCollection
        -> ActiveEffects[]
            -> FGridStatusEffectRuntimeState
```

Mais cette propriété est encore `Transient`, et le SaveGame possède un miroir séparé.

## 2. État runtime actuel

Chaque effet runtime contient :

```text
EffectId
SourceId
StackCount
DurationUnit
RemainingDuration
Potency
DefinitionAsset [Transient]
```

Les six premiers champs sont l'état mutable/stable réel.

`DefinitionAsset` est un cache de définition statique, reconstructible depuis `EffectId`.

## 3. Miroir Save personnage actuel

Le SaveGame porte encore :

```text
FGridCharacterStatusEffectSaveState
    CharacterId
    StatusEffects[]
        FGridStatusEffectSaveState

UGrimrockPartySaveGame::CharacterStatusEffectStates[]
```

Le pipeline est :

```text
Character.StatusEffects
    -> CaptureStatusEffectState()
    -> CharacterStatusEffectStates
    -> SaveGame
    -> RestoreStatusEffectState()
    -> Character.StatusEffects
```

C'est une duplication de l'état stable du personnage.

## 4. Snapshot sparse Active + Pool

`CaptureStatusEffectState()` :

1. parcourt `ActiveCharacters` ;
2. parcourt `CharacterPool` ;
3. ignore les personnages sans effet ;
4. exige un `CharacterId` valide et non ambigu ;
5. convertit chaque runtime effect via `FGridStatusEffectPersistence::CaptureCollection()` ;
6. trie les snapshots par `CharacterId`.

## 5. Restore de remplacement atomique

`RestoreStatusEffectState()` construit un `CandidateParty` :

```text
PartyInventoryState
    -> copie candidate
    -> ResetRuntimeStatusEffects(candidate)
    -> rehydrate chaque snapshot
    -> commit PartyInventoryState uniquement après succès complet
```

Conséquences à préserver :

```text
snapshot absent -> StatusEffects vide
snapshot présent -> remplace le runtime précédent
erreur de définition -> aucun commit partiel
```

## 6. Distinction essentielle : monstres

`FGridStatusEffectSaveState` ne doit pas être supprimé globalement.

Les monstres persistent leurs effets dans :

```text
FGridRuntimeMonsterState::StatusEffects
    TArray<FGridStatusEffectSaveState>
```

Leur pipeline est distinct :

```text
AGridMonsterActor::StatusEffects
    -> FGridStatusEffectPersistence::CaptureCollection()
    -> FGridRuntimeMonsterState::StatusEffects

FGridRuntimeMonsterState::StatusEffects
    -> FGridStatusEffectPersistence::RestoreCollection()
    -> AGridMonsterActor::StatusEffects
```

TD07.3.3.8 concerne l'autorité **personnage**. La persistance monster doit rester fonctionnelle.

## 7. Direction cible après gate

Après validation du gate :

```text
FGridCharacterInventoryState::StatusEffects
    durable
    autorité unique

FGridStatusEffectRuntimeState
    stable fields persistés directement
    DefinitionAsset transient / rehydraté
```

À supprimer côté personnage :

```text
FGridCharacterStatusEffectSaveState
UGrimrockPartySaveGame::CharacterStatusEffectStates
CaptureStatusEffectState()
RestoreStatusEffectState()
ResetRuntimeStatusEffects() si devenu inutile
```

À conserver pour les monstres :

```text
FGridStatusEffectSaveState
FGridStatusEffectPersistence::CaptureCollection()
FGridStatusEffectPersistence::RestoreCollection()
FGridRuntimeMonsterState::StatusEffects
```

Le SaveGame devrait ouvrir une nouvelle génération exact-match, vraisemblablement **v18**, sans migration.

## 8. Rehydration cible

Après chargement du `PartyInventoryState`, chaque `Character.StatusEffects` devra :

```text
valider EffectId / durée / stacks / identité
résoudre la définition canonique depuis EffectId
réaffecter DefinitionAsset
échouer atomiquement si la définition est absente ou incompatible
```

Il ne doit plus être nécessaire de convertir vers un snapshot miroir pour accomplir cette rehydration.

## 9. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_8.Characterization
```

Tests :

```text
RuntimeAuthorityBoundary
PartySparseSaveMirror
PartyRestoreReplacementBoundary
MonsterSnapshotIsolation
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 10. Invariants à préserver

```text
EffectId stable
SourceId stable
StackCount
DurationUnit
RemainingDuration
Potency
DefinitionAsset jamais autorité durable
tri déterministe
ActiveCharacters + CharacterPool
restore atomique
monster persistence intacte
periodic damage / buffs / debuffs inchangés
```

## 11. Hors périmètre

```text
Spellbook — TD07.3.3.7 clos
Skills — TD07.3.3.6 clos
Pending Level-Up notifications — TD07.3.3.9
Monster Status Effect authority redesign
DataAssets / Blueprints / maps
```

## 12. Validation

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_8.Characterization"
```

## 13. Stop condition du gate

- [x] runtime character authority documentée ;
- [x] frontière `DefinitionAsset` transient documentée ;
- [x] miroir Save personnage documenté ;
- [x] Active + Pool documentés ;
- [x] restore de remplacement / atomicité documenté ;
- [x] dépendance monster à `FGridStatusEffectSaveState` documentée ;
- [x] cible de suppression personnage documentée ;
- [x] 4 tests de caractérisation ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] 4/4 tests verts.

Validation locale du 27 août 2026 :

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_3_8.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-220118
```

Le gate est atteint. La normalisation peut commencer.
