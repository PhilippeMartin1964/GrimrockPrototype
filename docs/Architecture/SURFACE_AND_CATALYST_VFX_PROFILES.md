# VFX-RUNTIME01 / ARMEMENT01.17.4D — Autorité unique des profils VFX

## Contrat définitif (sans rétrocompatibilité)

**Un niveau décrit des cellules, objets, spawns, triggers, interactions et états
de gameplay. Il ne contient aucun réglage global de Niagara.**

- UGridLevelAsset : aucun SurfaceVfxProfile, aucun SurfaceVfxSettings,
  aucun réglage d'effet au sol.
- UGridSurfaceVfxProfileAsset : uniquement les réglages du feu persistant
  (FGridFireVfxSettings, UNiagaraSystem et paramètres utilisateur optionnels).
- UGridCatalystVfxProfileAsset : uniquement la présentation transitoire
  du catalyseur (FGridCatalystVfxSettings : mesh du flacon, matériaux d'onde,
  mesh d'étincelles, durées, arc et délai par case).
- AGridLevelRuntimeActor : référence FireVfxProfile et référence
  CatalystVfxProfile. Pas de doublon de paramètres sur l'acteur.
- FGridWorldObjectInstanceConfig.NiagaraOverrides : réglages rares par
  exemplaire d'objet générique placé, sans mutation de la définition partagée.

Ces profils sont **des présentations**, pas des sources de vérité gameplay :
la propagation, les dégâts, leur durée et la résolution des réactions
continuent d'être décidés par les systèmes RPG03.6 et C6.

## Suppressions réelles

Les structures et propriétés du prototype ARMEMENT01.17.4B/4C ont été
supprimées du code, **sans alias, redirect ni logique de migration** :
FGridLevelSurfaceVfxSettings, bOverrideRuntimeVisuals,
LevelAsset.SurfaceVfxSettings, LevelAsset.SurfaceVfxProfile,
DefaultSurfaceVfxProfile, FireSurfaceNiagaraSystem,
FireSurfaceNiagaraScale, FireSurfaceLightIntensity et
CatalystFlight/Pulse/WaveDelayPerCellSeconds sur le runtime actor.

La visualisation Fire n'utilise plus le chemin hardcodé vers
NS_Flame_8_Torch, ni le fallback à trois sphères lumineuses.
Aucun chemin FireFX commercial n'est codé en C++.

Les assets du catalyseur ne sont plus chargés à partir de chemins
hardcodés sur le runtime : le profil Catalyst fournit explicitement
ProjectileMesh, RunePulseMaterial, EmberMaterial et SparkMesh.
Cela permet de modifier la présentation sans recompiler.

**Conséquence voulue** : sans FireVfxProfile valide contenant un
GroundFireSystem, il n'y a pas de flammes au sol (mais les dégâts Fire
demeurent). Sans CatalystVfxProfile renseigné, certaines parties de
la présentation du flacon/onde/étincelles seront absentes. Cette
configuration explicite remplace les fallbacks implicites.

## Installation locale de FireFX

L'utilisateur a installé le pack dans Content/FireFX/.
Les fichiers commerciaux restent **strictement locaux** et exclus par
.gitignore. Le système exact inventorié est :

/Game/FireFX/FX/NS_FFX_Ground_Fire.NS_FFX_Ground_Fire

Ce chemin est une **indication d'asset à sélectionner dans UE**,
pas une dépendance ajoutée en C++ ou GitHub. Le pack commercial n'a
pas été ouvert, analysé ou testé par le connecteur GitHub.

1. Dans l'éditeur UE5, créer un Data Asset de classe
   UGridSurfaceVfxProfileAsset sous
   Content/GrimrockPrototype/LocalVFX/DA_FireFX_GroundFire.
2. Configurer Settings.GroundFireSystem avec NS_FFX_Ground_Fire.
   Adapter GroundFireScale / GroundFireCoverage (~0.9 par défaut).
   Vérifier les User Parameters réels du système Niagara ;
   configurer leurs noms/tags seulement si les types correspondent.
3. Assigner DA_FireFX_GroundFire dans FireVfxProfile du
   BP_GridLevelRuntimeActor **local**.
4. Créer un Data Asset UGridCatalystVfxProfileAsset, éventuellement
   sous LocalVFX/DA_Catalyst_Default. Choisir les assets internes du
   projet SM_Catalyst_01, M_SurfaceCatalyst_RunePulse,
   M_SurfaceCatalyst_Ember et un SparkMesh simple (ex. Engine Sphere).
   Réglages initiaux 0.42 s de vol, 0.95 s de pulse, 0.085 s/case,
   arc 82 cm. L'affecter à CatalystVfxProfile.
5. Tester en PIE l'huile, l'inflammation au catalyseur, l'explosion
   de poison, la fermeture/extinction, un changement de niveau et
   les dégâts dans le flux d'événements.

ATTENTION : ne pas publier de BP, DataAsset ou niveau versionné qui
sérialise une référence vers FireFX ou LocalVFX absent de GitHub.
La protection Git sur les dossiers tiers ne supprime pas les
références stockées par d'autres uasset.

## Objets placés dans le Dungeon Editor

Sur un objet générique (AGridGenericObjectActor), l'inspecteur
Visual Effects — Placed Object permet d'écraser uniquement le
Niagara actif, un multiplicateur d'échelle et un décalage local.
Les autres instances héritent de la définition commune.
Les objets ramassables et les mécanismes spécialisés utilisent
des chemins d'exécution différents ; ne pas promettre que cet
inspecteur règle leurs effets.

## Vérifications

- Grimrock.RPG.ARMEMENT01.17.4D.DataOwnership
- Grimrock.RPG.ARMEMENT01.17.4D.ProfileBounds
- Grimrock.RPG.ARMEMENT01.17.4D.IndependentOfLevel
- Grimrock.RPG.ARMEMENT01.17.4C.PlacedObjectNiagaraOverrides
- Grimrock.RPG.ARMEMENT01.17.4A.CatalystLaunchFeed
- Grimrock.RPG.ARMEMENT01.17.4A.SurfaceDamageFeed
- Tests de présentation du catalyseur, compilation et suite complète UE5.5.4.

La validation Windows -NullRHI ne remplace pas le test de rendu Niagara
en PIE sur la machine possédant les fichiers FireFX.
