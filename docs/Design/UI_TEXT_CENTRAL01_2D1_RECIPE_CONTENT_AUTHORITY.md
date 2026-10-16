# UI-TEXT-CENTRAL01.2D1 — Recipe content authority

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **SOURCE IMPLÉMENTÉE — matérialisation locale requise**

## Objectif

Sortir les quinze noms joueur de recettes Alchimiste du TextSet Talent legacy
avant sa suppression physique.

La nouvelle autorité est :

~~~text
Recipe_* gameplay identity
    -> URPGRecipeCatalogAsset
    -> DA_RPGRecipeCatalog
    -> FText DisplayName
~~~

Chemin de production :

~~~text
/Game/GrimrockPrototype/Core/DataAssets/RPG/Crafting/DA_RPGRecipeCatalog
~~~

Les noms de recettes restent du **contenu métier**. Ils ne sont pas déplacés
dans UI.csv / ST_UI.

## Migration contrôlée

Le commandlet :

~~~text
RPGRecipeCatalogAuthoring
~~~

lit temporairement les quinze anciennes entrées `Talent.Recipe.*` depuis
`DA_RPGTalentDetailTextSet`, extrait leur vraie identité `Recipe_*`, puis
crée ou synchronise `DA_RPGRecipeCatalog`.

Il exige exactement quinze recettes, des identités uniques et valides, et il
est idempotent. Le TextSet legacy reste présent uniquement comme source de
migration/parité jusqu'à UI-TEXT-CENTRAL01.2D2.

## Runtime

`FRPGTalentLocalizationText` n'accède plus à
`URPGTalentDetailTextSet::ResolveProductionText()`.

Pendant cette sous-étape uniquement, les anciens
`Talent.Recipe.Recipe_*` produits par le read-model sont acceptés comme
identités de compatibilité :

~~~text
Talent.Recipe.Recipe_Bomb_Fire
    -> Recipe_Bomb_Fire
    -> DA_RPGRecipeCatalog
    -> "Bombe incendiaire"
~~~

UI-TEXT-CENTRAL01.2D2 supprimera cette compatibilité et fera porter
`RecipeId` directement par le read-model.

## Tests

~~~text
Game.UI.TextCentral.CENTRAL01.RecipeCatalogModel
Game.UI.TextCentral.CENTRAL01.RecipeCatalogProduction
Game.UI.TextCentral.CENTRAL01.TalentRuntimeResolver
Grimrock.UI.RPG.DESC01
~~~

Le test de production D1 compare temporairement le nouveau catalogue avec les
quinze noms du TextSet legacy. Ce test de parité sera remplacé lors de 01.2D2,
en même temps que la suppression de l'ancien asset et de sa classe C++.
