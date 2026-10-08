# RPG-LEVELUX01.3 — Remove Legacy Level-Up Modal

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **SOURCE / DOCS IMPLÉMENTÉS — validation locale requise**

## Objectif

Clore réellement RPG-LEVELUX01 en supprimant les restes de l'ancien workflow
modal au lieu de conserver un chemin mort à côté du toast non modal validé.

## Code de production supprimé

Fichiers retirés :

```text
Source/GrimrockPrototype/Public/UI/RPGLevelUpWidget.h
Source/GrimrockPrototype/Private/UI/RPGLevelUpWidget.cpp
Source/GrimrockPrototype/Private/UI/RPGLevelUpWidgetSlate.cpp
```

Cela supprime notamment :

```text
URPGLevelUpWidget
FRPGLevelUpView
FRPGLevelUpChoiceView
FRPGLevelUpPresentationView
PendingChoiceIds
StageOrUnstageChoice()
ConfirmSelection()
CancelSelection()
CloseModal()
ApplyInputGuard()
RestoreInputGuard()
pause du jeu
DisableInput()
FInputModeUIOnly
fallback Slate du modal
```

L'acquisition des Talents reste exclusivement portée par la fenêtre autonome
COMPÉTENCES / TALENTS et `FRPGClassProgressionTransactionService`.

## Subsystem Level Up simplifié

`URPGLevelUpNotificationSubsystem` ne conserve plus que :

```text
PendingNotifications
ActiveNotification
ActiveToastTimerHandle
LevelUpDelegateHandle
```

Supprimés :

```text
RefreshFromPartyState()
ObservedPartyInventory
BindPartyInventory()
AcknowledgeNotification()
IsLevelUpModalOpen()
GetPendingLevelUpNotificationCount()
```

La queue est uniquement transitoire et sert à séquencer les toasts.

## Suppression de LastAcknowledgedLevel

`FGridCharacterInventoryState::LastAcknowledgedLevel` est retiré.

Il n'existe plus :

- de Level Up durable en attente ;
- de watermark de présentation dans l'état personnage ;
- de validation Save associée ;
- de restauration/catch-up au chargement ;
- d'initialisation de ce champ dans création/recrutement.

`Level` reste une projection transitoire de `Experience`.

## SaveGame v24 exact-match

Le retrait physique du champ ouvre :

```text
CurrentSaveVersion = 24
```

Politique inchangée :

```text
v24 -> accepté
v23 et antérieures -> rejet
aucune migration
```

Les anciennes sauvegardes peuvent être supprimées pendant le prototype.

## Tests historiques nettoyés

Supprimés :

```text
RPGMON2073TalentPresentationTests.cpp
GridTD07339LevelUpNotificationCharacterizationTests.cpp
GridTD07339LevelUpNotificationNormalizationTests.cpp
```

Les deux tests MON15.5 strictement liés au widget modal sont retirés.
Les tests encore utiles sur la transaction de progression et l'événement
source-aware restent présents.

`RPGMON155TestHelpers.h` ne dépend plus de l'ancien widget.

## Audit binaire Content

Le filtre RPG-LEVELUX01 contient maintenant un test qui scanne les
`.uasset` et `.umap` du dossier Content pour détecter les tokens sérialisés :

```text
RPGLevelUpWidget
LastAcknowledgedLevel
```

Le ticket ne sera considéré sûr que si ce scan retourne zéro référence.

## Automation

Filtre principal :

```text
Grimrock.RPG.LEVELUX01
```

Attendu : **8 tests**.

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.LEVELUX01"
```

Régressions recommandées ensuite :

```text
Grimrock.RPG.DEV01
Grimrock.UI.RPG04
Grimrock.UI.RPG05
Grimrock.UI.RPG06.Skills
Grimrock.MON20.8.SkillsPage
Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema
```

## PIE

Après Automation verte :

1. lancer le donjon ;
2. exécuter `Grimrock.RPG.SetSelectedLevel 2` ;
3. vérifier le toast ;
4. continuer immédiatement vers 5 puis 6 ;
5. vérifier qu'aucune popup, pause ou capture d'input Level Up n'apparaît ;
6. ouvrir K et vérifier que l'acquisition des Talents fonctionne toujours.

RPG-LEVELUX01 ne sera déclaré clos qu'après validation locale et PIE.
