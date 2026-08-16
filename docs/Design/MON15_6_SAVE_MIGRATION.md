# MON15.6 — Save / migration de la progression RPG

Statut : **IMPLEMENTÉ — EN ATTENTE DE VALIDATION UE5.5.4**  
Date : **16 août 2026**

---

## 1. Objectif

MON15.6 rend persistante la progression introduite par MON15.1 à MON15.5 :

- cohérence `Level` / `Experience` ;
- migration des anciennes sauvegardes v1-v3 ;
- restauration exacte des choix de classe déjà confirmés ;
- sauvegarde/restauration d'un Level Up encore à présenter ;
- reconstruction immédiate des requirements MON12 après chargement ;
- absence de mutation partielle lors d'une migration invalide.

Aucun `.uasset`, `.umap` ou WBP n'est requis.

---

## 2. SaveVersion 4

Le contrat devient :

```cpp
UGrimrockPartySaveGame::CurrentSaveVersion = 4;
UGrimrockPartySaveGame::MinimumCompatibleSaveVersion = 1;
```

La compatibilité de lecture v1-v3 est conservée.

Deux snapshots RPG sont ajoutés au SaveGame :

```cpp
FRPGCharacterProgressionSaveState
{
    CharacterId
    SelectedChoiceIds
}

FRPGPendingLevelUpSaveState
{
    CharacterId
    PreviousLevel
    NewLevel
    LevelsGained
}
```

Les données sont indexées par le `CharacterId` stable, jamais par le seul index du groupe.

---

## 3. Choix de progression persistants

MON15.5 conservait les choix confirmés dans un registry runtime transitoire.

MON15.6 conserve le même modèle runtime pendant la partie, mais `UGrimrockPartySaveGame::Serialize()` capture exactement un `FRPGCharacterProgressionSaveState` par personnage actif.

Au chargement :

1. le snapshot est migré/validé ;
2. les choix sont validés contre la classe, le niveau, le budget et les prérequis ;
3. `FRPGClassProgressionTransactionService::RestorePersistentState()` reconstruit un cache détaché par `CharacterId` ;
4. `FGridCombatActionCatalog::Build()` peut immédiatement retrouver `ChoiceId` et `GrantedRequirementIds` via `AppendRuntimeSatisfiedRequirements()` ;
5. lors du prochain accès du personnage, la projection détachée se rattache au `UGridPartyInventoryComponent` vivant.

Le SaveGame est la frontière persistante ; le registry reste un cache dérivé.

---

## 4. Migration Level / Experience

Les sauvegardes v1-v3 peuvent provenir d'une période où `Level` et `Experience` n'étaient pas encore réconciliés automatiquement.

La migration applique une politique conservatrice qui ne fait perdre aucun progrès :

```text
SafeStoredLevel = Clamp(StoredLevel, 1, 20)
NormalizedXP    = Clamp(StoredXP, 0, 190000)
LevelFloorXP    = XP minimale de SafeStoredLevel
MigratedXP      = max(NormalizedXP, LevelFloorXP)
MigratedLevel   = GetLevelForExperience(MigratedXP)
```

Exemples :

```text
Legacy Level=1, XP=6000
    -> Level=4, XP=6000

Legacy Level=3, XP=1000
    -> Level=3, XP=3000
```

Le champ qui représente le progrès le plus élevé est donc conservé.

---

## 5. Recalcul des statistiques pendant migration

Si la migration change réellement le niveau stocké, les statistiques de base sont recalculées avec :

```cpp
URPGCharacterRulesLibrary::CalculateDerivedStats(...)
```

La politique MON15.3 est conservée :

- déficit absolu de PV préservé ;
- déficit absolu de mana préservé ;
- un personnage à `0 HP` reste à `0 HP` ;
- aucun bonus d'équipement n'est bake dans `DerivedStats`.

Si une migration de niveau exige un recalcul mais que la définition de classe est invalide, la migration est rejetée plutôt que de fabriquer des statistiques.

---

## 6. Choix legacy v1-v3

Les versions v1-v3 ne contenaient pas les choix MON15.5.

La migration crée donc :

```text
un FRPGCharacterProgressionSaveState par personnage actif
SelectedChoiceIds = []
```

Aucun choix n'est inventé rétroactivement.

Les points de progression disponibles restent dérivés du niveau actuel par MON15.4 et peuvent être dépensés ensuite normalement.

---

## 7. Validation stricte v4

Une sauvegarde déjà en version 4 n'est pas réparée silencieusement.

Elle est refusée si :

- `Level` et `Experience` sont incohérents ;
- un `CharacterId` actif est invalide ou dupliqué ;
- il manque un snapshot de progression pour un personnage actif ;
- un snapshot de progression référence un personnage non actif ;
- un `ChoiceId` est vide, dupliqué ou inconnu ;
- les choix violent un niveau minimum, un prérequis ou le budget ;
- une notification Level Up référence un personnage absent ;
- deux notifications persistent pour le même personnage ;
- `PreviousLevel >= NewLevel` ;
- `LevelsGained != NewLevel - PreviousLevel` ;
- `NewLevel` ne correspond pas au niveau actuel du personnage.

Cette différence entre **migration legacy** et **validation v4** évite qu'une corruption future devienne une migration implicite permanente.

---

## 8. Sauvegarde avec Level Up disponible

MON15.5 avait une file runtime transitoire. Sans MON15.6, une sauvegarde réalisée alors qu'un Level Up était différé pendant un combat pouvait restaurer le niveau mais perdre l'ouverture automatique de la modal.

MON15.6 maintient un miroir persistant des notifications non acquittées :

- notification en file ;
- notification différée par `CombatActive` ;
- notification actuellement affichée.

Elles sont capturées dans `PendingLevelUpNotifications`.

Après chargement :

1. le miroir persistant est restauré ;
2. le `UGameInstanceSubsystem` attend un tick afin que `PartyInventoryState` soit réellement chargé ;
3. les `CharacterId` sont remappés vers les index actifs ;
4. la file MON15.5 est reconstruite ;
5. `TryPresentNextNotification()` repasse par le garde de combat existant.

Une notification restaurée ne peut donc ni être perdue ni s'ouvrir au milieu d'un combat.

---

## 9. Sérialisation autoritaire

`UGrimrockPartySaveGame::Serialize()` est le point d'intégration MON15.6.

### Save

Avant la sérialisation UE :

- force `SaveVersion = 4` ;
- capture les choix runtime ;
- capture les notifications Level Up non acquittées ;
- valide strictement le snapshot v4 ;
- marque l'archive en erreur si la capture/validation échoue.

### Load

Après la désérialisation UE :

- migre v1-v3 ou valide v4 ;
- restaure la projection de choix par `CharacterId` ;
- restaure le miroir de notifications ;
- expose l'échec par `IsCompatible()` et un log `[GridSaveMigration]`.

Le chemin existant `AGrimrockPartyPawn::SaveCurrentGame()` / `LoadCurrentGameData()` reste donc inchangé et bénéficie automatiquement du contrat v4.

---

## 10. Logs

Les opérations importantes utilisent :

```text
[GridSaveMigration]
```

Exemples attendus :

```text
[GridSaveMigration] Load SourceVersion=3 TargetVersion=4 Migrated=true Reconciled=1 Choices=1 PendingLevelUps=0 Result=Accepted
[GridSaveMigration] Load SourceVersion=4 TargetVersion=4 Migrated=false ... Result=Accepted
[GridSaveMigration] LoadValidation ... Result=Rejected Reason=...
```

La restauration UI utilise toujours :

```text
[GridLevelUpUI] Restored Pending=N
```

---

## 11. Tests automatisés MON15.6

Suite ajoutée :

```text
Grimrock.RPG.MON15.6.PersistentChoiceRoundTrip
Grimrock.RPG.MON15.6.LegacyExperienceAheadMigration
Grimrock.RPG.MON15.6.LegacyStoredLevelAheadMigration
Grimrock.RPG.MON15.6.RejectCurrentLevelExperienceMismatch
Grimrock.RPG.MON15.6.RejectInvalidChoiceSnapshot
Grimrock.RPG.MON15.6.PendingLevelUpRoundTrip
Grimrock.RPG.MON15.6.RejectInvalidPendingNotification
Grimrock.RPG.MON15.6.SaveVersionContract
```

Les tests MON15.4/MON15.5 qui vérifiaient explicitement `SaveVersion == 3` ont été adaptés à la nouvelle frontière v4 sans modifier leurs règles de progression.

---

## 12. Porte de sortie

MON15.6 pourra être déclaré **VALIDÉ ET CLOS** lorsque :

- le projet compile sous UE5.5.4 ;
- les 8 tests `Grimrock.RPG.MON15.6.*` réussissent ;
- MON15.1 à MON15.5 restent verts ;
- CC5 persistance reste verte ;
- MON9 SaveGame/monster persistence reste vert ;
- MON12 `ActionCatalog` reste vert ;
- un PIE réel confirme qu'un choix de classe confirmé survit à Save → Continue ;
- un PIE réel confirme qu'une notification Level Up encore à présenter survit à Save → Continue ;
- le log de chargement confirme `Version=4` et aucune migration inattendue sur une sauvegarde v4.
