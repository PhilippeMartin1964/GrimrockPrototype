# GrimrockPrototype — Vue d’ensemble du projet

## Objectif

GrimrockPrototype est un prototype Unreal Engine 5.5.4 en C++ inspiré de *Legend of Grimrock 2*.

Le but est de construire un dungeon crawler en vue subjective, avec :

- déplacement case par case ;
- rotation à 90° ;
- exploration de donjons sur grille ;
- édition directe de niveaux ;
- objets interactifs ;
- mécanismes reliés entre eux ;
- énigmes basées sur boutons, leviers, plaques, portes, réceptacles, triggers, timers, téléporteurs, etc.

Le projet vise une architecture simple, modulaire, orientée données, compatible avec une évolution future vers la création de niveaux par les joueurs.

## Combat tactique cible

Le combat cible est un tour par tour sur grille qui conserve le groupe dans
une cellule unique :

- une manche globale ;
- un tour individuel par personnage ou monstre ;
- un ordre commun déterminé par l'initiative ;
- des PA personnels dépensés pour les attaques, sorts, capacités, objets et
  déplacements ;
- une réserve de mobilité commune qui limite la distance parcourue par tout le
  groupe ;
- une interface construite depuis les actions réellement disponibles, et non
  depuis deux boutons fixes `MainHand / OffHand`.

Le document de référence est
`COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md`.

---

## Contraintes générales

- Unreal Engine : 5.5.4
- Langage : C++
- IDE principal : Visual Studio
- Dépôt GitHub : `GrimrockPrototype`
- Style d’architecture : données d’abord, classes C++ simples, comportement piloté par archétypes
- Taille de grille cible : 32x32
- Cellule standard : 200 x 200 cm
- Hauteur de cellule : environ 300 cm
- Objectif runtime : jeu fluide, lisible, proche de l’esprit Grimrock

---

## Principe fondamental

Chaque objet placé dans le donjon doit être considéré comme un **objet concret de gameplay**, mais son comportement doit rester factorisé.

Exemple :

- `Button_Normal`
- `Button_Secret`
- `Button_Wall`

sont trois objets distincts dans la palette et dans les DataAssets, car ils sont visuellement différents.

Mais ils peuvent partager la même classe C++ :

```cpp
AGridButtonActor
```

Même principe pour :

- `Receptacle_Alcove`
- `Receptacle_TorchHolder`
- `Receptacle_Altar`
- `Receptacle_OfferingBowl`

qui sont des objets concrets distincts, mais peuvent reposer sur :

```cpp
AGridReceptacleActor
```

---

## Séparation conceptuelle majeure

Le système doit distinguer :

1. les **événements émis** par un objet ;
2. les **commandes reçues** par un objet cible ;
3. les **liens logiques** qui relient les deux.

Exemple :

```text
Button_Secret_01.OnActivate -> Door_Secret_01.Open
```

La porte ne connaît pas le bouton.  
Le bouton ne connaît pas la porte.  
Le système de liens fait la jonction.

---

## Mémoire de projet

Les décisions importantes doivent être conservées dans les fichiers Markdown du dossier :

```text
Docs/Design/
```

Ces documents servent de mémoire stable pour :

- ChatGPT ;
- Codex ;
- les futures sessions de travail ;
- les revues de code ;
- les décisions d’architecture.

ChatGPT et Codex ne doivent pas être la mémoire principale du projet.  
La mémoire principale doit être le dépôt Git.

---

## Documents de référence

| Fichier | Rôle |
|---|---|
| `00_PROJECT_OVERVIEW.md` | Vue d’ensemble du projet |
| `01_GRID_OBJECT_SYSTEM.md` | Architecture du système d’objets |
| `02_OBJECT_ARCHETYPES.md` | Liste et rôle des archétypes d’objets |
| `03_EVENT_COMMAND_LINKS.md` | Modèle Events / Commands / Links |
| `04_IMPLEMENTATION_ROADMAP.md` | Feuille de route technique |
| `05_CODEX_TASKS.md` | Tâches Codex prêtes à exécuter |
| `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` | Cible des manches, initiative, PA, mobilité et actions de combat |
| `99_DECISIONS_LOG.md` | Journal des décisions validées |

---

## Règle de travail

Avant chaque grosse modification :

1. valider l’architecture dans un document Markdown ;
2. demander à Codex une tâche courte et ciblée ;
3. compiler dans Visual Studio / UE5 ;
4. tester dans l’éditeur ;
5. committer ;
6. mettre à jour `99_DECISIONS_LOG.md`.
