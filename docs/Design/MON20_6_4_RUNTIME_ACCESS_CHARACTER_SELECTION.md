# MON20.6.4 — Runtime Access / Character Selection API

Statut : **VALIDÉ UE5.5.4 — 24/24 SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.6 — Skills Data Model & Runtime**

---

## 1. Objectif

Exposer les compétences du personnage actif sans créer de registre parallèle et sans coupler le domaine Skill à une serrure, un trigger ou une UI particulière.

Le bridge doit permettre deux modes cohérents avec l'inventaire existant :

```text
personnage explicite par CharacterIndex
personnage actuellement sélectionné
```

---

## 2. Autorité conservée

L'autorité reste exclusivement :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState
        -> ActiveCharacters
            -> FGridCharacterInventoryState
                -> SkillRanks
```

MON20.6.4 n'ajoute aucun état global `CharacterId -> Skills` et ne copie pas `SkillRanks`.

---

## 3. Service runtime

Nouveau service sans état :

```text
FRPGSkillRuntimeService
```

Il ne contient aucune donnée membre. Il résout le personnage depuis `UGridPartyInventoryComponent`, puis délègue aux services purs MON20.6.2 / MON20.6.3.

### Lecture

```text
TryGetCharacterSkillRank
TryGetSelectedCharacterSkillRank
```

Un rang absent reste `0`. Un index invalide ou un état Skill structurellement invalide renvoie `false`.

### Mutation

```text
TrySetCharacterSkillRank
TrySetSelectedCharacterSkillRank
TryIncreaseCharacterSkillRank
TryIncreaseSelectedCharacterSkillRank
```

Les mutations délèguent à `FRPGSkillService`.

Lorsqu'une mutation réussit et modifie réellement le rang :

```text
PartyInventory->NotifyPartyInventoryChanged(CharacterIndex)
```

Une opération idempotente réussie ne déclenche pas de notification artificielle.

Un index invalide est rejeté comme `InvalidCurrentState` sans mutation.

### Check

```text
TryResolveCharacterSkillCheck
TryResolveSelectedCharacterSkillCheck
```

Les checks délèguent à `FRPGSkillCheckService` avec le `FRandomStream` fourni par le caller.

Un index invalide :

```text
RejectReason = InvalidCharacterState
bResolved = false
aucun RNG consommé
```

---

## 4. Sélection de personnage

Les variantes `SelectedCharacter` utilisent exclusivement :

```text
UGridPartyInventoryComponent::GetSelectedCharacterIndex()
```

Donc toute modification de sélection via l'API existante est immédiatement reflétée dans les lectures, mutations et checks Skill.

Il n'existe pas de sélection Skill indépendante.

---

## 5. Fichiers

Ajoutés :

```text
Source/GrimrockPrototype/Public/RPG/RPGSkillRuntimeService.h
Source/GrimrockPrototype/Private/RPG/RPGSkillRuntimeService.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2064SkillRuntimeAccessTests.cpp
```

Documentation :

```text
docs/Design/MON20_6_4_RUNTIME_ACCESS_CHARACTER_SELECTION.md
```

Aucun `.uasset`, `.umap` ou changement SaveGame.

---

## 6. Automation

Tests ajoutés :

```text
RuntimeExplicitRank
RuntimeSelectedRank
RuntimeSelectedMutation
RuntimeIncreaseMutation
RuntimeInvalidMutationAtomic
RuntimeExplicitCheck
RuntimeSelectedCheck
RuntimeInvalidCharacterCheckNoRandom
```

Le filtre cumulatif reste :

```text
Grimrock.MON20.6.Skills
```

Validation reçue sous UE5.5.4 le 24 août 2026 :

```text
24 / 24 Success
0 Fail
0 Error
```

Les 16 tests MON20.6.2 + MON20.6.3 sont également restés verts dans la même exécution cumulative.

---

## 7. Hors scope

MON20.6.4 ne fait pas encore :

- lockpicking réel ;
- pièges ;
- UI de dépense de points ;
- RequirementIds ;
- SaveGame ;
- Lua / Event -> Command ;
- définition d'un catalogue de compétences production.

Ces intégrations restent dans MON20.8 / MON20.9 ou dans leurs consommateurs dédiés.

---

## 8. Validation UE5.5.4

Statut final :

```text
Grimrock.MON20.6.Skills
24 / 24 Success
0 Fail
0 Error
```

Aucun PIE n'est requis : MON20.6.4 n'introduit ni UI ni interaction monde.

---

## 9. Suite

```text
MON20.6.5 — Automation Regression / Closure
```

La passe cumulative 24/24 sert de régression finale et permet la clôture de MON20.6 avant MON20.7.
