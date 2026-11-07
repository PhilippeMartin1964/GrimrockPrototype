# FLASK04 — Flasques réellement visibles en vol (huile et catalyseur)

## Défaut identifié

Les anciens tests n'exigeaient pas un projectile visible : si `Item.WorldMesh`
était vide, la flasque d'huile affichait directement le splash ; si
`CatalystVfxProfile.ProjectileMesh` et `Item.WorldMesh` étaient vides,
la réaction du catalyseur démarrait sans vol.

## Résolution

Dans `BP_GridLevelRuntimeActor` les Class Defaults (Combat / VFX / Flask) exposent :

- **Meshes / OilFlaskFallbackMesh** = `SM_Potion_01` par défaut, un mesh de bouteille existant et versionné ;
- **Meshes / CatalystFlaskFallbackMesh** = `SM_Catalyst_01` par défaut (asset d'artiste versionné) ;
- **Meshes / FlaskVisualHalfExtentCm** = 20 cm ;
- **Ballistics / OilFlaskLaunchSpeedCmPerSec** = 900 cm/s (pour le catalyseur, la vitesse est dans `CatalystVfxProfile.Settings.LaunchSpeedCmPerSec`) ;
- **Ballistics / FlaskLaunchForwardOffsetCm** = 48 cm ;
- **Ballistics / FlaskLaunchRightOffsetCm** = 25 cm ;
- **Ballistics / FlaskLaunchHeightCm** = 100 cm ;
- **Ballistics / FlaskImpactHeightCm** = 14 cm ;
- **Diagnostics / bLogFlaskPresentation** = true.

Priorités des meshes : huile = item `WorldMesh` puis `OilFlaskFallbackMesh` ;
catalyseur = `CatalystVfxProfile.Settings.ProjectileMesh`, puis item `WorldMesh`,
puis `CatalystFlaskFallbackMesh`. **Les deux secours appartiennent au jeu**
et ne dépendent ni de `Content/FireFX/` ni de `Content/F_FreeFlameFx_Pack/`.

Les coordonnées de départ sont décalées en avant et sur la droite de la
cellule de la troupe pour rendre le vol perceptible en vue subjective.
La solution balistique utilise la gravité en cm/s², sans frottement
de l'air et sans interpolation sinusoïdale.

Les logs `[FLASK04]` permettent de lire, en PIE, la cellule de départ,
la cible, le mesh réellement choisi, la vitesse et la durée physique.
Un ciblage impossible physiquement ou un mesh manquant affiche un
avertissement explicite et préserve les dégâts et surfaces de gameplay.

## Tests

`Grimrock.RPG.FLASK04.VersionedProjectileMeshes` vérifie que les deux
secours réels existent ; les tests MON12 d'huile **exigent maintenant**
que le projectile soit présent avant le splash en combat comme en exploration.

Il n'est plus possible d'obtenir des tests verts de l'huile en cachant
silencieusement l'absence de sa bouteille.

Les sons et Niagara sont facultatifs mais configurables indépendamment :
voir le guide FLASK/FIREFX de configuration.

## FLASK05 — Synchronisation de la flaque avec l'impact

L'ancien code dessinait l'huile persistante dès la confirmation, alors que la bouteille était encore en vol. Désormais le runtime retarde uniquement ses visuels via `BeginOilFlaskVisualBatch` puis `FinishOilFlaskVisualBatch` à l'impact. Les surfaces et ressources RPG03.6 sont toujours déterminées immédiatement, sans modification des dégâts ni PA. Les tests MON12 vérifient l'absence de flaque avant l'impact et sa présence après.
