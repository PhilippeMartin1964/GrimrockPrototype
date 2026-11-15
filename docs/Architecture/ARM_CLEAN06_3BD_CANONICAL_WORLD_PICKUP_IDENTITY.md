# ARM-CLEAN06.3BD — Autorité d'ItemId au sol, au ramassage et à la capture

**Parent :** ARM-CLEAN06.3BC (`11d044a`, deux statuts Windows validés).

## Défaut constaté

`GridLevelRuntimeActorWorldItems.cpp` et
`GridLevelRuntimeActorPersistence.cpp` utilisaient chacun un résolveur
qui accordait priorité à `UGridItemDefinitionAsset::ItemDefinitionId`.
De plus, `AGridItemActor::GetItemDefinitionId()` renvoyait lui-même ce
champ même lorsqu'un item physique déjà matérialisé portait son ItemId
canonique en `ItemDefinitionId`. Une définition ancienne contradictoire
pouvait donc renommer l'objet ramassé ou son snapshot SaveGame.

## Règles 06.3BD

1. L'identité d'un item enregistré provient de l'**ItemId physique**
   `AGridItemActor::ItemDefinitionId` et de l'entrée de monde
   `FGridSpawnedItemRuntimeEntry::ItemDefinitionId`, validés contre
   `UGridItemSubsystem` au niveau physique persistant.
2. Un désaccord entre l'item physique et l'entrée provoque un refus,
   jamais une substitution de définition, un reset à N1 ou une
   résolution via un vieux `DA_Item_*`.
3. `FGridWorldPickupIdentityReadModel` est commun au ramassage par
   cellule, au clic sur acteur et à la capture de niveau. Aucun cache
   ou duplicata de l'identité n'est ajouté au SaveGame.
4. `AGridItemActor::GetItemDefinitionId` privilégie cet identifiant
   lorsque le registre contient l'objet, évitant aussi un refus incorrect
   d'`ApplyCanonicalWorldVisual`. Hors registre, son ancien ordre de
   résolution reste inchangé.
5. Les objets non encore enregistrés gardent leur comportement historique
   jusqu'à la conversion des assets 06.4. Un item connu mais à niveau
   invalide échoue sans fallback.

## Couverture

Les cinq scénarios sont publiés séparément : `Presentation`, `RegisteredLevelGuards`, `LegacyIdentity`, `Capture` et `Pickup`, afin de rendre les échecs d'assertion identifiables dans le rapport Windows filtré.

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupIdentity` :
- `Item_Arrows_01` N0 réel, asset historique volontairement mensonger,
  restitution du véritable ItemId et mesh canonique ;
- `Weapon_StraightSword_01` N0 invalide et N20 valide ;
- incohérence entre acteur et entrée rejetée ;
- véritable `CaptureCurrentLevelRuntimeState` conservant ID/niveau/GUID ;
- ramassage physique avec `TryPickupItemActor`, une flèche dans le sac
  et aucune entrée fantôme sous le faux ID ;
- objet inconnu : même priorité legacy qu'avant.

**Non modifiés :** `.uasset`, `.umap`, WBP, localisation, schéma
SaveGame. Pipeline `UI.csv → ST_UI.uasset → WBP → Game/fr/en` inchangé.

Ce jalon ne convertit pas les jets ordinaires sans `ProjectilePolicy`,
les recettes de contenus binaires Editor, ni le jeu réel PIE/Chaos.
06.4–06.6 restent ouvertes.
