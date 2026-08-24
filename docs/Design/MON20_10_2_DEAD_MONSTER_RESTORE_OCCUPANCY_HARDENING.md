# MON20.10.2 — Dead Monster Restore Occupancy Hardening

Date : **24 août 2026**  
Statut : **IMPLÉMENTÉ — EN VALIDATION UE5.5.4**  
Jalon parent : **MON20.10 — Balance / Regression / Closure**

---

## 1. Objectif

Corriger la régression observée pendant le smoke test Save/Continue de MON20.9 : un monstre persistant déjà mort pouvait ne pas être recréé si sa cellule sauvegardée était occupée par le groupe au moment du chargement.

Log observé :

```text
[GridMonsterSpawn] ... Reason=PartyOccupiesCell
[GridMonsterState] MissingActor ...
```

Ce comportement violait le contrat MON17.8.6 : un monstre mort restauré doit conserver son Actor runtime, mais être immédiatement canonisé en état :

```text
Dead
CurrentHealth = 0
collision désactivée
mesh caché
aucune occupation de grille
aucune présentation de mort rejouée
```

---

## 2. Cause racine

`AGridLevelRuntimeActor::AddMonsterSpawnActor()` appliquait les règles d'occupation d'un monstre vivant avant d'appeler :

```cpp
AGridMonsterActor::RestoreRuntimeMonsterState()
```

L'ordre précédent était donc :

```text
RestoreState fournit la cellule sauvegardée
    -> GeneratedMonsterCellConflict
    -> PartyOccupiesCell
    -> MonsterOccupancyConflict
    -> InitializeMovement / RegisterMonster
    -> seulement ensuite RestoreRuntimeMonsterState
```

Le snapshot mort était ainsi rejeté avant que le pipeline MON17.8.6 puisse libérer l'occupation et cacher le monstre.

---

## 3. Correction runtime

Fichier :

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp
```

`AddMonsterSpawnActor()` détermine désormais si le snapshot sera restauré canoniquement mort avec exactement le même contrat logique que `RestoreRuntimeMonsterState()` :

```cpp
RestoreState &&
(
    RestoreState->bIsDead ||
    RestoreState->CurrentHealth <= 0 ||
    RestoreState->MonsterState == EGridMonsterState::Dead
)
```

### Snapshot vivant ou spawn frais

Le comportement reste inchangé :

```text
GeneratedMonsterCellConflict -> rejet
PartyOccupiesCell             -> rejet
MonsterOccupancyConflict      -> rejet
InitializeMovement / occupancy obligatoire
```

### Snapshot mort

Les contrôles d'occupation préalables sont ignorés et aucune occupation n'est enregistrée avant le restore :

```text
spawn Actor runtime
    -> pas d'occupation préalable
    -> RestoreRuntimeMonsterState
    -> RestoreCommittedDeathState
    -> Dead + hidden + no collision + no occupancy
```

Le changement ne permet donc jamais à un monstre vivant de partager la cellule du groupe.

---

## 4. Régression Automation ajoutée

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/
    GridMonsterMON20102DeadRestoreOccupancyTests.cpp
```

Filtre :

```text
Grimrock.MON20.10.2
```

Tests :

```text
Grimrock.MON20.10.2.DeadRestoreOverPartyCell
Grimrock.MON20.10.2.LivingRestoreStillRejectsPartyCell
```

### DeadRestoreOverPartyCell

Le test construit :

```text
MonsterSpawn persistant
+ snapshot MonsterPlacements bIsSpawned=true
+ snapshot MonsterState Dead
+ PartyPawn sur exactement la même cellule
```

Puis exécute `RebuildLevel()` et vérifie :

```text
spawn failure count = 0
Actor mort présent
CurrentHealth = 0
mesh caché
collision désactivée
aucune occupation de grille
ApplyCurrentLevelRuntimeState() accepté
Actor toujours présent et non occupant après application complète
```

### LivingRestoreStillRejectsPartyCell

Le même scénario est rejoué avec un snapshot vivant.

Attendu :

```text
Reason=PartyOccupiesCell
spawn failure count = 1
aucun Actor créé
```

Ce second test protège explicitement contre un relâchement involontaire des règles de collision/occupation des monstres vivants.

---

## 5. Impact sur la campagne MON20

Baseline avant MON20.10 :

```text
Grimrock.MON20   149 tests
```

MON20.10.2 ajoute 2 tests, portant la cible finale connue à :

```text
Grimrock.MON20   151 tests
```

La campagne complète 151/151 est réservée à MON20.10.4. Pour cette tranche, la validation ciblée attendue est :

```text
Grimrock.MON20.10.2   2/2 Success
0 Fail
0 Error
```

Une régression de sécurité MON17.8 est également recommandée :

```text
Grimrock.Monsters.MON17.8   8/8 Success
```

---

## 6. Fichiers modifiés

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp
Source/GrimrockPrototype/Private/Tests/GridMonsterMON20102DeadRestoreOccupancyTests.cpp
docs/Design/MON20_10_2_DEAD_MONSTER_RESTORE_OCCUPANCY_HARDENING.md
```

Aucun `.uasset` et aucun `.umap` ne sont modifiés.

---

## 7. Critères de validation

```text
[ ] Projet compile sous UE5.5.4
[ ] Grimrock.MON20.10.2 = 2/2 Success
[ ] Grimrock.Monsters.MON17.8 = 8/8 Success
[ ] aucun Fail / Error Automation inattendu
```

Après validation, MON20.10.2 pourra être marqué **VALIDÉ UE5.5.4** puis le travail passera à :

```text
MON20.10.3 — Log Hygiene / Known Diagnostics
```
