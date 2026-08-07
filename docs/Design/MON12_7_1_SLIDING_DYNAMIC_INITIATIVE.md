# MON12.7.1 — Chronologie d'initiative glissante et dynamique

## Objectif

La barre d'initiative représente désormais des **activations futures**, et non
la seule liste restante du round courant. Pendant un combat, elle projette un
nombre fixe de slots, configurable entre `7` et `10` et réglé à `8` par défaut.

La lecture se fait de gauche à droite :

- le premier slot est le combattant actif ;
- les slots suivants sont les prochaines activations autoritaires ;
- la projection continue automatiquement sur les rounds suivants ;
- un même combattant réapparaît donc lorsque sa prochaine activation appartient
  à un round ultérieur ;
- un séparateur `ROUND N` est inséré entre deux slots lorsque le numéro de round
  change. Le séparateur ne consomme jamais un slot.

## Autorité runtime

`UGridTurnManagerComponent::GetInitiativePreview()` produit des
`FGridInitiativePreviewEntry`. Chaque entrée contient :

- l'instantané `FGridCombatantInitiativeEntry` ;
- le numéro du round projeté ;
- l'index d'activation dans ce round ;
- l'indication du combattant actif ;
- l'indication qu'un séparateur doit précéder l'entrée.

Le HUD ne trie pas cette liste. Il la restitue telle quelle.

Les combattants `Defeated`, `Incapacitated` ou sans PV sont exclus de tous les
rounds projetés. Les combattants ayant déjà terminé leur tour sont exclus du
reste du round courant, mais réapparaissent au round suivant.

## Hâte et ralentissement

`SetCombatantInitiativeModifier()` applique un modificateur runtime absolu au
total d'initiative lancé au début de la rencontre :

```text
initiative effective = initiative du jet de rencontre + modificateur runtime
```

Le jet initial n'est jamais relancé. Lorsqu'un effet change le modificateur :

1. le combattant actif conserve son tour ;
2. les activations déjà terminées ne bougent pas ;
3. seules les entrées `Waiting` restantes sont retriées ;
4. la prévisualisation est diffusée immédiatement par
   `OnTurnOrderChanged` ;
5. le round suivant utilise l'ordre complet recalculé.

Un futur objet ou sort de hâte appelle cette API après l'application réussie de
son effet. La fin de l'effet remet son modificateur à la valeur appropriée,
généralement `0`. MON12.7.1 ne crée ni potion ni sort.

## HUD et pool fixe

`UGridCombatHudWidget::VisibleInitiativeSlotCount` règle la capacité visuelle :

```text
minimum = 7
défaut  = 8
maximum = 10
```

Le HUD crée le pool de slots une seule fois. Un rafraîchissement met à jour les
widgets existants et réordonne leurs références dans `Panel_Initiative`; il ne
recrée plus huit `UUserWidget` à chaque événement.

Les séparateurs sont des `UBorder` natifs mis en pool par le HUD. Ils portent le
texte `ROUND N`, s'insèrent avant le premier slot du nouveau round et ne
nécessitent aucun nouveau Widget Blueprint.

Chaque slot conserve le texte `PV actuel / maximum` et affiche en complément
une barre de progression rouge. Son remplissage correspond à :

```text
PV actuels / PV maximum, borné entre 0 et 1
```

Le TurnManager reste l'autorité des PV. Les dégâts, soins, changements d'état
et restaurations de partie déclenchent le rafraîchissement du HUD ; toutes les
occurrences futures du même combattant affichent donc le même état de santé.

L'ancien indicateur `Text_InitiativeOverflow` et son compteur C++ ont été
supprimés : la chronologie demande directement sa capacité configurée et la
prolonge sur les rounds suivants. Il n'existe donc plus d'activations cachées à
annoncer avec `+ N`.

## Configuration Unreal Editor

Dans `WBP_GridCombatHud`, vérifier uniquement :

| Propriété | Valeur attendue |
| --- | --- |
| `Panel_Initiative` | panneau horizontal vide dans le Designer |
| `Initiative Slot Widget Class` | `WBP_GridCombatHudInitiativeSlot` |
| `Visible Initiative Slot Count` | `8` |

Ne placer aucun séparateur ni slot manuellement dans `Panel_Initiative`.
Supprimer l'ancien `Text_InitiativeOverflow` du Widget Tree s'il est encore
présent dans `WBP_GridCombatHud`.

Dans `WBP_GridCombatHudInitiativeSlot`, la propriété facultative
`ProgressBar_Health` peut être liée à une `Progress Bar` placée sous le portrait.
Si elle est absente, le C++ crée automatiquement une barre rouge de `6 px` sous
`Text_Health` dans `VerticalBox_InitiativeTexts`, ce qui maintient la
compatibilité avec le Blueprint MON12.7.1 existant.

## Tests automatisés

Les tests MON12.7.1 vérifient :

- huit activations projetées sur plusieurs rounds ;
- l'actif toujours en première position ;
- la position et le numéro du séparateur de round ;
- la continuité avec `7` et `10` slots ;
- le retrait d'un vaincu de tous les rounds futurs ;
- le recalcul immédiat après hâte ;
- l'absence de modification rétroactive du tour actif ;
- le nouvel ordre complet au round suivant ;
- le refus d'un identifiant de combattant inconnu.
- le calcul, le bornage et la conservation du pourcentage de PV dans la vue HUD.

Suites ciblées :

```text
Grimrock.Monsters.MON12.CombatHUD.SlidingInitiative
Grimrock.Monsters.MON12.CombatHUD.DynamicInitiative
```

## Validation PIE

1. Démarrer un combat avec moins de huit participants.
2. Vérifier que huit portraits sont néanmoins visibles grâce à la projection
   sur les rounds suivants.
3. Vérifier que le premier portrait est l'actif.
4. Vérifier qu'un séparateur `ROUND 2` apparaît exactement entre les deux
   rounds et ne remplace aucun portrait.
5. Terminer plusieurs tours et vérifier le glissement d'un slot à chaque fois.
6. Tuer un monstre et vérifier sa disparition de toutes les occurrences
   futures.
7. Blesser un combattant et vérifier que son texte de PV et sa barre rouge se
   mettent à jour simultanément dans toutes ses occurrences visibles.
8. Lorsqu'un premier effet de hâte sera branché, vérifier que seules les
   activations futures se déplacent et que l'actif ne change jamais en cours de
   tour.
