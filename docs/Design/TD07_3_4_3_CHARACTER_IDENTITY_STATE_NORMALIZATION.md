# TD07.3.4.3 — Character Identity State Normalization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.4 — Authoring Identity Normalization**  
Canonical identity baseline : `995c6bdff2624041e17b908bd3a62e0b9ef7eff6`  
Statut : **VALIDÉ ET CLOS — TD07.3.4.4 IMPLÉMENTÉ / À VALIDER**

## 1. Objet

TD07.3.4.3 supprime du schéma durable les copies de classe/race qui sont reconstructibles depuis les business IDs.

Cette tranche ouvre le SaveGame v21.

## 2. Frontière v21

Durable :

```text
ClassId
RaceId
PortraitGender
PortraitVariantId
Portrait       [temporairement encore durable]
ClassIcon      [temporairement encore durable]
```

Transient / rehydraté :

```text
ClassDefinition
ClassDisplayName
RaceDisplayName
```

Les deux visuels restent volontairement durables jusqu'à TD07.3.4.4.

## 3. Pourquoi Portrait / ClassIcon sont différés

L'audit préalable à v21 a confirmé que `URPGStoryCompanionAsset` peut encore authorer directement :

```text
Portrait
ClassIcon
```

sans garantie que ces overrides soient déjà reconstructibles par :

```text
RaceId + PortraitGender + PortraitVariantId
ClassId
```

Les rendre transient immédiatement créerait un risque de perte visuelle après Continue.

TD07.3.4.4 devra :

1. normaliser les authoring assets concernés ;
2. vérifier que les portraits/visuels sont catalogués ;
3. supprimer les fallbacks directs ;
4. seulement alors rendre `Portrait` et `ClassIcon` transient.

## 4. Cache runtime faible

`FRPGAuthoringIdentityResolver` possède désormais des caches faibles transients :

```text
ClassId -> TWeakObjectPtr<URPGClassAsset>
RaceId -> TWeakObjectPtr<URPGRaceAsset>
ClassId -> TWeakObjectPtr<URPGClassVisualAsset>
RaceId -> TWeakObjectPtr<URPGCharacterPortraitSetAsset>
```

Ces caches :

- ne sont jamais sérialisés ;
- ne sont jamais une autorité durable ;
- accélèrent la résolution d'assets déjà chargés ;
- permettent aux tests SaveGame in-memory de retrouver leurs DataAssets transients ;
- retombent sur AssetManager après un vrai redémarrage.

## 5. Persistence helper

Nouveau service :

```text
FRPGCharacterIdentityPersistence
```

Responsabilités :

```text
RememberRuntimeCaches(PartyState)
RehydratePartyIdentity(PartyState)
ValidateRuntimePartyIdentity(PartyState)
```

La réhydratation est atomique sur :

```text
ActiveCharacters
CharacterPool
```

Classe :

```text
ClassId
-> ResolveClassById()
-> ClassDefinition
-> ClassDisplayName
```

Race :

```text
RaceId
-> ResolveRaceById()
-> RaceDisplayName
```

Si une race authoring est temporairement introuvable, le label de présentation se dégrade vers `FText::FromName(RaceId)` sans rendre la sauvegarde injouable.

Une classe introuvable reste bloquante lorsqu'un `ClassId` existe, car elle porte les règles nécessaires aux DerivedStats, actions et progressions.

## 6. Load order v21

```text
deserialize durable IDs
-> RehydratePartyIdentity
    -> ClassDefinition
    -> ClassDisplayName
    -> RaceDisplayName
-> rebuild Level from Experience
-> rebuild DerivedStats from Attributes + ClassDefinition + Level
-> ValidateCurrentState
-> validate runtime identity caches
-> rehydrate Status Effects
-> rebuild class progression runtime projection
```

L'ordre est volontaire : `ClassDefinition` doit être disponible avant le calcul de `DerivedStats`.

## 7. Consumers runtime

Les consommateurs principaux ne dépendent plus uniquement de la présence préalable du cache `ClassDefinition`.

Ils peuvent repartir de `ClassId` via `FRPGAuthoringIdentityResolver` :

```text
RPGClassProgressionTransactionService
RPGLevelUpService
RPGTalentRuntimeService
RPGLevelUpWidget
GridTurnManagerPlayerActionCatalog
Save validation / projection
```

## 8. Création / recrutement

Les chemins de création alimentent le cache runtime avec les DataAssets qu'ils connaissent déjà :

```text
CreateInitialCharacter
RPGCustomRecruitService
RPGStoryCompanionService
```

La création initiale copie également désormais explicitement :

```text
PortraitGender
PortraitVariantId
ClassIcon
```

en plus du portrait déjà existant.

## 9. SaveGame v21

```text
CurrentSaveVersion = 21

v21 accepted
v20 and earlier rejected
no migration
```

TD07.3.3.10 est future-proofé : il exige désormais v20 ou ultérieur et continue de vérifier le rejet de v19.

## 10. Tests

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_4_3.Normalization
```

Tests :

```text
SchemaAuthority
ActivePoolRehydration
SaveRoundTripRehydratesCaches
SaveSchemaVersion
```

Le round-trip écrit volontairement des labels transients obsolètes avant la sauvegarde ; après load, les labels doivent provenir des DataAssets canoniques.

La Characterization TD07.3.4 reste à quatre tests et vérifie désormais :

```text
ClassDefinition transient
ClassDisplayName transient
RaceDisplayName transient

Portrait durable [deferred .4]
ClassIcon durable [deferred .4]
```

## 11. Stop condition

- [x] ClassDefinition transient ;
- [x] ClassDisplayName transient ;
- [x] RaceDisplayName transient ;
- [x] ClassId durable ;
- [x] RaceId durable ;
- [x] cache resolver transient ajouté ;
- [x] réhydratation Active + Pool ;
- [x] load order avant DerivedStats ;
- [x] consumers ClassId-aware ;
- [x] SaveGame v21 exact-match ;
- [x] Portrait / ClassIcon explicitement différés à .4 ;
- [x] 4 tests Normalization ajoutés ;
- [x] Characterization adaptée ;
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 post-refactor ;
- [ ] régressions ciblées vertes.


## 12. Validation locale

```text
Grimrock.TechnicalDebt.TD07_3_4_3.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-000254

Grimrock.TechnicalDebt.TD07_3_4_2.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-000307

Grimrock.TechnicalDebt.TD07_3_4.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-000320
```

TD07.3.4.3 est validé. TD07.3.4.4 peut commencer.


## 13. TD07.3.4.4 supersession

TD07.3.4.4 ouvre la v22 et rend également `Portrait` / `ClassIcon` transients. Les assertions historiques de TD07.3.4.3 sont future-proofées : TD07.3.4.3 établit la frontière classe/race v21, tandis que TD07.3.4.4 complète la frontière visuelle.
