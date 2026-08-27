# TD07.3.4 — Authoring Identity Normalization — Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3 — Prototype Data Model Reset**  
Baseline : `8193fae0c704fbef3faf8c9981bf65d2b248f9e1`  
Statut : **CHARACTERIZATION VALIDÉE — TD07.3.4.2 IMPLÉMENTÉ / À VALIDER**

## 1. Contexte

TD07.3.3 est clos sur le schéma Save v20.

La frontière personnage est désormais propre pour l'état gameplay, mais plusieurs identités de contenu sont encore représentées simultanément par :

```text
stable ID
definition asset reference
display copy
visual copy
```

TD07.3.4 traite cette duplication sans réouvrir la normalisation gameplay de TD07.3.3.

## 2. Duplication courante dans FGridCharacterInventoryState

Classe :

```text
ClassId
ClassDisplayName
ClassDefinition
```

Race :

```text
RaceId
RaceDisplayName
```

Portrait :

```text
PortraitGender
PortraitVariantId
Portrait
```

Visuel de classe :

```text
ClassId
ClassIcon
```

Tous ces champs sont actuellement non-transient et participent donc au durable v20.

## 3. Authoring sources existantes

Classe gameplay :

```text
URPGClassAsset
    ClassId
    DisplayName
    Description
    progression / combat / stats
```

Race :

```text
URPGRaceAsset
    RaceId
    DisplayName
    Description
    AttributeBonuses
```

Portrait :

```text
URPGCharacterPortraitSetAsset
    RaceId
    MalePortraits[]
    FemalePortraits[]

FRPGCharacterPortraitVariant
    VariantId
    DisplayName
    Portrait
```

Visuel de classe :

```text
URPGClassVisualAsset
    ClassId
    DisplayName
    ClassIcon
    AccentColor
```

Story Companion :

```text
URPGStoryCompanionAsset
    RaceDefinition
    ClassDefinition
    PortraitGender
    PortraitVariantId
    Portrait
    ClassIcon
```

Le Story Companion authoré duplique donc encore certains visuels que les catalogues Race/Class peuvent déjà fournir.

## 4. Copies lors de la création

Les deux chemins de création principaux copient actuellement des valeurs de présentation dans le personnage :

```text
RPGCustomRecruitService
RPGStoryCompanionService
```

Ils écrivent notamment :

```text
RaceDisplayName
ClassDisplayName
Portrait
ClassIcon
```

Ces copies vieillissent indépendamment de leurs DataAssets sources.

## 5. UI — double source ClassIcon

Le widget Inventory possède déjà le bon sens de résolution :

```text
ClassId
    -> URPGClassVisualAsset
    -> ClassIcon
```

mais conserve un fallback :

```text
Character.ClassIcon
```

Le visuel de classe a donc aujourd'hui deux sources possibles.

## 6. Gap PrimaryAsset identity

À la baseline de Characterization, contrairement aux Status Effects, les DataAssets suivants n'exposaient pas encore un PrimaryAssetId canonique fondé sur l'identité métier :

```text
URPGClassAsset              ClassId
URPGRaceAsset               RaceId
URPGClassVisualAsset        ClassId
URPGCharacterPortraitSetAsset RaceId
```

Leur `UPrimaryDataAsset::GetPrimaryAssetId()` par défaut n'est pas garanti égal à :

```text
RPGClass:<ClassId>
RPGRace:<RaceId>
RPGClassVisual:<ClassId>
RPGPortraitSet:<RaceId>
```

Par conséquent, supprimer aujourd'hui `ClassDefinition` ou les références visuelles du personnage sans établir d'abord un resolver canonique créerait une régression de chargement.

### Mise à jour TD07.3.4.2

Le gap PrimaryAsset identity est désormais implémenté :

```text
RPGClass:<ClassId>
RPGRace:<RaceId>
RPGClassVisual:<ClassId>
RPGPortraitSet:<RaceId>
```

La Characterization post-refactor exige maintenant ces identités canoniques.

## 7. Duplications acceptables

Ne sont pas considérés comme autorités durables concurrentes :

```text
FRPGCharacterCreationRequest
FRPGCharacterVisualSelection
FGridInventoryCharacterSummary
widget AvailableClassDefinitions / AvailableRaceDefinitions
widget AvailablePortraitSets / AvailableClassVisuals
```

Ce sont des contextes de transaction, read models ou catalogues authoring/transient.

Le champ `CombatActionSourceClassDefinition` de la requête de création reste également un mécanisme transient de preview et n'est pas une autorité Save.

## 8. Cible TD07.3.4

### Runtime / Save durable

```text
ClassId
RaceId
PortraitGender
PortraitVariantId
```

### Caches / projections rehydratables

```text
ClassDefinition
ClassDisplayName
RaceDisplayName
Portrait
ClassIcon
```

Ces champs peuvent rester physiquement présents pendant une première normalisation s'ils deviennent `Transient`; leur suppression complète n'est pas obligatoire pour obtenir une autorité durable unique.

### Authoring

```text
URPGClassAsset
URPGRaceAsset
URPGCharacterPortraitSetAsset
URPGClassVisualAsset
URPGStoryCompanionAsset
```

Les authoring assets conservent des références de définitions, pas des snapshots runtime/save.

## 9. Séquence proposée

```text
TD07.3.4.1 — Characterization
    cartographier duplications et resolvers manquants

TD07.3.4.2 — Canonical Authoring Asset Identity
    PrimaryAssetId canonique pour Class / Race / ClassVisual / PortraitSet
    resolver strict par business ID
    unicité / mismatch rejetés

TD07.3.4.3 — Character Identity State Normalization
    ClassId / RaceId / PortraitGender / PortraitVariantId durables
    ClassDefinition / labels / Portrait / ClassIcon transient
    rehydration Active + Pool
    SaveGame v21 exact-match

TD07.3.4.4 — Companion / Creation Authoring Cleanup + Closure
    supprimer les copies authoring devenues inutiles
    réparer les assets courants si nécessaire
    régressions UI / creation / recruitment / save
    Shipping
```

## 10. Risques

### ClassDefinition

TD07.3.3.10 reconstruit `DerivedStats` avec `ClassDefinition`.

TD07.3.4.3 devra donc réhydrater la classe **avant** la reconstruction des DerivedStats lors d'un load v21.

Pipeline cible :

```text
deserialize IDs
-> resolve ClassDefinition from ClassId
-> resolve Race / labels / visuals
-> rebuild Level
-> rebuild DerivedStats
-> validate
-> other runtime rehydration
```

### Portrait

`PortraitVariantId` n'est canonique qu'à l'intérieur du couple :

```text
RaceId + PortraitGender
```

Le resolver doit utiliser les trois dimensions :

```text
RaceId
PortraitGender
PortraitVariantId
```

### Class visual

`ClassIcon` doit provenir du `URPGClassVisualAsset` associé à `ClassId`.

Aucun fallback durable vers une icône copiée dans le personnage ne devra subsister après normalisation.

## 11. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_4.Characterization
```

Tests :

```text
CharacterDurableIdentityDuplication
PrimaryAssetIdentityGap
CreationCopiesPresentationState
VisualFallbackDuplication
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 12. Stop condition du gate

- [x] duplication durable personnage cartographiée ;
- [x] sources authoring identifiées ;
- [x] contexts/read models transient exclus de l'autorité ;
- [x] double source ClassIcon caractérisée ;
- [x] gap PrimaryAsset identity démontré ;
- [x] cible durable/transient fixée ;
- [x] ordre de rehydration v21 documenté ;
- [x] séquence TD07.3.4.1–.4 définie ;
- [x] 4 tests ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] 4/4 tests verts.


## 13. Validation locale

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_4.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-233658
```

Le gate est atteint. TD07.3.4.2 — Canonical Authoring Asset Identity peut commencer.
