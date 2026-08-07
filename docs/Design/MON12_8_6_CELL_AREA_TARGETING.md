# MON12.8.6 — Ciblage de cellule et zones d'effet

## Résultat

Les actions de combat dont `TargetingPolicy` vaut `Cell` ou `Area` ne sont
plus exécutées comme des attaques axiales. Un clic sur leur raccourci ouvre un
mode de ciblage explicite :

1. survoler une cellule du donjon ;
2. contrôler la prévisualisation ;
3. cliquer pour confirmer ;
4. presser `Échap` pour annuler.

L'ouverture, le survol et l'annulation ne dépensent aucune ressource. Le
`TurnManager` reconstruit l'action et la cible au moment de la confirmation,
puis paie les PA, le mana et l'éventuel parchemin une seule fois seulement si
au moins une cible valide peut être résolue.

## Configuration d'une action sur cellule

Exemple de sort qui frappe l'ennemi occupant exactement la cellule choisie :

```text
ActionId                           Spell_ArcaneCellStrike
ActionType                         RangedAttack
SourcePolicy                       Spell
TargetingPolicy                    Cell
ResolutionProfile                 Attack
ActionPointCost                    2
ResourceCosts.ManaCost             3
RangeCells                         4
AreaRadiusCells                    0
OffensiveProfile.AttackId          Attack_ArcaneCellStrike
OffensiveProfile.RangeCells        4
```

`RangeCells` est la distance de grille maximale entre le groupe et la cellule
sélectionnée. Une cellule vide, hors niveau, hors portée ou occupée uniquement
par une entité étrangère au combat est refusée sans dépense.

## Configuration d'une zone

Exemple de sort à zone en diamant :

```text
ActionId                           Spell_ArcaneBurst
ActionType                         RangedAttack
SourcePolicy                       Spell
TargetingPolicy                    Area
ResolutionProfile                 Attack
ActionPointCost                    2
ResourceCosts.ManaCost             4
RangeCells                         4
AreaRadiusCells                    1
OffensiveProfile.AttackId          Attack_ArcaneBurst
OffensiveProfile.RangeCells        4
```

`AreaRadiusCells` est une distance de Manhattan autour du centre :

- `1` couvre le centre et ses quatre voisins cardinaux ;
- `2` étend le diamant à deux cellules ;
- seules les cellules existantes et non `Empty` sont conservées.

Tous les monstres vivants de la rencontre présents dans ce diamant reçoivent
une résolution d'attaque distincte. Les PA, le mana et la quantité de l'objet
source restent payés une seule fois pour l'action entière.

## Interaction et prévisualisation

`AGrimrockPlayerController` donne la priorité au ciblage sur les interactions
ordinaires avec les boutons, portes, réceptacles et objets. Le survol convertit
le point d'impact en cellule via le runtime du niveau.

La prévisualisation native utilise :

- jaune pour la cellule centrale valide ;
- vert pour les cellules couvertes par une zone valide ;
- rouge pour une cellule ou une zone invalide.

## Widget Blueprint optionnel

Le ciblage fonctionne sans modification de `WBP_GridCombatHud`. Pour afficher
également les instructions textuelles dans le HUD, ajouter facultativement :

```text
Panel_Targeting
Text_TargetingInstructions
Text_TargetingCell
```

Ces trois widgets doivent être nommés exactement ainsi et cochés `Is Variable`.
`Panel_Targeting` peut être un `Vertical Box`, un `Border` ou tout autre widget
de présentation. Le C++ le masque hors ciblage et alimente les deux textes.

## Contrat autoritaire

Le HUD ne conserve qu'une identité de raccourci temporaire. À chaque survol et
à la confirmation, `BuildCombatActionTargetingPreview()` retrouve l'action
dans le catalogue courant et revalide :

- personnage et tour actifs ;
- PA et mana disponibles ;
- quantité de l'objet source ;
- profil `Attack` et politique `Cell` ou `Area` ;
- cellule de niveau valide et portée ;
- monstres vivants appartenant à la rencontre.

`RequestCharacterCombatActionAtCell()` reconstruit encore cette prévisualisation
avant de muter l'état. Les résultats multiples sont exposés dans
`FGridCombatTargetedActionResult` avec la cellule centrale, les cellules
couvertes, les identifiants de monstres, les requêtes et les résultats de
chaque attaque.

## Tests automatisés

Filtre :

```text
Grimrock.Monsters.MON12.8.6
```

Les tests vérifient :

1. ouverture du ciblage sans dépense ;
2. cellule vide refusée et annulation sans dépense ;
3. attaque sur une cellule occupée ;
4. zone de rayon `1` couvrant deux monstres ;
5. une résolution par cible mais un paiement unique ;
6. refus `TargetRequired` lorsque l'ancien point d'entrée est appelé sans
   cellule ;
7. refus `InvalidTarget` sans dépense pour une zone vide.

## Suite

MON12.8 est désormais fonctionnel pour les raccourcis, objets, capacités,
sorts axiaux, cellules et zones. Le prochain jalon prévu est MON12.9 : défense,
réactions et effets de début/fin de tour.
