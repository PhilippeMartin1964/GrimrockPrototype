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
  mesh d'étincelles, vitesse de lancement, durée d'onde et délai par case).
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

## FIREFX05 — Profils Grimrock versionnés et pack externe ignoré

**Source de vérité :** `Content/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire.uasset`
est un Data Asset **versionné**. Il est affecté directement à la propriété
`FireVfxProfile` du Blueprint **versionné** `BP_GridLevelRuntimeActor`.
Le nom de dossier historique `Local` ne signifie plus que le contenu est privé.

Le pack tiers lui-même est sous `Content/F_FreeFlameFx_Pack/` et reste
exclu par `.gitignore`. Les chemins précédemment cités sous `/Game/FireFX/`
n'étaient qu'une hypothèse et ne doivent pas être imposés en C++.

Un `TSoftObjectPtr<UNiagaraSystem>` peut référencer un asset absent du dépôt,
sans incorporer son contenu. Un clone sans le pack peut compiler et tester le
gameplay, mais ne peut pas afficher les flammes sans installer le pack.

- Renseigner `GroundFireSystem` dans `DA_GroundFire` directement.
- Renseigner les noms des véritables Niagara User Parameters *uniquement*
  après consultation de leur nom et type dans l'asset.
- `FireCrackleLoopSound` est un champ du profil feu, à renseigner si
  un son bouclant versionnable existe.
- Les sons de flasque, `FlaskGlassBreakVfx` et `FireIgnitionBurstVfx` sont
  des propriétés directes du Blueprint runtime : **elles sont versionnées**.
- Un profil catalyseur `GridCatalystVfxProfileAsset` versionné est recommandé
  pour `SM_Catalyst_01`, `M_SurfaceCatalyst_RunePulse`,
  `M_SurfaceCatalyst_Ember` et le mesh d'étincelle. La vitesse balistique
  est `LaunchSpeedCmPerSec` ; l'ancienne durée/hauteur d'arc n'est
  plus l'autorité sur le vol.
- Le mécanisme de substitution `GrimrockLocalVfx.ini` a été supprimé :
  pas de configuration parallèle cachée dans `Saved/`.

Le test `Grimrock.RPG.FIREFX05.ProductionProfileWiring` contrôle la présence
du vrai Data Asset et le lien sérialisé dans les Class Defaults de la classe BP.
La vérification visuelle et sonore en PIE est distincte.

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
