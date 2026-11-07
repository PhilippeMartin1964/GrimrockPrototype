# FLASK / FIREFX — Mode d'emploi UE 5.5.4

**Périmètre :** lancer de flasques d'huile et de catalyseur depuis la troupe, vol balistique en cases de 200 cm, impact, sons et effets de feu. Gameplay RPG03.6/C6 inchangé.

## 1. Organisation Git confirmée

- **Versionnés** : `Content/GrimrockPrototype/`, notamment les flasques, meshes, `DA_GroundFire`, `BP_GridLevelRuntimeActor`, `L_Dungeon`.
- **Ignorés** : `Content/FireFX/` et `Content/F_FreeFlameFx_Pack/` (packs tiers).
- `Content/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire` est **versionné** malgré le nom historique de son dossier.
- Aucun `Saved/Config/GrimrockLocalVfx.ini` n'est utilisé.

## 2. Où régler les flasques

Ouvrir `BP_GridLevelRuntimeActor` → **Class Defaults** → `Combat|VFX|Flask`.

### Meshes (FLASK04)

| Champ | Valeur / effet |
|---|---|
| `OilFlaskFallbackMesh` | `SM_Potion_01` (secours C++ versionné) |
| `CatalystFlaskFallbackMesh` | `SM_Catalyst_01` (secours C++ versionné) |
| `FlaskVisualHalfExtentCm` | `20` (demi-dimension visuelle maximale cible, en cm) |

L'huile privilégie `DA_Item_Flask_Oil.WorldMesh`. Si absent, elle utilise `OilFlaskFallbackMesh`.
Le catalyseur privilégie `CatalystVfxProfile.Settings.ProjectileMesh`, puis `DA_Item_SurfaceCatalyst_01.WorldMesh`, puis `CatalystFlaskFallbackMesh`.
Un mesh de secours est disponible sans aucun pack commercial. Pour obtenir deux bouteilles réellement différentes, assigner un mesh d'huile authentique à son `WorldMesh` ou personnaliser son secours.

### Trajectoire physique (FLASK01, FLASK04)

| Champ (Class Defaults) | Départ proposé |
|---|---|
| `OilFlaskLaunchSpeedCmPerSec` | **900 cm/s** (9 m/s) |
| `FlaskLaunchForwardOffsetCm` | **48 cm** devant la troupe |
| `FlaskLaunchRightOffsetCm` | **25 cm** sur le côté droit du lancer |
| `FlaskLaunchHeightCm` | **100 cm** au-dessus du sol |
| `FlaskImpactHeightCm` | **14 cm** au-dessus du sol |
| `bLogFlaskPresentation` | `true` pour le diagnostic |

Pour **la vitesse du catalyseur**, utiliser le profil indépendant :
`BP_GridLevelRuntimeActor → CatalystVfxProfile` → Data Asset
`GridCatalystVfxProfileAsset` → `Settings.LaunchSpeedCmPerSec`.
La valeur C++ en l'absence de profil est **900 cm/s**.
L'asset de profil `DA_Catalyst_Default` cité dans des anciennes fiches
n'était **pas présent dans GitHub** lors de l'audit FLASK04 : créer un vrai Data Asset
de cette classe, le versionner et l'assigner **uniquement si l'on veut personnaliser** vitesse, mesh ou effets de réaction du catalyseur.
Le système reste opérationnel sans ce Data Asset grâce aux définitions d'objets et aux secours C++.

Pour les deux bouteilles, le calcul suit `P(t) = P0 + V0*t - (0,0,g*t²/2)` avec la gravité du monde (défaut UE : 980 cm/s²).
Aucune interpolation sinusoïdale et aucun frottement de l'air. L'angle est calculé vers la cellule valide, à partir de la vitesse configurée. Un tir physiquement hors de portée n'est pas maquillé par une fausse parabole.

**Important :** le ciblage et les collisions de gameplay restent ceux des actions C6/RPG03.6. Les composants visuels ne simulent pas encore les collisions intermédiaires avec les plafonds ou le bord d'un mur (CollisionEnabled=NoCollision). Un test visuel proche des obstacles reste nécessaire.

## 3. Où régler les sons (FLASK03 / FIREFX04)

Toujours dans `BP_GridLevelRuntimeActor → Class Defaults` :

| Propriété | Événement exact | Asset attendu |
|---|---|---|
| `OilFlaskThrowSound` | départ de la flasque d'huile | `SoundWave`, `SoundCue` ou MetaSound de jet/souffle |
| `OilFlaskBreakSound` | arrivée de l'huile au sol | bruit de verre brisé + liquide |
| `CatalystFlaskThrowSound` | départ du catalyseur | son bref de jet/souffle |
| `CatalystFlaskBreakSound` | arrivée du catalyseur | bruit de verre brisé |
| `FireIgnitionSound` | réaction qui produit du feu | « wouf » bref d'embrasement |

Ces propriétés sont **facultatives mais ne créent aucun son à elles seules** : renseigner des assets audio réellement présents dans Content Browser. Créer au besoin ces sons dans `Content/GrimrockPrototype/Audio/` (assets que le projet a le droit de distribuer). Configurer une atténuation spatiale adaptée dans chaque son ou MetaSound. Les sons de départ sont joués à la position de lancement ; les bris, à la position de l'impact.

Pour les **crépitements persistants** : ouvrir
`Content/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire` →
`Settings → Fire|Audio`.

| Champ | Valeur de départ |
|---|---|
| `FireCrackleLoopSound` | son spatial réellement bouclant, à assigner |
| `MaxFireCrackleSources` | **4** voix simultanées maximum |
| `FireCrackleVolume` | **0.65** |

L'audio du feu est regroupé par zones contiguës de cases enflammées, pas une voix par case. Il est arrêté lorsque les surfaces s'éteignent, que le niveau change ou que le runtime s'arrête. Le son assigné doit boucler réellement ; la boucle n'est pas fabriquée par le composant C++.

## 4. Où régler les Niagara

Dans `BP_GridLevelRuntimeActor → Combat|VFX` (ou dans les **Details de l'acteur placé dans L_Dungeon**, si vous souhaitez une valeur spécifique à cette carte) :

| Propriété | Réglage |
|---|---|
| `FlaskGlassBreakVfx` | Niagara de fragments de verre / poussière (facultatif) |
| `FlaskGlassBreakScale` | **1** |
| `FlaskGlassBreakMaxLifetimeSeconds` | **1 s** |
| `FireIgnitionBurstVfx` | sélectionner le **vrai** FireBurst présent dans votre pack ; le nom exact ne peut pas être déduit d'une documentation externe |
| `FireIgnitionBurstScale` | **1** |
| `FireIgnitionBurstMaxLifetimeSeconds` | **1,2 s** |

La référence FireBurst est facultative et peut résider dans les Class Defaults du Blueprint **ou** dans les overrides de l'acteur placé. Le test d'assets vérifie les meshes du Blueprint, pas la présence obligatoire de cet effet facultatif.

Le burst d'embrasement n'est émis qu'au premier impact du catalyseur si une réaction de feu est constatée ; il n'est pas déclenché pour un `Fizzle` ou une réaction de fonte de glace. Sa durée maximale empêche toute répétition infinie lorsque le Niagara du pack boucle.

Dans `DA_GroundFire → Settings` :
- `GroundFireSystem` : Niagara `NS_FFX_Ground_Fire` **déjà assigné dans le Data Asset versionné et confirmé non nul en automation**.
- `GroundFireScale` = **1**, `GroundFireCoverage` = **0.9**, `FlameHeightCm` = **75**, `FlameDensity` = **1**, `SmokeDensity` = **0.5** ; `MaxFireCells` = **48**, `MaxFireLights` = **8**, `FireLightIntensity` = **650** (valeurs C++ par défaut, les valeurs sauvegardées dans le Data Asset peuvent différer).
- `bAutoBindKnownFireControls = true` : **FIREFX08** recherche `SpawnBox`,
  `FlameRate`, `SmokeRate`, `Tint` et vérifie leurs types dans
  `NS_FFX_Ground_Fire` ; les cinq anciennes liaisons manuelles
  (`SpawnAreaParameter`, etc.) peuvent donc rester `None`.
- Pour une case de **200 × 200 cm** : `GroundFireScale=1`,
  `GroundFireCoverage=1` ; si `SpawnBox` est de type Vector3,
  l'adaptateur envoie un volume XY de 200 × 200 cm en conservant son Z
  d'émission d'origine. Cela ne garantit pas que les particules ne
  débordent jamais après leur naissance.
- `FlameDensity=1` conserve `FlameRate` natif, `SmokeDensity=.5`
  multiplie `SmokeRate` natif par 0,5.
- `Settings → User Parameter Overrides` permet de modifier d'autres
  paramètres du pack **sans copier son Niagara**. Ajouter des entrées
  `ParameterName / ValueType / valeur` seulement après avoir lu leur
  type dans le bouton de diagnostic sur l'acteur placé dans `L_Dungeon`.
  Ce tableau d'overrides est appliqué en dernier.
- `FlameHeightCm` règle l'échelle verticale du composant ; si les
  émetteurs simulent en World Space, il faut aussi ajuster des entrées
  de taille du Niagara (par ex. `FlameSizeMin/Max`, après vérification).

Un clone GitHub **sans pack tiers** conserve le gameplay et le profil mais n'affiche pas les particules du pack. Un packaging intégrant les assets de ce pack exige de vérifier sa licence.

## 5. Test PIE à mener dans L_Dungeon

1. Lancer une flasque d'huile à **2 puis 4 cases** dans un couloir dégagé. Vérifier la bouteille sortant du côté de la troupe, sa parabole, sa disparition à l'impact, puis le splash et la flaque.
2. Réessayer en exploration puis en combat. Consommation d'**une seule** flasque, aucun changement de PA ou dégâts par la présentation.
3. Répéter avec le catalyseur : la bouteille doit partir, se briser puis réagir. Si la zone contient de l'huile, observer le burst et le « wouf » **une seule fois**.
4. Tester une réaction sans feu : **aucun burst ni « wouf » d'embrasement**.
5. Déclencher du feu sur plusieurs cases voisines : entendre **une seule zone sonore** de crépitement ; vérifier son extinction, son atténuation avec la distance et l'absence de fuite audio après sortie de la carte.
6. Varier `OilFlaskLaunchSpeedCmPerSec` (par exemple 900 → 1100 cm/s) puis la vitesse du profil catalyseur, pour vérifier les temps de vol différents. Un tir trop lent peut être physiquement impossible vers une cible éloignée.
7. Ouvrir **Window → Developer Tools → Output Log**, filtrer `[FLASK04]` : une ligne par flasque indique la cellule de départ, d'arrivée, le mesh choisi, la vitesse initiale et la durée du vol.

**Ce que ne prouvent pas les tests automatiques :** la qualité des sons sélectionnés, la visibilité réelle du mesh en premier plan, l'apparence du burst, la compatibilité des paramètres Niagara du pack, les occultations par plafonds et la qualité du mixage. Ce sont les points de recette PIE/GPU.

## 6. État de la réalisation

| Composante | Code existant | Asset / réglage requis |
|---|---|---|
| Bouteille d'huile visible | Oui ; secours versionné | Possibilité de choisir un mesh plus spécifique |
| Bouteille de catalyseur visible | Oui ; secours versionné | Profil optionnel pour personnaliser |
| Parabole sans air | Oui ; solveur physique et vitesse | Ajuster la vitesse selon distance souhaitée |
| Splash et flaque après impact | Oui, FLASK05 | `OilSurfaceMaterial` existant |
| Bruit de lancer | Oui, FLASK03 | Assigner `OilFlaskThrowSound` et `CatalystFlaskThrowSound` |
| Bris de verre + son | Oui, FLASK02/03 | Assigner les sons ; Niagara facultatif |
| « Wouf » + FireBurst | Oui, FIREFX04 | Assigner `FireIgnitionSound` et vérifier `FireIgnitionBurstVfx` |
| Feu au sol | Oui, profil versionné | Pack tiers installé et Niagara résolu |
| Crépitements | Oui, FIREFX01 | Assigner un vrai son bouclant |
| Collision de la bouteille en vol avec murs/plafonds | Non, hors périmètre FLASK01–05 | À développer séparément si nécessaire |
