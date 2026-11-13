# ARMEMENT01.17.4A — Flux d'événements du catalyseur et feu au sol

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/test historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Non implémentée : zéro legacy dans le runtime final.**


Date : 5 novembre 2026. Base : 4ed239eb. Tests UE locaux indispensables.

## Flux d'événements

- Le lancer consommé produit UN événement Combat (code Combat.Surface.CatalystZone)
  dans les deux modes, exploration et combat. Aucun événement au survol ni
  au ciblage ou lors du chargement d'une sauvegarde.
- Les dégâts immédiats liés au poison explosif produisent
  Combat.Surface.CatalystBurstDamage pour chaque cible réellement touchée.
- Les dégâts périodiques Fire sur les monstres ou le groupe produisent
  Combat.Surface.FireDamage à chaque application effective, pendant combat
  et exploration. Aucun message si les dégâts bruts sont entièrement résistés.
- Les dégâts PV et les protections absorbées proviennent de FGridAttackResult,
  après résolution canonique ; aucune donnée de combat n'est inventée.
- Réutilisation de FGridCombatEventFeedFormatter, FGridHudEventEntry et
  GridEventFeedPublisher, sans journal concurrent ni double application RPG03.6.

## Présentation du feu au sol

Depuis ARMEMENT01.17.4D, le feu persistant est présenté exclusivement
par UGridSurfaceVfxProfileAsset, indépendant de UGridLevelAsset.
Le catalyseur a son propre UGridCatalystVfxProfileAsset. Le runtime ne charge
aucun effet de torche en fallback et n'a plus de champs de réglages Niagara
dupliqués. Voir docs/Architecture/SURFACE_AND_CATALYST_VFX_PROFILES.md.

## Tests

- Grimrock.RPG.ARMEMENT01.17.4A.CatalystLaunchFeed
- Grimrock.RPG.ARMEMENT01.17.4A.SurfaceDamageFeed
- Suite Grimrock complète sous UE 5.5.4/LocalAuto pour accepter le commit.
- Vérification visuelle PIE non couverte par l'automation headless -NullRHI.
