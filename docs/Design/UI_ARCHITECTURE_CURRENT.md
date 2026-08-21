# UI Architecture Current State

## Audit reference

HEAD audité : `0eb8dd576763d0afc177ffb9cccf95e46433ca9a`

Dernier jalon : `Implement MON18.7a spellbook UI hotbar bridge`.

## Organisation actuelle

Le dossier `Content/GrimrockPrototype/Blueprints/UI/` contient déjà une séparation partielle par domaines :

- `MainMenu/` : menus de démarrage et options.
- `Combat/` : interface combat.
- `RPG/` : widgets RPG.
- Widgets spécialisés : inventaire, journal, carte, codex, recettes.

Widgets identifiés :

- `WBP_GrimrockMenu`
- `WBP_GridInventory`
- `WBP_GridJournal`
- `WBP_GridMap`
- `WBP_GridCodex`
- `WBP_GridRecipes`
- `WBP_GridCombatHud`

## Etat actuel de WBP_GrimrockMenu

Contrairement à une création future, `WBP_GrimrockMenu` existe déjà dans le projet.

Son rôle actuel est de fournir l'accès à l'interface joueur, notamment à l'inventaire :

```
WBP_GrimrockMenu
        |
        +-- WBP_GridInventory
```

Cet écran est fonctionnel et doit être conservé.

L'objectif UI n'est donc pas de remplacer ce widget, mais de le faire évoluer progressivement vers un conteneur global des modules RPG.

## Flux actuel

L'interface repose déjà sur des widgets spécialisés.

Le flux actuel doit être audité avant extension :

- ouverture de `WBP_GrimrockMenu` ;
- création et affichage du widget ;
- connexion avec l'inventaire ;
- fermeture et retour au jeu.

## Navigation actuelle

La navigation existante est centrée sur le fonctionnement actuel du menu inventaire.

Points à compléter pour l'évolution RPG :

- gestion centralisée des panneaux joueur ;
- état global de navigation ;
- stratégie commune d'ouverture/fermeture ;
- navigation clavier commune entre modules.

## Dépendances

Les systèmes existants sont principalement :

- données C++ data-driven ;
- widgets Blueprint ;
- inventaire personnage ;
- hotbar combat ;
- spellbook runtime MON18.

## Points faibles

1. `WBP_GrimrockMenu` est limité actuellement à son usage inventaire.
2. Les futurs modules RPG doivent être intégrés sans casser l'existant.
3. La navigation entre écrans doit être harmonisée.
4. L'état UI global doit être séparé de la logique gameplay.

## Conclusion

Avant MON18.7b, il faut faire évoluer `WBP_GrimrockMenu` existant afin qu'il devienne progressivement le conteneur UI joueur global.

Cette évolution doit respecter les principes suivants :

- conserver l'inventaire actuel ;
- ajouter les nouveaux panneaux progressivement ;
- ne pas déplacer la logique métier dans les widgets ;
- réutiliser les systèmes C++ existants.
