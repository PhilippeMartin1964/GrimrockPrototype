# UI-LOCALIZATION01.7A — Status Effect presentation migration

> **Statut : CLOSED.** Migration : `6b8299cf3d85c86702a906016ff672d979a91c1b`, correctifs jusqu'à `042acb2f7af4182d5b32dd1bf018b63423a03aa6`. Matérialisation 01.7B : `08f6f54aa2c653ea9ca56d94469b41d9bacfab7c`.


Date : **13 octobre 2026**

## Objet

Migrer le vocabulaire joueur générique de la présentation des Status Effects
vers l'autorité UI commune :

~~~text
Localization/Source/UI.csv
    -> ST_UI
    -> Game localization target
~~~

Les noms, descriptions, icônes et mécaniques des statuts restent la
responsabilité des DA_Status_* existants.

## Périmètre migré

`GridStatusEffectPresentation.cpp` ne possède plus les chaînes joueur :

~~~text
Durée
Stacks
Initiative
Dégâts périodiques actifs
activation perdue
Sort bloqué
déplacement bloqué
Tn / Rn / PERM
tour(s) / manche(s) / Permanent
~~~

Ces textes utilisent désormais des clés `Status.*` stables dans `UI.csv`.

## Composition FText

Le builder ne reconstruit plus le tooltip via :

~~~text
FText -> FString -> FString::Join -> FText
~~~

Les lignes restent des `FText` et sont assemblées avec `FText::Join`.
Les valeurs dynamiques utilisent `FText::FormatNamed`.

Les pluriels de tours/manches sont authorés dans le catalogue :

~~~text
{Count} {Count}|plural(one=tour,other=tours)
{Count} {Count}|plural(one=manche,other=manches)
~~~

Le nombre est passé au formatter comme valeur numérique, afin que le
modificateur plural dispose bien d'un argument numérique.

## Garde-fou

Le test :

~~~text
Game.UI.Localization.LOCALIZATION01.StatusEffectPresentationCsvCoverage
~~~

vérifie :

- présence des 16 clés `Status.*` attendues ;
- couverture exacte des clés référencées par le builder ;
- disparition des anciens littéraux joueur ;
- conservation du tooltip en `FText`.

## Smoke anglais

Le fixture anglais couvre maintenant aussi :

~~~text
Status.Tooltip.Duration
Status.Control.BlockTranslation
Status.Tooltip.PeriodicDamage
Status.Duration.Turns
Status.Duration.Rounds
~~~

Le commandlet vérifie en plus le résultat formaté de deux manches :

~~~text
fr -> 2 manches
en -> 2 rounds
~~~

## 01.7B

01.7A ne modifie aucun artefact généré.

Après validation source :

1. régénérer `ST_UI.uasset` ;
2. Gather / Export / Import / Compile via `RunLocalizationSmoke.ps1` ;
3. exécuter les régressions MON16.6 ;
4. versionner séparément les projections générées modifiées.
