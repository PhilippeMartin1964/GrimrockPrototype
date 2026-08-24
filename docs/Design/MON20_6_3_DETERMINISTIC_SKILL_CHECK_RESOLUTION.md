# MON20.6.3 — Deterministic Skill Check Resolution

Statut : **VALIDÉ UE5.5.4 — 16/16 SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.6 — Skills Data Model & Runtime**

---

## 1. Objectif

Ajouter une résolution de tests de compétence pure, déterministe et réutilisable avant toute intégration aux serrures, pièges, dialogues ou scripts.

MON20.6.3 ne modifie ni SaveGame, ni UI, ni assets UE.

---

## 2. Service

Nouveau service :

```text
FRPGSkillCheckService
```

API principale :

```text
TryResolveSkillCheck(
    CharacterState,
    SkillDefinition,
    Difficulty,
    FRandomStream,
    OutResult)
```

Le caller fournit explicitement le `FRandomStream`. Aucune source aléatoire globale n'est utilisée.

---

## 3. Formule figée

```text
Total = d20 + SkillRank + AttributeModifier
Success = Total >= Difficulty
```

Le `d20` est obtenu par `RandomStream.RandRange(1, 20)` et le modificateur d'attribut réutilise `URPGCharacterRulesLibrary::GetAttributeModifier`.

Aucune règle spéciale n'est appliquée au 1 naturel ou au 20 naturel.

---

## 4. Attribut directeur

`ERPGSkillGoverningAttribute` sélectionne l'un des six attributs du personnage. Pour `None`, `AttributeValue` et `AttributeModifier` valent zéro.

---

## 5. Compétences non entraînées

```text
Rank = 0 + bAllowUntrainedChecks=true  -> test résolu
Rank = 0 + bAllowUntrainedChecks=false -> UntrainedNotAllowed
```

Le rejet ne consomme pas le `FRandomStream`.

---

## 6. Rejets

```text
None
InvalidDefinition
InvalidCharacterState
InvalidDifficulty
UntrainedNotAllowed
```

Tous les rejets conservent `bResolved=false` et ne consomment aucun tirage RNG.

---

## 7. Résultat inspectable

`FRPGSkillCheckResult` expose :

```text
bResolved
bSuccess
RejectReason
SkillId
Rank
GoverningAttribute
AttributeValue
AttributeModifier
Roll
Total
Difficulty
```

---

## 8. Automation

Tests MON20.6.3 :

```text
SkillCheckDeterministicSameSeed
SkillCheckFormula
SkillCheckDifficultyThreshold
SkillCheckUntrainedAllowed
SkillCheckUntrainedRejectedNoRandom
SkillCheckNoneAttribute
SkillCheckInvalidDifficultyNoRandom
SkillCheckInvalidStateNoRandom
```

Validation utilisateur UE5.5.4 du 24 août 2026 :

```text
16 / 16 Success
0 Fail
0 Error
```

Les huit tests MON20.6.2 sont restés verts.

---

## 9. Fichiers

Ajoutés :

```text
Source/GrimrockPrototype/Public/RPG/RPGSkillCheckService.h
Source/GrimrockPrototype/Private/RPG/RPGSkillCheckService.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2063SkillCheckTests.cpp
```

Étendu :

```text
Source/GrimrockPrototype/Public/RPG/RPGSkillTypes.h
```

Aucun `.uasset` / `.umap`.

---

## 10. Validation UE5.5.4

**VALIDÉ — 16/16 SUCCESS.**

Le journal fourni confirme les seize tests `Grimrock.MON20.6.Skills`, notamment la reproductibilité par seed, la formule, le seuil, les checks non entraînés et les rejets sans consommation RNG.

Aucun PIE n'est requis pour cette tranche purement logique.

---

## 11. Suite

```text
MON20.6.4 — Runtime Access / Character Selection API
```

Cette étape expose les rangs, mutations et checks pour un personnage actif explicite ou sélectionné, sans couplage à une serrure particulière.
