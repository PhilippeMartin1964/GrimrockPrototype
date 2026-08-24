# MON20.6.3 — Deterministic Skill Check Resolution

Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
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

La formule MON20.6.3 est :

```text
Total = d20 + SkillRank + AttributeModifier
Success = Total >= Difficulty
```

Le `d20` est obtenu par :

```cpp
RandomStream.RandRange(1, 20)
```

Le modificateur d'attribut réutilise la règle existante :

```text
URPGCharacterRulesLibrary::GetAttributeModifier
= floor((AttributeValue - 10) / 2)
```

Exemple :

```text
Dexterity = 14 -> +2
SkillRank  = 3
Roll       = 11
Total      = 16
DC         = 15
Résultat   = Success
```

---

## 4. Attribut directeur

`ERPGSkillGoverningAttribute` sélectionne l'un des six attributs du `CharacterState` :

```text
Strength
Dexterity
Constitution
Intelligence
Wisdom
Charisma
```

Pour :

```text
GoverningAttribute = None
```

le résultat expose :

```text
AttributeValue    = 0
AttributeModifier = 0
```

Le test dépend alors uniquement du d20 et du rang.

---

## 5. Politique des jets naturels

MON20.6.3 n'introduit volontairement aucune règle spéciale pour 1 naturel ou 20 naturel.

Donc :

```text
1 naturel  -> pas d'échec automatique
20 naturel -> pas de réussite automatique
```

La réussite reste exclusivement :

```text
Total >= Difficulty
```

Cette règle garde les DC prévisibles et évite de figer prématurément une convention de critique pour les compétences hors combat.

---

## 6. Compétences non entraînées

Si :

```text
Rank = 0
bAllowUntrainedChecks = true
```

le test est résolu normalement avec rang zéro.

Si :

```text
Rank = 0
bAllowUntrainedChecks = false
```

le test est rejeté avec :

```text
ERPGSkillCheckRejectReason::UntrainedNotAllowed
```

Le rejet ne consomme pas le `FRandomStream`.

---

## 7. Rejets

`ERPGSkillCheckRejectReason` contient :

```text
None
InvalidDefinition
InvalidCharacterState
InvalidDifficulty
UntrainedNotAllowed
```

Une difficulté doit être strictement positive.

Un `CharacterState` est rejeté si son état Skill sparse est invalide ou si le rang de la compétence dépasse `SkillDefinition.MaxRank`.

Tous les rejets ont deux propriétés importantes :

```text
bResolved = false
aucun tirage RNG consommé
```

Cela garantit qu'un échec de validation ne décale pas une séquence déterministe ultérieure.

---

## 8. Résultat inspectable

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

Ce résultat pourra être consommé plus tard par :

- lockpicking ;
- détection/désamorçage de pièges ;
- exploration ;
- dialogue ;
- Event -> Command / Lua ;
- UI de feedback.

Aucun de ces consommateurs n'est ajouté dans MON20.6.3.

---

## 9. Automation

Tests ajoutés :

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

Le filtre cumulatif reste :

```text
Grimrock.MON20.6.Skills
```

Après MON20.6.3, total attendu :

```text
16 / 16 Success
0 Fail
0 Error
```

Les 8 tests MON20.6.2 doivent rester verts.

---

## 10. Fichiers

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

## 11. Validation demandée

Après compilation `GrimrockPrototypeEditor`, exécuter :

```text
Grimrock.MON20.6.Skills
```

Attendu :

```text
16 / 16 Success
0 Fail
0 Error
```

Aucun PIE n'est nécessaire pour cette tranche purement logique.

---

## 12. Suite

Après validation :

```text
MON20.6.4 — Runtime Access / Character Selection API
```

Cette étape exposera proprement les rangs et checks pour un personnage actif sélectionné, sans encore coupler le système à une serrure particulière.
