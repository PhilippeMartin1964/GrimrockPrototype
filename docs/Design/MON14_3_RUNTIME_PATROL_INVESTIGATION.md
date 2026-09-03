# MON14.3 — Runtime Patrol & Investigation

## Statut

Implémentation C++ prête pour compilation et validation UE 5.5.4.

MON14.3 transforme les données de route introduites par MON14.2 en comportement
d'exploration réellement exécuté. Le principe reste celui du projet : aucune
boucle IA permanente par monstre.

## Objectifs

- exécuter les routes `Loop` et `PingPong` hors combat ;
- respecter l'ordre des waypoints, leur orientation d'arrivée et leur attente ;
- réutiliser le pathfinding MON4 et l'occupation/réservation MON3 ;
- interrompre proprement une patrouille dès qu'une perception pertinente arrive ;
- conserver la règle MON14.1 : **la vue peut engager automatiquement, l'ouïe seule non** ;
- faire enquêter un monstre qui entend le groupe ou conserve une dernière cellule connue ;
- chercher localement le groupe lorsqu'il atteint cette dernière position ;
- revenir à la patrouille après une recherche infructueuse ;
- suspendre atomiquement toute locomotion d'exploration lorsque le combat prend la main.

## Architecture

Nouveau service monde :

```cpp
UGridMonsterPatrolSubsystem : UWorldSubsystem
```

Le service orchestre uniquement les transitions. Il ne remplace aucun système
existant :

```text
MonsterSpawn / PatrolWaypoints       MON14.2 données
              │
              ▼
UGridMonsterPatrolSubsystem          MON14.3 orchestration
       │                 │
       ▼                 ▼
Behavior MON4        Movement MON3
perception/path      occupation/interpolation
       │                 │
       └───────┬─────────┘
               ▼
Automatic Engagement MON14.1
               │
               ▼
TurnManager MON5+
```

### Pas de Tick IA permanent

`UGridMonsterPatrolSubsystem` n'implémente aucun Tick. Les seules attentes sont
des timers one-shot :

- fin prévisible d'une interpolation de déplacement ;
- fin prévisible d'une rotation ;
- `WaitSeconds` d'un waypoint ;
- petit retry lorsqu'un chemin est temporairement bloqué par l'occupation.

`UGridMonsterMovementComponent` conserve son contrat MON3 : son Tick n'est actif
que pendant une interpolation et se désactive ensuite.

## Curseur de route déterministe

`FGridMonsterPatrolCursorRules` contient les règles pures :

### Initialisation

- si le monstre est exactement sur un waypoint, ce waypoint devient l'arrivée
  courante ;
- sinon il rejoint le waypoint de distance Manhattan minimale ;
- les égalités sont résolues par l'index le plus faible.

### Loop

```text
0 -> 1 -> 2 -> ... -> N-1 -> 0
```

### PingPong

```text
0 -> 1 -> 2 -> ... -> N-1 -> N-2 -> ... -> 1 -> 0 -> ...
```

La direction PingPong est un état runtime transitoire. Après un chargement, le
curseur est reconstruit depuis la cellule restaurée et la route statique du
LevelAsset.

## Exécution d'un waypoint

Pour chaque cible :

1. calcul du chemin avec `UGridMonsterBehaviorComponent::FindPathToCell()` ;
2. rotation vers le prochain segment si nécessaire ;
3. `UGridMonsterMovementComponent::TryMove()` d'une seule case ;
4. nouvelle perception à la fin de l'interpolation ;
5. répétition jusqu'à l'arrivée ;
6. application du `Facing` d'arrivée lorsque celui-ci est cardinal ;
7. attente de `WaitSeconds` ;
8. avancement du curseur.

Le déplacement reste donc strictement case par case et passe par le registre
d'occupation existant.

## Dormance

Un `MonsterSpawn` initialement `Dormant` ne parcourt pas sa route tant qu'il
n'est pas réveillé.

```text
Dormant + aucune perception
    -> stationnaire

Dormant + vue
    -> Alert
    -> demande d'engagement MON14.1

Dormant + ouïe seule
    -> Alert
    -> Investigation
```

Cette règle distingue volontairement :

- **Dormant** : acteur présent mais inactif ;
- `bInitiallyEnabled=false` : acteur absent.

## Investigation par l'ouïe

L'ouïe reste omnidirectionnelle et ne démarre jamais directement le combat.
Elle renseigne la dernière cellule connue du groupe via MON4.

```text
Patrolling / Dormant
      │
      └── entend le groupe
              │
              ▼
            Alert
              │
              ▼
        Investigating
              │
              ▼
 chemin vers LastKnownPartyCell
```

L'investigation utilise `FindPathToLastKnownPartyCell()`, qui sait déjà choisir
une cellule accessible adjacente lorsque la cellule mémorisée est occupée par le
groupe.

## Recherche locale

Lorsque le monstre arrive à la dernière position exploitable sans retrouver le
groupe :

1. la direction courante a déjà été testée par `RefreshPerception()` ;
2. le monstre effectue trois rotations à droite ;
3. après chaque rotation, la perception est réévaluée ;
4. les quatre directions cardinales ont ainsi été observées.

Si le groupe est vu pendant cette recherche : engagement MON14.1.

Si le groupe est seulement entendu : une nouvelle dernière cellule connue est
stockée et l'investigation repart vers cette position.

Si rien n'est perçu après le tour d'horizon :

```text
ClearLastKnownPartyCell()
    -> Idle
    -> rejoin / reprise de la patrouille
```

Un monstre sans route reste simplement `Idle`.

## Vue et engagement

Après chaque étape de locomotion d'exploration, MON14.3 rafraîchit la perception.
Une vision valide :

- annule le timer de patrouille ;
- annule proprement une interpolation encore active si nécessaire ;
- place l'activité d'exploration en `Engaging` ;
- demande une évaluation différée à
  `UGridAutomaticPerceptionEngagementSubsystem`.

La règle MON14.1 demeure l'autorité : seul un **source visuel** peut déclencher
le combat automatique.

Une garde contre la réentrance évite qu'une évaluation MON14.1 qui vient juste
d'échouer redemande immédiatement la même évaluation sans changement du monde.

## Prise de contrôle par le combat

Le sous-système se lie à :

```cpp
UGridTurnManagerComponent::OnPhaseChanged
UGridTurnManagerComponent::OnCombatEnded
```

Dès que la phase quitte `Exploration` :

- tous les timers d'exploration suivis sont annulés ;
- tout déplacement/turn d'exploration actif est annulé via
  `CancelCurrentAction()` ;
- la réservation MON3 revient à la cellule de départ ;
- l'activité devient `Suspended`.

Ceci s'applique aussi à un garde entraîné dans le combat par propagation d'aggro
MON7, même s'il n'était pas lui-même la source visuelle.

Après une victoire, les monstres vivants non concernés qui existent encore dans
le niveau peuvent reprendre leur activité. MON14.1 accepte désormais une
nouvelle évaluation sûre depuis l'état terminal `Victory`; `StartCombatInternal`
savait déjà remettre cet état en `Exploration` lors d'une rencontre suivante.

## Activités runtime observables

`EGridMonsterExplorationActivity` :

- `Inactive`
- `Patrolling`
- `Waiting`
- `Investigating`
- `Searching`
- `Engaging`
- `Suspended`

Ces valeurs ne remplacent pas `EGridMonsterState`. Elles décrivent uniquement
l'orchestration d'exploration et servent au diagnostic/tests.

## Persistance

MON9 continue à persister :

- cellule ;
- facing ;
- état du monstre ;
- dernière cellule connue du groupe.

La route elle-même reste dans le LevelAsset MON14.2. MON14.3 ne sérialise pas un
index de waypoint transitoire : après Continue, le curseur est reconstruit de
façon déterministe depuis la cellule restaurée. Cette stratégie évite un bump de
format de sauvegarde pour une donnée dérivable.

## Tests automatisés

Nouvelle suite :

```text
Grimrock.Monsters.MON14.3.CursorRules
Grimrock.Monsters.MON14.3.PatrolMovement
Grimrock.Monsters.MON14.3.HearingInvestigation
Grimrock.Monsters.MON14.3.BlockedHearingWait
Grimrock.Monsters.MON14.3.DormantPatrol
Grimrock.Monsters.MON14.3.CombatSuspension
```

Les helpers du fichier de tests portent tous le préfixe `MON143` afin d'éviter
les collisions de symboles en Unity Build rencontrées après MON14.2.

## Hors périmètre de cette étape runtime

L'édition visuelle avancée d'une route dans le Grid Editor reste une sous-étape
éditeur distincte : affichage de segments/numéros et création de waypoint par
clic. Les données sont déjà sérialisées et le runtime MON14.3 les consomme.
Cette séparation permet de valider d'abord le comportement de jeu avant de
multiplier les surfaces d'édition.


## Correction MON-AI-BLOCKED-INVESTIGATION01 — cible audible inaccessible

Une porte normale fermée laisse volontairement passer le son via `CanSoundTraverse()`, tout en restant infranchissable via `CanMove()`. Un monstre peut donc connaître précisément la cellule du groupe sans disposer d'aucun chemin vers elle.

Avant correction, `ProcessInvestigation()` interprétait l'absence de chemin comme une perte de cible et entrait immédiatement dans `BeginSearch()`. Tant que le groupe restait audible, l'investigation était relancée après le tour d'horizon, ce qui pouvait produire une succession de rotations in-place ressemblant à un tremblement derrière une porte grillagée.

Le contrat corrigé est :

```text
cible encore audible + aucun chemin
→ MonsterState reste Alert
→ Activity reste Investigating
→ aucun Turn de recherche
→ attente 0,25 s
→ nouvelle tentative de pathfinding
```

Si l'obstacle s'ouvre, la tentative suivante reprend normalement le déplacement. Si la cible n'est plus audible, le comportement historique de recherche MON14.3 reprend.
