# MON17.7 — Balance / Closure — Audit initial

Statut : **EN COURS — baseline automatisée validée sous UE5.5.4**
Date : **21 août 2026**

## Objectif

MON17.7 doit figer une baseline cohérente pour le Gobelin lanceur, vérifier son pacing face au Rat Géant, ajuster uniquement les données qui le nécessitent, exécuter les régressions finales puis clore MON17.

Cette étape n'introduit ni nouveau système de combat, ni nouveau profil IA, ni refactor du loot ou de l'XP.

## Baseline de production mesurée

Le test charge le vrai `DA_MON_GoblinThrower` et produit :

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

## Pacing XP observé

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

Ce décalage est réel mais faible à cette échelle. La première décision MON17.7 devra soit l'accepter explicitement, soit choisir une récompense divisible par quatre ; aucun changement d'asset n'est justifié avant une mesure de pacing en rencontre réelle.

## Baseline loot observée

Les trois entrées de production sont valides :

```text
GoblinKnife  Chance=1.0 Quantity=1
Stone        Chance=1.0 Quantity=1
EmptyVial    Chance=1.0 Quantity=1
```

L'espérance actuelle est donc exactement :

```text
3.0 objets par Gobelin
6.0 objets pour l'encounter de production à deux Gobelins
```

Cette fréquence est adaptée à une validation déterministe mais probablement trop généreuse pour le vertical slice. La prochaine mesure doit cibler le loot en premier, avec une seule famille de paramètres modifiée à la fois.

## Première étape implémentée

La suite suivante caractérise les vrais assets sans modifier le runtime ni aucun `.uasset` :

```text
Grimrock.Monsters.MON17.7.ProductionBalanceBaseline  Success
Grimrock.Monsters.MON17.7.ProductionLootBaseline     Success
Grimrock.Monsters.MON17.7.RewardPacingBaseline       Success

MON17.7 baseline                                     3/3 Success
```

Elle verrouille les contrats déjà validés et journalise les probabilités de loot afin que toute décision de balance future soit explicite et relisible.

## Étape suivante recommandée

1. exécuter au moins dix combats courts avec un et deux Gobelins ;
2. relever manches avant victoire, dégâts reçus, attaques réussies, durée et encombrement du loot ;
3. décider si `ExperienceReward=125` est conservé ;
4. réduire en priorité les chances de loot pour viser environ `0.8..1.2` objet attendu par Gobelin ;
5. ne modifier que `DA_MON_GoblinThrower` si les mesures confirment ce besoin ;
6. rejouer MON17.1–MON17.7 et les régressions MON6/MON8/MON13/MON14/MON15 ciblées ;
7. documenter la clôture de MON17.

La plage `0.8..1.2` est une cible de départ pour le test, pas une valeur finale déjà décidée.
