# TD07.3.3.9 — Level-Up Notification State Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `9080cf37385e89d6b08371898dbd28aca62dd357`  
Statut : **CHARACTERIZATION À VALIDER**

## 1. Objet

TD07.3.3.9 normalise l'état persistant utilisé pour rappeler au joueur qu'un personnage a gagné un ou plusieurs niveaux et que la modal Level Up n'a pas encore été acquittée.

Le comportement fonctionnel à préserver est :

```text
Level Up appliqué
    -> notification éventuellement différée
    -> Save / Continue possible
    -> modal Level Up présentée une fois
    -> acquittement
```

## 2. Représentations actuelles

Le système utilise simultanément :

```text
URPGLevelUpNotificationSubsystem
    PendingNotifications
    ActiveNotification
    PendingPersistentRestoreStates

RPGLevelUpNotificationSubsystem.cpp
    PersistentNotificationMirror [static]

UGrimrockPartySaveGame
    PendingLevelUpNotifications[]
        FRPGPendingLevelUpSaveState
```

Le Save persiste donc une **queue de présentation**, et non un état métier minimal.

## 3. Snapshot Save actuel

```text
FRPGPendingLevelUpSaveState
    CharacterId
    PreviousLevel
    NewLevel
    LevelsGained
```

Invariants actuels :

```text
CharacterId valide
personnage actif
une notification maximum par personnage
PreviousLevel < NewLevel
LevelsGained == NewLevel - PreviousLevel
NewLevel == Character.Level
```

## 4. Frontière Active-only actuelle

`CapturePersistentState()` ne conserve que les notifications dont le `CharacterId` appartient à `ActiveCharacters`.

Une notification attachée à un personnage du `CharacterPool` n'est pas capturée.

Cette frontière est caractérisée afin qu'elle soit modifiée consciemment, et non par accident.

## 5. Autorité actuelle

L'autorité de présentation runtime est distribuée entre :

```text
PendingNotifications
ActiveNotification
```

L'autorité persistante pratique est distribuée entre :

```text
PersistentNotificationMirror
PendingLevelUpNotifications
PendingPersistentRestoreStates
```

Il n'existe aujourd'hui aucun champ minimal durable dans `FGridCharacterInventoryState`.

## 6. Décision de cible — Option A

TD07.3.3.1 proposait deux options. TD07.3.3.9 retient **Option A : état minimal durable**.

Cible :

```text
FGridCharacterInventoryState
    LastAcknowledgedLevel
```

ou un nom strictement équivalent.

Sémantique :

```text
LastAcknowledgedLevel == Level
    aucune notification Level Up à présenter

LastAcknowledgedLevel < Level
    notification nécessaire

PreviousLevel = LastAcknowledgedLevel
NewLevel      = Level
LevelsGained  = Level - LastAcknowledgedLevel
```

## 7. Pourquoi cette cible

Elle préserve le comportement Save / Continue sans persister une queue UI.

Elle permet également au statut d'acquittement de voyager naturellement avec le personnage entre :

```text
ActiveCharacters
CharacterPool
```

Les objets de présentation restent transient :

```text
PendingNotifications
ActiveNotification
ActiveWidget
DeferredCombatTurnManager
```

## 8. Suppressions visées après gate

```text
FRPGPendingLevelUpSaveState
UGrimrockPartySaveGame::PendingLevelUpNotifications
PersistentNotificationMirror
PendingPersistentRestoreStates
CapturePersistentState()
RestorePersistentState()
TryAdoptPersistentRestoreState()
SchedulePersistentRestoreRetry()
HandlePersistentRestoreRetryTick()
SyncPersistentMirrorForCharacter()
```

Le détail exact dépendra de ce qui reste nécessaire pour la simple orchestration runtime.

## 9. Initialisation / acquittement cible

Nouveau personnage :

```text
LastAcknowledgedLevel = Level
```

Après Level Up :

```text
Level augmente
LastAcknowledgedLevel reste inchangé
=> notification dérivable
```

Après fermeture/acquittement de la modal :

```text
LastAcknowledgedLevel = Level
```

Au chargement / recrutement :

```text
scanner les personnages actifs
si LastAcknowledgedLevel < Level
    reconstruire une notification transient
```

## 10. SaveGame

La normalisation ouvrira une nouvelle génération exact-match, attendue :

```text
CurrentSaveVersion = 19
v18 et antérieures -> rejet
aucune migration
```

## 11. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_9.Characterization
```

Tests :

```text
RepresentationMultiplicity
PersistentMirrorRoundTrip
ActiveOnlyPersistenceBoundary
SaveValidationContract
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 12. Stop condition du gate

- [x] représentations runtime et persistantes cartographiées ;
- [x] contrat Save actuel caractérisé ;
- [x] frontière Active-only caractérisée ;
- [x] cible `LastAcknowledgedLevel` choisie ;
- [x] sémantique de reconstruction documentée ;
- [x] suppressions visées documentées ;
- [x] 4 tests ajoutés ;
- [ ] build UE5.5.4 vert ;
- [ ] 4/4 tests verts.
