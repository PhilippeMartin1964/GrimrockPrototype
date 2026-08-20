# MON17.5.1 — Goblin Thrower Exploration Integration Contract

Statut : **VALIDÉ ET CLOS sous UE5.5.4**

## Objectif

Prouver que `MON_GoblinThrower` et le profil `RangedKeeper` réutilisent correctement l'architecture d'exploration MON14 sans créer une seconde couche d'IA parallèle.

MON17.5.1 ne modifie volontairement aucun code de production lorsque MON14 fournit déjà le comportement attendu.

Les autorités conservées sont :

```text
UGridMonsterBehaviorComponent
    -> perception directionnelle / ouïe / LastKnownPartyCell

UGridMonsterPatrolSubsystem
    -> patrouille / investigation / recherche / suspension combat

UGridAutomaticPerceptionEngagementSubsystem
    -> engagement automatique sur source visuelle

FGridFastHarasserPlanner::SelectAggroTargets
    -> filtrage local same MonsterId / same EncounterGroup / distance
```

Le profil de combat `RangedKeeper` reste exclusivement responsable de la tactique en combat validée en MON17.4.

## Contrat Gobelin

Valeurs de référence :

```text
MonsterId                = MON_GoblinThrower
PrimaryAIProfile         = RangedKeeper
PreferredMinDistance     = 3
PreferredMaxDistance     = 5
ActionPointsPerTurn      = 3
SightRangeCells          = 8
HearingRangeCells        = 4
```

Valeurs de production retenues pour l'alarme :

```text
bSharesAggroWithGroup    = true
AggroPropagationRange    = 5
```

Le `MonsterSpawn` reste responsable de `EncounterGroupId` et de la route de patrouille.

## Pourquoi aucun nouveau code runtime n'est ajouté

L'audit MON14 confirme que les systèmes d'exploration ne filtrent pas sur `PrimaryAIProfile`.

`UGridMonsterPatrolSubsystem::ProcessMonsterInternal()` raisonne sur l'état du monstre, sa perception, sa route de patrouille, l'occupation, la dernière cellule connue et la sécurité du TurnManager. Il ne contient aucune branche `DirectMelee` / `FastHarasser` / `RangedKeeper`.

`UGridMonsterBehaviorComponent::RefreshPerception()` applique la vision directionnelle et l'ouïe à toute définition monstre valide.

Enfin, `HandleExplorationAlert()` réutilise le contrat MON7/MON14.4 :

```text
same MonsterId
+ same EncounterGroupId
+ dans AggroPropagationRange
+ vivant / enabled
=> ally Alert / Investigating
```

Un réveil par alarme ne déclenche pas à lui seul le combat ; le démarrage automatique reste sous l'autorité d'une perception visuelle valide.

## Tests automatisés

Filtre :

```text
Grimrock.Monsters.MON17.5.1
```

Résultats validés :

```text
PatrolRangedKeeper          Success
DirectionalPerception      Success
HearingAlarm                Success
VisionEngagementHandoff    Success

MON17.5.1                   4/4 Success
```

La campagne complète fournie lors de la validation a terminé **198 tests, 198 Success, 0 Fail**.

Régressions MON14 présentes dans cette campagne :

```text
MON14.1 + MON14.2 + MON14.3 + MON14.4   19/19 Success
MON14.3                                    5/5 Success
MON14.4                                    3/3 Success
```

### PatrolRangedKeeper

Un `MON_GoblinThrower` avec `PrimaryAIProfile=RangedKeeper` et une route `Loop` démarre correctement son mouvement de patrouille et passe en activité `Patrolling`.

### DirectionalPerception

Configuration synthétique :

```text
Goblin = (1,1)
Party  = (1,4)
Sight  = 8
Hearing = 4
```

Facing vers le groupe : vision + ouïe. Facing hors axe : plus de vision, ouïe conservée. Le profil `RangedKeeper` ne modifie donc pas le contrat MON14.2.

### HearingAlarm

Deux Gobelins `RangedKeeper` du même `EncounterGroupId` sont utilisés. La source entend le groupe sans le voir et partage l'aggro. L'allié `Dormant` devient `Alert`, reçoit `LastKnownPartyCell`, passe en `Investigating`, sans démarrage de combat sur l'ouïe seule.

### VisionEngagementHandoff

Un Gobelin patrouilleur voyant le groupe passe en `Alert` / `Engaging` et délègue le démarrage réel du combat à `UGridAutomaticPerceptionEngagementSubsystem`.

## Validation PIE de production à deux Gobelins

Configuration validée dans `Into_The_Dark` :

```text
Gobelin A
  MonsterId         = MON_GoblinThrower
  EncounterGroupId  = Encounter_GoblinThrowers_01

Gobelin B
  MonsterId         = MON_GoblinThrower
  EncounterGroupId  = Encounter_GoblinThrowers_01

DA_MON_GoblinThrower
  bSharesAggroWithGroup = true
  AggroPropagationRange = 5
```

Le log PIE confirme la chaîne :

```text
ExplorationAlert
  Source=BP_MON_GoblinThrower_C_2
  Group=Encounter_GoblinThrowers_01
  Range=5
  Alerted=1
  Reason=PerceptionHearing

Automatic combat started
  Reason=PatrolVision
```

Les deux Gobelins entrent dans l'initiative :

```text
013847CD... Total=32
3E48D6B8... Total=27
```

Le premier Gobelin exécute ensuite :

```text
Attack_ThrowKnife
ProjectileSource
Hit Elias 5 dégâts
```

Puis le second Gobelin exécute lui aussi `Attack_ThrowKnife` lors du même combat. Le pipeline d'exploration MON14 et le pipeline de combat `RangedKeeper` MON17.4 s'enchaînent donc sans fork spécifique.

Le scénario PIE n'isole pas une longue fenêtre « ouïe seule » : une LOS valide est trouvée dans la même évaluation et le combat démarre immédiatement après l'alarme. Ce point n'est pas une lacune, car `HearingAlarm` valide explicitement le contrat « alarme/ouïe seule ne démarre pas le combat ».

## Contrat final MON17.5.1

```text
Patrouille générique MON14.3              VALIDÉE
Vision directionnelle MON14.2             VALIDÉE
Ouïe omnidirectionnelle                   VALIDÉE
Alarme same MonsterId/group               VALIDÉE
Réveil Dormant -> Alert/Investigating     VALIDÉ
Pas de combat sur ouïe seule              VALIDÉ Automation
Engagement visuel automatique             VALIDÉ Automation + PIE
Handoff combat vers RangedKeeper          VALIDÉ PIE
Deux Gobelins dans le même combat         VALIDÉ PIE
```

## Hors périmètre

- créer un nouveau profil `FleeAndCallHelp` ;
- modifier le planner `RangedKeeper` MON17.4 ;
- faire démarrer le combat sur ouïe seule ;
- propagation d'alarme entre `MonsterId` différents ;
- logique de renforts inter-groupes ;
- scripts de portes spécifiques au Gobelin.

## Porte de sortie

**MON17.5.1 est VALIDÉ ET CLOS.**

La validation de production complète permet également de clore `MON17.5 — Patrol / Perception / Alarm Integration` et de passer à `MON17.6 — Encounter / Loot / XP Integration`.
