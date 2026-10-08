# RPG-TALENT-FIX07 — D04 Ennemi juré / présentation des catégories

Date : **8 octobre 2026**  
État : **SOURCE PRÊTE — matérialisation / validation locale requise**

## Problème

`Ennemi juré` génère correctement une variante exclusive par `CategoryId`
présente dans le bestiaire de production, mais son titre était construit depuis
l'identifiant brut :

```text
Ennemi juré — <CategoryId>
```

Le read-model réutilisait aussi `HumanizeId(CategoryId)` dans les effets.

## Autorité retenue

Une catégorie de créature est une entité partagée par plusieurs monstres et par
plusieurs systèmes (combat, Skills, Talents, futur Codex). Son nom joueur ne doit
donc appartenir ni au Ranger, ni à un widget, ni à un monstre particulier.

Nouvelle autorité :

```text
UGridMonsterCategoryAsset
    CategoryId
    DisplayName
    Description
```

`CategoryId` reste l'identité gameplay.

`UGridMonsterDefinitionAsset` conserve son `CategoryId` et référence
`CategoryDefinition`. Les assets de production utilisés par Ennemi juré doivent
posséder cette référence et l'identifiant doit correspondre.

## Production actuelle

```text
DA_MONCAT_Goblin
    CategoryId  = Goblin
    DisplayName = Gobelins

DA_MONCAT_Vermin
    CategoryId  = Vermin
    DisplayName = Vermine
```

Les deux monstres de production actuels sont raccordés :

```text
DA_MON_GoblinThrower -> DA_MONCAT_Goblin
DA_MON_RatGiant      -> DA_MONCAT_Vermin
```

## Authoring Ranger

La liste des variantes reste **dynamique** :

```text
monstres réellement présents
-> CategoryId uniques
-> CategoryDefinition
-> Ennemi juré — CategoryDefinition.DisplayName
```

L'authoring échoue si :

- une catégorie de production n'a pas de présentation canonique ;
- la référence de catégorie est invalide ;
- le `CategoryId` du monstre et celui de la catégorie divergent.

Il n'existe aucune table de noms dans l'UMG.

## Read-model

`FGridSkillsPageService` résout les catégories avec
`UGridMonsterCategoryAsset::ResolveByCategoryId()`.

Cela couvre :

- le titre des variantes matérialisé dans `DA_Class_Ranger` ;
- les contextes de `CombatModifiers` ;
- les catégories liées aux `SkillModifiers`.

`HumanizeId` reste un fallback de développement uniquement, jamais l'autorité
de production.

## Matérialisation

```powershell
.\Scripts\AuthorRPGRanger.ps1 `
    -EngineRoot D:\UE_5.5
```

Première matérialisation D04 attendue :

```text
DA_Class_Ranger
DA_Status_MarkedByRanger
DA_MONCAT_Goblin
DA_MONCAT_Vermin
DA_MON_GoblinThrower
DA_MON_RatGiant
```

Le script lance ensuite `Grimrock.RPG.RPG03.9.3`.

## Validation complémentaire

Après matérialisation :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.UI.RPG01.ProductionAssets"
```

Le filtre gagne un test `FavoredEnemyCategoryPresentation`.

Aucun UMG n'est modifié par D04.
