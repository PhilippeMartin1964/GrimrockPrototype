# ARM-CLEAN06.5H.39 — Réceptacles à dagues N1–N20 sans ancienne définition

## Migration

La fixture `GridARMEMENT0183ReceptacleItemLevelTests.cpp` n'ouvre plus `DA_Weapon_Dagger_01`. Son identité stable `Weapon_Dagger_01` est utilisée pour les placements initiaux, transferts, retraits et comparaisons.

Le registre `FGridCanonicalLootTestScope`, déjà attaché au monde temporaire avant `RebuildLevel`, résout obligatoirement la dague ARM N1. Un échec de cette projection interdit la construction de la fixture; aucune définition héritée ne sert de secours.

## Tests préservés

Les assertions couvrent toujours les deux niveaux initiaux N9 et N1, le transfert N12 depuis le curseur, le stockage physique, la sauvegarde et la restauration de contenu de réceptacle, la priorité de l'état sauvegardé N14 sur l'authoring N7, la conservation des GUID et le refus des transferts invalides.

Aucun changement au runtime, aucune mutation de `.uasset`, `.umap`, `.sav` ni modification du niveau de production. Les 53 packages historiques H.23 restent en place.

## Barrière de validation

Avant H.40, exiger `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA, compilations UE5.5.4 Editor + Shipping, Automation `Grimrock.RPG.ARMEMENT01.8.3` et régressions de réceptacles. Les sept sauvegardes natives restent à vérifier avant suppression de packages.
