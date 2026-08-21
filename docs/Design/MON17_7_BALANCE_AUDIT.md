# MON17.7 — Balance / Closure — Audit initial

Statut : **EN COURS — baseline automatisée implémentée, validation UE5.5.4 à faire**
Date : **21 août 2026**

## Objectif

MON17.7 doit figer une baseline cohérente pour le Gobelin lanceur, vérifier son pacing face au Rat Géant, ajuster uniquement les données qui le nécessitent, exécuter les régressions finales puis clore MON17.

Cette étape n'introduit ni nouveau système de combat, ni nouveau profil IA, ni refactor du loot ou de l'XP.

## Baseline de production attendue

Le test charge le vrai `DA_MON_GoblinThrower` et verrouille les valeurs actuellement authorées :

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

Le contrat tactique attendu reste cohérent : `RangedKeeper`, distance préférée `3..5`, `ThrowKnife` à portée `2..6`, mouvement `1 AP` puis attaque `2 AP`.

Ces valeurs ne seront considérées comme baseline validée qu'après exécution de `Grimrock.Monsters.MON17.7` sous UE5.5.4.

## Pacing XP calculé

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

Ce décalage est faible à cette échelle. MON17.7 doit décider explicitement si `ExperienceReward=125` reste la valeur de production ; aucun changement d'asset n'est justifié avant validation de la baseline.

## Baseline loot configurée

Les trois entrées de production actuellement authorées sont :

```text
GoblinKnife  Chance=1.0 Quantity=1
Stone        Chance=1.0 Quantity=1
EmptyVial    Chance=1.0 Quantity=1
```

L'espérance configurée est donc :

```text
3.0 objets par Gobelin
6.0 objets pour l'encounter de production à deux Gobelins
```

Cette fréquence était volontaire pour rendre MON17.6 déterministe. Elle est très probablement trop généreuse comme valeur de production finale ; MON17.7 doit la réduire après validation du contrat des trois vrais DataAssets.

## Première étape implémentée

La suite suivante caractérise les vrais assets sans modifier le runtime ni aucun `.uasset` :

```text
Grimrock.Monsters.MON17.7.ProductionBalanceBaseline  À VALIDER
Grimrock.Monsters.MON17.7.ProductionLootBaseline     À VALIDER
Grimrock.Monsters.MON17.7.RewardPacingBaseline       À VALIDER

MON17.7 baseline                                     0/3 validés
```

Elle verrouille les contrats déjà acquis et journalise les probabilités de loot afin que toute décision de balance soit explicite et relisible.

## Étape suivante

1. compiler puis exécuter `Grimrock.Monsters.MON17.7` ;
2. relever la baseline réellement chargée depuis les DataAssets de production ;
3. conserver `ExperienceReward=125` si le ratio avec le Rat Géant et le pacing MON15 sont confirmés ;
4. réduire en priorité le loot pour viser environ `0.8..1.2` objet attendu par Gobelin ;
5. modifier une seule famille de paramètres à la fois, idéalement uniquement `DA_MON_GoblinThrower.LootTable` ;
6. compléter la caractérisation automatisée du niveau de pression offensif avant tout autre ajustement de statistiques ;
7. effectuer ensuite un seul PIE final de sensation de jeu ;
8. rejouer MON17.1–MON17.7 et les régressions MON6/MON8/MON13/MON14/MON15 ciblées ;
9. documenter la clôture de MON17.

La plage `0.8..1.2` objet attendu par Gobelin est une cible de départ, pas une valeur finale déjà décidée.
