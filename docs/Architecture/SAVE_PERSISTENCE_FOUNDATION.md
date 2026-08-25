# Sauvegarde et persistance — Fondation d’architecture

Date de référence : **25 août 2026**

## Contrat courant

`UGrimrockPartySaveGame::CurrentSaveVersion = 8` et `MinimumCompatibleSaveVersion = 1`.

Le SaveGame conserve :

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

Le LevelAsset décrit l’état initial. Le dungeon runtime state décrit l’état vivant : objets, portes, items, réceptacles, monstres/placements/encounters et variables de niveau MON19.

Les monstres morts persistants restent des Actors runtime restaurés canoniquement `Dead`, cachés, sans collision ni occupation. Le durcissement MON20.10.2 autorise leur restauration même si leur ancienne cellule est occupée par le groupe, sans relâcher les règles applicables aux monstres vivants.

## RPG state

Les snapshots supplémentaires utilisent `CharacterId` comme clé stable. `CharacterPool` est déjà contenu dans `PartyInventoryState`; un personnage de réserve est donc persistant sans registre parallèle.

Persistance dédiée actuelle :

```text
ClassProgressionStates
PendingLevelUpNotifications
CharacterStatusEffectStates
CharacterSpellbookStates
CharacterSkillStates
```

Les données dérivées comme les `RequirementIds` ne sont pas sauvegardées ; elles sont reconstruites depuis les autorités persistantes.

## Migration

`FRPGSaveMigrationService` prépare les sauvegardes legacy puis applique une validation stricte du contrat courant.

Évolution récente :

```text
v7 -> v8 : snapshots SkillRanks MON20.9
```

La migration v7 -> v8 initialise un état Skill vide, car les rangs étaient volontairement runtime-only en v7. Elle n’invente aucun rang.

## Politique de sauvegarde en combat

`GridCombatSavePolicy` refuse la sérialisation durable quand l’état de combat n’est pas sauvegardable. Le tour/initiative/pending action ne sont pas un contrat durable. Le système utilise le checkpoint pré-combat pour revenir à un état cohérent.

Un log `PartySave SaveRejected ... CombatStateNotSaveable` en combat est donc attendu.

## Validation récente

MON20 a fermé la frontière complète :

```text
Grimrock.MON20 = 151/151 Success
Save v8 Result=Accepted
Continue CharacterCount=2
2 Gobelins RestoreDead
GridRuntimeState Apply DeadMonsters=2
Save v8 après Continue OK
```

## Dette technique liée à la persistance

La source autoritaire est `TECHNICAL_DEBT_REGISTER.md`.

Dette P1 confirmée : la permission runtime `bCanRemoveItem` des réceptacles peut être modifiée par commandes, mais `FGridRuntimeReceptacleState` ne la persiste pas encore. Ce point est suivi sous `TD-PERSIST-001`.

Toute future montée de version SaveGame doit répondre à un besoin d’état durable démontré et être accompagnée d’une migration/test explicite.

## Invariants

- identités valides et non ambiguës ;
- ownership d’items cohérent ;
- niveau/XP cohérents ;
- progression, spellbook et skills validables ;
- variables de niveau typées ;
- aucune sauvegarde de combat partielle ;
- aucune donnée dérivée persistée lorsqu’elle peut être reconstruite ;
- restore atomique/fail-closed sur snapshot incohérent.
