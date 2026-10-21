# ARMEMENT01.6.5 — Meshes réalistes dans le donjon et l'éditeur

**Date :** 21 octobre 2026. **Statut :** code et tests proposés ; compilation/validation locale en attente. **Référence connue avant ticket :** 1488/1488 tests Grimrock (TD04-20261021-113919), 0 warning/échec.

## Implémentation

Les chemins existants d'objets sont conservés : `AGridItemActor` pour le runtime (au sol, objet tenu, réceptacle et projectile héritent de la définition) et `UGridEditorPreviewComponent` pour les objets placés directement dans l'asset de niveau.

Un unique `FGridWorldItemVisualResolver` autorise les deux à demander le **même** `WorldMesh` à `FGridWeaponVisualResolver` (ARMEMENT01.6.2). Le catalogue soft est chargé seulement lors de la création d'un objet ou de la reconstruction des previews. Les objets avec `bUseWeaponVisualCatalogue=false`, non mappés ou aux meshes manquants conservent leur `WorldMesh` Legacy. Aucune silhouette d'un autre archétype n'est substituée.

La `WorldVisualTransform` du catalogue, quand la résolution est complète, s'applique **relativement au StaticMeshComponent** ; elle ne remplace pas le transform logique du placement en grille. Sur repli Legacy, la transformation locale est identité. Aucun `SetMaterial` n'est ajouté au mesh principal : les Material Slots du StaticMesh restent propriétaires du PBR.

**Invariant de gameplay :** aucune modification de `RuntimeObjectId`, `ItemDefinitionId`, interactions/pickups, collisions de base, simulation de physique, masses, émissions lumineuses, sparkles, calcul RPG, sauvegardes ou configuration des réceptacles. Aucun nouveau Blueprint ni asset de niveau.

## Catalogue de référence

| Archétype | VisualId | WorldMesh |
|---|---|---|
| ARM001 | VIS_Dagger_01 | SM_Weapon_Dagger_01 |
| ARM045 | VIS_ShortBow_01 | SM_Weapon_ShortBow_01 |
| ARM075 | VIS_RoundShield_01 | SM_Weapon_RoundShield_01 |

Les trois fichiers et les textures ont été publiés séparément avant cette étape. Le projet ne contient pas encore de définitions de collectible concrètes pour ces trois archétypes : **leur apparition automatique dans le donjon n'est donc pas revendiquée**. Une définition d'item devra activer explicitement `bUseWeaponVisualCatalogue`, définir son `WeaponArchetypeId` et sélectionner `DA_WeaponVisualCatalogue`, puis être placée via l'éditeur existant. L'asset de niveau `FGridLooseItemInstance` ne stocke pas encore ItemLevel ; un aperçu de définition ne crée donc aucun niveau imaginaire. La création/migration des définitions est un travail distinct.

## Tests ARMEMENT01.6.5

- `Grimrock.RPG.ARMEMENT01.6.World.RuntimeMeshAndLegacy` : runtime des 3 profils, transform local, identité, matériau principal provenant du mesh, collision initiale inchangée, repli Legacy/mapping absent.
- `Grimrock.RPG.ARMEMENT01.6.World.EditorPreviewSameMesh` : 3 loose items dans un niveau transitoire ; mêmes meshes/transforms que le runtime, identités stables, previews sans collision.

Exécuter sur le poste Windows UE5.5.4, Unreal Editor fermé :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.6.World"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"
```

**Objectif uniquement, non attesté :** 2/2 nouveaux tests ; 1490/1490 globaux ; 0 warning/échec. Pour une revue visuelle future : créer les trois définitions d'items, les poser via une cellule de l'éditeur, examiner leur mesh/transform, leur prise, et vérifier dimensions 30 cm (dague), 115 cm (arc), 65 cm (bouclier).

## Suite

ARMEMENT01.6.6 : audit de couverture des 82 archétypes et plan de silhouettes. L'authoring des trois vrais DataAssets de collectibles peut être traité immédiatement et séparément pour permettre une vraie démonstration en jeu.
