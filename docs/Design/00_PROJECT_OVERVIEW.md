# GrimrockPrototype — Vue d’ensemble du projet

## Objectif

GrimrockPrototype est un dungeon crawler Unreal Engine 5.5.4 en C++ inspiré de *Legend of Grimrock 2*.

Le projet vise :

- vue subjective ;
- déplacement case par case sur grille ;
- rotation à 90° ;
- édition directe des niveaux ;
- objets interactifs et connecteurs Event -> Command ;
- monstres data-driven ;
- combat tactique au tour par tour ;
- progression RPG persistante ;
- à terme, création et partage de niveaux par les joueurs.

L'architecture reste simple, modulaire et orientée données. Les DataAssets et l'état de grille sont les autorités logiques ; Actors, animations, VFX et widgets sont des projections runtime ou de présentation.

---

## État actuel — 16 août 2026

Les fondations suivantes sont disponibles :

- donjons multi-niveaux et `UGridLevelAsset` ;
- Grid Editor intégré avec palette, inspecteur, connecteurs et validation ;
- déplacement et interaction en vue subjective ;
- items, inventaire, équipement, réceptacles, portes, serrures et passages secrets ;
- création de personnage, races/classes et statistiques dérivées ;
- sauvegarde/Continue ;
- combat MON1–MON12 à initiative globale, PA/PAM et hotbar 0–9 ;
- pipeline `MonsterSpawn` MON13 avec rencontres, vagues et persistance ;
- exploration IA MON14 avec vision directionnelle, dormance, patrouille, investigation, édition visuelle de route et alarme locale ;
- progression XP/niveaux MON15 avec Level Up, progression de classe, modal, sauvegarde/migration et équilibrage initial.

Les jalons majeurs suivants sont **validés et clos** :

```text
MON13 — Monster Spawn / Encounter / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
```

Le prochain jalon autoritaire est :

```text
MON16.1 — Status Effect Definition & Runtime State
```

Le backlog actif autoritaire est :

```text
docs/Design/PROJECT_COMPLETION_ROADMAP.md
```

---

## Combat tactique — MON12

Le combat utilise :

- initiative globale ;
- tours individuels ;
- PA personnels ;
- PAM communs ;
- catalogue générique d'actions ;
- HUD orienté actions ;
- dix raccourcis persistants ;
- quick items ;
- sorts/capacités catalogués ;
- ciblage cellule/zone ;
- paiement transactionnel des ressources ;
- cooldowns.

Document de référence :

```text
COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md
```

---

## Monstres — MON13 / MON14

Le pipeline de contenu est :

```text
MonsterSpawn persistant
    -> MonsterDefinition
    -> Actor runtime
    -> état persistant par SpawnId
```

MON13 apporte :

- placement persistant ;
- preview et instanciation runtime ;
- `Spawn`, `Despawn`, `Teleport` ;
- rencontres et vagues ;
- sauvegarde/Continue ;
- vraie intégration PIE de production.

MON14 apporte :

- engagement automatique uniquement par vision valide ;
- ouïe = alerte/investigation, jamais combat automatique à elle seule ;
- vision directionnelle selon le Facing ;
- états initiaux `Idle` / `Dormant` ;
- patrouilles `Loop` / `PingPong` ;
- investigation vers `LastKnownPartyCell` ;
- recherche locale ;
- édition visuelle des routes ;
- suspension atomique pendant le combat ;
- alarme locale entre monstres.

Document de clôture :

```text
MON14_CLOSURE.md
```

---

## Progression RPG — MON15

MON15 est clos. Le document récapitulatif est :

```text
MON15_CLOSURE.md
```

### XP et niveaux

État canonique :

```cpp
int32 Level = 1;
int32 Experience = 0;
```

Courbe finale :

```text
XP cumulative niveau L = 1000 * (L - 1) * L / 2
Niveau maximum = 20
```

Seuils principaux :

```text
L1       0 XP
L2    1000 XP
L3    3000 XP
L4    6000 XP
L5   10000 XP
L20 190000 XP
```

### Récompenses

`UGridMonsterDefinitionAsset::ExperienceReward` reste data-driven.

Le Rat Géant de production utilise :

```text
ExperienceReward = 500
```

C'est un pool partagé entre les personnages actifs éligibles.

### Level Up

Le système :

- détecte un ou plusieurs seuils ;
- recalcule les statistiques via les règles RPG existantes ;
- conserve le déficit absolu de PV/mana ;
- maintient les morts à 0 PV ;
- applique les choix de progression de classe atomiquement ;
- projette les requirements acquis vers le catalogue MON12 ;
- différencie notification de Level Up et transaction de choix.

### UI

La modal Level Up :

- est différée pendant le combat ;
- s'ouvre au premier safe point ;
- supporte confirmer/annuler ;
- rend correctement la pause et les entrées au gameplay.

### Persistance

SaveVersion courant :

```text
4
```

Le SaveGame persiste :

- Level / Experience ;
- choix de progression ;
- notifications Level Up encore en attente.

Les sauvegardes v1-v3 sont migrées ; les snapshots v4 incohérents sont rejetés strictement.

### Validation finale MON15

```text
42 tests MON15 : 42 Success
Campagne finale : 95 tests, 95 Success
```

Le PIE final confirme :

```text
Rat 1 : 500 XP -> niveau 1
Rat 2 : 1000 XP -> niveau 2
Rat 3 : 1500 XP -> niveau 2
```

---

## Prochaine phase — MON16 à MON22

Ordre actif :

```text
MON16 — Status Effects
MON17 — Second Monster Family
MON18 — Magic & Spellbook
MON19 — Advanced Dungeon Logic / Scripting
MON20 — Recruitment / Skills / Talents
MON21 — Quests / Journal / Map / Codex
MON22 — 45–90 Minute Vertical Slice
```

Premier sous-jalon :

```text
MON16.1 — Status Effect Definition & Runtime State
```

Principes déjà retenus pour MON16 :

- un modèle commun personnages/monstres lorsque possible ;
- durées en tours/rounds, pas en secondes de présentation ;
- intégration événementielle au TurnManager ;
- réutilisation de `InitiativeModifier` pour Haste/Slow ;
- aucune dépendance Widget Blueprint pour le fonctionnement logique.

---

## Contraintes générales

- Unreal Engine : 5.5.4
- Langage : C++
- Dépôt : `GrimrockPrototype`
- Branche : `master`
- Architecture : data-first, classes C++ simples, comportement piloté par assets
- Grille cible : 32 × 32
- Cellule standard : 200 × 200 cm
- Hauteur de cellule : environ 300 cm

---

## Principes d'architecture

### Objets concrets, comportements factorisés

Des objets distincts dans la palette peuvent partager une même classe C++ si leur différence est portée par les données et la présentation.

### Event / Command / Link

Le modèle reste :

```text
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand
```

Les objets ne se connaissent pas directement.

### Mémoire stable

La connaissance durable du projet se trouve dans le dépôt, principalement sous :

```text
docs/Design/
```

Documents autoritaires à maintenir :

```text
00_PROJECT_OVERVIEW.md
PROJECT_COMPLETION_ROADMAP.md
MON14_CLOSURE.md
MON15_CLOSURE.md
99_DECISIONS_LOG.md
```

---

## Règle de travail

Pour chaque sous-jalon :

1. auditer le code existant ;
2. documenter le contrat ;
3. modifier peu de fichiers ;
4. compiler sous UE5.5.4 ;
5. exécuter les Automation Tests dédiés et les régressions utiles ;
6. valider en PIE lorsque des assets/WBP sont impliqués ;
7. produire un commit Git clair ;
8. pousser sur `origin/master` ;
9. mettre à jour la documentation durable à la clôture.
