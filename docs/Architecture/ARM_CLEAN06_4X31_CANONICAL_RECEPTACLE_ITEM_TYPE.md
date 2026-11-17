# ARM-CLEAN06.4X.31 — Catégorie de réceptacle : ARM canonique, non-ARM préservé

**Date :** 17 novembre 2026. **Parent :** `54f547b0802715492a8e0cb73639cbb85def501a`.
**Portée :** un lecteur C++ des conditions de liens et une régression transitoire ; aucun asset binaire ni SaveGame modifié.

## Anomalie ciblée

`AGridReceptacleActor::ContainsItemType` lisait uniquement `Item.ItemDefinition->ItemType`.
Or les objets ARM sans ancien `UGridItemDefinitionAsset` sont désormais présents
dans les réceptacles (X.21/X.22). La condition `ReceptacleContainsItemType`,
consommée par `GridActivationComponentLinks.cpp`, ne pouvait pas reconnaître
une arme ou un bouclier canonique sans pointeur.

## Comportement

- Pour un ItemId enregistré et un niveau valide, `ResolveItem` détermine
  la catégorie **ARM** : `Weapon.Kind=Shield` donne `Shield` ;
  `Weapon` et `Focus` donnent `Weapon`, comme la projection UI existante.
- Si l'ID est enregistré mais le niveau physique invalide (ARM N0 ou N21,
  non-ARM N1), la condition est fausse **sans** fallback à la catégorie legacy.
- Pour un **non-ARM enregistré**, `DA_ItemSystem` ne porte pas encore
  d'`EGridItemType` métier. Les conditions déjà authorées restent lisibles
  via une ancienne définition **seulement si son ItemDefinitionId concorde**.
  Sans ancien pointeur, la catégorie ne peut pas être inventée : la condition
  reste fausse, comme auparavant. Ne pas réutiliser le `Misc` provisoire
  de la projection UI pour trancher les énigmes `Key`, `Gem`, `Torch`, etc.
- Pour un ItemId réellement hors registre, conserver le contrôle historique.
  Ne supprimer aucun champ/asset pendant cette phase.

## Contrôles automatisés

`Grimrock.RPG.ARM.CLEAN06_4X31.CanonicalReceptacleTypeConditions` :
vrai registre chargé dans un monde transitoire, réceptacle concret,
dague N1/N20 sans pointeur, bouclier N1, catégorie legacy contradictoire,
refus ARM N0/N21, clé N0 avec son type historique, clé N1 invalide,
non-ARM sans pointeur et ID totalement hors registre.

Attendre **sur le SHA final** `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` SUCCESS. Un résultat automatique ne prouve
pas les énigmes en PIE ni la migration native des 22 ARM.

**ARM-CLEAN06.4 reste partielle. ARM-CLEAN06.5 reste BLOQUÉE.**
La taxonomie gameplay non-ARM doit être explicitement décidée et
matérialisée dans Unreal avant la suppression des anciens pointeurs.
