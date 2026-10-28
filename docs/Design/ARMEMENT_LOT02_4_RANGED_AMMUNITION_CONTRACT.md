# ARMEMENT-LOT02.4 — Munitions, fondation fonctionnelle

**Périmètre : règle runtime opt-in, tests de consommation et couverture des refus ; pas d'activation du kit Tireur.**

## Architecture

L'autorité demeure `UGridItemDefinitionAsset`. Un nouveau `FName RequiredAmmunitionItemDefinitionId`, vide par défaut, permet à un profil **ranged** d'indiquer le véritable identifiant d'item de munition. Ce champ n'introduit ni table de dégâts parallèle, ni consommation automatique d'armes existantes.

`FGridRangedAmmunitionPolicy` lit exclusivement le sac du personnage :
- sans munitions : l'attaque de l'arme équipée est désactivée dans le catalogue d'actions ;
- à l'exécution : refus avant consommation des PA si aucune munition n'est disponible ;
- attaque acceptée : retrait d'une unité via la fonction d'inventaire existante ;
- quand la pile est épuisée : suppression habituelle du slot, sans duplication ni quantité négative ;
- `NAME_None` : comportement historique, sans coût supplémentaire.

La validation `IsValidDefinition()` interdit la configuration de munitions sur les profils non catalogués, les objets passifs ou de mêlée, les armes jetables, les références à soi et les armes ayant un second coût de quantité à leur propre source. Ce dernier garde-fou protège l'atomicité du tir.

Le test `Grimrock.RPG.ARMEMENT.LOT02.AmmunitionInventoryContract` parcourt les consommations 3 → 2 → 1 → 0 et le refus sans munition. Il ne simule pas une scène de combat réelle.

## Ne pas déclarer ARM045 prêt prématurément

**Aucun DataAsset binaire n'est modifié par ce commit.** Le champ du vrai `DA_Weapon_ShortBow_01` reste non configuré : le jeu continue avec son comportement actuel. Il faudra fabriquer un véritable `UGridItemDefinitionAsset` pour les flèches (icône, mesh et quantité adaptée), l'enregistrer dans les résolveurs de sauvegarde et le fournir au kit Tireur, puis configurer `RequiredAmmunitionItemDefinitionId` sur ARM045. Aucune fausse flèche ni consommation de pierre n'est autorisée.

La disponibilité `AMMO_NOT_VERIFIED` dans `LOT02_3` ne peut être levée qu'après cette étape d'authoring et une **recette PIE** incluant PA, dégâts, tir, refus de tir à zéro flèche, sauvegarde/rechargement et équipement deux mains.

## Validation locale (UE5.5.4, éditeur fermé)

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.LOT02"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.9"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT.Industrial"
```

## Non-régression

Les 12 anciens kits, les 18 recettes proposées, la progression des classes et les sauvegardes n'ont pas été réécrits. Cette étape ne modifie pas les `.uasset`. Les callbacks/Blueprints ne sont pas modifiés ; le source de l'action et l'ItemLevel restent leurs valeurs canoniques.
