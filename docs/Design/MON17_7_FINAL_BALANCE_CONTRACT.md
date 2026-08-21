# MON17.7 — Final Balance Contract

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
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

Test :

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

## Validation UE5.5.4

Le contrat final a été exécuté avec succès :

```text
Grimrock.Monsters.MON17.7.FinalBalanceContract  Success
```

Sortie de référence :

```text
MON17.7 final balance: HP=10 Initiative=12 Accuracy=2 Evasion=3 AP=3 Damage=2..5 XP=125 LootExpected=1.000
```

`ProductionLootBaseline` a également été relancé sur les valeurs finales :

```text
GoblinKnife Chance=0.250 Quantity=1..1
Stone       Chance=0.500 Quantity=1..1
EmptyVial   Chance=0.250 Quantity=1..1
ExpectedItemsPerKill=1.000
Result=Success
```

Le `DA_MON_GoblinThrower.uasset` équilibré est versionné sur `origin/master`.

Le PIE final à deux Gobelins est également validé. Référence :

```text
docs/Design/MON17_7_FINAL_PIE_VALIDATION.md
```

La régression `Grimrock.Monsters.MON13.5.RealPIEIntegration`, perturbée par la coexistence des nouveaux monstres de production sur la carte de test, a été isolée de ces acteurs étrangers puis relancée avec succès sous UE5.5.4.

## Porte de sortie — FERMÉE

```text
1. DA_MON_GoblinThrower équilibré poussé sur origin/master   VALIDÉ
2. Compilation UE5.5.4                                      VALIDÉE
3. Grimrock.Monsters.MON17.7.FinalBalanceContract           SUCCESS
4. PIE final deux Gobelins                                   VALIDÉ
5. Régressions finales MON17 + MON6/8/13/14/15              VALIDÉES
6. MON17 closure                                             CLOS
```

Toutes les portes de sortie de MON17.7 sont considérées satisfaites. MON17.7 et le jalon majeur MON17 sont clos ; le prochain jalon autoritaire est `MON18 — Magic & Spellbook`.