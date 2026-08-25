# GrimrockPrototype

Prototype Unreal Engine 5.5.4 d’un dungeon crawler en vue subjective, à déplacement case par case, inspiré de *Legend of Grimrock 2*.

Le projet vise à développer une architecture simple, modulaire et orientée données, reposant sur un asset de niveau unique permettant de définir, éditer et exécuter un donjon jouable sur une grille 32x32.

## Objectif du projet

`GrimrockPrototype` est un prototype de jeu de type dungeon crawler old-school avec :

- déplacement case par case ;
- rotation à 90 degrés ;
- exploration en vue subjective ;
- niveaux construits sur une grille 32x32 ;
- génération runtime de la géométrie ;
- placement d’objets interactifs ;
- portes, boutons, leviers, plaques de pression et triggers ;
- liens logiques entre objets ;
- outils d’édition intégrés à Unreal Editor ;
- architecture extensible vers un futur système d’événements et de scripting léger.

À terme, le projet doit permettre la création de niveaux par les joueurs.

## Version cible

- Unreal Engine : **5.5.4**
- Langage : **C++**
- IDE : **Visual Studio 2022**
- Plateforme de développement principale : **Windows**
- Type de projet : **module Unreal C++**

## Structure générale

```text
Source/
└── GrimrockPrototype/
    ├── Public/
    │   ├── Core/
    │   ├── Runtime/
    │   └── EditorTools/
    └── Private/
        ├── Core/
        ├── Runtime/
        └── EditorTools/

## Documentation technique

- [Architecture Runtime / Editor Split](docs/Architecture_Runtime_Editor_Split.md)
