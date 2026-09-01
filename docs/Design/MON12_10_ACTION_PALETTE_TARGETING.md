# MON12.10 — Palette d’actions et ciblage

> **Obsolète depuis HOTBAR01.2 — Remove Legacy Action Palette (01.09.2026).**  
> La palette d’actions intermédiaire a été supprimée de l’architecture runtime. Ne pas recréer `Panel_ActionPalette` dans `WBP_GridCombatHud`. Les actions sont affectées directement depuis leur source métier (inventaire, Spellbook, futures pages de capacités) vers la barre `1–9,0`. `Panel_Targeting` reste actif pour les actions nécessitant une cible.


## Objectif

Clarifier la zone d’actions du HUD sans ajouter de contenu de jeu factice.

## Comportement

- En état normal, `Panel_ActionPalette` est visible lorsqu’il contient des actions.
- Pendant un ciblage `Cell` ou `Area`, la palette est masquée et `Panel_Targeting` occupe seul la zone.
- L’annulation, la confirmation ou le changement d’état restaure automatiquement la palette.
- Le titre indique l’action et la commande d’annulation.
- `Text_TargetingCell` affiche uniquement une consigne, une raison d’invalidité ou le nombre d’ennemis concernés.
- Les coordonnées techniques `(X,Y)` ne sont plus exposées dans le HUD.

## Validation

Le test `Grimrock.Monsters.MON12.10.ActionPaletteTargeting` utilise une action de zone synthétique. Aucun sort de test n’est ajouté aux Data Assets, personnages ou sauvegardes.
