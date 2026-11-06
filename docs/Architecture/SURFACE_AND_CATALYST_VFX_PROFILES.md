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

1. Dans l'éditeur UE 5.5.4, créer un Data Asset de classe
   `GridSurfaceVfxProfileAsset` sous
   `Content/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire`.
   **Ce dossier entier est ignoré par Git**, parce que les profils
   qui y résident peuvent référencer le pack FireFX commercial.
2. Dans `Settings`, affecter `NS_FFX_Ground_Fire` à
   `GroundFireSystem`. Les valeurs initiales du code sont :
   `GroundFireScale=1`, `GroundFireCoverage=0.9`,
   `FlameHeightCm=75`, `FlameDensity=1`,
   `bEnableSmoke=true`, `SmokeDensity=0.5`,
   `MaxFireCells=48`, `MaxFireLights=8`,
   `FireLightIntensity=650`. Ce sont des valeurs de départ,
   non des réglages universellement adaptés à FireFX.
   Laisser les paramètres utilisateur Niagara sur `None` tant que
   les noms **et les types** réellement exposés par le système n'ont
   pas été vérifiés dans son panneau User Parameters.
3. Renseigner `FireVfxProfile = DA_GroundFire` dans les
   Class Defaults de `BP_GridLevelRuntimeActor` **pour un essai local**.
   Attention : ce Blueprint est versionné dans Git. Ne pas
   committer/pousser sa référence vers un Data Asset ignoré :
   cela rendrait le projet cloné dépendant du pack commercial
   absent de GitHub.
4. Créer séparément un Data Asset `GridCatalystVfxProfileAsset`
   sous `Content/GrimrockPrototype/Effects/Alchimie/Catalyst/`,
   par exemple `DA_Catalyst_Default`. Il peut être versionné
   **si toutes ses références sont des assets autorisés du projet**.
   Affecter `SM_Catalyst_01` dans `ProjectileMesh`,
   `M_SurfaceCatalyst_RunePulse` dans `RunePulseMaterial`,
   `M_SurfaceCatalyst_Ember` dans `EmberMaterial`. Le
   `SparkMesh` est facultatif. Les valeurs initiales sont :
   0.42 s de vol, 0.95 s d'onde, 0.085 s/case, arc de 82 cm.
   Assigner le profil dans `CatalystVfxProfile` du runtime actor.
5. Tester en PIE le feu au sol, les réactions du catalyseur,
   l'huile, le poison, les effets d'impact et leur extinction.
   Vérifier indépendamment les dégâts gameplay et le rendu.

ATTENTION : un dossier ignoré par Git ne protège pas un
`.uasset` versionné qui sérialise une référence vers ce dossier.
Ne pas publier de BP, DataAsset ou niveau traînant des références
à `FireFX` ou à `Effects/Fire/Local` non distribués.
L'ancienne exclusion `LocalVFX` dans `.gitignore` reste
uniquement une précaution pour d'éventuels fichiers locaux
historiques : **ce n'est plus l'emplacement canonique**.

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
