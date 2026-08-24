# MON20.6.2 — Skill Definition + Character Runtime Ranks

Statut : **VALIDÉ UE5.5.4 — 8/8 SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.6 — Skills Data Model & Runtime**

---

## 1. Objectif

Introduire le premier socle concret du domaine Skill défini par MON20.6.1 :

- une définition data-driven de compétence ;
- un rang numérique sparse par personnage ;
- un service pur de lecture et mutation ;
- des rejets atomiques ;
- aucune migration SaveGame ni intégration UI prématurée.

---

## 2. Fichiers ajoutés

```text
Source/GrimrockPrototype/Public/RPG/RPGSkillTypes.h
Source/GrimrockPrototype/Public/RPG/RPGSkillAsset.h
Source/GrimrockPrototype/Public/RPG/RPGSkillService.h
Source/GrimrockPrototype/Private/RPG/RPGSkillAsset.cpp
Source/GrimrockPrototype/Private/RPG/RPGSkillService.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2062SkillRuntimeTests.cpp
```

Fichier étendu :

```text
Source/GrimrockPrototype/Public/Runtime/GridInventoryTypes.h
```

Aucun `.uasset` / `.umap` n'est requis pour cette tranche.

---

## 3. Définition d'une compétence

`URPGSkillAsset : UPrimaryDataAsset` contient :

```text
SkillId
DisplayName
Description
GoverningAttribute
MaxRank
bAllowUntrainedChecks
```

Une définition est structurellement valide lorsque :

```text
SkillId != None
MaxRank > 0
```

L'attribut directeur est défini par :

```text
ERPGSkillGoverningAttribute
    None
    Strength
    Dexterity
    Constitution
    Intelligence
    Wisdom
    Charisma
```

---

## 4. Rang runtime par personnage

Le rang sparse est :

```text
FRPGSkillRank
    SkillId
    Rank
```

`FGridCharacterInventoryState` possède désormais :

```text
SkillRanks : TArray<FRPGSkillRank>
```

avec propriété `Transient`.

Règles :

```text
rang absent -> 0
rang > 0    -> une entrée sparse
rang = 0    -> aucune entrée physique
SkillId dupliqué -> état structurellement invalide
```

Cette localisation maintient les compétences dans l'autorité de personnage existante :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState
        -> FGridCharacterInventoryState
            -> SkillRanks
```

Aucune map statique `CharacterId -> Skills` n'est introduite.

---

## 5. FRPGSkillService

API :

```text
ValidateSkillState
GetSkillRank
TrySetSkillRank
TryIncreaseSkillRank
```

### Mutation atomique

Avant toute mutation, le service valide :

- la définition ;
- la structure actuelle des rangs ;
- la plage `0..MaxRank` ;
- le delta positif pour `TryIncreaseSkillRank`.

Un échec ne modifie jamais `SkillRanks`.

### Rang zéro

```text
TrySetSkillRank(..., 0)
```

supprime l'entrée sparse correspondante.

### Opération idempotente

Assigner le rang déjà présent est accepté mais `bChanged == false`.

---

## 6. Persistance volontairement différée

`SkillRanks` est `Transient` dans MON20.6.

Donc :

- `UGrimrockPartySaveGame::CurrentSaveVersion` reste **7** ;
- aucune migration n'est ajoutée ;
- les anciennes sauvegardes ne sont pas modifiées ;
- MON20.9 introduira le snapshot persistant keyed by `CharacterId`.

---

## 7. Automation ajoutée

Filtre :

```text
Grimrock.MON20.6.Skills
```

Tests MON20.6.2 :

```text
DefinitionValidation
DefaultRuntimeState
SetRank
IncreaseRank
MaxRankAtomicReject
NegativeRankAtomicReject
ZeroRankRemovesSparseEntry
DuplicateSkillStateRejected
```

---

## 8. Validation UE5.5.4 — VALIDÉE

Validation utilisateur du **24 août 2026** avec le filtre :

```text
Grimrock.MON20.6.Skills
```

Résultat observé :

```text
DefaultRuntimeState             Success
DefinitionValidation            Success
DuplicateSkillStateRejected     Success
IncreaseRank                    Success
MaxRankAtomicReject             Success
NegativeRankAtomicReject        Success
SetRank                         Success
ZeroRankRemovesSparseEntry      Success
```

Bilan :

```text
8 / 8 Success
0 test en échec dans le log fourni
```

Aucune validation PIE n'est requise pour MON20.6.2 : cette tranche ne crée ni UI, ni interaction monde, ni asset d'authoring.

---

## 9. Suite

```text
MON20.6.3 — Deterministic Skill Check Resolution
```

Cette étape fixe explicitement la formule de test de compétence et utilise `FRandomStream` pour une résolution déterministe avant toute intégration aux serrures, pièges ou scripts.
