# Jalon — Runtime Dungeon State

## Objectif du jalon

Ce jalon valide la couche d’état runtime et sa persistance disque. Le jeu conserve l’état vivant des niveaux pendant les transitions et l’embarque dans le `UGrimrockPartySaveGame` utilisé par Continuer.

## Principe d’architecture

`UGridLevelAsset` représente l’état initial éditable d’un niveau. Il décrit les cellules, les murs, les objets, les réceptacles, les portes, les liens et les transitions tels qu’ils existent au départ.

`UGridDungeonAsset` représente la liste organisée des niveaux du donjon. Il associe des identifiants de niveau à des `UGridLevelAsset` et permet de définir la structure logique du donjon.

`FGridDungeonRuntimeState` représente l’état vivant du donjon. La propriété de l’Actor reste transitoire, mais son contenu est copié dans le `SaveGame` versionné lors d’une sauvegarde.

`FGridLevelRuntimeState` représente l’état vivant d’un niveau donné. Il permet de retrouver l’état d’un niveau déjà visité lorsque le joueur y revient.

Lors d’une transition entre niveaux :

1. l’état du niveau courant est capturé ;
2. le `LevelAsset` cible est chargé ;
3. le niveau est reconstruit ;
4. l’état runtime du niveau cible est réappliqué s’il existe ;
5. le groupe est placé sur la cellule de destination avec l’orientation cible.

Le `LevelAsset` reste donc la source de vérité initiale, tandis que le `RuntimeState` devient la source de vérité pendant la session.

## Périmètre validé

Les éléments suivants sont considérés comme validés pour le prototype actuel :

- portes ouvertes et fermées ;
- état de blocage cohérent des portes ;
- objets ramassés ;
- items placés au sol ;
- supports de torche ;
- réceptacles ;
- alcôves physiques ;
- contenus initiaux retirés ;
- items restaurés avec identité runtime stable ;
- inventaire et équipement du groupe ;
- monstres blessés, déplacés, désactivés ou morts ;
- orientation, armures et état logique des monstres ;
- corps morts visibles et non bloquants ;
- butin MON8 restauré par le système d’items sans nouvelle génération ;
- transitions entre niveaux ;
- conservation après un aller-retour, un arrêt du PIE et Continuer.

Ce périmètre permet déjà de tester un donjon multi-niveaux jouable sans que chaque niveau revienne automatiquement à son état de départ après une transition.

## Persistance disque et limites

La sauvegarde disque via `UGrimrockPartySaveGame` est disponible. Elle contient l’inventaire, les objets au sol, portes, interactifs, réceptacles, niveaux du donjon et, depuis MON9, les monstres.

Les sauvegardes version 1 restent compatibles. Elles ne contiennent pas de map `Monsters`, donc les monstres gardent leur état initial. La sauvegarde suivante réécrit le slot en version 2.

Ne sont volontairement pas repris :

- un combat actif au milieu d’un tour ;
- les actions ennemies et le monstre courant ;
- les réservations de cellules ;
- les positions interpolées ;
- les timers, Montages et animations transitoires ;
- `CombatRandomStream` ;
- les effets temporaires et scripts futurs non sérialisés.

Après chargement, le TurnManager revient à `Exploration`. La perception normale peut ensuite démarrer un nouveau combat.

## Règles importantes

- Les `DataAssets` ne doivent pas être modifiés pendant le runtime.
- La propriété runtime de l’Actor est `Transient`, puis copiée explicitement dans le `SaveGame`.
- Le `LevelAsset` reste la source de vérité initiale.
- Le `RuntimeState` devient la source de vérité pendant la session.
- Les identifiants `ObjectId` et `PersistentMonsterId` doivent rester uniques dans leur niveau.
- Dupliquer manuellement un `LevelAsset` sans régénérer les `ObjectId` peut produire des comportements incohérents pendant le développement.
- La création de niveau via le bouton `New Level` est le workflow recommandé.

## Workflow validé

Le workflow suivant est validé :

1. ouvrir `L_GrimrockEditor` ;
2. choisir ou créer un niveau via `DUNGEON LEVELS` ;
3. placer des objets et des escaliers ;
4. configurer les transitions ;
5. cliquer directement sur `Play` ;
6. modifier l’état du niveau en jeu : portes, torches, objets, réceptacles ;
7. changer de niveau ;
8. revenir ;
9. vérifier que l’état du niveau est conservé.

Ce workflow confirme que le donjon peut désormais être parcouru sur plusieurs niveaux tout en conservant un état vivant cohérent pendant la session.

## Résultat du jalon

Le Runtime Dungeon State est validé en mémoire et sur disque pour le groupe, les items, les mécanismes et les monstres. La limitation principale restante est le futur pipeline natif qui créera les Actors depuis les placements `MonsterSpawn`.
