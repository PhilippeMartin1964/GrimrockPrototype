# Project Context - GrimrockPrototype

## 1. Vision
Développer un clone de Legend of Grimrock 2 sous Unreal Engine 5.5.4 et Visual Studio 2026.

Développer un jeu de type dungeon crawler en vue subjective, à déplacement case par case, inspiré de Legend of Grimrock 2, avec une architecture reposant sur un asset de niveau unique.

Edition de niveaux, ajout de mécanismes pour la résolution de puzzles, inventaires des fonctionnalités de Legend og Grimrock 2 comprenant toutes améliorations possibles.


Source GitHub : https://github.com/PhilippeMartin1964/GrimrockPrototype

## 2. Current Goal
Obtenir une exploration case par case façon dungeon crawler avec déplacement fluide, collisions, interactions simples et affichage 3D minimal.

Le projet doit permettre :
- l’édition directe de niveaux (Nord-Sud, Est-Ouest et Hauteur-profondeur), chacun ayant une grille 32x32 ;
- la création de géométrie jouable : cellules, murs, portes, plafonds, passages secrets ;
- le placement d’objets interactifs : boutons, leviers, plaques de pression, téléporteurs, triggers, spawns ;
- la définition de liens logiques entre objets ;
- l’ajout de mécanismes programmables via un système d’événements et un langage léger à définir ;
- l’exécution du niveau dans un runtime jouable avec déplacement, rotation, interaction et résolution d’énigmes ;
- à terme, l’ouverture à la création de niveaux par les joueurs.

## 3. Gameplay Pillars
Les principes qui doivent guider les choix.
- Exploration en grille
- Ambiance donjon
- Progression par énigmes
- Combat temps réel ou tour par tour (à privilégier)
- Gestion de groupe, inventaire, sorts, etc.

## 4. Technical Stack
Le développement se fait en C++ sous Unreal Engine 5.5.4, avec Visual Studio 2026, en privilégiant une architecture simple, modulaire et orientée données.
- Unreal Engine 5.5.4
- C++ (Visual Studio 2026)
- FAB UE5
- Le jeu devra à terme pouvoir être lancé en mode standalone. Question ouverte : Comment rendre l'éditeur de niveau standalone ?

## 5. Repository Map
Vue rapide des dossiers/fichiers importants. Structure de projet (sujet à évolution et améioration) :
C++ :
Source/
└── GrimrockPrototype/
    ├── GrimrockPrototype.Build.cs
    ├── GrimrockPrototype.cpp
    ├── GrimrockPrototype.h
    ├── Public/
    │   ├── Core/
    │   │   ├── GridTypes.h
    │   │   └── GridLevelAsset.h
    │   ├── Runtime/
    │   │   ├── GridLevelRuntimeActor.h
    │   │   ├── GridDoorActor.h
    │   │   ├── GridButtonActor.h
    │   │   ├── GridLeverActor.h
    │   │   ├── GridPressurePlateActor.h
    │   │   └── GrimrockPartyPawn.h
    │   └── EditorTools/
    │       └── GridLevelEditorActor.h
    │
    └── Private/
        ├── Core/
        │   └── GridLevelAsset.cpp
        ├── Runtime/
        │   ├── GridLevelRuntimeActor.cpp
        │   ├── GridDoorActor.cpp
        │   ├── GridButtonActor.h
        │   ├── GridLeverActor.h
        │   ├── GridPressurePlateActor.h
        │   └── GrimrockPartyPawn.cpp
        └── EditorTools/
            └── GridLevelEditorActor.cpp

UE5 :
Content/
└── Grimrock/
    ├── Core/
    │   ├── DataAssets/
    │   │   └── DA_GridLevelAsset
    │   └── Input/
    │       ├── IA_MoveForward
    │       ├── IA_MoveBackward
    │       ├── IA_TurnLeft
    │       ├── IA_TurnRight
    │       ├── IA_StrafeLeft
    │       ├── IA_StrafeRight
    │       ├── IA_Use
    │       └── IMC_Grimrock
    │
    ├── Blueprints/
    │   ├── Runtime/
    │   │   ├── BP_GridLevelRuntimeActor
    │   │   ├── BP_GridDoorActor
    │   │   └── BP_GrimrockPartyPawn
    │   │
    │   └── Editor/
    │       └── BP_GridLevelEditorActor
    │
    ├── Maps/
    │   ├── L_EditorTest
    │   └── L_RuntimeTest
    │
    ├── Meshes/
    │   ├── SM_Floor
    │   ├── SM_Wall
    │   ├── SM_Door
    │   └── SM_Ceiling
    │
    ├── Materials/
    │   ├── M_Floor
    │   ├── M_Wall
    │   ├── M_Door
    │   └── M_Ceiling
    │
    ├── Icons/
    │   └── éventuellement plus tard
    │
    └── Dev/
        └── Tests/

## 6. Current State
Ce qui fonctionne déjà.
- Création d'un premier niveau
	- Ajout de cellule avec murs et plafonds
	- Ajout d'objets; porte, pressure plate, levier, bouton, porte secrète, ...
	- Link entre les objets pour les activer
- Déplacements, touches ADSW et QE, hochement de tête RBM
- Caméra, Head bob, camera sway
- Grimrock Grid Editor, interface à améliorer
- Ouverture/fermeture de portes animées.

## 7. Known Issues
Bugs ou limites connues.
- 
- ...

## 8. Next Tasks
Liste courte et priorisée.
- [ ] Validation de l'architecture
- [ ] Implémenter toutes les mécaniques fondamentales de Legend of Grimrock 2
- [ ] Inventaire de tous les objets possibles et leur utilisation ou non utlisation (éléments de décor ou d'ambiance)
- [ ] Ajouter un inventaire de groupe ou de personnage
- [ ] Elaboration de mécaniques JdR (Race, Classe, ...)
- [ ] Tester ...

## 9. Design Decisions
Décisions déjà prises, pour éviter de les rediscuter.
- Le déplacement est basé sur une grille sur chaque niveau 32 x 32
- Les niveaux sont décrits en cellule 32 x 32  avec des murs avec pilier
- Le joueur contrôle la classe GrimrockPartyPawn
- La caméra, Head bob, camera sway, avance avortée sur case non accessible.

## 10. Open Questions
Questions encore non tranchées.
- Combat temps réel ou tour par tour ? Tour par tour
- Format des niveaux ? 
- Style graphique ? Réaliste
- Plateforme cible ? Windows 11

## 11. ChatGPT Notes
Résumé des idées importantes produites dans le projet ChatGPT.

Trop pour résumé ici... on verra à mesure.

## 12. Codex Working Notes
À utiliser par Codex pour noter les changements importants, conventions découvertes, ou prochaines pistes.
