# MON12.8.6 — Ciblage de cellule et zones d'effet

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/tests historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Migration clôturée. Les schémas et procédures de migration de cette page sont historiques.**


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

La prévisualisation lit directement `FGridCombatActionTargetingPreview`.
Chaque entrée de `AffectedCells` donne un marqueur à son emplacement réel,
quelle que soit la forme de la zone. Si un aperçu invalide ne fournit aucune
cellule affectée mais désigne une cellule du niveau, cette cellule seule est
affichée en rouge.

- jaune pour la cellule centrale valide ;
- vert pour les cellules couvertes par une zone valide ;
- rouge pour une cellule ou une zone invalide.

### COMBAT-TARGETING01 — Rendu en jeu

Dans UE 5.5.4, `EngineDefines.h` désactive `UE_ENABLE_DEBUG_DRAWING` en
Shipping sans éditeur. Dans cette configuration, `DrawDebugHelpers.h`
définit `DrawDebugBox` comme une fonction vide.

Le contrôleur réutilise un acteur de présentation local avec trois composants
de mesh instancié. Ils utilisent le plan `/Engine/BasicShapes/Plane` et le
matériau `M_CombatTargetingCell`, tous deux référencés en dur par les
composants natifs. Le matériau non éclairé compense l'exposition du donjon
pour préserver la couleur du contour et la transparence du fond. Le test de
profondeur permet aux murs et aux objets opaques de
masquer le marqueur. Le plan couvre 92 % de la largeur de la cellule et se
situe 8 cm au-dessus du plan de grille pour éviter le z-fighting avec le sol.
Les marqueurs ne participent ni aux collisions ni à la navigation.

La confirmation, l'annulation et la perte du survol vident les instances.
Le contrôle de présentation les retire également à la perte du HUD, du pawn
ou du niveau, et quand la visée d'un lancer physique prend la main. Les
composants restent disponibles pour le prochain ciblage.

Pour la recette graphique en PIE, en Standalone et dans le package Win64
Shipping, viser une cellule valide, une zone valide puis une cellule hors
portée. Contrôler les trois couleurs, la forme de la zone et le masquage
derrière un obstacle opaque. Vérifier que sortir du donjon avec le pointeur,
appuyer sur Échap, ouvrir l'inventaire ou confirmer une cible retire les
marqueurs. Le contrôle du Cook peut être répété avec `UnrealPak -ListContainer`
sur le fichier `GrimrockPrototype-Windows.utoc` du package.

Le parcours du catalyseur de surface a confirmé le jaune et le vert en PIE,
Standalone et Win64 Shipping sur D3D12. En Shipping, le survol déplace la zone
et Échap ou la confirmation retirent les marqueurs. Le plan et le matériau
figurent dans le conteneur IoStore du package.

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

`Grimrock.Monsters.MON12.8.6.TargetingPresentation` vérifie les cellules
projetées, les groupes de marqueurs, l'absence de collision, la réutilisation
du présentateur et le retrait lors de la perte du survol ou du pawn, de
l'annulation et de la confirmation.

## Suite

MON12.8 est désormais fonctionnel pour les raccourcis, objets, capacités,
sorts axiaux, cellules et zones. Le prochain jalon prévu est MON12.9 : défense,
réactions et effets de début/fin de tour.
