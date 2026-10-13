# UI-LOCALIZATION01.9A — Combat HUD player-facing text migration

Date : **13 octobre 2026**

## Objet

Migrer le wording joueur brut du Combat HUD vers l'autorité UI commune
`Localization/Source/UI.csv`, sans déplacer la mécanique de combat, les
calculs de coût, l'initiative ni les règles de ciblage hors du C++.

## Périmètre

Trois sources identifiées P0 par `UI-LOCALIZATION01.2` sont migrées :

- `GridCombatHudWidget.cpp` ;
- `GridCombatActionPanelWidget.cpp` ;
- `GridTurnManagerPlayerActionCatalog.cpp`.

Elles consomment exactement 47 clés `Combat.*`.

Le périmètre couvre notamment :

- état ACTIF / INCAPACITÉ / VAINCU ;
- PA, PAM, PV et coûts mana avec formats nommés ;
- tooltip des raccourcis ;
- action physique « Lancer {ItemName} » ;
- raisons d'indisponibilité ;
- fin de tour ;
- séparateur de manche ;
- raisons de ciblage cellule / zone visibles au joueur.

## Frontières conservées

Restent en C++ comme notation mécanique :

- `xN` ;
- `current / max` sans libellé humain ;
- raccourcis clavier `1..0, ', ^` ;
- identifiants techniques utilisés uniquement en fallback.

Les noms/descriptions d'actions et d'items restent authorés dans leurs Data
Assets. Les textes déjà en `LOCTEXT/NSLOCTEXT` ne sont pas déplacés.

## FText

Le tooltip de raccourci assigné ne fait plus :

~~~text
FText -> FString -> concaténation -> FText
~~~

Il conserve les noms/descriptions authorés et les sections localisées en
`FText`, utilise `FText::FormatNamed`, puis `FText::Join`.

## Ciblage runtime

Le catalogue joueur possédait déjà des champs `FText DisabledReason` et
`FText InvalidReason`. Le ticket ne crée pas une nouvelle couche
architecturale : les phrases brutes sont remplacées par des identités
`Combat.Targeting.*` résolues par l'autorité UI commune.

Une migration future vers enum/reason stable n'est justifiée que si elle
réduit réellement des autorités ou devient nécessaire à d'autres
consommateurs.

## Test

Le garde-fou :

~~~text
Game.UI.Localization.LOCALIZATION01.CombatPresentationCsvCoverage
~~~

vérifie :

- présence des 47 clés `Combat.*` dans `UI.csv` ;
- référencement exact de ces 47 clés par les trois sources migrées ;
- disparition des principaux anciens littéraux joueur ;
- composition du tooltip en `FText` ;
- couverture du fixture anglais et du smoke runtime.

## Smoke anglais

Le fixture anglais couvre des représentants de chaque famille :

- `Combat.State.Active` -> `ACTIVE` ;
- `Combat.EndTurn.EnemyTurn` -> `Enemy turn` ;
- `Combat.Hotbar.EmptyTooltip` ;
- `Combat.Hotbar.Tooltip.Cost` ;
- `Combat.Initiative.Round` -> `ROUND {Round}` ;
- `Combat.Resource.ActionPoints` -> `AP {Current} / {Maximum}` ;
- `Combat.Targeting.Invalid.OutOfRange` ;
- blocage des sorts par Status Effect.

## 01.9B

Après validation source :

1. régénérer `ST_UI.uasset` ;
2. Gather / Export / Import / Compile ;
3. valider le smoke fr/en ;
4. exécuter les régressions Combat HUD et Status Control ;
5. versionner uniquement les projections générées modifiées.
