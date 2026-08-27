# TD07.3.4.2 — Canonical Authoring Asset Identity

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.4 — Authoring Identity Normalization**  
Characterization validée : `01d7f2f753132632908b8650e08e523af2803f4d`  
Statut : **VALIDÉ — TD07.3.4.3 ACTIVE**

## 1. Objet

TD07.3.4.2 donne aux principaux DataAssets RPG une identité AssetManager déterministe fondée sur leur ID métier.

Cette tranche ne modifie pas encore :

```text
FGridCharacterInventoryState
SaveGame v20
création personnage
Story Companion
UI
```

Elle crée uniquement le resolver canonique nécessaire à la normalisation .3.

## 2. PrimaryAssetIds canoniques

```text
URPGClassAsset
    ClassId = Fighter
    -> RPGClass:Fighter

URPGRaceAsset
    RaceId = Human
    -> RPGRace:Human

URPGClassVisualAsset
    ClassId = Fighter
    -> RPGClassVisual:Fighter

URPGCharacterPortraitSetAsset
    RaceId = Human
    -> RPGPortraitSet:Human
```

L'identité est indépendante du nom physique du UObject / .uasset.

## 3. Fallback pour définition incomplète

Si l'ID métier est `NAME_None`, chaque DataAsset conserve le comportement standard :

```cpp
return Super::GetPrimaryAssetId();
```

Le resolver strict, lui, refuse toujours `NAME_None`.

## 4. Resolver unique

Nouveau service :

```text
FRPGAuthoringIdentityResolver
```

API :

```text
MakeClassPrimaryAssetId(ClassId)
MakeRacePrimaryAssetId(RaceId)
MakeClassVisualPrimaryAssetId(ClassId)
MakePortraitSetPrimaryAssetId(RaceId)

ResolveClassById(ClassId)
ResolveRaceById(RaceId)
ResolveClassVisualByClassId(ClassId)
ResolvePortraitSetByRaceId(RaceId)
```

Le resolver construit le PrimaryAssetId canonique, consulte AssetManager, scanne /Game pour le type concerné si nécessaire, charge le DataAsset, puis vérifie la définition, l'ID métier et l'identité primaire exacte.

## 5. Types AssetManager

```text
RPGClass
RPGRace
RPGClassVisual
RPGPortraitSet
```

Ils sont centralisés dans `FRPGAuthoringIdentityResolver`.

## 6. Contrat strict

Exemple :

```text
ResolveClassById(Fighter)

accepté seulement si :
    asset valide
    asset.ClassId == Fighter
    asset.GetPrimaryAssetId() == RPGClass:Fighter
```

Même principe pour Race, ClassVisual et PortraitSet.

Un renommage physique de l'asset ne change donc pas l'identité runtime tant que l'ID métier reste stable.

## 7. Tests dédiés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_4_2.Normalization
```

Tests :

```text
ClassIdentityContract
RaceIdentityContract
VisualIdentityContract
StrictResolverContract
```

Les tests utilisent des DataAssets transients et ne dépendent d'aucun nom de fichier de contenu.

## 8. Characterization post-refactor

Le filtre historique :

```text
Grimrock.TechnicalDebt.TD07_3_4.Characterization
```

reste à quatre tests.

Après .2, le test PrimaryAssetIdentityGap vérifie que ce gap est fermé. Les autres duplications caractérisées restent intentionnellement présentes pour .3 et .4.

## 9. Save contract

Aucun changement de schéma dans cette sous-tranche :

```text
CurrentSaveVersion = 20
```

La v21 ne sera ouverte qu'au moment où les références/copies du personnage deviendront transient.

## 10. Prochaine tranche

```text
TD07.3.4.3 — Character Identity State Normalization
```

Cible :

```text
durable
    ClassId
    RaceId
    PortraitGender
    PortraitVariantId

transient / rehydrated
    ClassDefinition
    ClassDisplayName
    RaceDisplayName
    Portrait
    ClassIcon
```

Ordre de load cible :

```text
deserialize IDs
-> rehydrate authoring identity caches
-> rebuild Level
-> rebuild DerivedStats
-> validate
-> remaining runtime rehydration
```

## 11. Stop condition

- [x] Class PrimaryAssetId canonique ;
- [x] Race PrimaryAssetId canonique ;
- [x] ClassVisual PrimaryAssetId canonique ;
- [x] PortraitSet PrimaryAssetId canonique ;
- [x] resolver central ajouté ;
- [x] mismatch rejeté ;
- [x] NAME_None rejeté par resolver ;
- [x] Save v20 inchangé ;
- [x] 4 tests dédiés ajoutés ;
- [x] Characterization adaptée post-refactor ;
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 post-refactor.


## 12. Validation locale

```text
Grimrock.TechnicalDebt.TD07_3_4_2.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-234554

Grimrock.TechnicalDebt.TD07_3_4.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-234629
```

TD07.3.4.2 est validé. TD07.3.4.3 peut commencer.
