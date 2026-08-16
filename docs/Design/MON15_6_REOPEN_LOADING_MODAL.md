# MON15.6 — Réouverture : deadlock overlay de chargement / Level Up

Statut : **RÉOUVERT — CORRECTIF À REVALIDER SOUS UE5.5.4**  
Date : **16 août 2026**

## Incident

Le scénario PIE `Save -> Continue` avec une notification Level Up persistante a révélé un défaut fonctionnel après la première clôture MON15.6.

La sauvegarde et la restauration elles-mêmes sont correctes :

```text
[GridSaveMigration] ... PendingLevelUps=1 Result=Accepted
PartySave Continued Slot=GrimrockParty CharacterCount=1
[GridLevelUpUI] Restored Pending=1
[GridLevelUpUI] ModalGuard Applied Character=0 PausedByModal=true
[GridLevelUpUI] Opened Character=0 Previous=1 New=2
```

Mais l'écran `Chargement de la partie / Partie chargée / 100 %` restait visible au-dessus de la modal et empêchait toute interaction.

## Cause

`UGrimrockStartupModeComponent` ajoutait le widget de progression à `ZOrder=5000`, puis `CompleteBuildProgress()` planifiait son retrait avec un timer du monde.

La modal Level Up est ensuite ouverte à `ZOrder=200` et applique `SetGamePaused(true)`.

Le timer de retrait du widget de chargement ne progresse plus pendant la pause. L'overlay reste donc indéfiniment au-dessus de la modal et intercepte les clics.

## Correctif

`GrimrockStartupModeComponent.cpp` applique maintenant deux protections :

1. le widget de progression est `ESlateVisibility::HitTestInvisible`, puisqu'il est purement informatif ;
2. pour un chargement `Continue` terminé, l'overlay est retiré synchroniquement dès `Partie chargée`, avant qu'une notification Level Up restaurée puisse mettre le monde en pause.

`HideBuildProgress()` annule également explicitement un éventuel timer restant.

Log ajouté :

```text
GrimrockStartupMode LoadProgress HiddenImmediately Pawn=<...> Reason=LoadedGameReady
```

## Revalidation requise

Avant de refermer MON15.6 :

- compiler `Development Editor x64` sous UE5.5.4 ;
- relancer au minimum `Grimrock.RPG.MON15.6` ;
- refaire le scénario PIE avec `PendingLevelUps=1` puis `Continue` ;
- vérifier que l'écran de chargement disparaît avant la modal Level Up ;
- vérifier que `Maîtrise martiale`, `Confirmer` et `Annuler` sont cliquables ;
- fermer la modal et vérifier que le jeu reprend normalement.

La clôture `d4120b0...` est donc historiquement conservée mais **supersédée par cette réouverture jusqu'à revalidation**.
