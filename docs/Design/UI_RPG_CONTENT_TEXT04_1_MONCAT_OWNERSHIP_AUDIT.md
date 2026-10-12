# UI-RPG-CONTENT-TEXT04.1 — Audit DA_MONCAT text ownership and runtime fallback paths

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-CONTENT-TEXT04.1**

## 1. Objet

Caractériser l'autorité texte actuelle des `UGridMonsterCategoryAsset`
(`DA_MONCAT_*`) sans modifier les Data Assets et sans invoquer le chemin
d'authoring qui sauvegarde des packages.

TEXT04.1 est un audit uniquement.

## 2. Modèle sérialisé

`UGridMonsterCategoryAsset` expose :

```text
CategoryId
DisplayName
Description
```

`CategoryId` est l'identité gameplay utilisée par les monstres, les filtres
de combat, les Skills et le Ranger.

`ValidateDefinition()` exige :

- `CategoryId` non vide ;
- `DisplayName` non vide.

`Description` peut être vide.

## 3. Production actuelle

Deux catégories de production existent actuellement dans le périmètre Ranger :

```text
DA_MONCAT_Goblin
    CategoryId = Goblin

DA_MONCAT_Vermin
    CategoryId = Vermin
```

Leurs `DisplayName` sont sérialisés dans les assets et sont l'autorité de
présentation consommée par le Ranger/read-model.

TEXT04.1 compte aussi les `Description` sérialisées sans imposer qu'elles
soient renseignées.

## 4. Writer de production

Un seul writer de production de `UGridMonsterCategoryAsset` a été identifié :

```text
RPGRangerAuthoring.cpp
    EnsureProductionMonsterCategoryAuthority()
        -> FindOrCreateCategoryAsset()
        -> Category->CategoryId = Monster->CategoryId
        -> Category->DisplayName = FText::FromString(Spec->DisplayName)
        -> SaveAuthoredAsset(Category)
```

Le writer utilise actuellement une table C++ :

```text
ProductionCategorySpecs[]
    Goblin -> Gobelins
    Vermin -> Vermine
```

Il existe donc encore une double autorité pour `DisplayName` :

```text
DA_MONCAT_*.DisplayName
        +
ProductionCategorySpecs[].DisplayName
```

En revanche, aucun writer de `Category->Description` n'a été trouvé dans ce
chemin. Une `Description` déjà sérialisée n'est donc pas actuellement
réécrite par l'authoring Ranger.

## 5. Pourquoi l'audit n'appelle pas AuthorProductionAssets()

`FRPGRangerAuthoring::AuthorProductionAssets()` appelle
`EnsureProductionMonsterCategoryAuthority()` puis sauvegarde les catégories
et, si nécessaire, les monstres qui les référencent.

Utiliser ce chemin dans un test d'audit modifierait potentiellement les
`.uasset`.

TEXT04.1 ne l'appelle donc pas. Le test :

- charge les deux `DA_MONCAT_*` en lecture ;
- vérifie leur résolution runtime ;
- utilise `CollectProductionFavoredEnemyCategories()`, qui est en lecture ;
- inspecte le source de l'authoring uniquement pour caractériser le chemin
  d'écriture existant.

Cette technique de caractérisation source est déjà utilisée ailleurs dans les
tests architecturaux du projet.

## 6. Runtime / read-model

`UGridMonsterCategoryAsset::ResolveByCategoryId(CategoryId)` :

```text
CategoryId valide
    -> scan/load du Primary Asset GridMonsterCategory
    -> retourne l'asset seulement s'il est valide et si son CategoryId correspond

sinon
    -> nullptr
```

`GridSkillsPageService::MonsterCategoryDisplayName()` applique :

```text
ResolveByCategoryId(CategoryId)
    -> Category->DisplayName

échec de résolution
    -> FText::GetEmpty()
```

Il n'existe donc actuellement **aucun fallback joueur vers CategoryId ou
HumanizeId dans ce chemin**. Une catégorie sans présentation canonique est
simplement omise des lignes de contexte du read-model.

L'ancienne documentation D04 indiquant `HumanizeId` comme fallback de
développement ne décrit plus exactement le code actuel.

## 7. Projection Ranger

`FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories()` parcourt les
monstres de production, exige une `CategoryDefinition` valide et cohérente,
puis projette :

```text
CategoryId
CategoryDefinition.DisplayName
```

La production échoue en cas de catégorie absente, invalide ou incohérente.

Le test `Grimrock.UI.RPG01.ProductionAssets.FavoredEnemyCategoryPresentation`
vérifie déjà que les variantes `Ennemi juré` utilisent le
`Category->DisplayName` sérialisé.

## 8. Dépendances de tests

`RPGRPG0393RangerAuthoringTests.cpp` contient encore deux libellés de fixture :

```text
Goblin -> Gobelins
Vermin -> Vermine
```

Ils servent à tester `ConfigureClass()` avec un jeu transient fourni
explicitement au helper. Ils ne lisent ni n'écrivent les `DA_MONCAT_*` et ne
constituent donc pas une autorité de production.

Le test de production dans ce même fichier utilise
`CollectProductionFavoredEnemyCategories()` et compare ensuite le texte
Ranger au `DisplayName` provenant des assets.

## 9. Test de caractérisation

TEXT04.1 introduit :

```text
Grimrock.UI.RPG.CONTENTTEXT04.MonsterCategoryOwnershipAudit
```

Il vérifie :

- les deux catégories de production actuelles ;
- chargement des deux `DA_MONCAT_*` ;
- `CategoryId` canonique ;
- validité structurelle ;
- `DisplayName` sérialisé non vide ;
- résolution par `ResolveByCategoryId()` ;
- projection Ranger depuis le même `DisplayName` ;
- absence de résolution pour une catégorie inconnue ;
- lecture du source Ranger/read-model sans sauvegarde de package.

Il journalise également :

```text
[UI-RPG-CONTENT-TEXT04.1][MONCAT]
[UI-RPG-CONTENT-TEXT04.1][AUTHORING]
[UI-RPG-CONTENT-TEXT04.1][RUNTIME]
[UI-RPG-CONTENT-TEXT04.1][SUMMARY]
```

Les compteurs d'overwrite sont informatifs afin que le même audit reste vert
lorsque TEXT04.2/04.3 supprimeront cette double autorité.

## 10. Suite logique

Le prochain ticket doit préserver le `DisplayName` sérialisé d'un
`DA_MONCAT_*` existant lorsque l'authoring Ranger rematérialise son identité
et ses liens mécaniques.

`Description` n'a pas besoin d'une restauration spécifique tant que le writer
ne la touche pas, mais elle doit rester explicitement sous autorité du Data
Asset.
