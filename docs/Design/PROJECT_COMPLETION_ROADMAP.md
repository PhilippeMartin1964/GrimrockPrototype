# GrimrockPrototype — Active Completion Roadmap

Statut : **backlog actif après clôture de MON16**  
Date de référence : **20 août 2026**

Ce document est la feuille de route active. `04_IMPLEMENTATION_ROADMAP.md` reste historique.

---

## 1. État de départ

Jalons majeurs clos :

```text
MON13 — Monster Spawn / Encounters / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
MON16 — Status Effects
```

MON15 a fermé la boucle RPG combat -> XP -> Level Up -> progression de classe -> Save/Continue.

MON16 a ajouté le modèle générique d'effets d'état groupe/monstres : durée Turns/Rounds/Permanent, stacking, DoT, Haste/Slow, Stun/Silence/Immobilize, HUD/feedback, Save/Restore et identité primaire `GridStatusEffect:EffectId`.

Références :

```text
docs/Design/MON15_CLOSURE.md
docs/Design/MON16_CLOSURE.md
```

Le besoin structurant de MON17 est de prouver que l'architecture monstre MON1–MON16 n'est pas spécifique au Rat Géant en intégrant une seconde famille tactiquement distincte : le **Gobelin lanceur** (`MON_GoblinThrower`, `RangedKeeper`), conformément au Bestiaire des Profondeurs — Volume II.

---

## 2. Ordre des grands jalons

```text
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

Les jalons MON23+ restent un horizon de production.

---

# MON15 — XP & Level Progression — CLOS

Statut : **VALIDÉ ET CLOS sous UE5.5.4**.

Sous-jalons :

```text
MON15.1 — Modèle XP / niveaux              CLOS
MON15.2 — Attribution XP                   CLOS
MON15.3 — Level-up / recalcul stats        CLOS
MON15.4 — Progression de classe            CLOS
MON15.5 — Choix de progression + UI        CLOS
MON15.6 — Save / migration / restauration  CLOS
MON15.7 — Équilibrage final                CLOS
```

Contrats finaux :

- XP cumulative : `1000 * (L - 1) * L / 2` ;
- niveau maximum 20 ;
- récompenses monstres data-driven ;
- Rat Géant = 500 XP de pool total ;
- choix de progression persistants par `CharacterId` ;
- Level Up différable pendant combat et restaurable après Continue ;
- 42/42 tests MON15 verts.

La version globale de sauvegarde n'est plus celle de clôture MON15 : **SaveGame est maintenant en version 5 depuis MON16**.

---

# MON16 — Status Effects — CLOS

Statut : **VALIDÉ ET CLOS sous UE5.5.4**.

Sous-jalons :

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

Contrats finaux :

- un modèle générique commun aux personnages et monstres ;
- durée `Turns`, `Rounds` ou `Permanent` ;
- stacking data-driven ;
- DoT ;
- Haste / Slow intégrés à l'initiative ;
- Stun / Silence / Immobilize ;
- HUD et combat log ;
- Save / Restore ;
- `UGrimrockPartySaveGame::CurrentSaveVersion = 5` ;
- identité primaire `GridStatusEffect:EffectId` ;
- campagne finale : MON14 21/21, MON15 42/42, MON16 81/81, soit **144/144 Success**.

---

# MON17 — Second Monster Family — EN COURS

## Objectif

Prouver que l'architecture MON1–MON16 n'est pas spécifique au Rat Géant.

Seconde famille retenue :

```text
Gobelin lanceur
MonsterId = MON_GoblinThrower
PrimaryAIProfile = RangedKeeper
```

Cette créature est décrite dans `docs/ArtBook/Bestiaire_des_Profondeurs_Volume_II_Les_Salles_Interdites.md` comme un ennemi « Projectile / harcèlement », utilisant couteaux, pierres ou fioles acides. MON17 prend `RangedKeeper` comme profil runtime autoritaire ; l'intention ArtBook `FleeAndCallHelp` ne doit pas provoquer la création prématurée d'une seconde IA parallèle.

Comportement cible : perception -> recherche d'une distance favorable -> orientation vers le groupe -> attaque à distance si LOS valide -> repositionnement lorsque le groupe devient trop proche.

Une simple reskin du Rat Géant n'est pas suffisante et aucune seconde IA parallèle ne doit être créée.

Sous-jalons :

```text
MON17.1 — Definition / Assets / Spawn Contract          CLOS
MON17.2 — Skeletal Mesh / Skeleton / AnimBP             CLOS
MON17.3 — Distinct Attack Set                            CLOS
MON17.4 — Distinct AI Profile — RangedKeeper            CLOS
MON17.5 — Patrol / Perception / Alarm Integration       CLOS
MON17.6 — Encounter / Loot / XP Integration             EN COURS
MON17.7 — Balance / Closure                              À FAIRE
```

## MON17.1 — CLOS

MON17.1 a été validé sous UE5.5.4 :

- `Grimrock.Monsters.MON17.1` : **3/3 Success** ;
- `DA_MON_GoblinThrower` : `Danger=3`, `HP=10`, `Initiative=12`, `Accuracy=2`, `Evasion=3`, `AP=3`, `Sight=8`, `Hearing=4`, `Damage=2..5`, `XP=125` ;
- entrée palette `MON_GoblinThrower` avec `DA_MonsterSpawn` et `DA_MON_GoblinThrower` ;
- résolution du `MonsterSpawn` confirmée.

Référence :

```text
docs/Design/MON17_1_GOBLIN_THROWER_DEFINITION_SPAWN_CONTRACT.md
```

## MON17.2 — CLOS

MON17.2 a intégré la présentation réelle du Gobelin lanceur et validé le pipeline visuel générique :

```text
SK_GoblinThrower
SKEL_GoblinThrower
PHYS_GoblinThrower
A_GoblinThrower_Idle
A_GoblinThrower_Walk
ABP_MON_GoblinThrower
BP_MON_GoblinThrower
DA_MON_GoblinThrower
```

Contrat de présentation validé :

```text
MonsterActorClass     = BP_MON_GoblinThrower_C
SkeletalMesh          = SK_GoblinThrower
AnimationClass        = ABP_MON_GoblinThrower_C
VisualScale           = (1,1,1)
VisualOffset          = (0,0,0)
VisualRotationOffset  = (0,-90,0)
```

`VisualRotationOffset` est une correction générique de l'axe local du mesh ; `Facing` reste l'orientation logique autoritaire de la grille.

Validation UE5.5.4 acquise :

- `PresentationBridgeContract` : Success ;
- preview éditeur visible sans `MissingSkeletalMesh` ;
- orientation `InitialFacing=North` corrigée et validée ;
- animation Idle/Walk fonctionnelle en PIE, marche in-place sans Root Motion ;
- spawn runtime via `BP_MON_GoblinThrower_C` ;
- combat automatique rejoint correctement le TurnManager ;
- cycle réel HP 10 -> 0, mort, libération d'occupation et victoire ;
- récompense XP 125 appliquée ;
- aucun `PresentationWarning` ni erreur de spawn observé.

Le cas `MonsterTurnStarted -> EndingRound` à distance 1 était volontairement conservé en MON17.2 : `Attack_ThrowKnife.MinRangeCells=2`. MON17.3 a rendu l'attaque projectile exécutable lorsque la situation est déjà valide ; MON17.4 décide maintenant du repositionnement pour obtenir/conserver cette situation.

Référence :

```text
docs/Design/MON17_2_GOBLIN_THROWER_SKELETAL_ANIMATION.md
```

## MON17.3 — CLOS

`MON17.3 — Distinct Attack Set` est **VALIDÉ ET CLOS sous UE5.5.4**.

Contrat final de `Attack_ThrowKnife` :

```text
AttackId               = Attack_ThrowKnife
Delivery               = Projectile
MinRangeCells           = 2
RangeCells              = 6
bRequiresLineOfSight    = true
ActionPointCost         = 2
CooldownTurns           = 0
Priority                = 100
ExpectedDuration        = 2.20 s
ImpactTimeSeconds       = 1.00 s
ProjectileTravel       = 0.20 s
LaunchDelay             = 0.80 s
ProjectileSourceSocket = ProjectileSource
```

Sous-étapes validées :

```text
MON17.3.1 — Ranged Action Execution          CLOS
MON17.3.2 — Projectile Presentation          CLOS
MON17.3.3 — Throw Presentation               CLOS
MON17.3.4 — Cooldown / Regression / Closure  CLOS
```

MON17.3 a établi :

- exécution générique `RangedAttack` sans hardcoding Gobelin ;
- portée min/max et LOS réévaluées au runtime ;
- projectile visuel purement présentation, sans inventaire/persistence/dégâts parallèles ;
- source projectile data-driven via socket optionnel ;
- animation `A_GoblinThrower_ThrowKnife` retargetée depuis Mixamo ;
- montage `AM_GoblinThrower_ThrowKnife` intégré au Slot de `ABP_MON_GoblinThrower` ;
- lancement depuis la paume droite via `ProjectileSource` ;
- cooldown générique par `AttackId`, runtime-only ;
- `CooldownTurns=0` confirmé sans régression pour le Gobelin.

Validation automatisée finale :

```text
MON17.3.1–17.3.4   10/10 Success
MON6                3/3 Success
Total demandé       13/13 Success
```

Validation PIE finale : le Gobelin utilise `Attack_ThrowKnife` sur plusieurs manches successives avec `CooldownTurns=0`, le projectile part toujours de `ProjectileSource`, et Hit/Miss/dégâts restent corrects.

Références :

```text
docs/Design/MON17_3_1_RANGED_ATTACK_EXECUTION.md
docs/Design/MON17_3_2_PROJECTILE_PRESENTATION.md
docs/Design/MON17_3_3_THROW_PRESENTATION.md
docs/Design/MON17_3_4_MONSTER_ATTACK_COOLDOWN.md
```

## MON17.4 — CLOS

`MON17.4 — Distinct AI Profile — RangedKeeper` est **VALIDÉ ET CLOS sous UE5.5.4**.

Le profil possède désormais un comportement tactiquement distinct :

```text
perception
→ évaluer distance / LOS
→ tirer depuis la case actuelle si elle est favorable
→ sinon rechercher une case de tir valide
→ maintenir PreferredMinDistance..PreferredMaxDistance
→ reculer/repositionner si le groupe est trop proche
→ approcher si nécessaire
→ ne jamais charger inutilement au contact comme DirectMelee
```

Contrat du Gobelin :

```text
PreferredMinDistance = 3
PreferredMaxDistance = 5
ThrowKnife range      = 2..6
AP                     = 3
Move                   = 1 AP
ThrowKnife             = 2 AP
```

Validation :

```text
Grimrock.Monsters.MON17.4.1     3/3 Success
MON17.3 + MON6 regressions       13/13 Success
Campagne exécutée                16/16 Success
PIE courte portée                VALIDÉ
PIE distance préférée            VALIDÉ
Approche multi-tour depuis loin  VALIDÉE en Automation
```

Référence :

```text
docs/Design/MON17_4_1_RANGED_KEEPER_POSITIONING.md
```

## MON17.5 — CLOS

`MON17.5 — Patrol / Perception / Alarm Integration` est **VALIDÉ ET CLOS sous UE5.5.4**.

Le Gobelin `RangedKeeper` réutilise sans fork les systèmes MON14 :

```text
patrouille
→ perception directionnelle / ouïe
→ investigation / recherche
→ alarme locale same MonsterId + same EncounterGroup
→ engagement visuel automatique
→ TurnManager
→ RangedKeeper
```

Valeurs de production retenues :

```text
DA_MON_GoblinThrower
bSharesAggroWithGroup = true
AggroPropagationRange = 5
```

Validation automatisée :

```text
Grimrock.Monsters.MON17.5.1    4/4 Success
MON14 présent dans la campagne 19/19 Success
Campagne complète              198/198 Success
```

Validation PIE de production : deux `MON_GoblinThrower` dans `Encounter_GoblinThrowers_01`; `ExplorationAlert Range=5 Alerted=1`, puis démarrage automatique `Reason=PatrolVision`; les deux Gobelins rejoignent l'initiative et exécutent `Attack_ThrowKnife` via le pipeline `ProjectileSource`.

Références :

```text
docs/Design/MON17_5_1_GOBLIN_EXPLORATION_INTEGRATION.md
docs/Design/MON17_5_CLOSURE.md
```

## MON17.6 — EN COURS

Objectif : prouver que le Gobelin lanceur utilise les contrats existants d'encounter, loot et XP sans logique spécifique : groupes/vagues MON13, mort exactement une fois, loot data-driven MON8, récompense `ExperienceReward=125` MON15 et persistance associée.

La validation doit couvrir au minimum :

```text
EncounterGroup / wave participation
MonsterDied exactly once
LootTable Gobelin
XP = 125 appliquée exactement une fois
Victory / occupancy release
Save/Continue sans duplication de loot/XP
```

---

# MON18 — Magic & Spellbook

## Objectif

Transformer l'infrastructure de sorts MON12 en système RPG complet.

Sous-jalons :

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

Cas de validation recommandés : `Magic Missile`, `Fireball`, `Heal`, `Haste`.

---

# MON19 — Advanced Dungeon Logic / Scripting

## Objectif

Permettre au level designer de construire des énigmes riches sans ajouter du C++ spécifique.

Sous-jalons :

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

## Objectif

Passer du groupe techniquement multi-personnages à un système RPG de compagnons et de progression avancée.

Sous-jalons :

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

## Objectif

Donner une structure de campagne aux pages UI actuellement surtout décoratives.

Sous-jalons :

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

## Objectif

Suspendre volontairement l'ajout de grandes infrastructures et construire un jeu testable de bout en bout.

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
2. Travail sur `master`, sans branche de fonctionnalité.
3. Un commit logique par étape lorsque possible.
4. Pousser sur `origin/master` après chaque étape validée.
5. Aucun refactor massif préventif.
6. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
7. Les tests C++ valident la logique ; assets/WBP/maps exigent une validation UE/PIE lorsqu'ils sont impliqués.
8. À la clôture d'un jalon majeur, mettre à jour overview, roadmap et document de clôture.

---

## Prochain travail autoritaire

```text
MON17.6 — Encounter / Loot / XP Integration
```
