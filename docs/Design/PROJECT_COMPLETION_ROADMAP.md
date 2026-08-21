# GrimrockPrototype — Active Completion Roadmap

Statut : **backlog actif après clôture de MON17**  
Date de référence : **21 août 2026**

Ce document est la feuille de route active et autoritaire du projet. `04_IMPLEMENTATION_ROADMAP.md` reste historique.

---

## 1. État de référence

Jalons majeurs clos :

```text
MON13 — Monster Spawn / Encounters / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
MON16 — Status Effects
MON17 — Second Monster Family / Gobelin lanceur
```

MON15 a fermé la boucle RPG combat -> XP -> Level Up -> progression de classe -> Save/Continue.

MON16 a ajouté le modèle générique d'effets d'état commun au groupe et aux monstres : durée Turns/Rounds/Permanent, stacking, DoT, Haste/Slow, Stun/Silence/Immobilize, HUD/feedback, Save/Restore et identité primaire `GridStatusEffect:EffectId`.

MON17 a prouvé que l'architecture monstre n'est pas spécifique au Rat Géant en intégrant le **Gobelin lanceur** (`MON_GoblinThrower`) avec attaque projectile, profil tactique `RangedKeeper`, perception/patrouille/alarme MON14, encounter, loot, XP et persistance génériques.

MON17.7 est **VALIDÉ ET CLOS sous UE5.5.4**. La campagne finale et le PIE de production ont validé perception, alarme, engagement automatique, initiative, `RangedKeeper`, `Attack_ThrowKnife`, projectile, mort, loot, 125 XP par Gobelin, libération d'occupation et Victory. Le test `Grimrock.Monsters.MON13.5.RealPIEIntegration` a également été remis au vert après isolation de sa fixture d'encounter.

Références de clôture :

```text
docs/Design/MON15_CLOSURE.md
docs/Design/MON16_CLOSURE.md
docs/Design/MON17_CLOSURE.md
docs/Design/MON17_7_BALANCE_AUDIT.md
docs/Design/MON17_7_FINAL_BALANCE_CONTRACT.md
docs/Design/MON17_7_FINAL_PIE_VALIDATION.md
docs/Design/MON17_FINAL_REGRESSION_PLAN.md
```

---

## 2. Ordre des grands jalons

```text
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

Les jalons MON23+ restent un horizon de production.

---

# MON15 — XP & Level Progression — CLOS

Statut : **VALIDÉ ET CLOS sous UE5.5.4**.

```text
MON15.1 — Modèle XP / niveaux              CLOS
MON15.2 — Attribution XP                   CLOS
MON15.3 — Level-up / recalcul stats        CLOS
MON15.4 — Progression de classe            CLOS
MON15.5 — Choix de progression + UI        CLOS
MON15.6 — Save / migration / restauration  CLOS
MON15.7 — Équilibrage final                CLOS
```

Contrats principaux : XP cumulative, niveau maximum 20, récompenses monstres data-driven, choix de progression persistants par `CharacterId`, Level Up différable pendant combat et restaurable après Continue.

---

# MON16 — Status Effects — CLOS

Statut : **VALIDÉ ET CLOS sous UE5.5.4**.

```text
MON16.1 — Status Effect Definition & Runtime State  CLOS
MON16.2 — Duration / Turn / Round Lifecycle         CLOS
MON16.3 — Damage-over-time                          CLOS
MON16.4 — Haste / Slow and InitiativeModifier      CLOS
MON16.5 — Stun / Silence / Immobilize               CLOS
MON16.6 — HUD and Combat Feedback                   CLOS
MON16.7 — Save / Restore                            CLOS
MON16.8 — Closure and Regression                    CLOS
```

Contrats principaux : modèle commun personnages/monstres, stacking data-driven, DoT, Haste/Slow, Stun/Silence/Immobilize, HUD/combat log, Save/Restore et identité primaire `GridStatusEffect:EffectId`.

La version globale de sauvegarde est actuellement **SaveGame v5** depuis MON16.

---

# MON17 — Second Monster Family — CLOS

Statut : **VALIDÉ ET CLOS sous UE5.5.4**.

Objectif atteint : démontrer que l'architecture MON1–MON16 n'est pas spécifique au Rat Géant.

Seconde famille retenue :

```text
MonsterId            MON_GoblinThrower
DangerLevel          3
MaxHealth            10
Initiative           12
Accuracy / Evasion   2 / 3
ActionPointsPerTurn  3
PrimaryAIProfile     RangedKeeper
PreferredDistance    3..5
ExperienceReward     125
```

Attaque de production :

```text
AttackId               Attack_ThrowKnife
Delivery               Projectile
Damage                 2..5
MinRangeCells           2
RangeCells              6
ActionPointCost         2
CooldownTurns           0
ProjectileSourceSocket ProjectileSource
ProjectileTravel       0.20 s
```

Loot final :

```text
GoblinKnife  25 %
Stone        50 %
EmptyVial    25 %
ExpectedItemsPerKill = 1.000
```

Sous-jalons :

```text
MON17.1 — Definition / Assets / Spawn Contract          CLOS
MON17.2 — Skeletal Mesh / Skeleton / AnimBP             CLOS
MON17.3 — Distinct Attack Set                            CLOS
MON17.4 — Distinct AI Profile — RangedKeeper            CLOS
MON17.5 — Patrol / Perception / Alarm Integration       CLOS
MON17.6 — Encounter / Loot / XP Integration             CLOS
MON17.7 — Balance / Closure                              CLOS
```

MON17 a notamment établi un projectile visuel de présentation sans dégâts/inventaire/persistance parallèles, une source projectile data-driven via socket optionnel, un cooldown générique par `AttackId` et la réutilisation sans fork des systèmes MON14.

---

# MON18 — Magic & Spellbook — JALON ACTIF

## Objectif général

Établir un véritable système de magie **générique, data-driven et extensible** pour les personnages, sans créer quelques sorts codés en dur et sans dupliquer les systèmes déjà existants.

Le système doit s'intégrer à :

- personnages, classes, statistiques et progression MON15 ;
- mana, PA et transactions d'actions ;
- catalogue d'actions et hotbar 0–9 MON12 ;
- inventaire, objets et parchemins ;
- ciblage et ligne de vue ;
- combat groupe/monstres ;
- Status Effects MON16 ;
- dégâts et soins ;
- projectile/presentation/VFX/audio MON11/MON17 ;
- SaveGame / Continue ;
- UI.

Le système magique ne doit créer ni second moteur de combat, ni second système de Status Effects, ni second gestionnaire de hotbar, ni second pipeline projectile.

Chaîne architecturale cible, à confronter et raccorder aux contrats existants :

```text
SpellDefinition
    ↓
SpellCastRequest
    ↓
Validation
    ↓
Target Resolution
    ↓
Cost Transaction
    ↓
Spell Resolution
    ↓
Effects
    ↓
Presentation
    ↓
Persistence / UI notification
```

## MON18.1 — Spell Data Model & Cast Contract — ACTIF

Objectif : définir le contrat C++ data-driven minimal sur lequel le reste de MON18 pourra s'appuyer.

Périmètre :

- identifiant stable de sort ;
- nom / description / école ou type ;
- coût mana ;
- coût PA ;
- portée minimale/maximale ;
- règle de ligne de vue ;
- mode de ciblage ;
- cooldown déclaratif ;
- liste d'effets déclaratifs ;
- contrat de requête de lancement ;
- validation structurelle du modèle et de la requête ;
- Automation Tests du contrat.

Hors périmètre MON18.1 :

- paiement effectif mana/PA ;
- application réelle de dégâts/soins/Status Effects ;
- UI Spellbook ;
- assets de production ;
- VFX/audio/projectiles de production ;
- SaveGame migration.

Principes autoritaires :

1. Les coûts PA/mana du `SpellDefinition` sont des données de contrat ; le paiement transactionnel reste dans le runtime d'action existant et sera raccordé en MON18.3.
2. Le ciblage doit réutiliser les représentations grille/groupe existantes ; pas de second targeting subsystem.
3. Les effets de statut doivent être résolus par MON16 ; pas de `SpellStatusEffect` parallèle.
4. Les projectiles magiques futurs réutiliseront le pipeline projectile de présentation MON17.
5. Les identifiants doivent être stables et sérialisables ; aucun état persistant ne doit dépendre d'un pointeur d'acteur.
6. MON18.1 n'exige aucun Blueprint, DataAsset, WBP, `.uasset` ou `.umap`.

Porte de sortie : le code compile sous UE5.5.4 et les tests MON18.1 fournis sont exécutés avec succès par l'utilisateur. La validation UE n'est déclarée qu'après retour explicite des résultats.

## MON18.2 — Spell Knowledge / Spellbook

- sorts connus par personnage ;
- apprentissage ;
- anti-duplication ;
- relation avec classe/niveau ;
- Spellbook / liste de sorts ;
- préparation ou sélection si le modèle de classe l'exige ;
- persistance du modèle de connaissance.

## MON18.3 — Spell Casting Runtime

- requête de cast runtime ;
- validation contextuelle ;
- paiement transactionnel PA/mana ;
- aucun paiement en cas d'échec ;
- cooldown runtime ;
- résolution générique ;
- erreurs stables et exploitables par UI/log.

## MON18.4 — Targeting

- Self ;
- PartyMember ;
- Enemy ;
- GridCell ;
- extension ultérieure vers zone/AoE ;
- portée et ligne de vue ;
- intégration au système de ciblage existant.

## MON18.5 — First Production Spells

Créer un petit ensemble représentatif pour valider l'architecture :

- un projectile offensif ;
- un soin ;
- un buff ou debuff utilisant MON16.

Les noms de production sont décidés au moment de l'intégration artistique. `Fire Bolt`, `Heal`, `Haste` ou équivalents sont des cas de validation, pas des obligations de nomenclature.

## MON18.6 — Spell Presentation

- animation ;
- projectile si nécessaire ;
- VFX ;
- audio ;
- impact ;
- timing ;
- réutilisation prioritaire des pipelines MON11/MON17.

## MON18.7 — Spellbook / Hotbar UI

- affichage des sorts connus ;
- tooltip ;
- drag & drop vers les slots 0–9 ;
- activation depuis la hotbar ;
- état Disabled selon mana, PA, cooldown et cible ;
- réutilisation du type/action `Spell` déjà prévu par MON12.

## MON18.8 — Persistence

- sorts connus ;
- préparation/sélection éventuelle ;
- cooldowns si leur persistance est retenue ;
- migration SaveGame si le schéma évolue ;
- restauration après Continue ;
- absence de duplication lors des migrations.

## MON18.9 — Balance / Regression / Closure

- équilibrage des premiers sorts ;
- campagne de régression MON12/MON15/MON16/MON17 ;
- validation PIE des sorts de production ;
- documentation de clôture ;
- mise à jour du project overview et de cette roadmap.

---

# MON19 — Advanced Dungeon Logic / Scripting

Objectif : permettre au level designer de construire des énigmes riches sans ajouter du C++ spécifique.

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

Le langage léger doit compléter `Event -> Condition -> Command`, pas le remplacer.

---

# MON20 — Recruitment / Skills / Talents

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

---

# MON21 — Quests / Journal / Map / Codex

```text
MON21.1 — Quest Definition
MON21.2 — Quest Runtime State
MON21.3 — Journal
MON21.4 — Dialogues / Choices
MON21.5 — Map Discovery
MON21.6 — Codex / Bestiary Unlocks
MON21.7 — Save / Closure
```

---

# MON22 — 45–90 Minute Vertical Slice

Objectif : suspendre volontairement l'ajout de grandes infrastructures et construire un jeu testable de bout en bout.

Contenu cible :

- 1 donjon cohérent ;
- 3 à 5 niveaux ;
- 45 à 90 minutes de jeu ;
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

Porte de sortie : une personne extérieure peut lancer une build autonome, comprendre les règles, terminer le slice sans commande de debug et reprendre une sauvegarde.

---

# Horizon MON23+

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

## Règles de conduite

1. Un sous-jalon doit être petit, compilable et testable.
2. Travail sur `master`, sans branche ni Pull Request.
3. Un commit logique par étape lorsque possible.
4. Pousser sur `origin/master` après chaque étape.
5. Aucun refactor massif préventif.
6. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
7. Les tests C++ valident la logique ; assets/WBP/maps exigent une validation UE/PIE lorsqu'ils sont impliqués.
8. Ne jamais déclarer compilation ou tests UE5.5.4 validés sans résultat fourni par l'utilisateur.
9. Documentation projet dans `docs/Design/`, jamais dans `Documentation/`.
10. À la clôture d'un jalon majeur, mettre à jour overview, roadmap et document de clôture.

---

## Prochain travail autoritaire

```text
MON18.1 — Spell Data Model & Cast Contract
```
