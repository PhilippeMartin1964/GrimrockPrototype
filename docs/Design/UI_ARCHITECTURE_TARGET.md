# UI Architecture Target

## Objectif

Créer une architecture UI globale avant l'intégration complète du Spellbook.

## Conteneur principal

`WBP_GrimrockMenu` devient le shell UI joueur.

Structure cible :

```
WBP_GrimrockMenu
├── WBP_GridInventory
├── WBP_GridSkills
├── WBP_GridSpellbook
├── WBP_GridJournal
├── WBP_GridMap
├── WBP_GridCodex
└── WBP_GridRecipes
```

## Responsabilités

### WBP_GrimrockMenu

Responsable de :

- navigation entre modules ;
- affichage/masquage des panneaux ;
- raccourcis clavier ;
- état actif du menu.

Il ne contient pas de logique métier.

### Widgets spécialisés

Chaque module reste autonome :

- Inventory : objets et équipement.
- Skills : compétences.
- Spellbook : connaissance des sorts et affectation hotbar.
- Journal : quêtes et événements.
- Map : exploration.
- Codex : connaissances.
- Recipes : artisanat.

## Flux C++ ↔ Blueprint

Principe :

```
C++ Data Model
      ↓
UI View Models / Libraries
      ↓
Blueprint Widgets
      ↓
Interaction joueur
```

Les widgets affichent et déclenchent des commandes, mais ne possèdent pas la logique système.

## Évolution prévue

Étapes :

1. Audit UI actuel.
2. Création du conteneur `WBP_GrimrockMenu`.
3. Navigation entre panneaux.
4. Intégration `WBP_GridSpellbook`.
5. Validation PIE.

## Contraintes

- ne pas modifier inutilement les assets binaires ;
- conserver l'architecture data-driven existante ;
- réutiliser les systèmes MON18 existants ;
- éviter les duplications de logique.
