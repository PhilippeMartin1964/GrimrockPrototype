# MON15.6 — Closure

Statut : **VALIDÉ ET CLOS — UE5.5.4**  
Date : **16 août 2026**

---

## 1. Résumé

MON15.6 ferme la persistance de la progression RPG introduite par MON15.1 à MON15.5.

Le format de sauvegarde est passé à **SaveVersion 4** avec rétrocompatibilité v1-v3. Les choix de progression confirmés et les notifications Level Up non acquittées survivent désormais à `Save -> Continue`.

---

## 2. Contrat final

### SaveGame

```cpp
CurrentSaveVersion = 4
MinimumCompatibleSaveVersion = 1
```

Nouveaux snapshots :

```text
FRPGCharacterProgressionSaveState
FRPGPendingLevelUpSaveState
```

Les données sont corrélées au personnage par `CharacterId` stable.

### Migration legacy

La migration v1-v3 réconcilie `Level` et `Experience` sans perdre de progression :

```text
Level=1 XP=6000 -> Level=4 XP=6000
Level=3 XP=1000 -> Level=3 XP=3000
```

Les déficits absolus de PV et mana sont préservés lorsque le niveau doit être recalculé.

### Validation v4

Une sauvegarde déjà v4 est validée strictement. Les incohérences de niveau/XP, CharacterId, choix, budget, prérequis ou notification Level Up sont rejetées au lieu d'être réparées silencieusement.

---

## 3. Choix de progression

Validation automatisée et PIE :

```text
choix confirmé
-> Save v4
-> arrêt PIE
-> Continue
-> choix toujours Acquis
```

Le chargement réel a confirmé :

```text
[GridSaveMigration] Load SourceVersion=4 TargetVersion=4 Migrated=false Reconciled=0 Choices=1 PendingLevelUps=0 Result=Accepted
PartySave Continued Slot=GrimrockParty CharacterCount=1
```

Le choix confirmé reste acquis et ne peut pas être acheté une seconde fois.

---

## 4. Notification Level Up persistante

Le scénario final a été validé depuis une nouvelle partie : deux rats tués, Level Up 1 -> 2 non acquitté, sauvegarde, arrêt PIE, puis `Continue`.

Le SaveGame chargé contenait bien :

```text
PendingLevelUps=1
```

Puis le runtime a produit :

```text
[GridLevelUpUI] Restored Pending=1
[GridLevelUpUI] ModalGuard Applied Character=0 PausedByModal=true
[GridLevelUpUI] Opened Character=0 Previous=1 New=2
```

La modal `NIVEAU SUPÉRIEUR` s'est affichée une seule fois après restauration du groupe.

Le combat automatique peut démarrer juste après cette restauration si un monstre est déjà en situation d'engagement ; la modal conserve alors le contrôle via le garde et la pause du jeu.

---

## 5. Automation

### MON15

Campagne `Grimrock.RPG.MON15` : **38/38 tests uniques Success**.

Répartition :

```text
MON15.1  4/4
MON15.2  5/5
MON15.3  6/6
MON15.4  7/7
MON15.5  8/8
MON15.6  8/8
```

### MON15.6

Les 8 tests dédiés sont verts :

```text
PersistentChoiceRoundTrip
LegacyExperienceAheadMigration
LegacyStoredLevelAheadMigration
RejectCurrentLevelExperienceMismatch
RejectInvalidChoiceSnapshot
PendingLevelUpRoundTrip
RejectInvalidPendingNotification
SaveVersionContract
```

### Régressions

Validées :

```text
Grimrock.CharacterCreation.CC5
Grimrock.Monsters.MON9
Grimrock.Monsters.MON12.ActionCatalog
```

Le test historique MON9 `SaveVersionCompatibility` a été rendu version-agnostique afin de tester `CurrentSaveVersion` et `CurrentSaveVersion + 1` au lieu de valeurs codées en dur.

---

## 6. Compilation

Compilation `Development Editor x64` validée sous UE5.5.4 après correction d'une collision Unity Build entre helpers anonymes de MON15.6 et helpers existants MON15.3/MON15.5.

Les helpers de migration sont isolés dans :

```cpp
namespace MON156SaveMigrationPrivate
```

L'éditeur UE5.5.4 démarre correctement après compilation.

---

## 7. Commits MON15.6

Implémentation :

```text
bbf12f0c47f4977a801e500e96bc0a3968dcfcff  Implement MON15.6 save migration
```

Correctif Unity Build :

```text
eb19e7b39f5c81cbdb65bbae2533c4ca9620b5a0  Fix MON15.6 unity build helper collisions
```

Adaptation des régressions MON15.2/MON15.3 au SaveVersion courant :

```text
b73edd909861fba1febaf0aed1f5f089ae2489ea  Fix MON15 regression save version assertions
```

Adaptation du test MON9 :

```text
0d275f5d31492ad63b247cc82e136b6ccfb21c96  Fix MON9 save version compatibility test
```

---

## 8. Observations non bloquantes

Lors de l'affichage du menu principal, la vérification de plusieurs slots utilise `LoadGameFromSlot()`. Un slot comportant `PendingLevelUps=1` peut donc déclencher temporairement :

```text
[GridLevelUpUI] PersistentRestore Deferred Result=Abandoned Pending=1 Reason=PartyNotReady
```

avant qu'un `PartyPawn` existe. Le chargement effectif du slot sélectionné recharge ensuite le même snapshot et la notification a été restaurée avec succès. Aucun état joueur n'a été perdu dans le scénario validé.

Des anciens slots secondaires v4 incomplets peuvent également produire :

```text
LoadValidation ... 0 états de progression pour 1 personnages actifs
```

Le slot principal utilisé par `Continue` était valide et accepté avec `Choices=1`.

Ces messages pourront être nettoyés ultérieurement si l'on souhaite rendre l'inspection des slots totalement silencieuse, mais ils ne bloquent pas la clôture fonctionnelle de MON15.6.

---

## 9. Frontière de MON15.6

Aucun nouveau système de gameplay n'est introduit ici :

- pas de nouvel effet de statut ;
- pas de nouveau monstre ;
- pas de nouveau sort ;
- pas de changement de règle de combat ;
- pas de changement de Blueprint/WBP nécessaire à l'implémentation.

MON15.6 se limite à la persistance, la migration, la validation et la restauration de la progression RPG.

---

## 10. Décision

**MON15.6 — VALIDÉ ET CLOS.**

Prochain jalon :

**MON15.7 — équilibrage et clôture complète de MON15.**
