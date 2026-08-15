# GrimrockPrototype — Active Completion Roadmap

Statut : **backlog actif** à partir de la clôture de MON14.

Date de référence : 15 août 2026.

Ce document remplace `04_IMPLEMENTATION_ROADMAP.md` comme feuille de route active. `04_IMPLEMENTATION_ROADMAP.md` reste un document historique utile pour comprendre la construction initiale du système d'objets et de connecteurs, mais il ne doit plus être utilisé pour décider du prochain jalon.

---

## 1. Point de départ

La clôture de MON14 signifie que le vertical slice des monstres couvre désormais :

- `MonsterSpawn` data-driven et persistant ;
- rencontres et vagues ;
- perception logique directionnelle ;
- engagement exploration → combat automatique par la vision ;
- état initial `Idle` / `Dormant` ;
- patrouilles `Loop` / `PingPong` ;
- investigation vers `LastKnownPartyCell` ;
- recherche locale ;
- suspension atomique de l'IA d'exploration pendant le combat ;
- édition visuelle des routes dans `L_GrimrockEditor` ;
- alarme locale entre monstres via le contrat MON7.

Le prochain besoin structurant n'est donc plus une nouvelle couche d'IA d'exploration. Le projet doit maintenant fermer la boucle RPG, généraliser le contenu et transformer les fondations en jeu.

---

## 2. Ordre des grands jalons

```text
MON15 — XP & Level Progression
        ↓
MON16 — Status Effects
        ↓
MON17 — Second Monster Family
        ↓
MON18 — Magic & Spellbook
        ↓
MON19 — Advanced Dungeon Logic / Scripting
        ↓
MON20 — Recruitment / Skills / Talents
        ↓
MON21 — Quests / Journal / Map / Codex
        ↓
MON22 — 45–90 Minute Vertical Slice
```

Les jalons MON23+ sont conservés comme horizon de production, mais ne doivent pas détourner le développement de MON15–MON22.

---

# MON15 — XP & Level Progression

## Objectif

Transformer le combat et les récompenses en progression durable des personnages.

Le projet possède déjà deux champs persistants dans `FGridCharacterInventoryState` :

```cpp
int32 Level = 1;
int32 Experience = 0;
```

et `URPGClassAsset` possède déjà :

```cpp
HealthAtLevelOne
HealthPerLevel
ManaAtLevelOne
ManaPerLevel
```

Enfin, `URPGCharacterRulesLibrary::CalculateDerivedStats()` accepte déjà un niveau. MON15 doit donc réutiliser ce socle plutôt que créer un second modèle de personnage.

## MON15.1 — Modèle XP et niveau

### Périmètre

- définir une courbe XP autoritaire et déterministe ;
- définir le niveau maximum du système actuel ;
- fournir les helpers purs :
  - XP cumulative requise pour un niveau ;
  - niveau correspondant à une XP totale ;
  - XP restante vers le niveau suivant ;
  - validation et clamp des valeurs ;
- garantir la monotonie de la courbe ;
- conserver `Level` et `Experience` dans `FGridCharacterInventoryState` comme état persistant autoritaire ;
- ne pas encore attribuer d'XP après les combats ;
- ne pas encore ouvrir d'UI de montée de niveau ;
- ne pas encore distribuer de points de compétences/dons.

### Décision de conception initiale

`Experience` représente une **XP totale cumulative**, jamais un compteur remis à zéro à chaque niveau.

Le niveau doit pouvoir être reconstruit depuis l'XP pour les validations et migrations, mais le champ `Level` reste sérialisé afin de ne pas casser le modèle existant et de permettre des contrôles de cohérence.

La courbe doit être centralisée dans une règle unique et testable. Aucun widget, monstre ou composant de combat ne doit contenir sa propre table de seuils.

### Critères de sortie

- tests des seuils exacts ;
- tests avant/après chaque seuil ;
- XP négative normalisée ;
- XP très élevée clampée au niveau maximum ;
- courbe strictement croissante ;
- aucun changement de combat ou de récompense dans ce sous-jalon.

## MON15.2 — Attribution XP après combat

- utiliser `UGridMonsterDefinitionAsset::ExperienceReward` ;
- attribuer l'XP une seule fois par mort réellement validée ;
- empêcher le double gain après rebuild, Continue ou callbacks répétés ;
- définir la politique de partage entre personnages actifs ;
- diffuser un événement de gain XP ;
- conserver la transaction indépendante du loot.

### Porte de sortie

Tuer un Rat Géant attribue exactement la récompense attendue une fois et la valeur survit à la sauvegarde/Continue.

## MON15.3 — Montée de niveau et recalcul des statistiques

- détecter le franchissement d'un ou plusieurs seuils ;
- recalculer les statistiques via les règles RPG existantes ;
- appliquer `HealthPerLevel` et `ManaPerLevel` ;
- définir la politique sur `CurrentHealth` / `CurrentMana` au level-up ;
- supporter plusieurs niveaux gagnés en une transaction ;
- ne jamais perdre les dégâts ou ressources courantes par un recalcul naïf.

## MON15.4 — Progression propre aux classes

- points/choix accordés par niveau ;
- prérequis ;
- capacités débloquées ;
- préparation du raccord avec MON16 et MON18 ;
- rester data-driven dans `URPGClassAsset` ou un asset de progression dédié si la complexité le justifie.

## MON15.5 — Interface Level Up

- notification de niveau disponible ;
- écran/modal de progression ;
- comparaison avant/après ;
- choix atomique et validation ;
- aucune modification partielle si le joueur annule ou si la transaction échoue.

## MON15.6 — Sauvegarde / restauration / migration

- validation `Level` ↔ `Experience` ;
- migration des sauvegardes v1–v3 ;
- sauvegarde pendant un niveau disponible ;
- restauration exacte des choix déjà effectués.

## MON15.7 — Équilibrage et clôture

- courbe finale du vertical slice ;
- XP du Rat Géant ;
- temps moyen par niveau ;
- tests de non-régression combat/inventaire/save ;
- documentation de clôture.

---

# MON16 — Status Effects

## Objectif

Créer une infrastructure commune pour les états temporaires et permanents affectant personnages et monstres.

## Sous-jalons proposés

```text
MON16.1 — Status Effect Definition & Runtime State
MON16.2 — Duration / Turn / Round Lifecycle
MON16.3 — Damage-over-time: Poison / Bleeding / Burning
MON16.4 — Haste / Slow and InitiativeModifier
MON16.5 — Stun / Silence / Immobilize
MON16.6 — HUD and Combat Feedback
MON16.7 — Save / Restore
MON16.8 — Closure and Regression
```

## Principes

- un seul modèle d'effet pour le groupe et les monstres lorsque possible ;
- durée autoritaire en tours/rounds, pas en secondes de présentation ;
- intégration événementielle avec le TurnManager ;
- `InitiativeModifier` MON12.7.1 doit être réutilisé pour Haste/Slow ;
- un effet ne doit jamais dépendre d'un Widget Blueprint pour fonctionner.

---

# MON17 — Second Monster Family

## Objectif

Prouver que l'architecture MON1–MON14 n'est pas spécifique au Rat Géant.

Le second monstre doit imposer un comportement différent, par exemple :

- squelette `DirectMelee / SlowPressure` ; ou
- archer/gobelin `RangedKeeper`.

Une simple reskin du Rat Géant n'est pas suffisante pour valider MON17.

## Sous-jalons proposés

```text
MON17.1 — Definition / Assets / Spawn Contract
MON17.2 — Skeletal Mesh / Skeleton / AnimBP
MON17.3 — Distinct Attack Set
MON17.4 — Distinct AI Profile
MON17.5 — Patrol / Perception / Alarm Integration
MON17.6 — Encounter / Loot / XP Integration
MON17.7 — Balance / Closure
```

## Porte de sortie

Le nouveau monstre doit utiliser le même pipeline `MonsterSpawn`, la même persistance, le même TurnManager et les mêmes systèmes MON14 sans code gameplay spécifique à `RatGiant`.

---

# MON18 — Magic & Spellbook

## Objectif

Transformer l'infrastructure de sorts MON12 en système RPG complet.

## Sous-jalons proposés

```text
MON18.1 — Spell Definition / Identity
MON18.2 — Known Spells per Character
MON18.3 — Spellbook
MON18.4 — Learning Rules
MON18.5 — Availability / Preparation
MON18.6 — First Production Offensive Spell
MON18.7 — First Buff / Debuff
MON18.8 — Scroll Learning / Casting
MON18.9 — Save / Closure
```

## Premiers cas de validation recommandés

- `Magic Missile` : cible unique ;
- `Fireball` : zone ;
- `Heal` : soi/allié ;
- `Haste` : effet MON16.

---

# MON19 — Advanced Dungeon Logic / Scripting

## Objectif

Permettre au level designer de construire des énigmes riches sans ajouter du C++ spécifique.

## Sous-jalons proposés

```text
MON19.1 — Timer
MON19.2 — Counter
MON19.3 — Logic Relay
MON19.4 — Hazards / Traps
MON19.5 — Advanced Connector Conditions
MON19.6 — Sequences
MON19.7 — Lightweight Script Language
MON19.8 — Sandbox / Validation / Limits
```

## Exemple cible

```text
PressurePlate.Activated
    -> Counter +1
    -> if Counter == 3
    -> Door.Open
    -> Timer 5 s
    -> Door.Close
```

Le langage léger ne doit arriver qu'après validation des briques Timer/Counter/Relay. Il doit compléter `Event -> Condition -> Command`, pas le remplacer.

---

# MON20 — Recruitment / Skills / Talents

## Objectif

Passer du groupe techniquement multi-personnages à un système RPG de compagnons et de progression de personnage.

## Sous-jalons proposés

```text
MON20.1 — Recruitment / Party Roster
MON20.2 — Skills
MON20.3 — Skill Points
MON20.4 — Feats / Talents
MON20.5 — Specializations
MON20.6 — Character Sheet Integration
MON20.7 — Lockpicking Integration
MON20.8 — Save / Closure
```

Le crochetage déjà prévu dans `GRIMROCK_LOCK_SYSTEM.md` doit consommer les compétences/outils de ce système plutôt qu'introduire sa propre progression parallèle.

---

# MON21 — Quests / Journal / Map / Codex

## Objectif

Donner une structure de campagne aux pages UI actuellement surtout décoratives.

## Sous-jalons proposés

```text
MON21.1 — Quest Definition
MON21.2 — Quest Runtime State
MON21.3 — Journal
MON21.4 — Dialogues / Choices
MON21.5 — Map Discovery
MON21.6 — Codex / Bestiary Unlocks
MON21.7 — Save / Closure
```

Les états doivent être data-driven et persistants ; les widgets restent des projections de l'état runtime.

---

# MON22 — 45–90 Minute Vertical Slice

## Objectif

Suspendre volontairement l'ajout de grandes infrastructures et construire un jeu testable de bout en bout.

## Contenu cible

- 1 donjon cohérent ;
- 3 à 5 niveaux ;
- 45 à 90 minutes de jeu pour un nouveau joueur ;
- plusieurs énigmes multi-étapes ;
- secrets ;
- 2 à 4 familles de monstres ;
- au moins un boss ;
- progression XP/niveaux ;
- sorts et effets de statut ;
- objets, butin, clés et serrures ;
- journal minimal ;
- sauvegarde et Continue ;
- conclusion claire du slice.

## Porte de sortie

Une personne extérieure au développement peut lancer une build autonome, comprendre les règles, terminer le slice sans commande de debug et reprendre une sauvegarde en cours de partie.

---

# Horizon MON23+

Ces chantiers restent importants mais viennent après la porte de sortie MON22 :

```text
MON23 — Containers / Lock Traps / Crafting
MON24 — Production Audio / VFX / Atmosphere
MON25 — Menus / Options / Accessibility
MON26 — Performance / Optimization
MON27 — Packaging / Shipping / Installer
MON28 — Standalone Player Level Editor
MON29 — Dungeon Publication / Validation / Sharing
MON30 — Full Campaign
```

---

## Règles de conduite de la roadmap

1. Un sous-jalon doit être petit, compilable et testable.
2. Ne pas créer de branche de travail : le projet travaille sur `master` selon la pratique actuelle.
3. Un commit logique par étape lorsque possible.
4. Les modifications doivent être poussées sur `origin/master` après l'étape.
5. Aucun refactor massif « préventif ».
6. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
7. Les tests C++ valident la logique ; les `.uasset`, `.umap`, WBP, meshes et AnimBP exigent une validation UE/PIE lorsqu'ils sont impliqués.
8. À la clôture d'un jalon majeur, mettre à jour :
   - `00_PROJECT_OVERVIEW.md` ;
   - `PROJECT_COMPLETION_ROADMAP.md` ;
   - `99_DECISIONS_LOG.md` ;
   - la cartographie du projet si nécessaire.

---

## Prochain travail autoritaire

```text
MON15.1 — XP & Level Model
```

MON15.1 doit commencer par un audit précis du modèle RPG existant. Les champs `Level` et `Experience` existent déjà ; il ne faut pas les dupliquer. La première implémentation doit porter sur les règles pures de progression, les validations et les tests, sans attribuer encore d'XP depuis les combats.