# TD07.3.4 — Authoring Identity Normalization — Closure

Date : **28 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Statut : **VALIDÉ ET CLOS**  
SaveGame final : **v22 exact-match**

## 1. Résultat

TD07.3.4 supprime les autorités parallèles entre identité métier, références de DataAssets et copies de présentation du personnage.

```text
Durable identity
    ClassId
    RaceId
    PortraitGender
    PortraitVariantId

Transient / reconstructed
    ClassDefinition
    ClassDisplayName
    RaceDisplayName
    Portrait
    ClassIcon
```

## 2. Canonical authoring identity

```text
URPGClassAsset
    RPGClass:<ClassId>

URPGRaceAsset
    RPGRace:<RaceId>

URPGClassVisualAsset
    RPGClassVisual:<ClassId>

URPGCharacterPortraitSetAsset
    RPGPortraitSet:<RaceId>
```

## 3. Authoring cleanup

`URPGStoryCompanionAsset` ne contient plus de copies de `Portrait` ou `ClassIcon`.

Les créations/recrutements peuvent conserver des soft references dans leurs requêtes transientes, mais celles-ci ne sont plus des autorités Save.

## 4. Runtime reconstruction

`FRPGAuthoringIdentityResolver` et `FRPGCharacterIdentityPersistence` reconstruisent pour Active + Pool :

```text
ClassDefinition
ClassDisplayName
RaceDisplayName
Portrait
ClassIcon
```

avant les projections dépendantes comme `DerivedStats`.

## 5. Save contract

```text
CurrentSaveVersion = 22
v22 accepted
v21 and earlier rejected
no backward migration
```

## 6. Validation

Gates TD07.3.4 :

```text
TD07.3.4.4 Normalization   4/4
TD07.3.4.3 Normalization   4/4
TD07.3.4.2 Normalization   4/4
TD07.3.4 Characterization  4/4
--------------------------------
TOTAL                     16/16
Warnings                     0
Failures                     0
```

Régressions fonctionnelles :

```text
TOTAL                     124/124
Warnings                     0
Failures                     0
Not run                      0
```

Dernier rerun :

```text
Grimrock.MON20.4.RecruitmentUI
Succeeded              : 18
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260828-002908
```

## 7. Shipping

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : Saved/Packaging/TD04/TD04-Shipping-20260828-002416/Windows/GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 906000955
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260828-002416
[OK] Cook / package validated.
```

## 8. Stop condition

- [x] one durable identity authority per class/race/portrait datum;
- [x] canonical PrimaryAssetIds;
- [x] no persisted class/race presentation copies;
- [x] no persisted Portrait/ClassIcon copies;
- [x] Active + Pool rehydration;
- [x] Story Companion visual duplicates removed;
- [x] SaveGame v22 exact-match;
- [x] gates 16/16;
- [x] regressions 124/124;
- [x] zero Automation warnings;
- [x] Shipping Win64 green.

## 9. Next

```text
TD07.3.5 — Combat Data Schema Reset
```
