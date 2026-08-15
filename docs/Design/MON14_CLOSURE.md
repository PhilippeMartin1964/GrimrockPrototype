# MON14 — Exploration Monster AI Closure

Statut : **VALIDÉ / CLOS**

Date de clôture : 15 août 2026.

MON14 ferme le chantier consacré au passage entre exploration et combat ainsi qu'au comportement hors combat des monstres.

---

## 1. Objectif initial

Le besoin de départ était le suivant :

- ne plus déclencher les combats manuellement ;
- démarrer automatiquement un combat lorsqu'un monstre voit le groupe ;
- permettre à un monstre d'être dormant ;
- réveiller un monstre lorsque le groupe entre dans sa perception ;
- permettre à des gardes de patrouiller dans le donjon ;
- faire abandonner la patrouille lorsqu'un garde repère le groupe ;
- permettre à un monstre ayant entendu le groupe d'enquêter sans déclencher immédiatement le combat.

Ce périmètre est désormais couvert.

---

## 2. MON14.1 — Automatic Perception Engagement

### Résultat

`UGridAutomaticPerceptionEngagementSubsystem` raccorde la perception logique MON4 au TurnManager sans commande gameplay F5.

Règle autoritaire :

```text
Vision réelle valide
    -> peut démarrer automatiquement le combat

Ouïe seule
    -> Alert + LastKnownPartyCell
    -> ne démarre pas automatiquement le combat
```

Les évaluations sont différées et coalescées afin de ne pas démarrer un combat pendant un rebuild, un spawn atomique, une transition ou une action encore en cours.

`StartEncounter` reste une transaction de rencontre/spawn et ne force jamais directement `StartCombat()`.

---

## 3. MON14.2 — Directional Vision, Dormancy & Patrol Data

### Résultat

La vision est désormais directionnelle : une cible doit se trouver sur le rayon axial avant du `Facing` cardinal courant du monstre.

`MonsterSpawn` possède un état initial :

```text
Idle
Dormant
```

`Dormant` signifie : acteur présent mais inactif.

```text
bInitiallyEnabled=false
```

continue de signifier : acteur absent.

Les routes de patrouille sont sérialisées dans le LevelAsset :

```text
PatrolMode = None / Loop / PingPong
PatrolWaypoints[]
    Cell
    Facing
    WaitSeconds
```

---

## 4. MON14.3 — Runtime Patrol & Investigation

### Résultat

`UGridMonsterPatrolSubsystem` exécute les routes hors combat sans Tick IA permanent.

Le sous-système utilise :

- timers one-shot ;
- `UGridMonsterMovementComponent` pour l'interpolation ;
- pathfinding MON4 ;
- occupation/réservation MON3 ;
- perception MON4 ;
- engagement MON14.1.

### Patrouille

```text
Loop
0 -> 1 -> 2 -> ... -> N-1 -> 0

PingPong
0 -> 1 -> ... -> N-1 -> N-2 -> ... -> 0
```

À chaque waypoint :

1. déplacement case par case ;
2. Facing d'arrivée éventuel ;
3. attente `WaitSeconds` ;
4. waypoint suivant.

### Investigation

```text
monstre entend le groupe
    -> Alert
    -> LastKnownPartyCell
    -> Investigating
    -> déplacement vers la dernière position connue
    -> Searching
    -> quatre directions testées
    -> reprise de patrouille si échec
```

Une vision réelle pendant l'investigation ou la recherche donne la main à MON14.1.

### Combat

Toute locomotion d'exploration est suspendue atomiquement quand le combat prend la main.

---

## 5. MON14.3.1 — Visual Patrol Route Editor

### Résultat

Les routes sont éditables directement dans `L_GrimrockEditor`.

Fonctions validées :

- sélection d'un `MonsterSpawn` ;
- touche `P` pour entrer/sortir du mode d'édition ;
- ajout/sélection de waypoint par clic ;
- affichage numéroté des points ;
- segments de route ;
- représentation `Loop` / `PingPong` ;
- Facing par waypoint ;
- temps d'attente ;
- réordonnancement ;
- suppression ;
- persistance dans le LevelAsset.

Validation manuelle effectuée sur `L_GrimrockEditor` le 15 août 2026.

---

## 6. MON14.4 — Exploration Alarm Coordination

### Résultat

Un monstre qui perçoit le groupe peut alerter des alliés proches hors combat.

MON14.4 ne crée pas un second système d'aggro. Il réutilise le contrat MON7 :

```text
bSharesAggroWithGroup = true
même MonsterId
même EncounterGroupId
distance <= AggroPropagationRange
```

Un allié alerté reçoit :

```text
MonsterState = Alert
LastKnownPartyCell = cellule connue par la source
Activity = Investigating
```

### Règle essentielle

```text
Alarme seule
    -> investigation
    -> jamais StartCombat

Vision réelle
    -> MON14.1
    -> combat automatique
```

La propagation reste une seule vague. Un allié réveillé par une alarme ne propage pas automatiquement l'alerte à tout le donjon sans perception propre.

Les alarmes répétées vers la même cellule ne réinitialisent pas inutilement une investigation déjà en cours.

---

## 7. Tests automatisés

### MON14.1

La suite couvre notamment :

- vue directe ;
- portée ;
- murs et portes ;
- ouïe seule ;
- dormance ;
- coalescence ;
- visibilité de rencontre ;
- garde de transition/rebuild.

### MON14.2

```text
Grimrock.Monsters.MON14.2.BehaviorFacingIntegration
Grimrock.Monsters.MON14.2.DirectionalSight
Grimrock.Monsters.MON14.2.FreshSpawnConfiguration
Grimrock.Monsters.MON14.2.SpawnModelValidation
```

### MON14.3

```text
Grimrock.Monsters.MON14.3.CursorRules
Grimrock.Monsters.MON14.3.PatrolMovement
Grimrock.Monsters.MON14.3.HearingInvestigation
Grimrock.Monsters.MON14.3.DormantPatrol
Grimrock.Monsters.MON14.3.CombatSuspension
```

Validation : **5/5 Success**.

### MON14.4

```text
Grimrock.Monsters.MON14.4.AlarmFiltering
Grimrock.Monsters.MON14.4.HearingAlarmPropagation
Grimrock.Monsters.MON14.4.SharingDisabled
```

Validation du 15 août 2026 : **3/3 Success**.

Logs fonctionnels observés :

```text
[MON14.4] ExplorationAlert ... Reason=PerceptionHearing
```

avec le nombre attendu d'alliés alertés.

---

## 8. Validation fonctionnelle finale

Le scénario suivant a été validé manuellement :

```text
Rat A : patrouille / perception
Rat B : Dormant
même groupe d'alarme

Rat A entend/perçoit
    -> Rat B est réveillé et passe en investigation
    -> pas de combat par l'alarme seule

un Rat obtient ensuite une vraie ligne de vue frontale
    -> engagement automatique
    -> TurnManager prend la main
```

La route visuelle MON14.3.1 a également été validée dans `L_GrimrockEditor`.

---

## 9. Architecture finale MON14

```text
MonsterSpawn / Patrol Data
        |
        v
Monster Behavior MON4
  vision / hearing / last known
        |
        +---------------------+
        |                     |
        v                     v
Patrol Subsystem          Alarm MON14.4
MON14.3                   (MON7 rules)
        |                     |
        +----------+----------+
                   |
                   v
       Automatic Engagement MON14.1
                   |
                   v
             TurnManager MON5+
```

Aucune autorité n'est dupliquée :

- MON4 = perception/pathfinding ;
- MON3 = occupation/mouvement ;
- MON7 = sélection de propagation d'aggro/alarme ;
- MON14.3 = orchestration exploration ;
- MON14.1 = décision d'engagement automatique ;
- TurnManager = combat.

---

## 10. Commits de clôture

Implémentation MON14.4 :

```text
18c3ecc1d0179f01165c8765c08e6f7d713369ed
Implement MON14.4 exploration alarm coordination
```

Clôture d'assets/carte et XMind :

```text
d66e7d84e10866067ca1362157d860702b20f6e5
Close MON14
```

---

## 11. Décision de clôture

MON14 est **clos**.

Les évolutions suivantes ne doivent pas rouvrir MON14 sauf régression. Les nouvelles familles de monstres devront consommer ces systèmes tels quels et démontrer leur généricité dans MON17.

Le backlog actif passe à :

```text
MON15 — XP & Level Progression
```

Document de roadmap :

```text
docs/Design/PROJECT_COMPLETION_ROADMAP.md
```
