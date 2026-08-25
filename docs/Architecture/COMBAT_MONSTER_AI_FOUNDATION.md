# Combat, monstres et IA — Fondation d’architecture

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

- Rat géant : baseline mêlée et vertical slice historique ;
- Gobelin lanceur : attaque à distance/projectile et maintien de distance.

## Récompenses et présentation

Mort, dissolve, loot et XP sont séparés du planner. Les récompenses s’intègrent au pipeline MON15.

## Persistance

L’état vivant des monstres est capturé dans le dungeon runtime state. La sauvegarde durable est bloquée pendant un combat actif afin de ne pas sérialiser un tour partiel.

MON20.10 a également verrouillé la restauration des monstres déjà morts : Actor conservé, état `Dead`, mesh caché, collision et occupation désactivées.

## Dette technique vs contenu

Le petit nombre de familles de monstres **n'est pas une dette technique**. C'est un manque de contenu de production : l'architecture existante doit d'abord être exploitée avec davantage de variantes avant d'envisager un nouveau framework d'IA.

La dette technique transversale liée au runtime est suivie dans :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Elle concerne notamment la concentration de `AGridLevelRuntimeActor`, certaines frontières Event -> Command et la maintenabilité des gros orchestrateurs ; elle ne remet pas en cause le modèle de combat/IA actuel.
