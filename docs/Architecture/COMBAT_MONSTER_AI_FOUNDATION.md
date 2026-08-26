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

## Event -> Command

`TD-EVENT-001` est **RÉSOLU**. La sémantique Gameplay / StateOnly / Unsupported est explicite et protégée par tests. Le bus Event -> Command reste le chemin d’effet gameplay et n’est pas remis en cause.

La dette encore suivie sous `TD-ARCH-005` concerne uniquement la concentration interne de `UGridActivationComponent`. Une extraction future ne doit créer ni second bus, ni second état.

## Dette technique runtime

La dette transversale est suivie dans :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

`AGridLevelRuntimeActor` reste la concentration runtime la plus forte. TD05.1 l’a rebaseliné à 3 359 lignes / 107 095 octets pour le `.cpp`, en plus d’un header public de 22 161 octets. Les extractions Persistence et World Items existent déjà ; la prochaine frontière ciblée est Diagnostics, après caractérisation.

Ce constat ne justifie pas un framework de combat/IA parallèle ni un refactor massif des monstres.
