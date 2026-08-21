# UI01.3 — Navigation Menu

## Objectif

Définir le comportement du futur `WBP_GrimrockMenu` comme conteneur principal de l'interface joueur.

Cette étape décrit le contrat de navigation avant toute modification Blueprint.

## Responsabilité de WBP_GrimrockMenu

`WBP_GrimrockMenu` devient le shell UI joueur.

Il est responsable de :

- ouverture et fermeture du menu joueur ;
- sélection du panneau actif ;
- visibilité des modules UI ;
- conservation de l'état de navigation courant ;
- transmission des demandes utilisateur aux widgets spécialisés.

Il ne contient pas :

- logique d'inventaire ;
- règles de magie ;
- progression personnage ;
- calculs gameplay.

## Panneaux gérés

```text
WBP_GrimrockMenu
|
+-- Inventory
+-- Skills
+-- Spellbook
+-- Journal
+-- Map
+-- Codex
+-- Recipes
```

## Modèle de navigation

Le menu fonctionne avec un panneau actif unique.

Flux :

```text
Input joueur
    |
    v
WBP_GrimrockMenu
    |
    v
SelectPanel(PanelId)
    |
    v
Afficher widget cible
Masquer widgets secondaires
```

## Contrat Blueprint futur

Fonctions attendues :

- OpenMenu()
- CloseMenu()
- SelectPanel()
- GetCurrentPanel()
- RefreshCurrentPanel()

## Interaction C++

Le C++ fournit les données via les systèmes existants :

```text
Gameplay Systems
        |
        v
UI Libraries / View Models
        |
        v
WBP Widgets
```

Le widget de navigation ne doit pas dépendre directement des systèmes métier.

## Contraintes

- pas de création d'écran Spellbook isolé ;
- pas de duplication des systèmes MON18 ;
- pas de modification inutile des assets binaires ;
- conserver une architecture data-driven.

## Étapes suivantes

- création Blueprint de `WBP_GrimrockMenu` ;
- branchement des panneaux existants ;
- intégration Spellbook ;
- validation PIE.
