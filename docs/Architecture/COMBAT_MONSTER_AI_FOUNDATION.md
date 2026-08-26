# Combat, monstres et IA — Fondation d’architecture

Date de référence : **26 août 2026**

## Combat

`UGridTurnManagerComponent` est l’orchestrateur du combat. Son implémentation est répartie entre initiative, phases, tours joueurs, actions, catalogue et mouvement du groupe. Le combat reste déterministe et basé sur la grille.

### Ressources

- PA individuels ;
- PAM communs ;
- mana ;
- quantités d’items ;
- cooldowns.

Les actions sont payées transactionnellement : une action refusée ne doit pas laisser de ressource partiellement consommée.

### Ciblage et résolution

Les règles utilisent cellules, arêtes, portée, murs, portes et première cible bloquante. `GridCombatResolver` applique la résolution ; les composants de présentation, audio/VFX et HUD ne décident pas du résultat.

## Monstres

`UGridMonsterDefinitionAsset` porte la définition data-driven. `AGridMonsterActor` s’appuie sur des composants spécialisés de mouvement, comportement, combat, mort, audio, VFX et variations idle.

### Spawn et encounters

`MonsterSpawn` est un objet de niveau persistant. Les placements gardent spawn/despawn, cellule, orientation, état de combat et mort. Les encounter groups supportent vagues et `EncounterCompleted`.

### IA exploration

- occupation et pathfinding sur grille ;
- perception directionnelle ;
- engagement automatique ;
- dormance/réveil ;
- patrouille ;
- investigation ;
- alarmes ;
- planners spécialisés (`RangedKeeper`, ranged attack, fast harasser).

Le NavMesh ne remplace pas l’autorité de la grille.

## Familles de référence

- Rat géant : baseline mêlée ;
- Gobelin lanceur : attaque à distance/projectile et maintien de distance.

Le petit nombre de familles de monstres n’est pas une dette technique : il s’agit d’un manque de contenu de production.

## Persistance

L’état vivant des monstres est capturé dans le dungeon runtime state. La sauvegarde durable est bloquée pendant un combat actif afin de ne pas sérialiser un tour partiel. MON20.10 a verrouillé la restauration des monstres déjà morts : Actor conservé, état `Dead`, mesh caché, collision et occupation désactivées.

SaveGame courant : **v9**.

## Event -> Command et Quests

`TD-EVENT-001` est **RÉSOLU**. La sémantique Gameplay / StateOnly / Unsupported est explicite et protégée par tests. Le bus Event -> Command reste le chemin d’effet gameplay.

MON21.3 ajoute les commandes campagne :

```text
QuestStart
QuestCompleteObjective
QuestComplete
QuestFail
```

Elles délèguent à `UGridQuestSubsystem`, sans créer de second bus ni de second état Quest.

La dette encore suivie sous `TD-ARCH-005` concerne uniquement la concentration interne de `UGridActivationComponent`. Une extraction future ne doit créer ni second bus, ni second état.

## Dette technique runtime

Le registre autoritaire est :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Les deux gros orchestrateurs audités ont atteint leur stop condition :

```text
TD05.9  AGridLevelRuntimeActor
TD06.9  UGridPartyInventoryComponent
```

Le RuntimeActor reste volontairement façade du niveau ; PartyInventory reste l’autorité groupe/inventaire. Aucun refactor massif de Combat/IA n’est justifié par ces clôtures.

Réouvrir une dette structurelle uniquement en présence d’un signal concret : duplication d’autorité, régression récurrente, difficulté de test ou nouvelle responsabilité autonome importante.
