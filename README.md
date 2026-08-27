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
- Event -> Command, Logic et Lua ;
- groupe RPG, inventaire, combat, monstres, magie et recrutement.

À terme, le projet doit permettre la création de niveaux par les joueurs.

## Version cible

- Unreal Engine : **5.5.4**
- Langage : **C++**
- IDE : **Visual Studio 2022**
- Plateforme de développement principale : **Windows**
- Type de projet : **modules Unreal C++**

## Structure générale

```text
Source/
├── GrimrockLua/
├── GrimrockPrototype/
└── GrimrockPrototypeEditor/
```

## Installation / environnement

La procédure de référence est :

```text
docs/Design/DEVELOPMENT_ENVIRONMENT_SETUP.md
```

Contrôle des plugins/dépendances d'un environnement :

```powershell
.\Scripts\CheckProjectDependencies.ps1 -EngineRoot D:\UE_5.5
```

**Meshy est optionnel et désactivé par défaut.** Il peut être installé localement pour de la production d'assets, mais n'est ni requis pour compiler/jouer ni versionné dans le repository.

## Validation

Editor + Automation :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "<filtre>"
```

Shipping :

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

## Documentation technique

- [Architecture Runtime / Editor Split](docs/Architecture_Runtime_Editor_Split.md)
- [Index architecture](docs/Architecture/ARCHITECTURE_INDEX.md)
- [Registre de dette technique](docs/Architecture/TECHNICAL_DEBT_REGISTER.md)
- [Roadmap active](docs/Design/PROJECT_COMPLETION_ROADMAP.md)
