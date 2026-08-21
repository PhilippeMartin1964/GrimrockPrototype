# UI01.2 — Architecture UI cible détaillée

## Objectif

Définir une architecture UI globale cohérente avant l'intégration du Spellbook.

Le principe retenu est de disposer d'un conteneur joueur unique permettant d'accéder aux différents panneaux RPG sans créer des écrans indépendants.

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
- affichage et masquage des panneaux ;
- raccourcis clavier ;
- état actif du menu ;
- orchestration des événements UI.

Ne contient pas :

- règles gameplay ;
- calculs de combat ;
- validation inventaire ;
- logique des sorts.

### WBP_GridInventory

Responsable de :

- affichage inventaire ;
- interaction avec objets ;
- déplacement d'objets ;
- interaction équipement/raccourcis.

La logique métier reste dans les composants C++ existants.

### WBP_GridSkills

Responsable de :

- affichage des compétences ;
- progression ;
- choix disponibles.

### WBP_GridSpellbook

Responsable de :

- affichage des sorts connus ;
- informations sort ;
- coûts ;
- affectation Hotbar.

La validation et l'exécution restent dans MON18.

### WBP_GridJournal

Responsable de :

- quêtes ;
- événements ;
- historique joueur.

### WBP_GridMap

Responsable de :

- exploration ;
- progression cartographique ;
- découvertes.

### WBP_GridCodex

Responsable de :

- lore ;
- bestiaire ;
- connaissances.

### WBP_GridRecipes

Responsable de :

- recettes connues ;
- fabrication ;
- ingrédients.

# Flux C++ ↔ Blueprint

Architecture :

```
C++ Data Model
        ↓
UI View Models / UI Libraries
        ↓
Blueprint Widgets
        ↓
Commandes utilisateur
```

Les Widgets :

- affichent des données préparées ;
- déclenchent des commandes ;
- ne reproduisent pas les règles métier.

## Gestion de navigation

`WBP_GrimrockMenu` possède :

- panneau actif courant ;
- références aux panneaux enfants ;
- événements Open/Close ;
- transitions éventuelles.

Flux type :

```
OpenMenu()
    |
    +-- SelectPanel(Spellbook)
            |
            +-- RefreshView()
```

## Gestion de l'état UI

L'état UI doit rester séparé du gameplay :

- onglet courant ;
- personnage sélectionné ;
- filtres d'affichage ;
- état d'ouverture.

## Principes de conception

- Pas de logique métier dans les Widgets.
- Pas de dépendance directe entre panneaux.
- Un seul point d'entrée UI joueur.
- Réutilisation des systèmes MON12/MON18.
- Architecture extensible pour futurs modules.

## Évolution prévue

UI01.3 : création du shell `WBP_GrimrockMenu`.

UI01.4 : intégration `WBP_GridSpellbook`.

UI01.5 : validation PIE et flux utilisateur.
