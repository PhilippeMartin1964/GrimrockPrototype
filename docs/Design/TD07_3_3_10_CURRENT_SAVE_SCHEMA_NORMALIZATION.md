# TD07.3.3.10 — Current Save Schema / Regressions / Closure Normalization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Characterization validée : `c050f608410653b75712eecf938b8056d3d49c4a`  
Statut : **VALIDÉ ET CLOS — TD07.3.3 CLOS**

## 1. Objet

TD07.3.3.10 finalise la frontière durable/transient de l'état personnage.

Le dernier reliquat identifié après TD07.3.3.9 était :

```text
FGridCharacterInventoryState::DerivedStats
```

Cette structure ne contient plus que des valeurs calculables, mais restait sérialisée.

## 2. Normalisation

Avant :

```text
Attributes       durable
Experience       durable
Level            transient
DerivedStats     durable malgré son caractère calculable
Resources        durable mutable
```

Après :

```text
Attributes       durable
Experience       durable
Level            transient
DerivedStats     transient
Resources        durable mutable
```

Déclaration :

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "RPG")
FRPGDerivedStats DerivedStats;
```

## 3. Reconstruction au chargement

Le pipeline v20 devient :

```text
Super::Serialize(load)
    -> rebuild Level from Experience
    -> resolve ClassDefinition
    -> rebuild DerivedStats from Attributes + ClassDefinition + Level
    -> validate durable character state
    -> rehydrate Status Effect definition caches
    -> rebuild class progression runtime projection
```

La reconstruction s'applique à :

```text
ActiveCharacters
CharacterPool
```

## 4. Resources

`FRPGCharacterResources` reste durable.

Aucun recalcul de :

```text
CurrentHealth
CurrentMana
CurrentPhysicalArmor
CurrentMagicalArmor
```

n'est effectué pendant la reconstruction de `DerivedStats`.

Cela préserve les dégâts, soins, dépenses de mana et armures courantes.

## 5. SaveGame v20

```text
CurrentSaveVersion = 20

v20
    -> DerivedStats absent du durable
    -> Level absent du durable
    -> reconstruction après load

v19 et antérieures
    -> rejet exact-match
    -> aucune migration
```

## 6. Snapshots personnage annexes

Toujours absents :

```text
ClassProgressionStates
CharacterSkillStates
CharacterSpellbookStates
CharacterStatusEffectStates
PendingLevelUpNotifications
```

L'enveloppe durable personnage reste exclusivement :

```text
PartyInventoryState
    ActiveCharacters[]
    CharacterPool[]
```

## 7. Tests

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_10.Normalization
```

Tests :

```text
SchemaAuthority
ActiveDerivedStatsRoundTrip
PoolDerivedStatsRoundTrip
SaveSchemaVersion
```

Le round-trip corrompt volontairement `DerivedStats` avant la sauvegarde et vérifie que la corruption ne traverse pas l'archive.

La Characterization post-refactor reste également à exécuter :

```text
Grimrock.TechnicalDebt.TD07_3_3_10.Characterization
```

## 8. Régressions finales requises

La clôture TD07.3.3 exigera une campagne couvrant au minimum :

```text
TD07.3.3.2 à TD07.3.3.10
TD07.3.2 SaveContract
MON15 progression
MON16 status effects
MON18 magic/save
CharacterCreation Save round-trip
MON20 recruitment
```

puis Win64 Shipping.

## 9. Frontière TD07.3.4

Non modifiés :

```text
ClassId + ClassDefinition + ClassDisplayName
RaceId + RaceDisplayName
PortraitVariantId + Portrait
ClassId + ClassIcon
```

Ces sujets appartiennent toujours à :

```text
TD07.3.4 — Authoring Identity Normalization
```

## 9.1 Validation locale — gates finaux

Validation du 27 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_3_10.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-231740

Grimrock.TechnicalDebt.TD07_3_3_10.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-231753
```

Les deux gates finaux sont verts. Il reste la campagne de régression globale puis le Win64 Shipping.

## 9.2 Campagne finale TD07.3.3

Campagne de clôture du 27 août 2026 :

```text
TD07.3.3 complet                          71
TD07.3.2 SaveContract                      6
MON15 progression                         34
MON16 Status Effects                      80
MON20 Skills                              47
MON18 Magic / Spellbook / Save            40
CharacterCreation CC5                      1
MON20 Recruitment                         35
--------------------------------------------
TOTAL                                    314

Succeeded                                314
Succeeded with warnings                    0
Failed                                     0
Not run                                    0
```

Rapport noyau TD07.3.3 :

```text
Saved/Automation/TD04/TD04-20260827-232129
71/71
```

Dernier filtre de la campagne :

```text
Grimrock.MON20.5.CustomRecruit
23/23
Report : Saved/Automation/TD04/TD04-20260827-232617
```

## 9.3 Shipping final v20

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : Saved/Packaging/TD04/TD04-Shipping-20260827-232723/Windows/GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 905473595
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260827-232723
[OK] Cook / package validated.
```

La stop condition TD07.3.3 est entièrement atteinte.

## 10. Stop condition

- [x] DerivedStats marqué transient ;
- [x] reconstruction Active ajoutée ;
- [x] reconstruction CharacterPool ajoutée ;
- [x] Resources conservé durable ;
- [x] SaveGame v20 exact-match ;
- [x] v19 rejeté sans migration ;
- [x] tests Normalization ajoutés ;
- [x] Characterization réécrite post-refactor ;
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 post-refactor ;
- [x] régressions finales vertes ;
- [x] Shipping Win64 vert ;
- [x] TD07.3.3 clos.
