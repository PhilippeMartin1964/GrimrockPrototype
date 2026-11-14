# ARM-CLEAN06.3AJ — Niveaux d'objets de niveau et dépôts monde

**Base vérifiée :** 06.3AI (`74dc472`) ; compilateur UE5.5.4 et
suite complète `Grimrock` verts sur `master`.

## Problème

Deux consommateurs restaient liés au `TryGetWeaponCatalogueProfile`
historique :
- `AGridLevelRuntimeActor::AddPlacedItemActor` construit un **nouvel
  item authoré** depuis `FGridLooseItemInstance` du `LevelAsset` ;
- `AGridLevelRuntimeActor::TryDropItemInstanceAtCell` dépose un
  **item déjà créé**, venant d'un curseur, inventaire, projectile
  au sol, loot ou transfert inter-niveau à travers une trappe.

Confondre ces deux cas transformait un `ItemLevel=0` enregistré
mais invalide pour une arme existante en niveau N1 implicite, selon
l'ancien `DefaultWeaponItemLevel`. À l'inverse, des munitions
persistées N0 sont valides : le N0 ne doit jamais devenir un
niveau d'arme.

## Correction

`FGridWorldItemLevelReadModel` expose deux intentions nommées et
réutilise le lecteur statique déjà éprouvé en 06.3AI
(`FGridReceptacleItemMetadataReadModel`) :

- `ResolveAuthoredPlacement` : uniquement pour création depuis
  l'asset de niveau, où un ARM N0 hérite **une seule fois** de
  `DefaultWeaponItemLevel` et matérialise un niveau N1–N20
  valide dans le registre. Le niveau d'une arme déjà authoré
  N1–N20 est conservé tel quel.
- `ResolvePersistedInstance` : uniquement pour les dépôts d'items
  existants. Un ARM canonique N0 **échoue avant le spawn**
  et ne peut pas utiliser une valeur héritée ; un ARM N1–N20
  conserve son niveau, même après chute, dépôt, passage
  d'étage ou restauration.
- Non-ARM enregistré : `ItemLevel = 0` est le niveau
  persistant véritable. Toute progression inventée est rejetée.
- Les identités hors registre conservent provisoirement le chemin
  historique de validation du catalogue d'armes (migration 06.4
  non exécutée). Une identité connue invalide n'a aucun
  fallback N1 ni poids/nom contradictoire.

L'identité, les GUID, la quantité, la transform, les offsets,
les règles de portée de main, la physique Chaos, le `Pit`
forwarding, les états lumineux, les surfaces murales et le
rafraîchissement des plaques de pression restent inchangés.
Aucun `DA_GridLevel_*` ni `FGridRuntimeItemState` n'est réécrit.

## Non-régression

Test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldItemLevelBySource`
avec la vraie racine `DA_ItemSystem` sur ARM014/031/054/075/081,
N1/N20, héritage N7, rejet N0 pour un item déjà existant,
refus du N0 d'une arme nouvellement authorée sans définition,
flèches réellement N0, ancienne définition contradictoire,
identités hors registre et validation stricte de l'ItemId.

Les suites historiques `Grimrock.RPG.ARMEMENT01.8.PlacedItems.*`,
`Grimrock.TechnicalDebt.TD02_1.WorldItemsContract`,
`Grimrock.PIT.*` et `Grimrock` globale servent de
garde-fous sur les acteurs, le runtime, la physique et
Save/Continue.

Pas d'asset UE binaire, WBP, String Table, SaveGame ni
localisation modifiés : **UI.csv → ST_UI → WBP → Game/fr/en
inchangé.**

**Contrôle de publication :** un seul commit atomique, puis
`UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS
sur le SHA exact. Les effets visuels et physiques en PIE ne
sont pas prouvés par `-NullRHI`.
