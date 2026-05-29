# Jalon — Runtime Dungeon State

## Objectif du jalon

Ce jalon valide la couche d’état runtime du donjon, c’est-à-dire la capacité du jeu à conserver l’état vivant des niveaux pendant une session lorsque le joueur passe d’un niveau à un autre.

L’objectif n’est pas de sauvegarder définitivement une partie sur disque, mais de garantir qu’un donjon multi-niveaux ne revient pas systématiquement à son état initial à chaque transition.

## Principe d’architecture

`UGridLevelAsset` représente l’état initial éditable d’un niveau. Il décrit les cellules, les murs, les objets, les réceptacles, les portes, les liens et les transitions tels qu’ils existent au départ.

`UGridDungeonAsset` représente la liste organisée des niveaux du donjon. Il associe des identifiants de niveau à des `UGridLevelAsset` et permet de définir la structure logique du donjon.

`FGridDungeonRuntimeState` représente l’état vivant du donjon pendant la session. Il est conservé en mémoire uniquement.

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
- transitions entre niveaux ;
- conservation de l’état après un aller-retour entre niveaux.

Ce périmètre permet déjà de tester un donjon multi-niveaux jouable sans que chaque niveau revienne automatiquement à son état de départ après une transition.

## Ce qui n’est pas encore couvert

Le Runtime Dungeon State est validé comme état vivant en mémoire, mais il ne constitue pas encore un système complet de sauvegarde de partie.

Ne sont pas encore couverts :

- sauvegarde disque via `USaveGame` ;
- persistance après arrêt du PIE ;
- persistance après fermeture du jeu ;
- menu `Continuer` ;
- sauvegarde complète des monstres, combats, scripts, timers et effets temporaires ;
- versioning de sauvegarde si les `LevelAssets` changent pendant le développement.

À ce stade, arrêter le PIE ou quitter le jeu réinitialise donc l’état vivant du donjon. Une nouvelle session repart de l’état initial des `DataAssets`.

## Règles importantes

- Les `DataAssets` ne doivent pas être modifiés pendant le runtime.
- Le `RuntimeState` est `Transient`.
- Le `LevelAsset` reste la source de vérité initiale.
- Le `RuntimeState` devient la source de vérité pendant la session.
- Les identifiants `ObjectId` doivent rester uniques au sein d’un donjon.
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

Le jalon Runtime Dungeon State est validé pour le prototype actuel.

La prochaine extension naturelle de cette couche sera son branchement ultérieur sur un système `USaveGame`, lorsque le gameplay de base sera suffisamment stabilisé.
