# RPG-LEVELUX01.1 — Non-Modal Level Up — source

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **SOURCE IMPLÉMENTÉE — validation locale puis matérialisation Persistent HUD requises**

## Objectif

Supprimer l'interruption modale lors d'une montée de niveau.

Le joueur ne doit plus :

- perdre le contrôle ;
- subir une pause de jeu ;
- fermer une popup pour acquitter la montée ;
- passer par l'ancien `URPGLevelUpWidget` pour dépenser ses Talents.

Les choix de Talents restent dans la page COMPÉTENCES / TALENTS.

## Nouveau flux

```text
XP
 -> FRPGLevelUpService
 -> Level / DerivedStats / Resources
 -> LastAcknowledgedLevel = Level
 -> projection Talent
 -> OnCharacterLevelUpAppliedWithSource
 -> URPGLevelUpNotificationSubsystem
 -> file transitoire de toasts
 -> WBP_GridPersistentHud
 -> WBP_RPGNotification
```

Aucune popup et aucun changement de mode d'input.

## LastAcknowledgedLevel

Le champ reste durable pour ne pas modifier aveuglément le schéma SaveGame
courant.

Pour toute nouvelle progression :

```text
Level = N
LastAcknowledgedLevel = N
```

Un ancien/current save peut encore contenir :

```text
LastAcknowledgedLevel < Level
```

Au chargement, `RefreshFromPartyState()` transforme ce delta une seule fois en
feedback non modal puis synchronise le watermark avec `Level`.

## Toast global

Le toast de progression quitte la seule fenêtre Skills et peut être projeté
par le Persistent HUD.

`UGridPersistentHudWidget` expose :

```text
Notification_Progression [BindWidgetOptional]
ShowProgressionNotification(View)
```

Tant que RPG-LEVELUX01.2 n'est pas matérialisé, l'absence de cette surface ne
bloque jamais la progression : le feedback visuel est simplement ignoré.

## Contenu du toast

Le feedback est construit depuis les autorités existantes.

Exemple 4 -> 5 :

```text
Niveau 5 atteint

Elias passe du niveau 4 au niveau 5.
+1 point de compétence.
Rang maximal des compétences : 3.
```

Exemple 5 -> 6 :

```text
Niveau 6 atteint

Elias passe du niveau 5 au niveau 6.
+1 point de compétence.
+1 point de talent.
```

Un saut multi-niveaux de RPG-DEV01 agrège les gains.

Les points de caractéristiques ne sont pas mentionnés tant que leur économie
n'est pas implémentée.

## Combat

Le toast est informatif et non interactif.

Il n'est donc plus différé jusqu'à la fin du combat. L'ancien
`DeferredCombatTurnManager` disparaît du chemin Level-Up.

## RPG-DEV01

La commande :

```text
Grimrock.RPG.SetSelectedLevel <niveau>
```

peut désormais être appelée successivement :

```text
2 -> 6 -> 10 -> 14 -> 18 -> 20
```

sans fermer une popup entre les étapes.

## RPG-LEVELUX01.2 — matérialisation UMG à faire après validation source

Dans `WBP_GridPersistentHud`, conserver la barre basse existante et ajouter
une surcouche de notification dans `CanvasPanel_Root`.

Hiérarchie cible :

```text
CanvasPanel_Root
├── HorizontalBox_BottomBar
└── SB_ProgressionNotification              [SizeBox]
    └── Notification_Progression            [WBP_RPGNotification] Is Variable = YES
```

Réglages recommandés :

```text
SB_ProgressionNotification
Width Override  = 500
Height Override = 110

Canvas Slot
Anchors              = Top Right
Alignment            = 1.0, 0.0
Position X           = -34
Position Y           = 34
Auto Size            = false

Notification_Progression
Is Variable           = YES
Visibility            = Collapsed
Horizontal Alignment  = Fill
Vertical Alignment    = Fill
```

`WBP_RPGNotification` existe déjà et reste réutilisé.

Aucun Event Graph et aucun binding Blueprint.

## Automation

Filtre :

```text
Grimrock.RPG.LEVELUX01
```

Attendu : **6 tests**.

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.LEVELUX01"
```

Régressions ensuite :

```text
Grimrock.RPG.DEV01
Grimrock.UI.RPG05
Grimrock.TechnicalDebt.TD07_3_3_9
```

RPG-LEVELUX01 ne sera clos qu'après validation Automation et PIE sans popup,
avec toast visible dans le Persistent HUD.
