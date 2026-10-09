# Sauvegarde et persistance — Fondation d'architecture

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> `UGrimrockPartySaveGame::CurrentSaveVersion = 24`. Le prototype utilise un
> contrat **exact-match** : aucune migration arrière n'est maintenue.

## 1. Principe

Le Save contient uniquement l'état mutable nécessaire à un Continue cohérent.
Les définitions permanentes restent dans les DataAssets et les projections
reconstructibles ne deviennent pas des autorités persistantes.

```text
Authoring DataAssets
    -> état initial

Runtime state
    -> état vivant

SaveGame
    -> snapshot durable minimal du runtime
```

## 2. Enveloppe courante

`UGrimrockPartySaveGame` contient :

```text
SaveVersion
PartyInventoryState
DungeonRuntimeState
CurrentDungeonLevelId
PartyCellX
PartyCellY
PartyFacing
```

Contrat :

```text
SaveVersion == 24
    -> ValidateCurrentState()
    -> restore

SaveVersion != 24
    -> reject
```

Il n'existe plus de `MinimumCompatibleSaveVersion`, de pipeline général de
migration historique ou de réécriture silencieuse du numéro de version.

## 3. État personnage

L'état personnage durable voyage directement dans
`FGridPartyInventoryState::ActiveCharacters` et `CharacterPool`.

```text
Durable
    CharacterId / identity
    ClassId
    RaceId
    PortraitGender
    PortraitVariantId
    Experience
    SelectedClassProgressionChoiceIds
    Attributes
    Resources
    SkillRanks
    KnownSpellIds
    StatusEffects
    InventorySlots
    CombatHotbarSlots

Transient / reconstruit
    ClassDefinition
    ClassDisplayName
    RaceDisplayName
    Level
    DerivedStats
    Portrait
    ClassIcon
    read models UI / combat
```

Les snapshots parallèles historiques
`ClassProgressionStates`, `CharacterSkillStates`,
`CharacterSpellbookStates`, `CharacterStatusEffectStates` et
`PendingLevelUpNotifications` n'existent plus.

`LastAcknowledgedLevel` a également été supprimé : le feedback Level-Up est
désormais entièrement transitoire.

## 4. Dungeon runtime state

`FGridDungeonRuntimeState` et ses `FGridLevelRuntimeState` portent les
deltas vivants du donjon, notamment selon le niveau :

- portes et objets interactifs ;
- présence/visuels runtime nécessaires ;
- items monde et items entrants ;
- pits ;
- réceptacles ;
- monstres, placements et encounters ;
- variables Bool/Int ;
- exploration Map.

Les définitions permanentes et leur composition visuelle ne sont pas copiées
comme seconde autorité.

## 5. Receptacles

`FGridRuntimeReceptacleState` persiste actuellement :

```text
ObjectId
bCanRemoveItem
bCanInsertItems
ContainedItems[]
```

Les items contenus conservent leurs identités runtime et définition ; la
représentation Actor est reconstruite.

## 6. Monsters

L'état monstre durable conserve les informations nécessaires au restore :
identité persistante, spawn, définition, cellule/facing, ressources, Status
Effects, encounter, awareness utile et mort.

La sauvegarde durable est refusée pendant un état de combat qui ne peut pas être
capturé atomiquement. Le checkpoint pré-combat reste la politique de secours.

## 7. Map

MON21.6 persiste l'exploration dans
`FGridLevelRuntimeState::MapExploration` :

```text
ExploredCells
DiscoveredSecretObjectIds
```

Il n'existe pas de snapshot Map parallèle. Le read model cartographique est
reconstruit à la demande.

## 8. Quest — frontière encore ouverte

`UGridQuestSubsystem` possède actuellement un
`FGridCampaignQuestRuntimeState` **transient**.

Le Save v24 ne contient encore aucun snapshot Quest.

Donc :

```text
MON21.2 Quest runtime       VALIDÉ
MON21.3 Event -> Command    VALIDÉ
MON21.4 Quest Persistence   EN ATTENTE
```

La future persistance Quest devra rester identifiée par `QuestId` /
`ObjectiveId`, validée contre les définitions courantes et intégrée sans
dupliquer le runtime state.

## 9. Validation au chargement

`UGrimrockPartySaveGame::ValidateCurrentState()` est la frontière de
validation du snapshot courant. Elle doit échouer sans mutation si l'état est
incompatible avec le schéma ou les définitions canoniques.

Le chargement reconstruit les caches/projections transientes avant leur
consommation.

## 10. Historique de versions

Les passages v10…v23 restent documentés dans les tickets TD07/MON21 historiques.
Ils ne sont pas répétés ici comme contrat actif.

La seule version courante est :

```text
v24 exact-match
```

## 11. Invariants

1. Une seule autorité durable par donnée.
2. Toute donnée reconstructible reste transient.
3. Une ancienne version est rejetée, pas migrée.
4. Les identités stables remplacent les pointeurs comme vérité durable lorsque
   le domaine le permet.
5. Le restore est fail-closed sur snapshot incohérent.
6. UI/read models ne sont jamais persistés.
7. Quest Persistence ne doit pas être déclarée terminée avant MON21.4.
