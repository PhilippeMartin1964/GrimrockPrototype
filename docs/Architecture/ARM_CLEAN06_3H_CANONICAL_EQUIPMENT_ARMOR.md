# ARM-CLEAN06.3H — Protections d'équipement à source unique

**Point de départ vérifié :** `c12a751`, `UE5.5.4/SilentTask = success` et `UE5.5.4/LocalAuto = success` (suite `Grimrock`).

## Autorité et périmètre

La protection physique et magique **nominale** des 22 ARM présents dans `DA_ItemSystem` est résolue exclusivement via `UGridItemSubsystem::ResolveArmorReserveForInstance`, le niveau propre à chaque `FGridItemInstance`, et la progression canonique N1–N20. La dépense est uniquement `AbsorbedEquipmentPhysicalArmor` / `AbsorbedEquipmentMagicalArmor` de l'instance persistée. La vue `FGridEquipmentArmorReadView` est transitoire et n'est jamais sauvegardée.

Les lecteurs suivants utilisent la même vue en mémoire : ressources d'armure effectives, absorption des dégâts physiques / magiques, restauration ciblée, bonus d'armure nominaux pour les calculs et le HUD, aperçu d'équipement et armure restante des objets déplacés. Un `ItemId` enregistré qui échoue à la résolution (niveau 0, réserve invalide, etc.) **ne relit pas** le `DA_Weapon_*` historique. Les objets non enregistrés conservent temporairement la logique des définitions existantes.

Les autres bonus statistiques (`StrengthBonus`, `CarryWeightBonus`…), résistances et présentations ne sont **pas** migrés par 06.3H : ils continuent temporairement à provenir des définitions existantes ; leur lecture n'est pas présentée comme un basculement complet. Pour les ARMs connus, le lecteur de bonus copie ces seules propriétés non-armure et remplace systématiquement `ArmorBonus` et `MagicalArmorBonus` par les nominals canoniques.

## Invariants

- Les valeurs consommées suivent le GUID de l'instance à travers sac, équipement, curseur et sauvegarde ; aucune remise à zéro lors d'une simple lecture.
- `ApplyPhysicalArmorDamage` et `ApplyMagicalArmorDamage` dépensent la réserve disponible sur l'équipement d'abord, puis la réserve de base du personnage.
- La restauration ciblée n'est pas un chargement de sauvegarde ; seule une action explicitement autorisée restaure des points. Le remplissage global après repos / victoire conserve son mécanisme existant.
- La prévisualisation n'écrit jamais le nouvel état sur le personnage ou les items.
- Aucun asset binaire, `.umap`, WBP ni SaveGame n'a été réécrit.

## Validation de non-régression

Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentArmorReadModel` charge le vrai `DA_ItemSystem` et vérifie : ARM075 à N20 et ARM081 à N1/N20 sans ancien DataAsset, niveau invalide 0 sans fallback malgré une définition historique contradictoire, protection épuisée sans recharge implicite, dette persistante, et comportement des items hors registre. Les tests historiques de boucliers, restauration, dégâts et prévisualisation restent actifs dans la suite complète `Grimrock`.

**Critère de sortie :** un unique commit `master`, `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` **success sur le SHA exact**. PIE reste un contrôle distinct après automatisation et avant suppression de lecteurs historiques en 06.5.
