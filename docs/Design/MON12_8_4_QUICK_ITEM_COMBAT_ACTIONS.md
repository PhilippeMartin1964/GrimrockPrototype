# MON12.8.4 — Potions et parchemins dans la barre de combat

## Résultat

Une potion ou un parchemin configuré comme objet de combat rapide contribue
maintenant au catalogue du personnage et peut être exécuté depuis les dix
raccourcis MON12.8.

Le raccourci conserve l'identité stable suivante :

```text
Use_<ItemDefinitionId>
```

Il ne dépend jamais du `RuntimeObjectId` d'une pile. Le catalogue additionne
les quantités de toutes les piles de la même définition dans l'inventaire du
personnage actif.

## Configuration du Data Asset

Dans le `GridItemDefinitionAsset` d'une potion ou d'un parchemin :

1. conserver `ItemType` à `Potion` ou `Scroll` ;
2. activer `Provides Quick Item Combat Action` ;
3. renseigner `Quick Item Combat Action`.

`ActionId`, `SourcePolicy` et un coût source inférieur à un sont normalisés à
l'exécution :

- `ActionId` devient automatiquement `Use_<ItemDefinitionId>` ;
- `SourcePolicy` devient automatiquement `QuickItem` ;
- `SourceItemQuantityCost` vaut au minimum `1`.

Le nom, la description et l'icône utilisent ceux de l'item lorsque les champs
correspondants de l'action sont vides.

### Potion de soins ou de mana

Configuration type :

```text
ActionType                         Ability
TargetingPolicy                    Self
ResolutionProfile                 Effect
ActionPointCost                    1
ResourceCosts.ManaCost             0
EffectProfile.RestoreHealth        20
EffectProfile.RestoreMana          0
RangeCells                         0
```

Au moins une des deux restaurations doit être strictement positive. Une action
qui ne restaurerait actuellement ni PV ni mana est grisée et refusée sans
consommation.

### Parchemin offensif

Configuration type :

```text
ActionType                         RangedAttack
TargetingPolicy                    FirstAxialTarget
ResolutionProfile                 Attack
ActionPointCost                    2
RangeCells                         3
OffensiveProfile.AttackId          Attack_FireScroll
OffensiveProfile.RangeCells        3
OffensiveProfile.DamageScaling     None
```

Le `RangeCells` de l'action doit être identique à celui du profil offensif. Le
parchemin réutilise le ciblage axial, les obstacles, la résolution des dégâts
et les refus du pipeline d'attaque existant.

## Transaction autoritaire

Le HUD ne décrémente jamais une pile lui-même. Il reconstruit l'action puis le
TurnManager revalide :

- le combat, la phase et le personnage actif ;
- le repos du groupe ;
- les PA et le mana ;
- la quantité totale disponible ;
- le profil d'effet ou d'attaque ;
- la cible et la portée d'un parchemin offensif.

Une potion applique son effet et consomme exactement la quantité déclarée
seulement après acceptation. Un parchemin offensif n'est consommé qu'après une
attaque acceptée. Un refus ne dépense ni PA, ni mana, ni objet.

## Quantité zéro et remplacement de pile

Lorsque la dernière unité est consommée :

- le binding persistant n'est pas effacé ;
- le slot reste résolu grâce à la définition enregistrée ;
- il devient gris avec `InsufficientSourceItems` ;
- sa quantité affichée devient `x1/0`.

Ajouter ensuite une nouvelle pile de la même définition réactive
automatiquement le même raccourci, même si cette pile possède un autre
`RuntimeObjectId`.

## Widget Blueprint

Aucune modification `.uasset` n'est requise. Le coût d'un consommable est
affiché sous la forme `x<coût>/<quantité totale>` dans `Text_ActionCost` de
`WBP_GridCombatHudAction` lorsqu'il existe.

## Tests automatisés

Le filtre suivant couvre ce jalon :

```text
Grimrock.Monsters.MON12.8.4
```

Il vérifie :

1. restauration des PV et du mana, dépense exacte des PA et consommation
   d'une unité ;
2. refus d'une potion sans effet utile, sans aucune dépense ;
3. maintien du raccourci à quantité zéro ;
4. réactivation avec une nouvelle pile de même définition ;
5. refus d'un parchemin offensif sans cible, sans consommation ;
6. attaque axiale acceptée et consommation exacte d'un parchemin ;
7. rechargement de la définition référencée uniquement par un raccourci à
   quantité zéro.

## Suite

MON12.8.5 ajoute la palette explicite pour affecter sans objet source les
capacités de classe et les sorts appris, puis exécute les attaques axiales et
effets personnels compatibles. Voir
`MON12_8_5_CLASS_ACTION_PALETTE.md`.
