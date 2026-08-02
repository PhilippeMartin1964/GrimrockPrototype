# MON12.6 — Définitions et catalogue d'actions

## Résultat

MON12.6 remplace l'identité provisoire « bouton de main » par une action de
combat provenant d'une source concrète.

Le catalogue commun peut désormais agréger :

- plusieurs actions fournies par une arme équipée ;
- les capacités fournies par la classe ;
- les sorts fournis par la classe ;
- l'attaque universelle à mains nues ;
- les anciennes armes MON11 non encore migrées.

La construction du catalogue est une lecture sans effet de bord : elle ne
résout aucun dégât et ne consomme ni PA, ni mana, ni objet.

## Définition orientée données

`FGridCombatActionDefinition` contient :

- `ActionId`, `DisplayName`, `Description` et `Icon` ;
- `ActionType` ;
- `SourcePolicy` : `Universal`, `Equipment`, `Ability`, `Spell` ou
  `QuickItem` ;
- `TargetingPolicy` ;
- `ResolutionProfile` ;
- `ActionPointCost` et `ResourceCosts` ;
- `RangeCells`, `Requirements` et `CooldownRounds` ;
- `PresentationProfileId` ;
- le premier payload exécutable, `OffensiveProfile`, pour une attaque.

`UGridItemDefinitionAsset::CombatActions` permet à une arme de fournir
plusieurs actions distinctes. Une épée pourra ainsi déclarer `Coup tranchant`
et `Estoc` avec des coûts, profils d'attaque et présentations différents.

`URPGClassAsset::CombatActions` constitue la source commune des capacités et
des sorts de classe. Le personnage conserve une référence souple vers sa
définition de classe dans `FGridCharacterInventoryState::ClassDefinition`.

## Contribution concrète

`FGridCombatActionContribution` associe la définition abstraite à sa source
runtime :

- identifiant de l'item ou de la classe ;
- identifiant runtime de l'item ;
- slot d'équipement réel ;
- quantité actuellement disponible.

Le slot `MainHand` ou `OffHand` reste donc une provenance. Il n'est plus
l'identité de l'action.

## Catalogue disponible

`FGridCombatActionCatalog::Build()` transforme les contributions en
`FGridAvailableCombatAction` à partir d'un contexte immuable.

Chaque résultat ajoute :

- l'index et l'identifiant du personnage ;
- la source concrète ;
- les coûts courants en PA, mana et quantité d'item ;
- `bEnabled` ;
- `AvailabilityReason` et un texte localisable ;
- la première cible axiale suggérée lorsqu'elle existe.

Les vérifications actuelles couvrent :

- combat actif ;
- personnage vivant et actif ;
- groupe au repos ;
- PA et mana suffisants ;
- quantité de la source ;
- prérequis ;
- cooldown ;
- disponibilité de l'exécuteur correspondant.

## Autorité et exécution

Le HUD appelle :

```text
GetAvailableCombatActions(CharacterIndex)
```

Puis il transmet l'identité stable de l'action et sa provenance à :

```text
RequestCharacterCombatAction(
    CharacterIndex,
    ActionId,
    SourcePolicy,
    SourceDefinitionId,
    SourceEquipmentSlot)
```

Le `UGridTurnManagerComponent` reconstruit le catalogue avant chaque requête.
Un ancien instantané ne peut donc pas contourner un changement de tour,
d'équipement ou de ressources.

Une action refusée ne modifie aucune ressource. Une attaque acceptée emprunte
ensuite le pipeline MON11 existant : ciblage axial, résolution, PA, journal,
présentation, projectile éventuel, dégâts et victoire.

## Compatibilité MON11

Lorsque `UGridItemDefinitionAsset::CombatActions` est vide et que l'ancien
`OffensiveProfile` est valide, le catalogue construit automatiquement une
définition de compatibilité :

- `ActionId` reprend `AttackId` ;
- le coût reprend `PlayerAttackActionPointCost` ;
- l'icône, le nom, la portée et le profil offensif restent ceux de l'item ;
- un item `bThrowable` annonce un coût d'une unité de source ;
- la présentation MON11.4.2 reste résolue par l'item existant.

Le shuriken et les armes déjà créées ne nécessitent donc aucune modification
de DataAsset pour valider MON12.6.

Une torche sans `CombatActions` et sans profil offensif ne contribue aucune
action. Si aucun équipement offensif n'est disponible, le catalogue ajoute
une action universelle `Attack_Unarmed`.

## Interface transitoire

`FGridCombatActionPanelView::AvailableActions` expose déjà l'instantané au
widget actuel. Les deux gros boutons de mains restent toutefois en place
jusqu'à MON12.7, qui les remplacera par la barre d'actions.

Le rafraîchissement reste événementiel. Aucun `Tick` de catalogue ou de widget
n'est ajouté.

## Limites volontaires

MON12.6 exécute uniquement les actions dont `ResolutionProfile` vaut
`Attack` et dont la source est `Equipment` ou `Universal`.

Les capacités, sorts et objets rapides sont déjà catalogués. Quand leurs
ressources sont suffisantes, ils portent provisoirement la raison
`ExecutionNotImplemented` :

- sorts, mana, cellules et zones : MON12.8 ;
- défense, objets rapides et réactions : MON12.9.

## Tests automatisés

```text
Grimrock.Monsters.MON12.ActionCatalog.Contributions
Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle
```

Ils vérifient :

- deux actions provenant d'une même arme ;
- la provenance réelle `MainHand` ;
- l'absence d'action fournie par une torche ;
- la contribution d'une capacité et d'un sort ;
- le refus du sort sans mana ;
- l'adaptation automatique d'une arme MON11 ;
- l'exécution de cette attaque par la requête générique ;
- la dépense exacte de PA ;
- la revalidation et le refus sans mutation lorsque les PA sont devenus
  insuffisants.

## Validation PIE

1. conserver les DataAssets d'armes existants sans les modifier ;
2. démarrer un combat et attendre le tour d'un personnage ;
3. appeler `LogSelectedCharacterAvailableCombatActions` sur le TurnManager ;
4. filtrer l'Output Log sur `GridActionCatalog` ;
5. vérifier que le shuriken expose son `AttackId`, `MainHand`, son coût en PA,
   sa portée et la cible axiale ;
6. vérifier qu'une torche n'ajoute aucune ligne ;
7. demander l'action générique et contrôler les lignes
   `Accepted=true` de `GridActionCatalog` puis `GridPlayerAttack` ;
8. contrôler la non-régression des PA, du projectile, de la quantité, des
   dégâts et de la victoire.
