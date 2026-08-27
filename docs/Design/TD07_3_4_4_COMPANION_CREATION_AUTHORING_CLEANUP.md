# TD07.3.4.4 — Companion / Creation Authoring Cleanup

Date : **28 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.4 — Authoring Identity Normalization**  
Baseline v21 validée : `089fba0097f923d22af2cc39fdd3bb93778724c4`  
Statut : **GATE 16/16 VALIDÉ — RÉGRESSIONS / SHIPPING REQUIS**

## 1. Objet

TD07.3.4.4 ferme la dernière double autorité de présentation du personnage.

Avant cette tranche :

```text
durable
    ClassId
    RaceId
    PortraitGender
    PortraitVariantId
    Portrait
    ClassIcon

transient
    ClassDefinition
    ClassDisplayName
    RaceDisplayName
```

Après cette tranche :

```text
durable
    ClassId
    RaceId
    PortraitGender
    PortraitVariantId

transient / reconstructed
    ClassDefinition
    ClassDisplayName
    RaceDisplayName
    Portrait
    ClassIcon
```

## 2. SaveGame v22

```text
CurrentSaveVersion = 22

v22 accepted
v21 and earlier rejected
no migration
```

Le prototype reste strict exact-match.

## 3. Portrait identity

L'identité persistante d'un portrait est :

```text
RaceId
+ PortraitGender
+ PortraitVariantId
```

La texture est une projection :

```text
RPGPortraitSet:<RaceId>
    -> gender collection
    -> PortraitVariantId
    -> Portrait
```

Un chemin de texture seul n'est plus une identité persistante.

## 4. Class icon identity

L'identité persistante du visuel de classe est simplement :

```text
ClassId
```

Projection :

```text
RPGClassVisual:<ClassId>
    -> ClassIcon
    -> AccentColor
```

Le fallback historique `Character.ClassIcon` disparaît de l'UI.

## 5. Runtime visual cache

`FRPGAuthoringIdentityResolver` possède deux caches visuels transients supplémentaires :

```text
RaceId|Gender|PortraitVariantId -> Portrait soft reference
ClassId                         -> ClassIcon soft reference
```

Ils ne sont jamais sérialisés.

Ils servent uniquement :

- aux transactions de création/recrutement déjà chargées ;
- aux tests in-memory ;
- comme cache après résolution d'un DataAsset canonique.

Après un vrai redémarrage, `RPGPortraitSet` et `RPGClassVisual` restent les sources canoniques.

## 6. Story Companion cleanup

`URPGStoryCompanionAsset` ne contient plus :

```text
Portrait
ClassIcon
```

Il conserve :

```text
PortraitGender
PortraitVariantId
FullBody
RaceDefinition
ClassDefinition
```

`FullBody` reste légitime : il s'agit d'un asset narratif propre au compagnon et non d'une projection du portrait de combat/inventaire.

Le candidat runtime résout désormais :

```text
Portrait  <- RaceId + Gender + VariantId
ClassIcon <- ClassId
```

## 7. Creation / Custom Recruit

`FRPGCharacterCreationRequest::Portrait` et `ClassIcon` restent autorisés comme données transientes de transaction/preview.

Ils servent à alimenter le cache runtime pendant la création, mais ne sont plus copiés comme autorité persistante.

Le personnage conserve uniquement les IDs nécessaires à la reconstruction.

## 8. Runtime consumers

Les lectures directes sont supprimées des chemins principaux :

```text
GetCharacterSummary
GetCharacterVisualSelection
GridTurnManager initiative
GridInventoryWidget class icon
```

Le résumé inventaire et l'initiative utilisent le portrait résolu.

Le widget de classe utilise :

```text
AvailableClassVisuals
ou
FRPGAuthoringIdentityResolver
```

mais jamais un fallback durable depuis le personnage.

## 9. Character identity rehydration

`FRPGCharacterIdentityPersistence::RehydratePartyIdentity` reconstruit désormais atomiquement pour Active + Pool :

```text
ClassDefinition
ClassDisplayName
RaceDisplayName
Portrait
ClassIcon
```

Ordre v22 :

```text
deserialize durable IDs
-> rehydrate all identity/presentation caches
-> rebuild Level
-> rebuild DerivedStats
-> validate current schema
-> status effect rehydration
-> progression projection
```

## 10. Historique CC6

Le test historique `CC6.PortraitSelectionPersists` utilisait auparavant une texture sans `PortraitVariantId`.

Il est aligné sur le contrat actuel :

```text
PortraitVariantId = ElfMage
Portrait          = texture transactionnelle
```

Le portrait est ensuite projeté depuis l'identité.

## 11. Tests

Nouveau filtre :

```text
Grimrock.TechnicalDebt.TD07_3_4_4.Normalization
```

Tests :

```text
SchemaAuthority
CanonicalVisualResolver
SaveRoundTripRehydratesVisuals
AuthoringCleanup
```

La Characterization historique est également mise à jour et doit rester 4/4.

## 12. Validation requise

Gate immédiat :

```text
TD07.3.4.4 Normalization     4
TD07.3.4.3 Normalization     4
TD07.3.4.2 Normalization     4
TD07.3.4 Characterization    4
--------------------------------
TOTAL                       16
```

Puis régressions ciblées :

```text
Grimrock.CharacterCreation.CC6
Grimrock.MON20.3.StoryCompanion
Grimrock.MON20.5.CustomRecruit
TD07.3.2 SaveContract
TD07.3.3.10 Normalization
```

Enfin Shipping Win64 v22.

## 13. Stop condition TD07.3.4

- [x] business IDs canoniques ;
- [x] PrimaryAssetIds canoniques ;
- [x] ClassDefinition transient ;
- [x] labels classe/race transient ;
- [x] Portrait transient ;
- [x] ClassIcon transient ;
- [x] Story Companion duplicate Portrait removed ;
- [x] Story Companion duplicate ClassIcon removed ;
- [x] visual resolver canonique ;
- [x] Active + Pool rehydration ;
- [x] SaveGame v22 exact-match ;
- [x] 4 tests .4 ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] gate 16/16 vert ;
- [ ] régressions ciblées vertes ;
- [ ] Shipping Win64 vert ;
- [ ] TD07.3.4 clos.


## 14. Validation locale — gate TD07.3.4.4

```text
Grimrock.TechnicalDebt.TD07_3_4_4.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-001641

Grimrock.TechnicalDebt.TD07_3_4_3.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-001654

Grimrock.TechnicalDebt.TD07_3_4_2.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-001707

Grimrock.TechnicalDebt.TD07_3_4.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260828-001720
```

Gate total :

```text
16/16
Warnings 0
Failures 0
Not run 0
```

Reste : régressions ciblées puis Win64 Shipping v22.
