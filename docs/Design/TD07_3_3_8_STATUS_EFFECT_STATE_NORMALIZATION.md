# TD07.3.3.8 — Status Effect State Normalization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Characterization validée : `07f10dacd81d4d42c07a5bb6449bc7fb314b0b76`  
Statut : **IMPLÉMENTÉ — VALIDATION UE / RÉGRESSIONS / SHIPPING REQUISES**

## 1. Autorité durable personnage

```text
FGridCharacterInventoryState::StatusEffects
    -> FGridStatusEffectCollection
        -> ActiveEffects[]
            -> FGridStatusEffectRuntimeState
```

`Character.StatusEffects` n'est plus `Transient`.

Les champs durables sont :

```text
EffectId
SourceId
StackCount
DurationUnit
RemainingDuration
Potency
```

Le seul cache runtime est :

```text
DefinitionAsset [Transient]
```

## 2. Suppression du miroir Save personnage

Supprimés :

```text
FGridCharacterStatusEffectSaveState
UGrimrockPartySaveGame::CharacterStatusEffectStates
UGrimrockPartySaveGame::CaptureStatusEffectState()
UGrimrockPartySaveGame::RestoreStatusEffectState()
ResetRuntimeStatusEffects()
FindCharacterById() / CountCharacterId() utilisés uniquement par ce miroir
```

Le SaveGame sérialise désormais les Status Effects avec le reste de `PartyInventoryState`.

## 3. Validation directe

`FGridStatusEffectPersistence` expose deux niveaux de validation :

```text
ValidatePartyStatusEffects()
    valide les champs durables
    DefinitionAsset peut être null après désérialisation

ValidateRuntimePartyStatusEffects()
    validation de sauvegarde
    exige un DefinitionAsset valide et cohérent
```

Les invariants restent :

```text
CharacterId valide et unique Active + Pool
EffectId non vide
aucun doublon d'EffectId par collection
StackCount >= 1
Potency >= 0
durée cohérente
DefinitionAsset.EffectId == EffectId au runtime
DefinitionAsset.DurationUnit == DurationUnit
StackCount <= DefinitionAsset.MaxStacks
```

## 4. Rehydration directe et atomique

Au chargement :

```text
PartyInventoryState désérialisé
    -> stable StatusEffects présents
    -> DefinitionAsset == null
    -> ValidatePartyStatusEffects()
    -> RehydratePartyStatusEffects(candidate)
        EffectId -> définition canonique
        BuildRuntimeState()
        tri déterministe
    -> commit PartyInventoryState uniquement après succès complet
```

Une définition manquante ou incompatible rejette le chargement sans commit partiel.

## 5. Persistance monster préservée

TD07.3.3.8 ne modifie pas le contrat monster :

```text
AGridMonsterActor::StatusEffects [Transient runtime]
    -> CaptureCollection()
    -> FGridRuntimeMonsterState::StatusEffects
        TArray<FGridStatusEffectSaveState>
    -> RestoreCollection()
    -> AGridMonsterActor::StatusEffects
```

Restent donc nécessaires :

```text
FGridStatusEffectSaveState
CaptureCollection()
RestoreCollection()
ValidateSavedCollection()
FGridRuntimeMonsterState::StatusEffects
```

## 6. SaveGame v18

```text
CurrentSaveVersion = 18

v18
    -> exact-match
    -> durable Character.StatusEffects
    -> DefinitionAsset rehydraté

v17 et antérieures
    -> rejet
    -> aucune migration
```

## 7. Tests dédiés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_8.Normalization
```

Tests :

```text
SchemaAuthority
DirectDurableValidation
AtomicRehydration
SaveSchemaVersion
```

Attendu : 4/4 sans warning.

Le filtre Characterization doit également rester 4/4 après refactor.

## 8. Régressions prioritaires

```text
Grimrock.RPG.MON16.7
Grimrock.RPG.MON16.8
Grimrock.RPG.MON16.1
Grimrock.RPG.MON16.2
Grimrock.RPG.MON16.3
Grimrock.RPG.MON16.4
Grimrock.RPG.MON16.5
Grimrock.RPG.MON16.6
Grimrock.Magic.MON18.5
Grimrock.Magic.MON18.9.2
Grimrock.Save.MON18.9.1
Grimrock.TechnicalDebt.TD07_3_2.SaveContract
Grimrock.TechnicalDebt.TD07_3_3_7.Normalization
```

Puis Win64 Shipping.

## 9. Stop condition

- [x] Character.StatusEffects rendu durable ;
- [x] DefinitionAsset reste transient ;
- [x] miroir Save personnage supprimé ;
- [x] Capture/Restore party supprimé ;
- [x] validation durable directe ajoutée ;
- [x] validation runtime de sauvegarde ajoutée ;
- [x] rehydration party directe et atomique ajoutée ;
- [x] persistance monster préservée ;
- [x] SaveGame v18 exact-match ;
- [x] tests dédiés ajoutés ;
- [ ] build UE5.5.4 vert ;
- [ ] Normalization 4/4 ;
- [ ] Characterization 4/4 post-refactor ;
- [ ] régressions Status/Magic/Save vertes ;
- [ ] Shipping Win64 vert.

Prochaine tranche après validation complète :

```text
TD07.3.3.9 — Normalize Level-Up Notification State
```
