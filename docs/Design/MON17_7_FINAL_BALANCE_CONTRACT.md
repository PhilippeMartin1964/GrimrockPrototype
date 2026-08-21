# MON17.7 — Final Balance Contract

Statut : **IMPLÉMENTÉ — validation UE5.5.4 à faire**  
Date : **21 août 2026**

## Objectif

Transformer les décisions de balance MON17.7 en contrat de production automatisé et empêcher une régression silencieuse des valeurs finales du Gobelin lanceur.

Ce contrat ne modifie ni le runtime, ni l'IA, ni les DataAssets. Il charge les vrais assets de production et vérifie leur configuration finale.

## Décisions de balance verrouillées

Statistiques conservées :

```text
MonsterId             MON_GoblinThrower
DangerLevel           3
MaxHealth             10
Initiative            12
Accuracy              2
Evasion               3
ActionPointsPerTurn    3
PrimaryAIProfile      RangedKeeper
PreferredDistance     3..5
```

Attaque :

```text
Attack_ThrowKnife
Damage                2..5
Range                 2..6
ActionPointCost       2
```

Récompense XP :

```text
Gobelin lanceur       125 XP
Rat Géant             500 XP
4 Gobelins            = 1 Rat Géant
```

Loot final :

```text
GoblinKnife  DropChance=0.25 Quantity=1..1
Stone        DropChance=0.50 Quantity=1..1
EmptyVial    DropChance=0.25 Quantity=1..1

ExpectedItemsPerKill = 1.000
```

Les trois tirages restent indépendants ; ce contrat ne crée aucune table exclusive ou règle spéciale au Gobelin.

## Automation

Nouveau test :

```text
Grimrock.Monsters.MON17.7.FinalBalanceContract
```

Le test charge :

```text
DA_MON_GoblinThrower
DA_MON_RatGiant
```

et vérifie :

```text
identités stables
stats finales Gobelin
profil RangedKeeper
portée préférée 3..5
ThrowKnife 2..5, portée 2..6, coût 2 AP
XP Gobelin = 125
XP Rat Géant = 500
4 × 125 = 500
LootTable = exactement 3 entrées
GoblinKnife = 0.25
Stone = 0.50
EmptyVial = 0.25
quantités = 1..1
ExpectedItemsPerKill = 1.000
```

## Porte de sortie

```text
1. DA_MON_GoblinThrower équilibré poussé sur origin/master   À FAIRE
2. Compilation UE5.5.4                                      À VALIDER
3. Grimrock.Monsters.MON17.7.FinalBalanceContract           À VALIDER
4. PIE final deux Gobelins                                   À VALIDER
5. Régressions finales MON17 + MON6/8/13/14/15              À VALIDER
6. MON17 closure                                             À FAIRE
```

Le PIE final reste un contrôle de sensation et de lisibilité. Les valeurs de production sont désormais couvertes par Automation.
