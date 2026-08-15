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
- énigmes basées sur boutons, leviers, plaques, portes, réceptacles, triggers, timers, téléporteurs, etc. ;
- monstres data-driven capables de patrouiller, percevoir, enquêter et engager automatiquement le groupe ;
- combat tactique au tour par tour ;
- progression RPG ;
- à terme, création et partage de niveaux par les joueurs.

Le projet vise une architecture simple, modulaire, orientée données. La grille et les DataAssets restent les autorités logiques ; les Actors, animations, VFX et widgets sont des représentations runtime ou de présentation.

---

## État actuel — 15 août 2026

Le projet a dépassé le stade du prototype de déplacement. Les fondations suivantes sont disponibles :

- donjons multi-niveaux et `UGridLevelAsset` ;
- Grid Editor intégré avec palette, inspecteur, connecteurs et validation ;
- déplacement et interaction en vue subjective ;
- items, inventaire, équipement, réceptacles, portes, serrures et passages secrets ;
- création de personnage, races/classes et statistiques dérivées ;
- sauvegarde/Continue ;
- combat MON1–MON12 à initiative globale, PA/PAM et raccourcis 0–9 ;
- pipeline `MonsterSpawn` MON13 avec rencontres, vagues et persistance ;
- exploration IA MON14 avec perception directionnelle, dormance, patrouille, investigation, édition visuelle de route et alarme locale.

Le chantier **MON14 est validé et clos**. La feuille de route active commence désormais par :

```text
MON15 — XP & Level Progression
```

Le backlog actif autoritaire est :

```text
docs/Design/PROJECT_COMPLETION_ROADMAP.md
```

`04_IMPLEMENTATION_ROADMAP.md` est désormais un document historique.

---

## Combat tactique

Le combat est un tour par tour sur grille qui conserve le groupe dans une cellule unique :

- une manche globale ;
- un tour individuel par personnage ou monstre ;
- un ordre commun déterminé par l'initiative ;
- des PA personnels dépensés pour les attaques, sorts, capacités, objets et déplacements ;
- une réserve de mobilité commune PAM ;
- une barre d'initiative glissante ;
- une interface construite depuis les actions réellement disponibles ;
- dix raccourcis persistants par personnage.

Le document de référence est :

```text
COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md
```

MON12 fournit désormais :

- initiative globale ;
- tours individuels ;
- PA personnels ;
- PAM communs ;
- catalogue générique d'actions ;
- HUD orienté actions ;
- hotbar 0–9 ;
- quick items ;
- sorts/capacités dans le catalogue ;
- ciblage cellule/zone ;
- paiement transactionnel des ressources ;
- cooldowns.

---

## Apparition des monstres — MON13

MON13 remplace les monstres posés manuellement par un pipeline orienté données :

```text
MonsterSpawn persistant
    → MonsterDefinition
    → Actor runtime
    → état persistant par SpawnId
```

MON13.1 définit le placement persistant et son édition.

MON13.2 crée l'aperçu squelettique et l'Actor runtime.

MON13.3 ajoute les commandes `Spawn`, `Despawn` et `Teleport`, les événements de cycle de vie et la persistance de présence.

MON13.4 ajoute les rencontres persistantes pilotées par `EncounterGroupId`, les vagues ordonnées et `StartEncounter`.

MON13.5 clôt le pipeline par des contrats automatisés et une véritable session PIE couvrant les assets de production, les vagues, le playtest frais et Continue.

---

## Exploration et engagement — MON14

MON14 est clos. Le document récapitulatif est :

```text
MON14_CLOSURE.md
```

### MON14.1 — Engagement automatique

`UGridAutomaticPerceptionEngagementSubsystem` raccorde la perception MON4 au TurnManager sans commande gameplay manuelle.

Règle :

```text
Vision réelle valide -> combat automatique possible
Ouïe seule           -> Alert / LastKnownPartyCell, jamais combat automatique
```

Les demandes d'évaluation sont différées et coalescées. `StartEncounter` reste une transaction de rencontre/spawn et ne force jamais directement `StartCombat()`.

### MON14.2 — Vision directionnelle et données de patrouille

La vue ne compte que dans le `Facing` cardinal avant du monstre.

`MonsterSpawn` peut démarrer en :

```text
Idle
Dormant
```

La dormance ne doit jamais être confondue avec `bInitiallyEnabled=false`, qui signifie absence de l'Actor.

Les routes utilisent :

```text
PatrolMode = None / Loop / PingPong
PatrolWaypoints[] = Cell + Facing + WaitSeconds
```

### MON14.3 — Patrol & Investigation

`UGridMonsterPatrolSubsystem` exécute les routes hors combat sans Tick IA permanent.

Les gardes :

- rejoignent et parcourent leurs waypoints ;
- respectent orientation et attente ;
- interrompent leur patrouille sur perception ;
- enquêtent vers `LastKnownPartyCell` ;
- effectuent une recherche locale ;
- reprennent leur patrouille après échec ;
- suspendent atomiquement leur locomotion lorsque le combat prend la main.

### MON14.3.1 — Édition visuelle

Les routes sont éditables dans le Grimrock Grid Editor :

- ajout/sélection par clic ;
- ordre ;
- `Loop/PingPong` ;
- Facing ;
- attente ;
- rendu numéroté dans le viewport.

Ce jalon a été validé manuellement dans `L_GrimrockEditor`.

### MON14.4 — Alarm Coordination

MON14.4 réutilise le contrat MON7 :

```text
bSharesAggroWithGroup
AggroPropagationRange
même MonsterId
même EncounterGroupId
```

Un monstre qui perçoit le groupe peut réveiller des alliés proches et leur transmettre sa dernière cellule connue. Les alliés passent en investigation.

L'alarme seule ne déclenche jamais le combat ; une vraie vision MON14.1 reste nécessaire.

Les trois tests dédiés MON14.4 sont validés :

```text
AlarmFiltering           Success
HearingAlarmPropagation  Success
SharingDisabled          Success
```

Le scénario fonctionnel final a également été validé manuellement.

---

## Prochaine phase — MON15 à MON22

L'ordre de travail validé est :

```text
MON15 — XP & Level Progression
MON16 — Status Effects
MON17 — Second Monster Family
MON18 — Magic & Spellbook
MON19 — Advanced Dungeon Logic / Scripting
MON20 — Recruitment / Skills / Talents
MON21 — Quests / Journal / Map / Codex
MON22 — 45–90 Minute Vertical Slice
```

Le premier sous-jalon est :

```text
MON15.1 — XP & Level Model
```

Les champs suivants existent déjà dans `FGridCharacterInventoryState` et doivent être réutilisés :

```cpp
int32 Level = 1;
int32 Experience = 0;
```

`URPGClassAsset` contient déjà les gains de PV/mana par niveau et `URPGCharacterRulesLibrary::CalculateDerivedStats()` accepte déjà le niveau. MON15 ne doit donc pas créer un second état de progression parallèle.

---

## Contraintes générales

- Unreal Engine : 5.5.4
- Langage : C++
- IDE : Visual Studio
- Dépôt : `GrimrockPrototype`
- Branche de travail : `master`
- Style : données d'abord, classes C++ simples, comportement piloté par assets
- Taille de grille cible : 32 × 32
- Cellule standard : 200 × 200 cm
- Hauteur de cellule : environ 300 cm
- Runtime : fluide, lisible et proche de l'esprit Grimrock

---

## Principe fondamental

Chaque objet placé dans le donjon est un **objet concret de gameplay**, mais son comportement doit rester factorisé.

Exemple :

```text
Button_Normal
Button_Secret
Button_Wall
```

sont trois objets distincts dans la palette et dans les DataAssets, mais peuvent partager :

```cpp
AGridButtonActor
```

Même principe pour les réceptacles, portes, monstres et futures variantes de contenu.

---

## Séparation Event / Command / Link

Le système distingue :

1. les événements émis par une source ;
2. les commandes reçues par une cible ;
3. les liens logiques entre les deux.

Exemple :

```text
Button_Secret_01.OnActivate -> Door_Secret_01.Open
```

La porte ne connaît pas le bouton. Le bouton ne connaît pas la porte. Le système de liens fait la jonction.

---

## Mémoire stable du projet

Les décisions importantes doivent rester dans le dépôt Git, principalement sous :

```text
docs/Design/
```

ChatGPT et Codex ne sont pas la mémoire principale du projet.

Documents à maintenir lors des grands jalons :

```text
00_PROJECT_OVERVIEW.md
PROJECT_COMPLETION_ROADMAP.md
99_DECISIONS_LOG.md
```

et, lorsque la cartographie change significativement :

```text
docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md
docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.xmind
```

---

## Documents de référence

| Fichier | Rôle |
|---|---|
| `00_PROJECT_OVERVIEW.md` | Vue d'ensemble actuelle |
| `PROJECT_COMPLETION_ROADMAP.md` | **Backlog actif MON15+** |
| `MON14_CLOSURE.md` | Clôture fonctionnelle de MON14 |
| `01_GRID_OBJECT_SYSTEM.md` | Architecture du système d'objets |
| `02_OBJECT_ARCHETYPES.md` | Archétypes d'objets |
| `03_EVENT_COMMAND_LINKS.md` | Events / Commands / Links |
| `04_IMPLEMENTATION_ROADMAP.md` | Roadmap historique, non autoritaire pour la suite |
| `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md` | Architecture combat |
| `MON13_1_MONSTER_SPAWN_MODEL.md` | Placement persistant MonsterSpawn |
| `MON13_2_MONSTER_SPAWN_PIPELINE.md` | Aperçu et instanciation runtime |
| `MON13_3_MONSTER_RUNTIME_COMMANDS.md` | Spawn/Despawn/Teleport |
| `MON13_4_MONSTER_ENCOUNTER_WAVES.md` | Rencontres et vagues |
| `MON13_5_MONSTER_SPAWN_CLOSURE.md` | Clôture MON13 |
| `MON14_1_AUTOMATIC_PERCEPTION_ENGAGEMENT.md` | Engagement automatique |
| `MON14_2_DIRECTIONAL_PERCEPTION_PATROL_DATA.md` | Vision directionnelle et données de patrouille |
| `MON14_3_RUNTIME_PATROL_INVESTIGATION.md` | Patrouille et investigation runtime |
| `MON14_3_1_VISUAL_PATROL_ROUTE_EDITOR.md` | Édition visuelle des routes |
| `MON14_4_EXPLORATION_ALARM_COORDINATION.md` | Alarme locale entre monstres |
| `99_DECISIONS_LOG.md` | Journal chronologique des décisions |

---

## Règle de travail

Pour chaque sous-jalon :

1. auditer le code existant avant d'ajouter une nouvelle abstraction ;
2. documenter le contrat ;
3. modifier peu de fichiers ;
4. compiler sous UE5.5.4 ;
5. exécuter les Automation Tests dédiés et les régressions utiles ;
6. faire la validation PIE/manuelle lorsque des assets ou WBP sont impliqués ;
7. produire un commit Git clair ;
8. pousser sur `origin/master` ;
9. mettre à jour la documentation durable à la clôture du jalon.
