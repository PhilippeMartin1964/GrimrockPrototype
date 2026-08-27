# TD07.3.3.9 — Level-Up Notification State Normalization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Characterization validée : `240e1e752c52dfb7e2115905b09cc39ccf72719e`  
Statut : **IMPLÉMENTÉ — VALIDATION UE / RÉGRESSIONS / SHIPPING REQUISES**

## 1. Autorité durable

La notification Level-Up n'est plus persistée comme une queue UI.

L'unique état durable est désormais :

```text
FGridCharacterInventoryState::LastAcknowledgedLevel
```

Sémantique :

```text
LastAcknowledgedLevel == Level
    aucun Level Up à présenter

LastAcknowledgedLevel < Level
    Level Up non acquitté
```

La présentation dérive :

```text
PreviousLevel = LastAcknowledgedLevel
NewLevel      = Level
LevelsGained  = Level - LastAcknowledgedLevel
```

## 2. Suppressions

Supprimés :

```text
FRPGPendingLevelUpSaveState
UGrimrockPartySaveGame::PendingLevelUpNotifications
PersistentNotificationMirror
PendingPersistentRestoreStates
PersistentRestoreRetryCount
CapturePersistentState()
RestorePersistentState()
HandlePersistentStateRestored()
TryAdoptPersistentRestoreState()
SchedulePersistentRestoreRetry()
HandlePersistentRestoreRetryTick()
SyncPersistentMirrorForCharacter()
SubsystemInstances
```

Le SaveGame n'appelle plus le subsystem Level-Up pendant `Serialize()`.

## 3. Queue runtime conservée

Restent transient :

```text
PendingNotifications
ActiveNotification
ActiveWidget
DeferredCombatTurnManager
ObservedPartyInventory
```

Le subsystem observe le `UGridPartyInventoryComponent` connu et reconstruit la queue depuis l'état durable.

## 4. Reconstruction

`RefreshFromPartyState()` :

1. lie le subsystem au `PartyInventory` ;
2. scanne les `ActiveCharacters` ;
3. dérive une notification pour chaque `LastAcknowledgedLevel < Level` ;
4. trie par index de personnage ;
5. passe par le garde existant combat / player controller / modal.

Si une modal est déjà active pour le même personnage, son `NewLevel` devient la base effective afin de ne pas recréer la même notification.

## 5. Acquittement

À la fermeture de la modal :

```text
LastAcknowledgedLevel =
    max(LastAcknowledgedLevel, Notification.NewLevel)
```

Le service de Level Up ne modifie volontairement pas ce champ.

Donc :

```text
ApplyPendingLevelUp()
    Level augmente
    LastAcknowledgedLevel reste inchangé
    => gap durable
```

Si un niveau supplémentaire est gagné pendant qu'une modal est ouverte, la prochaine notification est dérivée du `NewLevel` de la modal active vers le nouveau `Level`.

## 6. Active / Pool

Le champ voyage avec le personnage :

```text
ActiveCharacters <-> CharacterPool
```

Un personnage en réserve peut donc conserver un Level Up non acquitté.

Lorsqu'il redevient actif, `OnPartyInventoryChanged` permet au subsystem déjà lié de reconstruire la notification.

Cette normalisation supprime l'ancienne limitation Active-only du snapshot MON15.6.

## 7. Initialisation

Nouveau personnage initial :

```text
LastAcknowledgedLevel = Level = 1
```

Custom recruit :

```text
LastAcknowledgedLevel = Level = 1
```

Story companion :

```text
LastAcknowledgedLevel = Definition.Level
```

Ainsi, un compagnon scénarisé commençant au niveau >1 n'affiche pas un faux Level Up lors de sa première activation.

## 8. Continue / restauration UI

Le SaveGame ne déclenche plus de restauration de subsystem pendant la désérialisation.

La projection UI est déclenchée lorsque le Pawn est réellement prêt :

```text
automatic Continue
    AGrimrockPartyPawn::BeginPlay()
    -> RefreshFromPartyState()

manual LoadCurrentGame()
    setup runtime + input + menus
    -> RefreshFromPartyState()
```

Cela supprime les retries historiques `PartyNotReady`.

## 9. Validation Save

`ValidateCharacterProgression()` impose :

```text
MinimumLevel <= LastAcknowledgedLevel <= Level
```

La règle s'applique aux personnages actifs et au `CharacterPool`.

## 10. SaveGame v19

```text
CurrentSaveVersion = 19

v19
    -> LastAcknowledgedLevel durable par personnage
    -> aucune queue Level-Up persistée

v18 et antérieures
    -> rejet exact-match
    -> aucune migration
```

## 11. Tests dédiés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_9.Normalization
```

Tests :

```text
SchemaAuthority
LevelUpCreatesDurableGap
ActivePoolDurability
SaveSchemaVersion
```

Attendu : **4/4**, zéro warning.

La Characterization est conservée après refactor et doit également rester **4/4**.

## 12. Régressions prioritaires

```text
Grimrock.RPG.MON15.3
Grimrock.RPG.MON15.4
Grimrock.RPG.MON15.5
Grimrock.TechnicalDebt.TD07_3_2.SaveContract
Grimrock.TechnicalDebt.TD07_3_3_5.Characterization
Grimrock.TechnicalDebt.TD07_3_3_8.Normalization
Grimrock.Save.MON18.9.1
```

Puis Win64 Shipping.

## 13. Stop condition

- [x] LastAcknowledgedLevel durable ajouté ;
- [x] snapshot Save Level-Up supprimé ;
- [x] miroir statique supprimé ;
- [x] restore/retry persistant supprimé ;
- [x] queue UI maintenue transient ;
- [x] reconstruction depuis état durable ajoutée ;
- [x] acquittement écrit dans le personnage ;
- [x] Active/Pool normalisés ;
- [x] initialisation nouveaux personnages normalisée ;
- [x] Continue reconnecté après restauration du Pawn ;
- [x] SaveGame v19 exact-match ;
- [x] tests dédiés ajoutés ;
- [ ] build UE5.5.4 vert ;
- [ ] Normalization 4/4 ;
- [ ] Characterization 4/4 post-refactor ;
- [ ] régressions progression / Save vertes ;
- [ ] Shipping Win64 vert.

Prochaine tranche après validation complète :

```text
TD07.3.3.10 — Audit remaining character snapshots / close Character State Normalization
```
