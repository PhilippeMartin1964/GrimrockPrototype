# MON17.5.1 — Goblin Thrower Exploration Integration Contract

Statut : **VALIDÉ / CLOS sous UE5.5.4**

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

Les tests synthétiques utilisent les valeurs de référence :

```text
MonsterId                = MON_GoblinThrower
PrimaryAIProfile         = RangedKeeper
PreferredMinDistance     = 3
PreferredMaxDistance     = 5
ActionPointsPerTurn      = 3
SightRangeCells          = 8
HearingRangeCells        = 4
```

Pour les scénarios d'alarme, les valeurs sont volontairement injectées dans la définition de test :

```text
bSharesAggroWithGroup    = true
AggroPropagationRange    = 2
EncounterGroupId         = GoblinAlarm_A
```

Cela permet de vérifier le contrat sans modifier prématurément un `.uasset` de production.

## Pourquoi aucun nouveau code runtime n'est ajouté

L'audit MON14 montre que les systèmes d'exploration ne filtrent pas sur `PrimaryAIProfile`.

`UGridMonsterPatrolSubsystem::ProcessMonsterInternal()` raisonne sur :

- état du monstre ;
- perception ;
- route de patrouille ;
- occupation ;
- dernière cellule connue ;
- sécurité du TurnManager.

Il ne contient aucune branche `DirectMelee` / `FastHarasser` / `RangedKeeper`.

De même, `UGridMonsterBehaviorComponent::RefreshPerception()` applique la vision directionnelle et l'ouïe à toute définition monstre valide.

Enfin, `HandleExplorationAlert()` réutilise le contrat MON7/MON14.4 :

```text
same MonsterId
+ same EncounterGroupId
+ dans AggroPropagationRange
+ vivant / enabled
=> ally Alert / Investigating
```

Un réveil par alarme ne déclenche pas à lui seul le combat.

## Tests automatisés

Filtre :

```text
Grimrock.Monsters.MON17.5.1
```

Tests :

```text
PatrolRangedKeeper
DirectionalPerception
HearingAlarm
VisionEngagementHandoff
```

### PatrolRangedKeeper

Crée un `MON_GoblinThrower` :

```text
PrimaryAIProfile = RangedKeeper
PatrolMode        = Loop
Route             = (1,1) <-> (4,1)
```

Le groupe est hors perception.

Le test vérifie :

- la définition conserve `RangedKeeper` ;
- `ProcessMonsterNow()` réussit ;
- `UGridMonsterMovementComponent` démarre un mouvement de grille ;
- l'activité devient `Patrolling`.

Le profil de combat n'empêche donc pas le Gobelin d'utiliser la patrouille générique MON14.3.

### DirectionalPerception

Configuration :

```text
Goblin = (1,1)
Party  = (1,4)
Sight  = 8
Hearing = 4
```

Facing North :

```text
bCanSeeParty  = true
bCanHearParty = true
```

Facing East :

```text
bCanSeeParty  = false
bCanHearParty = true
```

Le test confirme que `RangedKeeper` conserve exactement la perception directionnelle MON14.2 et l'ouïe omnidirectionnelle.

### HearingAlarm

Deux Gobelins `RangedKeeper` du même `EncounterGroupId` sont créés.

La source :

- tourne le dos au groupe ;
- ne le voit pas ;
- l'entend ;
- partage l'aggro avec son groupe.

L'allié est `Dormant`.

Le test vérifie :

```text
Source hears party
-> ExplorationAlert
-> ally Dormant -> Alert
-> LastKnownPartyCell transférée
-> activity = Investigating
-> combat reste inactif
```

Cela confirme que le Gobelin peut réveiller un allié sans contourner la règle MON14.1 : l'ouïe seule ne démarre pas directement le combat.

### VisionEngagementHandoff

Un Gobelin `RangedKeeper` patrouilleur voit le groupe dans son rayon cardinal.

Le test vérifie :

```text
bCanSeeParty = true
MonsterState = Alert
ExplorationActivity = Engaging
```

Le démarrage réel du combat reste différé vers `UGridAutomaticPerceptionEngagementSubsystem`, comme pour les autres monstres.

## Validation UE5.5.4 — 20 août 2026

La campagne complète fournie par l'utilisateur contient :

```text
Tests terminés : 198
Success         : 198
Fail            : 0
```

### MON17.5.1

Résultat : **4/4 Success**.

```text
DirectionalPerception      Success
HearingAlarm               Success
PatrolRangedKeeper         Success
VisionEngagementHandoff    Success
```

Le log `HearingAlarm` confirme également l'utilisation du pipeline MON14.4 :

```text
[MON14.4] ExplorationAlert
Source=GridMonsterActor_0
Group=GoblinAlarm_A
Cell=(1,4)
Range=2
Alerted=1
Reason=PerceptionHearing
```

### Régressions d'exploration MON14

Tous les tests MON14 présents dans cette campagne sont verts :

```text
MON14.1    7/7 Success
MON14.2    4/4 Success
MON14.3    5/5 Success
MON14.4    3/3 Success
Total     19/19 Success
```

Les sous-systèmes directement réutilisés par MON17.5.1 sont donc couverts sans régression : patrouille, investigation, perception directionnelle, suspension combat et alarme locale.

### Régressions MON17

La même campagne confirme aussi :

```text
MON17.1                3/3 Success
MON17.2                2/2 Success
MON17.3.1–17.3.4      10/10 Success
MON17.4.1              3/3 Success
MON17.5.1              4/4 Success
```

### Intégration PIE déjà observée

`Grimrock.Monsters.MON13.5.RealPIEIntegration` termine également en `Success` dans cette campagne.

Le log PIE montre un Gobelin réel `MON_GoblinThrower` participant au handoff d'exploration vers le combat :

```text
[MON14.1] Automatic combat started
Reason=PatrolVision
```

Puis le TurnManager démarre son tour :

```text
MonsterTurnStarted Message="Tour de Gobelin lanceur."
Attack=Attack_ThrowKnife
```

Cela confirme que l'intégration d'exploration n'interrompt ni `RangedKeeper` ni le pipeline projectile MON17.3.

## Warnings observés dans la campagne

La campagne contient des warnings provenant de fixtures historiques de tests, notamment :

- `MissingMonsterMovement` dans certains tests de mort ;
- `PresentationWarning` sur des définitions synthétiques MON8 sans mesh/AnimBP ;
- `Party=None` dans des tests MON9 d'ordre d'initialisation ;
- rejets `InvalidClassDefinition` volontairement exercés par MON15 ;
- un échec volontaire de placement de loot MON15.2 sans RuntimeActor.

Ils n'entraînent aucun échec Automation et aucun de ces warnings n'est produit par les quatre tests MON17.5.1.

## Valeurs de production à valider en PIE

Le contrat étant validé, l'étape suivante peut fixer un comportement de groupe réel sur `DA_MON_GoblinThrower`.

Valeur de départ retenue pour la validation de production :

```text
bSharesAggroWithGroup = true
AggroPropagationRange = 5
```

`5` est volontairement au milieu de la plage envisagée `4..6` et reste cohérent avec un Gobelin ayant `SightRangeCells=8` et `HearingRangeCells=4`.

La valeur définitive restera révisable à l'équilibrage MON17.7.

## Hors périmètre

- créer un nouveau profil `FleeAndCallHelp` ;
- modifier le planner `RangedKeeper` MON17.4 ;
- faire démarrer le combat sur ouïe seule ;
- propagation d'alarme entre MonsterId différents ;
- logique de renforts inter-groupes ;
- scripts de portes spécifiques au Gobelin.

## Porte de sortie MON17.5.1 — ACQUISE

```text
1. Compilation / exécution UE5.5.4                    VALIDÉE
2. Grimrock.Monsters.MON17.5.1                       4/4 Success
3. Régressions MON14.3 / MON14.4                     8/8 Success
4. Régressions MON14 présentes dans la campagne     19/19 Success
5. Campagne automatisée fournie                    198/198 Success
6. Handoff Gobelin PatrolVision -> combat            OBSERVÉ EN PIE
```

MON17.5.1 est donc **VALIDÉ / CLOS**.

Étape suivante : **MON17.5.2 — validation PIE de production avec deux Gobelins, route de patrouille et groupe d'alarme réel**.
