# TD07.3.3.8 — Status Effect State Normalization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Characterization validée : `07f10dacd81d4d42c07a5bb6449bc7fb314b0b76`  
Statut : **VALIDÉ ET CLOS**

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

## 8.1 Validation locale — build + Normalization

Validation du 27 août 2026 :

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_3_8.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-221234
```

Le build UE5.5.4 Development Editor et le gate Normalization sont validés.

## 8.2 Validation des régressions post-refactor

Validation locale du 27 août 2026 :

```text
TD07.3.3.8 Characterization     4/4
MON16.1                         7/7
MON16.2                        10/10
MON16.3                        11/11
MON16.4                        11/11
MON16.5                        11/11
MON16.6                        10/10
MON16.7                        10/10
MON16.8                        10/10
MON18.5                         6/6
MON18.9.2                       5/5
MON18.9.1 Save                  6/6
TD07.3.2 Save Contract          6/6
TD07.3.3.7 Spellbook            4/4

Total                         111/111
Warnings                        0
Failures                        0
Not run                         0
```

MON16.3 a été relancé après correction du fixture warning :

```text
Report : Saved/Automation/TD04/TD04-20260827-222122
Succeeded : 11
Warnings  : 0
Failed    : 0
```

Le bloc de régressions post-refactor est entièrement vert. Il ne reste que la validation Win64 Shipping.

## 8.3 Validation Win64 Shipping

Validation finale du 27 août 2026 :

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : Saved/Packaging/TD04/TD04-Shipping-20260827-222314/Windows/GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 905590363
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260827-222314
[OK] Cook / package validated.
```

La stop condition TD07.3.3.8 est entièrement atteinte.

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
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 post-refactor ;
- [x] régressions Status/Magic/Save vertes ;
- [x] Shipping Win64 vert.

Prochaine tranche :

```text
TD07.3.3.9 — Normalize Level-Up Notification State
```
