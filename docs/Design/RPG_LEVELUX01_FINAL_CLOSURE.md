# RPG-LEVELUX01 — Clôture finale Non-Modal Level Up

Date : **8 octobre 2026**  
État : **VALIDÉ / CLOS**

## Résultat

La montée de niveau est désormais entièrement non modale.

```text
XP
 -> FRPGLevelUpService
 -> Level / stats / ressources / projection de classe
 -> OnCharacterLevelUpAppliedWithSource
 -> URPGLevelUpNotificationSubsystem
 -> file transitoire
 -> WBP_GridPersistentHud
 -> WBP_RPGNotification
```

Le joueur conserve le contrôle du jeu. Aucun écran Level Up n'interrompt la partie.

## Legacy supprimé

```text
URPGLevelUpWidget
RPGLevelUpWidget.h/.cpp/.Slate.cpp
FRPGLevelUpView
FRPGLevelUpChoiceView
PendingChoiceIds
modal pause/input guard
LastAcknowledgedLevel
RefreshFromPartyState()
ObservedPartyInventory
BindPartyInventory()
AcknowledgeNotification()
IsLevelUpModalOpen()
GetPendingLevelUpNotificationCount()
```

## Audit binaire

Le filtre `Grimrock.RPG.LEVELUX01` scanne les packages Content pour détecter
des références sérialisées à `RPGLevelUpWidget` ou `LastAcknowledgedLevel`.
Validation finale : **aucune référence sérialisée détectée**.

## SaveGame

`UGrimrockPartySaveGame::CurrentSaveVersion = 24`.
v24 est accepté ; v23 et antérieures sont rejetées ; aucune migration.

## Validation Automation

```text
Grimrock.RPG.LEVELUX01                              8/8
Grimrock.RPG.DEV01                                  5/5
Grimrock.UI.RPG04                                  12/12
Grimrock.UI.RPG05                                   5/5
Grimrock.UI.RPG06.Skills                            7/7
Grimrock.MON20.8.SkillsPage                         8/8
Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema 5/5
Warnings                                            0
Failures                                            0
```

Total régressions : **42/42**, 0 warning, 0 échec.

## Validation PIE

Validé manuellement après nettoyage : aucune popup Level Up, aucune pause,
aucune capture d'input, toast visible, sauts successifs possibles, `K` toujours
fonctionnel et acquisition de Talent préservée.

## Jalons

```text
RPG-LEVELUX01.1   CLOS — pipeline non modal
RPG-LEVELUX01.2   CLOS — toast Persistent HUD
RPG-LEVELUX01.3   CLOS — legacy modal supprimé
RPG-LEVELUX01.3A  CLOS — cohérence SaveGame v24
RPG-LEVELUX01     CLOS
```

## Suite

Prochain chantier : `RPG-ATTR01 — Attribute Point Economy & Allocation`,
pour matérialiser les points de caractéristiques des niveaux 4 / 8 / 12 / 16 / 20
sans recréer de double autorité.

## Supplanté : UI-PARTYHUD02 — Indicateurs persistants (2026-10-19)

La file transitoire de toasts Level Up de RPG-LEVELUX01 est retirée. Les notifications de **montée de niveau** ne passent plus par `WBP_RPGNotification` : `FGridPartyPortraitView::bHasUnspentProgressionPoints` est recalculé à partir des soldes canoniques des points de compétence, attribut et talent. Les portraits montrent `Image_LevelUpAlert` jusqu'à consommation des points. La surcharge suit `FGridInventoryCharacterSummary::bOverloaded` via `Image_WeightAlert`.

Le présent texte conserve l'historique du jalon original. `UGridRPGNotificationWidget` et son binding dans le HUD persistant restent utilisables **uniquement pour les autres feedbacks de progression** (ex. points d'attribut), pas pour une montée de niveau. L'asset UMG doit être adapté dans Unreal Editor ; aucun `.uasset` n'est supprimé par édition de texte.
