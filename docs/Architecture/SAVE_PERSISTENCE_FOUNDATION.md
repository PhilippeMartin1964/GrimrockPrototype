# Sauvegarde et persistance — Fondation d’architecture

## Contrat courant

`UGrimrockPartySaveGame::CurrentSaveVersion = 7` et `MinimumCompatibleSaveVersion = 1`.

Le SaveGame conserve :

- `FGridPartyInventoryState` ;
- progression de classe ;
- notifications Level Up ;
- Status Effects ;
- Spellbooks ;
- `FGridDungeonRuntimeState` ;
- niveau courant ;
- cellule et orientation du groupe.

## Dungeon state

Le LevelAsset décrit l’état initial. Le dungeon runtime state décrit l’état vivant : objets, portes, items, réceptacles, monstres/placements et variables de niveau MON19.

## RPG state

Les snapshots supplémentaires utilisent `CharacterId` comme clé stable. `CharacterPool` est déjà contenu dans `PartyInventoryState`; un personnage de réserve est donc persistant sans registre parallèle.

## Migration

`FRPGSaveMigrationService` prépare les sauvegardes legacy puis applique une validation stricte du contrat courant. Les migrations récentes ont ajouté spellbooks puis variables de niveau sans détruire les domaines déjà autoritaires.

## Politique de sauvegarde en combat

`GridCombatSavePolicy` refuse la sérialisation durable quand l’état de combat n’est pas sauvegardable. Le tour/initiative/pending action ne sont pas un contrat durable. Le système utilise le checkpoint pré-combat pour revenir à un état cohérent.

## MON20

MON20.3 n’exige pas de version 8 : `URPGStoryCompanionAsset.CharacterId` est copié dans `FGridCharacterInventoryState`, déjà persistant. Une v8 ne sera introduite que lorsqu’un nouveau champ durable (par exemple type de membre/réserve) aura un besoin runtime démontré et une migration définie.

## Invariants

- identités valides et non ambiguës ;
- ownership d’items cohérent ;
- niveau/XP cohérents ;
- progression et spellbook validables ;
- variables de niveau typées ;
- aucune sauvegarde de combat partielle.
