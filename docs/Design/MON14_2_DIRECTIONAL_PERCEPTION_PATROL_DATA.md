# MON14.2 — Perception directionnelle, état initial et données de patrouille

## Statut

MON14.2 prolonge MON14.1 sans modifier le TurnManager ni le protocole de démarrage du combat.

Le jalon introduit trois fondations :

1. un état initial `Idle` ou `Dormant` sérialisé par `MonsterSpawn` ;
2. une vision axiale directionnelle qui tient compte du `Facing` courant du monstre ;
3. un modèle de données de patrouille sérialisé, mais sans exécution de mouvement avant MON14.3.

## Principes conservés de MON14.1

Le raccord automatique reste :

```text
événement runtime
    -> UGridAutomaticPerceptionEngagementSubsystem
    -> évaluation différée/coalescée
    -> StartCombatFromPerception()
```

Une source directe automatique doit **voir** le groupe. L'ouïe seule peut mettre le monstre en `Alert` et mémoriser `LastKnownPartyCell`, mais ne lance pas automatiquement le combat.

Le chemin manuel/diagnostic `StartCombatFromPerception()` conserve son contrat historique vue **ou** ouïe.

Aucun `Tick` IA n'est ajouté.

## 1. État initial du MonsterSpawn

`FGridLevelObjectData` possède maintenant :

```cpp
EGridMonsterState InitialMonsterState = EGridMonsterState::Idle;
```

Les seules valeurs authoring valides sont :

- `Idle` ;
- `Dormant`.

Les états `Alert`, `Pursuing`, `Attacking`, `Repositioning`, `Hurt` et `Dead` sont des états runtime et ne peuvent pas être utilisés comme état de départ d'un placement frais.

### Présence et dormance restent distinctes

La règle MON14.1 est maintenue :

```text
bInitiallyEnabled = false
    => le monstre est absent

bInitiallyEnabled = true + InitialMonsterState = Dormant
    => le monstre est présent mais dormant
```

`Dormant` ne doit donc jamais être simulé en désactivant le `MonsterSpawn`.

### Fresh game et Continue

Lors d'une création fraîche, `AGridMonsterActor::InitializeMonster()` lit la configuration du `MonsterSpawn` possédant le même `SpawnId` et applique `Idle` ou `Dormant` avant `BeginPlay`.

Lors d'un Continue, MON9/MON13 restaure ensuite l'état runtime sauvegardé. L'état initial du placement n'écrase jamais un état persistant déjà connu.

## 2. Champ de vision directionnel

MON4 utilisait déjà une géométrie de vue simple et adaptée au dungeon crawler :

- même ligne X ou Y ;
- portée en cellules ;
- chaque edge traversé doit être praticable ;
- murs et portes fermées bloquent la vue ;
- pas de vision autour d'un angle.

MON14.2 ajoute le `Facing` comme contrainte supplémentaire.

Pour un monstre en `(X,Y)` :

```text
North : cible sur X identique et Y supérieur
East  : cible sur Y identique et X supérieur
South : cible sur X identique et Y inférieur
West  : cible sur Y identique et X inférieur
```

Le modèle reste volontairement un **rayon cardinal**, pas un cône angulaire. C'est cohérent avec le déplacement case par case et permet de produire des gardes lisibles sans introduire de physique ou de perception continue.

### API pure

`FGridMonsterPerception` conserve :

```cpp
HasStraightLineOfSight(...)
```

comme contrat géométrique MON4 indépendant du Facing.

MON14.2 ajoute :

```cpp
IsTargetInFacingDirection(...)
HasDirectionalLineOfSight(...)
```

`UGridMonsterBehaviorComponent::RefreshPerception()` utilise désormais `HasDirectionalLineOfSight()`.

### Ouïe

L'ouïe n'est pas modifiée : elle reste omnidirectionnelle et fondée sur la distance de Manhattan.

## 3. Données de patrouille

MON14.2 introduit :

```cpp
EGridMonsterPatrolMode
{
    None,
    Loop,
    PingPong
};
```

et :

```cpp
FGridMonsterPatrolWaypoint
{
    FIntPoint Cell;
    EGridEdge Facing;
    float WaitSeconds;
};
```

Chaque `MonsterSpawn` possède :

```cpp
EGridMonsterPatrolMode PatrolMode;
TArray<FGridMonsterPatrolWaypoint> PatrolWaypoints;
```

Le runtime copie ces données vers l'Actor lors d'un spawn frais.

### Sémantique des waypoints

- `Cell` est la cellule d'arrivée ;
- `Facing=None` signifie que le waypoint n'impose pas d'orientation finale ;
- une direction cardinale impose l'orientation d'arrivée future ;
- `WaitSeconds=0` signifie aucune attente ;
- une route `Loop` reviendra du dernier waypoint vers le premier ;
- une route `PingPong` parcourra la liste dans les deux sens.

Ces deux dernières règles sont **des contrats de données seulement dans MON14.2**.

## 4. Ce que MON14.2 n'implémente pas

MON14.2 ne fait pas encore marcher un monstre hors combat.

Il n'ajoute donc pas :

- de scheduler de patrouille ;
- de déplacement automatique entre waypoints ;
- d'attente runtime ;
- de retournement automatique au waypoint ;
- d'investigation après bruit ;
- de retour à la patrouille après perte du groupe ;
- de mouvement IA dans `Tick`.

Ces comportements appartiennent à MON14.3.

## 5. Validation des MonsterSpawn

`UGridLevelAsset::ValidateMonsterSpawns()` valide maintenant :

- `InitialMonsterState` doit être `Idle` ou `Dormant` ;
- une patrouille active (`Loop` ou `PingPong`) requiert au moins deux waypoints ;
- chaque waypoint doit être dans la grille ;
- sa cellule doit être non vide et autoriser l'occupation ;
- son `Facing` doit être `None` ou cardinal ;
- `WaitSeconds` doit être fini et positif ou nul.

Une liste de waypoints peut rester stockée avec `PatrolMode=None`. Cela permet de désactiver temporairement une route sans perdre son authoring.

## 6. Compatibilité des assets existants

Aucune migration de version n'est nécessaire pour les LevelAssets :

```text
ancien MonsterSpawn
    -> InitialMonsterState = Idle
    -> PatrolMode = None
    -> PatrolWaypoints = []
```

La migration existante de `LocalYaw -> InitialFacing` reste inchangée.

## 7. Édition

Les nouvelles données sont sérialisées directement dans `FGridLevelObjectData` et exposées comme propriétés `Monster` / `Monster|Patrol`.

Le panneau sélectionné spécialisé du Grimrock Grid Editor conserve pour l'instant son UI MON13 compacte. L'ajout d'un véritable éditeur visuel de route (ajout/suppression/ordre des waypoints directement dans le viewport) est volontairement associé à MON14.3, lorsque les routes deviennent exécutables et testables en exploration.

## 8. Tests automatisés

La suite :

```text
Grimrock.Monsters.MON14.2
```

couvre :

- les quatre directions cardinales ;
- l'absence de vision arrière/latérale ;
- la portée et les edges bloquants ;
- la conservation du helper géométrique MON4 ;
- la validation `Idle/Dormant` ;
- la validation des routes et waypoints ;
- le transfert `Dormant`, `Facing`, `PatrolMode` et waypoints vers un Actor frais ;
- l'absence de mouvement automatique en MON14.2 ;
- l'intégration réelle du Facing dans `UGridMonsterBehaviorComponent`.

La suite MON14.1 est adaptée afin que ses monstres de test soient explicitement orientés vers le groupe.

## 9. Régressions recommandées

Après compilation UE 5.5.4 :

```text
Grimrock.Monsters.MON4
Grimrock.Monsters.MON5
Grimrock.Monsters.MON7
Grimrock.Monsters.MON13
Grimrock.Monsters.MON14.1
Grimrock.Monsters.MON14.2
Grimrock.Monsters.MON
```

## 10. Suite proposée — MON14.3

MON14.3 pourra maintenant implémenter la véritable exploration des gardes :

```text
Idle + PatrolMode != None
    -> chemin vers waypoint
    -> déplacement case par case
    -> orientation d'arrivée
    -> attente
    -> waypoint suivant

vue du groupe
    -> Alert
    -> abandon de patrouille
    -> engagement MON14.1
```

L'implémentation devra rester événementielle : un timer/scheduler discret ou les callbacks de fin de mouvement sont préférables à une logique IA permanente dans `Tick`.
