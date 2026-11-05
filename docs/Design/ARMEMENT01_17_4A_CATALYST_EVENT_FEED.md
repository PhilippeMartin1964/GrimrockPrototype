# ARMEMENT01.17.4A — Flux d'événements du catalyseur et feu au sol

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

- Dans AGridLevelRuntimeActor, FireSurfaceNiagaraSystem permet de sélectionner
  un effet Niagara dédié au sol. Tant qu'il est vide, NS_Flame_8_Torch
  reste le fallback visuel existant et n'est jamais modifié.
- FireSurfaceNiagaraScale (0.38 par défaut) et FireSurfaceLightIntensity
  (650 par défaut) sont éditables sur l'instance/BP du runtime actor.
- Le nouvel asset Niagara doit émettre sur 200 x 200 cm (180 x 180 cm utiles)
  et disposer de ses propres paramètres utilisateur de densité, hauteur,
  vitesse, luminosité et fumée. Aucun asset tiers n'est inclus par ce commit.
- Une personnalisation propre au niveau passera ensuite par un profil
  référencé par UGridLevelAsset pour conserver l'autorité des données uniques.

## Tests

- Grimrock.RPG.ARMEMENT01.17.4A.CatalystLaunchFeed
- Grimrock.RPG.ARMEMENT01.17.4A.SurfaceDamageFeed
- Suite Grimrock complète sous UE 5.5.4/LocalAuto pour accepter le commit.
- Vérification visuelle PIE non couverte par l'automation headless -NullRHI.
