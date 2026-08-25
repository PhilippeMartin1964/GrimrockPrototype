# MON20.10.2 — Dead Monster Restore Occupancy Hardening

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — CLOS**  
Jalon parent : **MON20.10 — Balance / Regression / Closure**

---

## 1. Objectif

Corriger la régression observée pendant le smoke test Save/Continue de MON20.9 : un monstre persistant déjà mort pouvait ne pas être recréé si sa cellule sauvegardée était occupée par le groupe au moment du chargement.

Ancien diagnostic :

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

## 2. Cause racine

`AGridLevelRuntimeActor::AddMonsterSpawnActor()` appliquait les règles d'occupation d'un monstre vivant avant `AGridMonsterActor::RestoreRuntimeMonsterState()`.

Le snapshot mort pouvait donc être rejeté avant que le pipeline MON17.8.6 puisse libérer l'occupation et cacher le monstre.

## 3. Correction runtime

Fichier :

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp
```

Le runtime détermine désormais si le snapshot doit être restauré canoniquement mort :

```cpp
RestoreState &&
(
    RestoreState->bIsDead ||
    RestoreState->CurrentHealth <= 0 ||
    RestoreState->MonsterState == EGridMonsterState::Dead
)
```

Snapshot vivant ou spawn frais : les règles d'occupation restent inchangées et bloquantes.

Snapshot mort : les contrôles d'occupation préalables sont ignorés et aucune occupation n'est enregistrée avant le restore.

```text
spawn Actor runtime
    -> pas d'occupation préalable
    -> RestoreRuntimeMonsterState
    -> RestoreCommittedDeathState
    -> Dead + hidden + no collision + no occupancy
```

Le changement ne permet jamais à un monstre vivant de partager la cellule du groupe.

## 4. Régression Automation

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridMonsterMON20102DeadRestoreOccupancyTests.cpp
```

Tests :

```text
Grimrock.MON20.10.2.DeadRestoreOverPartyCell
Grimrock.MON20.10.2.LivingRestoreStillRejectsPartyCell
```

Validation UE5.5.4 :

```text
Grimrock.MON20.10.2   2/2 Success
Grimrock.Monsters.MON17.8   8/8 Success
0 Fail
0 Error
```

Le test mort confirme : Actor présent, HP=0, mesh caché, collision désactivée, aucune occupation et application complète du runtime state acceptée.

Le test vivant confirme que `Reason=PartyOccupiesCell` reste un rejet normal pour un monstre vivant.

## 5. Validation PIE finale

MON20.10.5 a validé le cas réel via `L_MainMenu -> Continuer` avec deux Gobelins lanceurs morts persistés dans `GrimrockParty`.

Le chargement produit :

```text
[GridSaveMigration] Load SourceVersion=8 TargetVersion=8 ... Result=Accepted
[GridMonsterState] RestoreDead ... MON_GoblinThrower ... PresentationHidden=true
[GridMonsterState] RestoreDead ... MON_GoblinThrower ... PresentationHidden=true
GridRuntimeState Apply ... Monsters=4 DeadMonsters=2
PartySave Continued Slot=GrimrockParty CharacterCount=2
```

Aucun `PartyOccupiesCell` ni `MissingActor` ne réapparaît pour ces snapshots morts.

Le runtime re-capture ensuite correctement les deux Gobelins en :

```text
State=Dead HP=0 Dead=true
```

puis resauvegarde le slot principal en version 8.

## 6. Impact campagne MON20

MON20.10.2 porte la campagne cumulative MON20 à :

```text
Grimrock.MON20   151 tests
```

La régression complète MON20.10.4 est validée :

```text
151/151 Success
0 Fail
0 Error
```

## 7. Fichiers concernés

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp
Source/GrimrockPrototype/Private/Tests/GridMonsterMON20102DeadRestoreOccupancyTests.cpp
docs/Design/MON20_10_2_DEAD_MONSTER_RESTORE_OCCUPANCY_HARDENING.md
```

Aucun `.uasset` et aucun `.umap` n'ont été requis.

## 8. Critères de clôture

```text
[OK] projet recompilé sous UE5.5.4
[OK] Grimrock.MON20.10.2 = 2/2 Success
[OK] Grimrock.Monsters.MON17.8 = 8/8 Success
[OK] campagne Grimrock.MON20 = 151/151 Success
[OK] Continue réel v8 avec 2 DeadMonsters
[OK] aucun PartyOccupiesCell / MissingActor pour snapshot mort
[OK] dead actors retenus, cachés, sans occupation
```

Conclusion : **MON20.10.2 est VALIDÉ UE5.5.4 et CLOS.**
