# Sauvegarde et persistance — Fondation d’architecture

Date de référence : **26 août 2026**

## Contrat courant

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 9
UGrimrockPartySaveGame::MinimumCompatibleSaveVersion = 1
```

Le SaveGame conserve notamment :

- `FGridPartyInventoryState` ;
- progression de classe / talents ;
- notifications Level Up ;
- Status Effects ;
- Spellbooks ;
- Skill ranks MON20 ;
- `FGridDungeonRuntimeState` ;
- niveau courant ;
- cellule et orientation du groupe.

## Dungeon state

Le `UGridLevelAsset` décrit l’état initial. Le dungeon runtime state décrit l’état vivant : présence d’objets, portes, items, réceptacles, monstres/placements/encounters, variables de niveau et permissions runtime persistantes.

Les monstres morts persistants restent des Actors runtime restaurés canoniquement `Dead`, cachés, sans collision ni occupation. Le durcissement MON20.10.2 autorise leur restauration même si leur ancienne cellule est occupée par le groupe, sans relâcher les règles applicables aux monstres vivants.

## Réceptacles — Save v9

TD01.1 a fermé `TD-PERSIST-001` :

```text
FGridRuntimeReceptacleState::bCanRemoveItem
```

est désormais capturé et restauré. SaveGame est passé de v8 à v9. Les sauvegardes v1-v8 migrent avec la politique legacy explicite :

```text
bCanRemoveItem = true
```

afin de préserver le comportement historique des sauvegardes antérieures.

## RPG state

Les snapshots supplémentaires utilisent `CharacterId` comme clé stable. `CharacterPool` est contenu dans `PartyInventoryState`; un personnage de réserve est donc persistant sans registre parallèle.

Persistance dédiée :

```text
ClassProgressionStates
PendingLevelUpNotifications
CharacterStatusEffectStates
CharacterSpellbookStates
CharacterSkillStates
```

Les données dérivées comme les `RequirementIds` ne sont pas sauvegardées ; elles sont reconstruites depuis les autorités persistantes.

## Migrations principales récentes

```text
v7 -> v8 : snapshots SkillRanks MON20.9
v8 -> v9 : permission de retrait des réceptacles TD01.1
```

`FRPGSaveMigrationService` prépare les sauvegardes legacy puis applique la validation stricte du contrat courant. Toute future montée de version doit correspondre à un état durable démontré et être accompagnée d’une migration et de tests explicites.

## Politique de sauvegarde en combat

`GridCombatSavePolicy` refuse la sérialisation durable quand l’état de combat n’est pas sauvegardable. Tour, initiative et pending action ne constituent pas un contrat durable. Le système utilise le checkpoint pré-combat pour revenir à un état cohérent.

Un log `PartySave SaveRejected ... CombatStateNotSaveable` en combat est attendu.

## Validation récente

La frontière de persistance est couverte par les validations MON20 et TD01.1. Les tests TD01.1 couvrent notamment :

```text
DisabledRoundTrip
EnabledRoundTrip
V8Migration
```

Les tests `Grimrock.MON20.9.SkillPersistence` sont restés verts après la migration v9.

## Dette technique liée à la persistance

La source autoritaire est `TECHNICAL_DEBT_REGISTER.md`.

`TD-PERSIST-001` est **RÉSOLU**. Il n’existe actuellement aucune dette P1 de persistance ouverte dans le registre. Une future donnée Quest/Map/Codex ne devra déclencher une nouvelle SaveVersion que lorsqu’elle devient réellement autoritaire et persistante.

## Invariants

- identités valides et non ambiguës ;
- ownership d’items cohérent ;
- niveau/XP cohérents ;
- progression, Spellbook et Skills validables ;
- variables de niveau typées ;
- permission runtime des réceptacles restaurée ;
- aucune sauvegarde de combat partielle ;
- aucune donnée dérivée persistée lorsqu’elle peut être reconstruite ;
- restore atomique/fail-closed sur snapshot incohérent.
