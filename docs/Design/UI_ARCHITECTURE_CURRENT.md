# UI Architecture Current State

## Audit reference

HEAD audité : `0eb8dd576763d0afc177ffb9cccf95e46433ca9a`

Dernier jalon : `Implement MON18.7a spellbook UI hotbar bridge`.

## Organisation actuelle

Le dossier `Content/GrimrockPrototype/Blueprints/UI/` contient déjà une séparation partielle par domaines :

- `MainMenu/` : menus de démarrage et options.
- `Combat/` : interface combat.
- `RPG/` : widgets RPG.
- Widgets racine : inventaire, journal, carte, codex, recettes.

Widgets identifiés :

- `WBP_GridInventory`
- `WBP_GridJournal`
- `WBP_GridMap`
- `WBP_GridCodex`
- `WBP_GridRecipes`
- `WBP_GridCombatHud`

## Flux actuel

L'interface est actuellement organisée autour de widgets spécialisés.

Le Spellbook MON18.7a a ajouté un pont logique vers la hotbar, mais il n'existe pas encore de conteneur UI global fédérant les écrans RPG.

## Navigation actuelle

La navigation repose encore sur des ouvertures de widgets spécifiques.

Absence constatée :

- gestionnaire central de panneaux joueur ;
- état global de navigation ;
- stratégie commune d'ouverture/fermeture ;
- navigation clavier commune.

## Dépendances

Les systèmes déjà existants sont principalement :

- données C++ data-driven ;
- widgets Blueprint ;
- hotbar combat ;
- inventaire personnage ;
- spellbook runtime.

## Points faibles

1. Risque de multiplication d'écrans isolés.
2. Navigation incohérente entre inventaire, compétences, sorts et journal.
3. Difficulté d'ajouter de nouveaux modules RPG.
4. Absence d'un shell UI joueur persistant.

## Conclusion

Avant MON18.7b, il faut introduire un conteneur UI principal `WBP_GrimrockMenu` chargé de coordonner les modules RPG sans déplacer leur logique métier.
