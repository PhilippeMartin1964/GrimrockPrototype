# MON17.5.1 — Goblin Thrower Exploration Integration Contract

Statut : **IMPLÉMENTÉ — compilation et validation UE5.5.4 à faire**

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

Nouveau filtre :

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

Des PIE MON17 précédents ont déjà confirmé le handoff complet pour le Gobelin avec :

```text
[MON14.1] Automatic combat started ... Reason=PatrolVision
```

## Valeurs de production à décider après Automation

MON17.5 ne doit pas imposer silencieusement des valeurs d'alarme au DataAsset.

Après validation des tests, la configuration de production de `DA_MON_GoblinThrower` devra être arrêtée explicitement :

```text
bSharesAggroWithGroup
AggroPropagationRange
```

Le choix recommandé pour un garde Gobelin coordonné est :

```text
bSharesAggroWithGroup = true
AggroPropagationRange = 4 à 6 cellules
```

La valeur finale sera fixée en fonction du scénario PIE et de l'équilibrage MON17.7.

## Hors périmètre

- créer un nouveau profil `FleeAndCallHelp` ;
- modifier le planner `RangedKeeper` MON17.4 ;
- faire démarrer le combat sur ouïe seule ;
- propagation d'alarme entre MonsterId différents ;
- logique de renforts inter-groupes ;
- scripts de portes spécifiques au Gobelin.

## Porte de sortie MON17.5.1

```text
1. Compilation UE5.5.4                              À VALIDER
2. Grimrock.Monsters.MON17.5.1                     4/4 attendus
3. Régressions MON14.3 / MON14.4                   À VALIDER
4. PIE patrouille / vision Gobelin                  déjà partiellement acquis
5. PIE alarme entre deux Gobelins                   à faire après Automation
```

Si ces contrats sont verts, MON17.5 pourra passer à la validation de production du Gobelin dans une vraie route de patrouille et un vrai groupe d'alarme, sans modification architecturale supplémentaire.
