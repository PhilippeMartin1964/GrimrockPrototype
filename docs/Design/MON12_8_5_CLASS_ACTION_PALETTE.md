# MON12.8.5 — Palette des capacités et sorts de classe

## Résultat

Le HUD peut maintenant afficher les actions sans objet source du personnage
actif dans une palette distincte de la barre fixe. Une entrée `Universal`,
`Ability` ou `Spell` se glisse vers l'un des dix raccourcis, puis le binding
personnel existant conserve uniquement son identité stable.

```text
ActionId + SourcePolicy + ClassId
```

Aucun objet fictif, identifiant runtime ou emplacement d'équipement n'est
créé pour une capacité ou un sort.

## Configuration de la classe

Dans le `URPGClassAsset` du personnage, ajouter chaque capacité apprise ou
sort disponible à `Combat Actions`.

Une action de classe doit respecter les règles suivantes :

- `SourcePolicy` vaut `Ability`, `Spell` ou `Universal` ;
- `SourceItemQuantityCost` vaut toujours `0` ;
- une action `Universal` ne porte pas de coût en mana ;
- chaque `ActionId` est unique dans la classe ;
- les coûts en PA et mana restent déclarés dans la définition commune.

### Sort direct axial

Configuration type :

```text
ActionId                           Spell_ArcaneBolt
ActionType                         RangedAttack
SourcePolicy                       Spell
TargetingPolicy                    FirstAxialTarget
ResolutionProfile                 Attack
ActionPointCost                    2
ResourceCosts.ManaCost             3
ResourceCosts.SourceItemQuantityCost 0
RangeCells                         4
OffensiveProfile.AttackId          Attack_ArcaneBolt
OffensiveProfile.RangeCells        4
```

Le profil offensif utilise le pipeline d'attaque existant : ciblage axial,
portes et murs bloquants, premier monstre rencontré, résistances, armures,
dégâts, journal, présentation et victoire.

### Capacité personnelle immédiate

Configuration type :

```text
ActionId                           Ability_Recovery
ActionType                         Ability
SourcePolicy                       Ability
TargetingPolicy                    Self
ResolutionProfile                 Effect
ActionPointCost                    1
ResourceCosts.ManaCost             2
EffectProfile.RestoreHealth        6
EffectProfile.RestoreMana          0
RangeCells                         0
```

MON12.8.5 limite volontairement le profil `Effect` aux restaurations
personnelles immédiates déjà représentées par `EffectProfile`. Les buffs,
altérations, cellules choisies et zones demandent leurs propres modèles.

## Transaction autoritaire

Le HUD ne paie aucune ressource. Avant chaque exécution, le TurnManager
reconstruit l'action depuis la classe et revalide le personnage actif, la
phase, les PA, le mana, les prérequis et le profil de résolution.

Pour un sort direct, le mana est réservé avant les événements de présentation.
Si le ciblage ou l'attaque est refusé, il est immédiatement restauré et aucun
PA n'est dépensé. Une attaque acceptée conserve exactement le coût en mana et
le pipeline d'attaque dépense exactement les PA déclarés.

Pour un effet personnel, les PA et le mana sont payés uniquement lorsque
l'effet améliore réellement les PV ou le mana. Une capacité inutile sur un
personnage déjà au maximum est grisée et refusée sans dépense.

## Widget Blueprint

Modifier `WBP_GridCombatHud` :

1. ajouter un `Wrap Box` au-dessus de `Panel_Actions` ;
2. le nommer exactement `Panel_ActionPalette` ;
3. cocher `Is Variable` ;
4. le laisser vide dans le Designer ;
5. conserver `Action Widget Class = WBP_GridCombatHudAction` ;
6. compiler et sauvegarder le WBP.

Le C++ crée une entrée par action compatible. Il réutilise
`WBP_GridCombatHudAction`, masque le numéro de raccourci pour la palette et
garde les coûts, l'icône, le tooltip et l'état disponible/indisponible.

Sans `Panel_ActionPalette`, la barre et l'exécution continuent de fonctionner,
mais aucune source visuelle ne permet d'affecter une capacité ou un sort.

## Tests automatisés

Le filtre suivant couvre ce jalon :

```text
Grimrock.Monsters.MON12.8.5
```

Il vérifie :

1. projection de deux actions de classe dans la palette ;
2. création des widgets de palette dans un `WrapBox` ;
3. drag d'un sort vers un raccourci sans source objet ;
4. conservation de `ActionId`, `Spell` et `ClassId` dans le binding ;
5. refus d'un sort direct sans cible, sans dépense de PA ni mana ;
6. attaque axiale acceptée avec coût exact de 2 PA et 3 mana ;
7. effet personnel accepté avec restauration et coûts exacts ;
8. refus sans dépense lorsque l'effet personnel est inutile.

## Suite réalisée

MON12.8.6 ajoute la sélection explicite d'une cellule et les zones d'effet.
Les profils `Cell` et `Area` sont désormais exécutables par le HUD et le
contrôleur joueur. Voir `MON12_8_6_CELL_AREA_TARGETING.md`.
