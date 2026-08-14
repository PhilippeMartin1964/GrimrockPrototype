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
- une barre d'initiative affichant le portrait et l'état du combattant actif,
  puis ceux des prochains personnages ou monstres dans l'ordre réel ;
- une interface construite depuis les actions réellement disponibles, et non
  depuis deux boutons fixes `MainHand / OffHand`.

Le document de référence est
`COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md`.

MON12.5 implémente désormais le déplacement de combat : chaque translation
coûte `1 PA` au personnage actif et `1 PAM` sur les `2 PAM` communs restaurés
à chaque manche. Les rotations restent gratuites, les refus ne consomment
rien et l'exploration hors combat est inchangée.

MON12.6 fournit le catalogue générique validé. MON12.7 construit désormais le
HUD depuis ce catalogue : quatre panneaux au maximum, barre d'actions du
personnage actif, huit prochains tours au maximum et PAM communs.

## Apparition des monstres

MON13 remplace les monstres posés manuellement par un pipeline orienté données :

```text
MonsterSpawn persistant
    → MonsterDefinition
    → Actor runtime
    → état MON9 par SpawnId
```

MON13.1 définit le placement persistant et son édition. MON13.2 crée l'aperçu
squelettique et l'Actor runtime. MON13.3 ajoute les commandes `Spawn`,
`Despawn` et `Teleport`, leurs événements de cycle de vie et la persistance de
la présence ainsi que du dernier état du monstre.
MON13.4 ajoute les rencontres persistantes pilotées par `EncounterGroupId`,
les vagues ordonnées, la commande `StartEncounter` et une progression fondée
uniquement sur les morts réellement validées.
MON13.5 clôt le pipeline par des contrats automatisés sur les assets de
production et une véritable session PIE couvrant la carte de référence, le
playtest frais, les vagues, les composants de combat et le chargement Continue.

## Engagement exploration → combat

MON14.1 raccorde désormais la perception logique MON4 au TurnManager MON5 sans
commande gameplay manuelle. Une ligne de vue orthogonale valide depuis un
monstre vivant et actif vers le groupe peut démarrer automatiquement le combat ;
l'ouïe seule continue à mettre le monstre en `Alert` et à renseigner sa dernière
cellule connue, mais ne déclenche pas l'engagement automatique.

Les demandes d'évaluation sont centralisées par
`UGridAutomaticPerceptionEngagementSubsystem`, différées et coalescées. Elles
sont émises après les points runtime pertinents : changement de cellule du
groupe, rebuild, ouverture de porte, apparition de `MonsterSpawn` et vague
MON13.4. `StartEncounter` reste une transaction de rencontre/spawn et ne force
jamais directement `StartCombat()`.

La propagation d'aggro MON7 reste l'autorité pour ajouter les alliés du monstre
qui voit le groupe. Le chemin manuel F5 conserve le contrat historique
vue/ouïe comme outil de diagnostic.

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
docs/Design/
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
| `MON12_4_GLOBAL_INITIATIVE_INDIVIDUAL_TURNS.md` | Implémentation de l'initiative globale et des tours individuels |
| `MON12_5_PARTY_MOVEMENT_ACTION_POINTS.md` | Implémentation du déplacement payant et des PAM communs |
| `MON12_6_COMBAT_ACTION_CATALOG.md` | Définitions, contributions et catalogue générique d'actions |
| `MON12_7_ACTION_ORIENTED_COMBAT_HUD.md` | HUD de quatre personnages, actions, initiative et PAM |
| `MON13_1_MONSTER_SPAWN_MODEL.md` | Placement persistant, identité et validation des MonsterSpawn |
| `MON13_2_MONSTER_SPAWN_PIPELINE.md` | Aperçu squelettique et instanciation runtime |
| `MON13_3_MONSTER_RUNTIME_COMMANDS.md` | Commandes Spawn/Despawn/Teleport et persistance du cycle de vie |
| `MON13_4_MONSTER_ENCOUNTER_WAVES.md` | Rencontres persistantes, vagues atomiques et progression par mort |
| `MON13_5_MONSTER_SPAWN_CLOSURE.md` | Clôture transversale des assets, vagues, PIE frais et Continue |
| `MON14_1_AUTOMATIC_PERCEPTION_ENGAGEMENT.md` | Engagement exploration → combat par perception visuelle, différé et coalescé |
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
