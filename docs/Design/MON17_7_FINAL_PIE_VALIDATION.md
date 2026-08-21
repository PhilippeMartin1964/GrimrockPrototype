# MON17.7 — Final PIE Validation

Statut : **VALIDÉ sous UE5.5.4**  
Date : **21 août 2026**

## Objectif

Valider en PIE la configuration finale du Gobelin lanceur après équilibrage MON17.7, avec les deux Gobelins de production de `Encounter_GoblinThrowers_01`.

Le PIE sert de contrôle de comportement, de sensation de combat et de lisibilité du loot. Les valeurs chiffrées sont verrouillées séparément par `Grimrock.Monsters.MON17.7.FinalBalanceContract`.

## Engagement

Les deux Gobelins sont spawné normalement puis la chaîne MON14 s'exécute :

```text
[MON14.4] ExplorationAlert
Reason=PerceptionHearing
Range=5
Alerted=1

[MON14.1] Automatic combat started
Reason=PatrolVision
```

Les deux `MON_GoblinThrower` rejoignent l'initiative avec `Base=12`.

## Combat à distance

Le pipeline projectile est exécuté à plusieurs reprises :

```text
Attack=Attack_ThrowKnife
Travel=0.200
SourceSocket=ProjectileSource
```

Les deux acteurs Gobelin participent au combat et les jets Hit/Miss continuent d'utiliser le resolver de combat existant.

Le personnage de test est Elias niveau 1, seul, à mains nues. Le combat dure 14 manches et comprend de nombreux échecs du joueur contre `Defense=13`. Cette durée ne justifie pas une modification des stats Gobelin : elle est fortement influencée par la configuration volontairement pauvre du personnage de test.

## Mort, XP et occupation

Première mort :

```text
Reward=125 Applied=125
Previous=0 New=125
RemainingLiving=1
Victory=false
OccupancyReleased=true
```

Seconde mort :

```text
Reward=125 Applied=125
Previous=125 New=250
RemainingLiving=0
Victory=true
OccupancyReleased=true
Phase=Victory
```

Le pool réel de deux Gobelins vaut donc bien `250 XP`.

## Loot probabiliste final

Les probabilités utilisées par le runtime sont bien celles figées en MON17.7 :

```text
GoblinKnife  Chance=0.250
Stone        Chance=0.500
EmptyVial    Chance=0.250
```

### Gobelin 1

```text
GoblinKnife  Roll=0.743  Dropped=false
Stone        Roll=0.035  Dropped=true  Quantity=1
EmptyVial    Roll=0.329  Dropped=false
Summary      Evaluated=3 Dropped=1 Placed=1 Failed=0
```

Une pierre est déposée au sol avec un `RuntimeId` valide.

### Gobelin 2

```text
GoblinKnife  Roll=0.787  Dropped=false
Stone        Roll=0.505  Dropped=false
EmptyVial    Roll=0.379  Dropped=false
Summary      Evaluated=3 Dropped=0 Placed=0 Failed=0
```

Aucun drop sur le second Gobelin est un résultat valide. Les trois tirages sont indépendants ; l'espérance `1.000 objet/Gobelin` est une moyenne à long terme, pas une garantie par kill.

## Résultat

```text
Engagement MON14                    VALIDÉ
Deux Gobelins dans l'initiative     VALIDÉ
RangedKeeper / ThrowKnife           VALIDÉ
ProjectileSource                    VALIDÉ
Loot 0.25 / 0.50 / 0.25             VALIDÉ
Loot Failed=0                       VALIDÉ
XP 125 par Gobelin                  VALIDÉ
XP total 250                        VALIDÉ
Occupancy release                   VALIDÉ
Victory                             VALIDÉ
```

Aucun défaut MON17.7 bloquant n'est observé dans ce PIE.

## Suite

Le seul verrou restant avant la clôture de MON17 est la campagne de régression ciblée décrite dans `MON17_FINAL_REGRESSION_PLAN.md`.