# ARM-CLEAN06.4X.35 — Spawn de projectile : pas de lancement physique N1 non-ARM

**Date :** 17 novembre 2026. **Parent :** `e41a6b89e87f8c3e2799d9674f9b32d9cb9fac11`.
**Périmètre :** `GridLevelRuntimeActorWorldItems.cpp`, régression
`Grimrock.RPG.ARM.CLEAN06_3.CanonicalSpecializedThrowWithoutLegacy.InvalidAndLegacy`,
documentation. Aucun binaire UE, WBP ni SaveGame.

## Cause

La porte `AGridLevelRuntimeActor::SpawnThrownItemProjectile`
distingue projectile ARM dédié, munition de combat N0 et
lancer physique générique avec ancien `UGridItemDefinitionAsset`.
Lorsqu'un ItemId non-ARM était enregistré et encore lié à une
définition historique `IsPhysicallyThrowable()`, elle pouvait
accepter son `ItemLevel=N1` car `ResolveItem` autorise encore
les projections de famille N1 (compatibilité 06.3V).

Cela créait une instance de vol impossible, que la pose au sol
refusait ensuite à niveau physique N1. Les lecteurs visuels X.34
sont plus stricts, mais ne suffisent pas à empêcher le spawn
ni les traitements d'interaction/impact.

## Correction

Avant le spawn, pour les **seuls IDs inscrits** à
`DA_ItemSystem`, appeler
`FGridWorldItemLevelReadModel::ResolvePersistedInstance`
avec le niveau exact et `LegacyDefinition=nullptr`.
Refuser si le niveau physique n'est pas autorisé, même lorsqu'un
ancien asset correspondant promet une balistique valide :
ARM N1–N20, non-ARM N0. Pour un ID réellement hors registre,
la branche historique est intégralement préservée.
Aucune conversion N0 vers N1, aucun changement de projectile
ou de vitesse pour les sources valides.

## Test

Étendre la régression de projectile spécialisé existante :
le lecteur de famille accepte toujours une flèche N1, la
définition transitoire `IsPhysicallyThrowable` est valide,
mais `SpawnThrownItemProjectile` refuse la même instance
physique sans créer d'acteur. Conserver le refus ARM N0/N21,
le lancer ARM N20 et le comportement hors registre.

Attendre les contrôles Windows `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` SUCCESS sur le SHA final.
Ni lancer de torche en PIE, ni chute dans une fosse, ni
migration des 22 profils ARM ne sont validés par ce test.

**06.4 partielle ; 06.5 BLOQUÉE ; SaveGame v27 intact.**
