# MON16.7 — Save / Restore des status effects

## Statut

**IMPLEMENTED — validation UE5.5.4 en attente.**

Base :

```text
71f38ea0638bdd99c81e268b26afc768fb196f57
Close MON16.6 status HUD feedback
```

MON16.7 rend persistants les status effects introduits par MON16.1–6 sans sérialiser les pointeurs runtime ni créer un second système de statuts.

## 1. Snapshot persistant

Le runtime reste inchangé :

```text
FGridStatusEffectRuntimeState
- EffectId
- SourceId
- StackCount
- DurationUnit
- RemainingDuration
- Potency
- DefinitionAsset (Transient)
```

MON16.7 ajoute :

```text
FGridStatusEffectSaveState
- EffectId
- SourceId
- StackCount
- DurationUnit
- RemainingDuration
- Potency
```

`DefinitionAsset` est volontairement absent du format SaveGame.

`EffectId` reste l'identité stable. La définition statique est rechargée au restore à partir du PrimaryAssetId :

```text
GridStatusEffect:EffectId
```

## 2. Service de conversion

Nouveau service :

```text
FGridStatusEffectPersistence
```

Responsabilités :

- validation structurelle d'une collection sauvegardée ;
- capture runtime -> snapshot ;
- restauration snapshot -> runtime ;
- résolution de la définition canonique par `EffectId` ;
- ordre déterministe par `EffectId` ;
- rejet des doublons ;
- restauration atomique.

### Capture

Une capture est refusée si :

- l'état runtime est invalide ;
- `DefinitionAsset` est absent ou invalide ;
- l'EffectId runtime ne correspond plus à la définition ;
- l'unité de durée ne correspond plus ;
- le nombre de stacks dépasse `MaxStacks`.

Aucun pointeur UObject n'entre dans le snapshot.

### Restore

La restauration construit d'abord une collection candidate complète.

Elle est refusée si :

- le snapshot est structurellement invalide ;
- un EffectId est dupliqué ;
- la définition canonique est introuvable ;
- la définition a changé d'EffectId ;
- l'unité de durée ne correspond plus ;
- le nombre de stacks dépasse le contrat actuel ;
- `BuildRuntimeState()` refuse la réhydratation.

En cas d'échec, la collection runtime préexistante reste intacte.

## 3. Résolution des définitions

La restauration de production utilise `UAssetManager`.

Ordre :

1. PrimaryAssetId `GridStatusEffect:EffectId` ;
2. objet primaire déjà chargé si disponible ;
3. `GetPrimaryAssetPath()` ;
4. si nécessaire, scan synchrone des `GridStatusEffect` sous `/Game` ;
5. chargement du chemin canonique ;
6. validation du PrimaryAssetId obtenu.

Un EffectId inconnu n'est jamais ignoré silencieusement : la restauration échoue.

## 4. Groupe

`FGridCharacterInventoryState::StatusEffects` reste `Transient`.

`UGrimrockPartySaveGame` ajoute :

```text
FGridCharacterStatusEffectSaveState
- CharacterId
- StatusEffects[]

CharacterStatusEffectStates[]
```

Les personnages actifs et le `CharacterPool` sont couverts.

Seuls les personnages possédant au moins un effet actif génèrent un snapshot. Le `CharacterId` doit être valide et désigner exactement un personnage dans l'état de groupe.

Au chargement :

1. copie candidate de `PartyInventoryState` ;
2. remise à zéro des collections runtime candidates ;
3. restauration de toutes les collections sauvegardées ;
4. commit de la copie candidate uniquement si tout est valide.

La restauration du groupe est donc atomique.

## 5. Monstres

`AGridMonsterActor::StatusEffects` reste `Transient`.

`FGridRuntimeMonsterState` ajoute :

```text
StatusEffects : TArray<FGridStatusEffectSaveState>
```

La persistance réutilise le pipeline MON13 existant :

```text
AGridMonsterActor::CaptureRuntimeMonsterState()
AGridMonsterActor::RestoreRuntimeMonsterState()
```

Il n'existe pas de second registre de monstres.

Les snapshots présents dans `FGridLevelRuntimeState::Monsters` et dans les `MonsterPlacements` sont donc naturellement embarqués dans `FGridDungeonRuntimeState`.

## 6. SaveGame version 5

MON16.7 fait évoluer :

```text
CurrentSaveVersion : 4 -> 5
MinimumCompatibleSaveVersion : 1 (inchangé)
```

### Migration v4 -> v5

La version 4 contient déjà l'état de progression autoritatif MON15.6.

Elle ne doit surtout pas être traitée comme une sauvegarde legacy v1-v3.

La migration v4 -> v5 :

- valide le snapshot MON15.6 existant ;
- conserve `PartyInventoryState` ;
- conserve `ClassProgressionStates` ;
- conserve `PendingLevelUpNotifications` ;
- conserve le dungeon runtime ;
- initialise les nouveaux status snapshots à vide ;
- effectue zéro réconciliation de niveau.

### Versions v1-v3

Le chemin de migration historique MON15.6 reste inchangé pour la progression. Les nouvelles collections de statuts sont initialisées à vide.

## 7. Sérialisation

À la sauvegarde :

```text
capture status party
-> capture progression MON15.6
-> capture pending level-ups
-> validation courante
-> Super::Serialize
```

Au chargement :

```text
Super::Serialize
-> migration / validation version
-> restore status party
-> restore progression
-> restore pending level-ups
```

Les monstres sont restaurés ensuite par le pipeline de niveau existant.

## 8. Compatibilité avec MON16.1–6

Après restauration, les mêmes `FGridStatusEffectRuntimeState` sont reconstruits avec leur `DefinitionAsset` valide.

Cela réactive automatiquement les systèmes existants :

- MON16.2 : durée / expiration ;
- MON16.3 : dégâts périodiques ;
- MON16.4 : InitiativeModifier ;
- MON16.5 : Stun / Silence / Immobilize ;
- MON16.6 : projection HUD / feedback.

MON16.7 ne duplique aucune de ces règles.

## 9. Hors périmètre

MON16.7 n'ajoute pas :

- de nouvelle logique de status effect ;
- de nouvel effet hard-coded ;
- de second lifecycle ;
- de second système de monstres ;
- de modification WBP ;
- de `.uasset` / `.umap` ;
- de VFX/audio ;
- de nouvelle dépendance de module.

## 10. Automation

Namespace :

```text
Grimrock.RPG.MON16.7
```

Tests :

```text
CollectionCapture
CollectionRestore
AtomicRestoreFailure
DuplicateEffectRejected
DefinitionContractMismatch
PartyActiveAndPoolRoundTrip
PartyAtomicFailure
V4MigrationPreservesProgression
MonsterSnapshotContract
SaveVersionContract
TransientRuntimeBoundary
```

Attendu : **11/11 Success**.

Après validation ciblée, relancer MON16.6 -> MON14 selon la checklist.

Prochaine étape après validation : **MON16.8 — clôture / régression finale du milestone Status Effects**.
