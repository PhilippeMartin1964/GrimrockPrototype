# MON15.6 — Réouverture : deadlock overlay de chargement / Level Up

Statut : **RÉSOLU — REVALIDÉ ET CLOS SOUS UE5.5.4**  
Date : **16 août 2026**

## Incident

Après la première clôture MON15.6, le scénario PIE `Save -> Continue` avec une notification Level Up persistante a révélé un défaut fonctionnel : l'écran `Chargement de la partie / Partie chargée / 100 %` restait visible au-dessus de la modal et empêchait toute interaction.

La sauvegarde et la restauration étaient correctes :

```text
[GridSaveMigration] ... PendingLevelUps=1 Result=Accepted
PartySave Continued Slot=GrimrockParty CharacterCount=1
[GridLevelUpUI] Restored Pending=1
[GridLevelUpUI] ModalGuard Applied Character=0 PausedByModal=true
[GridLevelUpUI] Opened Character=0 Previous=1 New=2
```

## Cause

`UGrimrockStartupModeComponent` ajoutait le widget de progression à `ZOrder=5000`, puis `CompleteBuildProgress()` planifiait son retrait avec un timer du monde.

La modal Level Up appliquait ensuite `SetGamePaused(true)`. Le timer de retrait du widget de chargement ne progressait plus pendant la pause. L'overlay restait donc indéfiniment au-dessus de la modal et interceptait les clics.

## Correctif

Commit :

```text
bda4f8866faeb375ad9c0a68d17fd1bce9db3fee  Fix MON15.6 loading overlay modal deadlock
```

`GrimrockStartupModeComponent.cpp` applique trois protections :

1. le widget de progression est `ESlateVisibility::HitTestInvisible`, puisqu'il est purement informatif ;
2. lors d'un `Continue` terminé, l'overlay est retiré synchroniquement dès que l'état atteint `Partie chargée` ;
3. `HideBuildProgress()` annule explicitement tout timer de masquage encore présent.

Log ajouté :

```text
GrimrockStartupMode LoadProgress HiddenImmediately Pawn=<...> Reason=LoadedGameReady
```

## Revalidation PIE finale

Le scénario a été rejoué après correction avec `PendingLevelUps=1`.

Ordre observé :

```text
PartySave Continued Slot=GrimrockParty CharacterCount=1
GrimrockStartupMode LoadProgress HiddenImmediately Pawn=BP_GrimrockPartyPawn_C_0 Reason=LoadedGameReady
[GridLevelUpUI] Restored Pending=1
[GridLevelUpUI] ModalGuard Applied Character=0 PausedByModal=true
[GridLevelUpUI] Opened Character=0 Previous=1 New=2
```

La modal est ensuite réellement interactive :

```text
[GridClassProgression] Character=0 ... Level=2 Committed=1 Granted=1 Spent=1 Remaining=0
[GridLevelUpUI] ModalGuard Restored Character=0
```

Après fermeture, le gameplay reprend normalement :

- déplacement accepté ;
- attaque acceptée ;
- tour ennemi exécuté ;
- round suivant exécuté.

Le même run a également validé un nouveau Level Up pendant le combat :

```text
[GridLevelUpUI] Queued Character=0 Previous=2 New=3 Gained=1 Pending=1
[GridLevelUpUI] Deferred Character=0 Previous=2 New=3 Reason=CombatActive ...
...
[GridCombat] ... Phase=Victory ...
[GridLevelUpUI] CombatSafePoint Result=EGridCombatPhase::Victory Pending=1
[GridLevelUpUI] Opened Character=0 Previous=2 New=3
[GridLevelUpUI] ModalGuard Restored Character=0
```

La restauration d'entrée fonctionne donc aussi pour les Level Up suivants.

## Revalidation Automation finale

Après le correctif, la suite suivante a été relancée :

```text
Grimrock.RPG.MON15.6
```

Résultat : **8/8 Success**.

```text
LegacyExperienceAheadMigration       Success
LegacyStoredLevelAheadMigration      Success
PendingLevelUpRoundTrip              Success
PersistentChoiceRoundTrip            Success
RejectCurrentLevelExperienceMismatch Success
RejectInvalidChoiceSnapshot          Success
RejectInvalidPendingNotification     Success
SaveVersionContract                  Success
```

Les round-trips v4 confirment notamment `Choices=1` et `PendingLevelUps=1` lorsque ces états sont présents.

## Décision finale

Le deadlock découvert après la première clôture est corrigé et revalidé à la fois en Automation et en PIE réel.

**MON15.6 — VALIDÉ ET CLOS.**

La clôture initiale `d4120b0...` est complétée par le correctif `bda4f886...` et par cette revalidation finale.

Suite : **MON15.7 — équilibrage et clôture complète de MON15.**
