# ARM-CLEAN06.3G — Équipement et occupation des mains canoniques

**Point de départ :** ARM-CLEAN06.3F, commit `85254b9e`, validations Windows vertes ; recette PIE ciblée des armes confirmée par le propriétaire le 14 novembre 2026.

## Raccordement de ce jalon

Pour les **22 identités ARM déjà enregistrées** dans le seul `DA_ItemSystem`, les chemins d'équipement suivants utilisent `UGridItemSubsystem::ResolveItem(ItemId, ItemLevel)` et `FGridCanonicalResolvedWeapon.HandPolicy` :

- `CanEquipItemToSlot` (main permise, niveau N1–N20 valide) ;
- `IsTwoHandedEquipped` / `ResolveEquipmentSourceSlot` ;
- équipement depuis le sac, équipement depuis le curseur, prévisualisation de déplacement des deux mains ;
- restauration d'un objet équipé après extraction pour transfert dans le monde.

La règle « deux mains » de l'archétype détermine l'emplacement physique `MainHand`, la libération de `OffHand` et la liste des objets déplacés. Un `ItemId` canonique connu mais non résoluble ne revient **jamais** à `DA_Weapon_*` ; `ItemLevel=0` est refusé par `CanEquipItemToSlot`. Les objets non enregistrés conservent temporairement leur ancienne logique.

## Limites explicites

Ce changement **ne migre pas** les bonus d'équipement, l'armure restante, les résistances, les poids, les icônes, les projectiles ni les sauvegardes. Ces lecteurs historiques existent encore et seront convertis avant 06.5. La restitution depuis le monde conserve ses contraintes transactionnelles existantes. Aucun `.uasset`, `.umap` ou fichier SaveGame n'est modifié.

## Validation

Le test persistant `Grimrock.RPG.ARM.CLEAN06_3.RealNativeRegistryAndActions` vérifie pour chaque ARM du manifeste que la politique des deux mains correspond au `BuildKind`, que la main déclarée est permise et que `None` est impossible. Les tests d'identité N1–N20 déjà existants restent actifs.

**Portes de sortie :** `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA exact, puis recette PIE des transactions de mains/sac/curseur sur armes une main, deux mains et passives. La validation PIE précédente de 06.3F n'est pas attribuée à tort à ce nouveau code.
