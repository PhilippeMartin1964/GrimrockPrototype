# MON15.6 — Validation Checklist

Statut : **VALIDÉ ET CLOS — UE5.5.4**  
Date : **16 août 2026**

---

## 1. Compilation

- [x] `GrimrockPrototype` compile sous UE5.5.4.
- [x] UHT accepte les nouveaux `USTRUCT` SaveGame.
- [x] `UGrimrockPartySaveGame::Serialize()` compile et surcharge correctement `UObject::Serialize`.
- [x] Aucun `.uasset`, `.umap` ou WBP modifié par MON15.6.

---

## 2. Suite MON15.6

Exécuté :

```text
Grimrock.RPG.MON15.6
```

Résultat : **8/8 Success**.

- [x] `PersistentChoiceRoundTrip`
- [x] `LegacyExperienceAheadMigration`
- [x] `LegacyStoredLevelAheadMigration`
- [x] `RejectCurrentLevelExperienceMismatch`
- [x] `RejectInvalidChoiceSnapshot`
- [x] `PendingLevelUpRoundTrip`
- [x] `RejectInvalidPendingNotification`
- [x] `SaveVersionContract`

---

## 3. Régressions MON15

- [x] MON15.1 vert.
- [x] MON15.2 vert.
- [x] MON15.3 vert.
- [x] MON15.4 vert.
- [x] MON15.5 vert.
- [x] Campagne complète `Grimrock.RPG.MON15` : **38/38 tests uniques Success**.

Les anciennes assertions de tests qui figeaient explicitement `SaveVersion == 3` ont été rendues version-agnostiques après le passage v4.

---

## 4. Régressions sauvegarde

- [x] `Grimrock.CharacterCreation.CC5.RejectInvalidSnapshotAtomically` vert.
- [x] `Grimrock.CharacterCreation.CC5.SaveMemoryRoundTrip` vert.
- [x] `Grimrock.Monsters.MON9` vert après adaptation du test de compatibilité de version.
- [x] Les sauvegardes ordinaires sans choix de progression continuent de fonctionner.

---

## 5. Régression combat / requirements

- [x] `Grimrock.Monsters.MON12.ActionCatalog.Contributions` vert.
- [x] `Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle` vert.
- [x] Aucun changement de règle MON12.

---

## 6. Migration legacy v1-v3

- [x] `Level=1, XP=6000` migre vers `Level=4, XP=6000`.
- [x] Le déficit absolu de PV est préservé.
- [x] Le déficit absolu de mana est préservé.
- [x] `Level=3, XP=1000` migre vers `Level=3, XP=3000`.
- [x] Aucun choix de classe n'est inventé pour une sauvegarde legacy.
- [x] La sauvegarde migrée devient v4.

---

## 7. Validation stricte v4

- [x] Un couple `Level/Experience` incohérent est refusé sans mutation.
- [x] Un choix inconnu est refusé.
- [x] Un choix dupliqué est refusé par le validateur de snapshot.
- [x] Un budget/prérequis invalide est refusé.
- [x] Une notification Level Up incohérente est refusée.
- [x] Une sauvegarde v4 valide ne subit aucune migration.

---

## 8. Round-trip des choix

Scénario automatisé validé :

```text
Level 3
Choice_A + Choice_B confirmés
SaveGameToMemory
ResetRuntimeState
LoadGameFromMemory
```

- [x] `SaveVersion == 4`.
- [x] `Choice_A` restauré.
- [x] `Choice_B` restauré.
- [x] `Feature_A` restauré dans les requirements.
- [x] La projection est disponible avant rattachement au composant live.

---

## 9. Round-trip notification Level Up

Scénario automatisé validé :

```text
personnage Level 2
notification persistante 1 -> 2
SaveGameToMemory
clear mirror
LoadGameFromMemory
```

- [x] une notification est présente dans `PendingLevelUpNotifications`.
- [x] le `CharacterId` est identique.
- [x] `PreviousLevel=1`.
- [x] `NewLevel=2`.
- [x] le miroir runtime est reconstruit au chargement.

---

## 10. PIE — choix confirmé puis Continue

Scénario validé :

1. confirmer un choix de progression ;
2. sauvegarder ;
3. arrêter PIE ;
4. relancer depuis `L_MainMenu` ;
5. `Continue`.

Résultat :

- [x] niveau et XP restaurés.
- [x] choix confirmé toujours acquis.
- [x] choix non rachetable.
- [x] projection du choix restaurée.
- [x] log v4 accepté sans migration :

```text
[GridSaveMigration] Load SourceVersion=4 TargetVersion=4 Migrated=false Reconciled=0 Choices=1 PendingLevelUps=0 Result=Accepted
PartySave Continued Slot=GrimrockParty CharacterCount=1
```

---

## 11. PIE — notification Level Up encore disponible

Scénario validé :

1. nouvelle partie ;
2. Level Up 1 -> 2 obtenu mais non acquitté ;
3. sauvegarde avec notification persistante ;
4. arrêt PIE ;
5. `Continue`.

Résultat :

- [x] `PendingLevelUps=1` présent dans le snapshot v4.
- [x] `[GridLevelUpUI] Restored Pending=1` après restauration du groupe.
- [x] la modal s'ouvre une seule fois.
- [x] la modal restaurée présente `Previous=1 New=2`.
- [x] le garde modal est appliqué avec pause du jeu.

Extrait validant :

```text
[GridSaveMigration] Load SourceVersion=4 TargetVersion=4 Migrated=false Reconciled=0 Choices=1 PendingLevelUps=1 Result=Accepted
PartySave Continued Slot=GrimrockParty CharacterCount=1
[GridLevelUpUI] Restored Pending=1
[GridLevelUpUI] ModalGuard Applied Character=0 PausedByModal=true
[GridLevelUpUI] Opened Character=0 Previous=1 New=2
```

Observation non bloquante : pendant `L_MainMenu`, l'inspection des slots peut produire `PersistentRestore ... PartyNotReady` avant création du `PartyPawn`. Le chargement effectif du slot choisi restaure ensuite correctement la notification.

---

## 12. Aucun changement de contenu UE

- [x] Aucun Blueprint modifié par le code MON15.6.
- [x] Aucun WBP modifié par le code MON15.6.
- [x] Aucune map modifiée par le code MON15.6.
- [x] Les éventuelles valeurs de récompense XP utilisées manuellement en PIE restent des fixtures temporaires et ne font pas partie du commit de clôture.

---

## 13. Porte de clôture

Toutes les vérifications nécessaires au runtime réel ont été validées.

**MON15.6 — VALIDÉ ET CLOS.**

Suite : **MON15.7 — équilibrage et clôture complète de MON15.**
