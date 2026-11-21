# ARM-CLEAN06.5H.26 — Outils de désamorçage canoniques dans Issue181

## Périmètre

La fixture `Grimrock.Pit.Issue181.ExplicitLocalDisarmAndMemoryRestore` n'a plus besoin de charger `DA_Item_DisarmTools_01` pour faire échouer le désamorçage réel puis déclencher la fosse. Son objet porte désormais directement l'identité `Item_DisarmTools_01` au niveau N0, résolue par `UGridItemSubsystem` dans le monde transitoire, puis validée avec `UGridPartyInventoryComponent::ApplyItemDefinitionToInstance` avant l'ajout à l'inventaire.

Les assertions de désamorçage, échec du piège, chute du groupe, physique Chaos, transfert local des armes, GUID et SaveGame du scénario restent intactes.

## Exclusion volontaire

Le second chargement historique dans ce fichier, `DA_Weapon_Dagger_01`, demeure pour le test distinct du chemin **legacy** `SpawnThrownItemProjectile`. Il doit être migré séparément avec son API de lancer, sans masquer son rôle par une définition canonique artificielle.

Aucun `.uasset`, `.umap`, `.sav`, catalogue, ciblage, effet d'huile, gameplay ou schéma persistant n'est modifié. Aucun package du périmètre H.23 (53 packages, dont 48 définitions historiques) n'est supprimé.

## Barrière de validation

Ce commit requiert sur son SHA exact les statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` = SUCCESS, comprenant compilation Development Editor / Shipping et Automation, notamment `Grimrock.Pit.Issue181` et les régressions de désamorçage. Sans ces deux statuts, ne pas publier H.27.

La migration des anciens assets n'est pas autorisée par ce seul nettoyage : inventaire des consommateurs et preuve native renouvelée sur les sept SaveGames restent obligatoires avant tout retrait.
