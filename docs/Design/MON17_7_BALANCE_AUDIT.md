# MON17.7 — Balance / Closure — Audit initial

Statut : **EN COURS — contrat final VALIDÉ sous UE5.5.4, PIE final et régressions à faire**
Date : **21 août 2026**

## Objectif

MON17.7 doit figer une baseline cohérente pour le Gobelin lanceur, vérifier son pacing face au Rat Géant, ajuster uniquement les données qui le nécessitent, exécuter les régressions finales puis clore MON17.

Cette étape n'introduit ni nouveau système de combat, ni nouveau profil IA, ni refactor du loot ou de l'XP.

## Baseline de production validée

La suite charge le vrai `DA_MON_GoblinThrower` et a confirmé sous UE5.5.4 :

```text
Danger              3
HP                  10
Armures             0 / 0
Initiative          12
Accuracy / Evasion  2 / 3
AP                  3
Attaques            1
Dégâts bruts        2..5
Dégâts bruts moyens 3.5
XP                  125
```

Le contrat tactique reste cohérent : `RangedKeeper`, distance préférée `3..5`, `ThrowKnife` à portée `2..6`, mouvement `1 AP` puis attaque `2 AP`.

Aucun changement de statistiques de combat n'est justifié. Elles restent figées pour la clôture MON17.

## Pacing XP validé

Avec la courbe MON15 actuelle :

```text
4 Gobelins = 1 Rat Géant = 500 XP
8 Gobelins atteignent le niveau 2 en solo
```

Pour quatre personnages actifs, `125` se partage en `32/31/31/31`. Avec l'ordre stable MON15.2 :

```text
personnage 0      niveau 2 après 32 Gobelins
personnages 1..3  niveau 2 après 33 Gobelins
```

Le décalage d'un Gobelin entre le premier personnage et les autres est faible et ne justifie pas de déformer la récompense uniquement pour obtenir une divisibilité parfaite par quatre.

Décision MON17.7 :

```text
ExperienceReward = 125  CONSERVÉ
```

## Baseline loot initiale

La configuration temporaire de MON17.6 était :

```text
GoblinKnife  Chance=1.0 Quantity=1
Stone        Chance=1.0 Quantity=1
EmptyVial    Chance=1.0 Quantity=1
ExpectedItemsPerKill = 3.000
```

Cette valeur rendait MON17.6 déterministe mais était trop généreuse comme balance finale.

## Validation Automation baseline

```text
Grimrock.Monsters.MON17.7.ProductionBalanceBaseline  Success
Grimrock.Monsters.MON17.7.ProductionLootBaseline     Success
Grimrock.Monsters.MON17.7.RewardPacingBaseline       Success

MON17.7 baseline                                     3/3 Success
```

Sortie de référence :

```text
Danger=3 HP=10 Armor=0/0 Initiative=12 Accuracy=2 Evasion=3
AP=3 Attacks=1 Damage=2..5 Average=3.50 XP=125
```

## Décision de balance finale : loot

MON17.7 ne modifie qu'une famille de paramètres. Les statistiques de combat et l'XP restent inchangées ; l'ajustement final porte uniquement sur les `DropChance` du `LootTable`.

Valeurs retenues et validées sous UE5.5.4 :

```text
GoblinKnife  DropChance = 0.25
Stone        DropChance = 0.50
EmptyVial    DropChance = 0.25

MinQuantity = 1
MaxQuantity = 1

ExpectedItemsPerKill = 1.000
ExpectedItems pour 2 Gobelins = 2.000
```

`Grimrock.Monsters.MON17.7.ProductionLootBaseline` a été relancé après modification et a retourné `Success` avec `ExpectedItemsPerKill=1.000`.

Les trois tirages restent indépendants : un Gobelin peut ne rien laisser, laisser un seul objet ou exceptionnellement plusieurs objets. Cela conserve le système de loot existant sans introduire de table exclusive ou de logique spécifique au Gobelin.

Le `DA_MON_GoblinThrower.uasset` équilibré est versionné sur `origin/master` depuis le commit :

```text
dbb3b14392f209c02b2ce3c220e28fcb821a777d
Last MON17.7 tests
```

## Contrat final validé

Le test suivant verrouille les décisions finales de MON17.7 :

```text
Grimrock.Monsters.MON17.7.FinalBalanceContract
```

Il charge les vrais DataAssets `DA_MON_GoblinThrower` et `DA_MON_RatGiant`, puis vérifie les statistiques finales du Gobelin, `ThrowKnife`, le ratio XP `4 x 125 = 500`, les trois entrées de loot et l'espérance finale `1.000`.

Validation UE5.5.4 :

```text
FinalBalanceContract  Success
MON17.7 final balance: HP=10 Initiative=12 Accuracy=2 Evasion=3 AP=3 Damage=2..5 XP=125 LootExpected=1.000
```

Référence :

```text
docs/Design/MON17_7_FINAL_BALANCE_CONTRACT.md
```

## Étape suivante

1. effectuer un seul PIE final de sensation de jeu avec les deux Gobelins de production ;
2. vérifier l'engagement, le comportement `RangedKeeper`, `ThrowKnife`, la victoire, `125 XP` par Gobelin et la lisibilité des drops probabilistes ;
3. rejouer MON17.1–MON17.7 et les régressions MON6/MON8/MON13/MON14/MON15 ciblées ;
4. documenter et clore MON17.

Le PIE final sert uniquement à vérifier la sensation de combat et la lisibilité du loot ; il ne remplace pas les contrats Automation.