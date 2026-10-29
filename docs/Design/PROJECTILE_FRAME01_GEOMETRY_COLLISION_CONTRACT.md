# PROJECTILE-FRAME01 — Convention universelle du projectile
## Contrat géométrique
- Monde Unreal : +X Est, +Y Nord, +Z haut. La direction du tir provient des positions monde de la caméra et de la cible, **jamais** de l'orientation du mesh placé au sol.
- Acteur projectile : son axe +X est toujours tangent à la vitesse via `bRotationFollowsVelocity`.
- Mesh projectile directionnel : `UGridItemDefinitionAsset.ProjectileMeshForwardAxis` déclare **l'axe de la pointe du StaticMesh IMPORTÉ dans Unreal**, pas l'axe dans Blender. Le runtime aligne cet axe vers +X de l'acteur, quelle que soit l'orientation du groupe.
- Visuels `Stable` et munitions de tir : alignement automatique. Visuels `Spin`/`Tumble` : animation héritée, non modifiée.
- Les paramètres `ThrowVisualRelativeRotation` et `ThrowVisualRelativeScale` demeurent pour les visuels historiques ; pour un projectile stable directionnel, `ProjectileMeshForwardAxis` est l'autorité de rotation.
- L'orientation d'un pickup au sol et celle d'un projectile en vol sont **indépendantes**. Le pivot sur la pointe n'empêche pas le déplacement (même s'il peut affecter l'apparence).

## Politique collision et récupération
- Tous les projectiles ignorent leur lanceur ; les flèches ignorent aussi les acteurs de type `AGridItemActor` déjà ramassables, sans ignorer les murs, portes et monstres.
- L'impact physique est journalisé avec l'acteur rencontré, le point et la distance parcourue.
- Conversion : privilégier cellule cible à proximité du monstre, sinon cellule de l'impact, sinon dernière cellule réellement parcourue ; la cellule source n'est qu'un ultime recours et génère un avertissement `[GridProjectile] SourceFallback`.
- La physique n'autorise ni second calcul de dégâts ni seconde consommation de munition.

## Asset existant `DA_Item_Arrows_01`
Dans le Static Mesh Editor Unreal, la flèche placée pointe actuellement vers le Nord (+Y). Régler :
`ProjectileMeshForwardAxis = PositiveY`.
Ne plus réexporter le modèle dans Blender pour « corriger » la trajectoire. Vérifier en PIE les directions N/E/S/W ; le vol devrait suivre la cible et la pointe précéder le fût.
Le pickup reste `WorldMesh = SM_Item_Arrows_01`; collision simple et gravité restent à vérifier séparément si l'objet devient invisible au sol.

## Validation locale UE5.5.4
Fermer l'éditeur puis :
```powershell
cd D:\Development\GrimrockPrototype
git fetch origin master
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.Monsters.MON11.Presentation'
```
Le test automatisé teste l'orientation dans quatre directions, mais **la physique en PIE doit encore être observée** : lire les lignes `[GridProjectile] Hit` et `SourceFallback` pour identifier un obstacle éventuel, et tester les tirs manqués, le ramassage, les sauvegardes.

## PROJECTILE-FRAME02 — Monstres et sorts
Un repère commun `GridProjectileGeometry.h` définit les six axes du mesh
importé dans Unreal et les aligne sur le +X de l'acteur projectile.
`AGridThrownItemActor` (flèches / items physiques) et
`AGridCombatProjectileActor` (monstres et sorts visuels) utilisent
exactement la même fonction. Aucun calcul de dégâts ni collision de gameplay
n'est transféré aux projectiles visuels.
Propriétés d'authoring : `ProjectileMeshForwardAxis` dans les définitions
d'items et d'attaques monstres, `MeshForwardAxis` dans les profils de sorts.
Les anciennes valeurs par défaut +X maintiennent la compatibilité. Les
offsets de rotation historiques restent appliqués.
Tests d'orientation : 6 axes x 4 orientations, dans
`Grimrock.Monsters.MON17.3.2.ProjectileTrajectory`.
Validations locales additionnelles : filtres
`Grimrock.Monsters.MON17.3.2` et `Grimrock.Magic.MON18.6`.

## PROJECTILE-FRAME03 — Régression porte secrète derrière le groupe (29.10.2026)
Recette réelle : groupe (28,22) vers le Nord, monstre (28,26), porte secrète
sur (28,22) Edge Sud. Impact journalisé sur
`BP_GridSecretDoor_C_3` dès `(5700,4500.1,93.43)`, après seulement 9,7 cm.
La cellule (28,22) a son centre à `(5700,4500)` : collision intempestive
dans la cellule de départ, **sans rapport avec l'orientation visuelle du mesh**.

`FGridProjectileGeometry::IsRearBoundaryDoor` identifie précisément
la porte située sur la frontière arrière du tireur, que celle-ci soit
définie sur sa cellule ou sur la cellule voisine. Cette porte est ajoutée
au `MoveIgnoreActors` du projectile avant sa première mise à jour physique.
La vérification est partagée par tous les projectiles physiques d'items :
flèches et armes jetées. Les projectiles des monstres et des sorts sont
déjà visuels sans collision et ne sont pas concernés par ce filtrage.
La collision avec une porte **devant le tir** n'est pas désactivée.

Journal attendu lors du tir décrit :
`[GridProjectile] IgnoreRearDoor Item=Item_Arrows_01 Door=BP_GridSecretDoor_C_3 SourceCell=(28,22) DoorCell=(28,22) DoorEdge=South Heading=North`
(les valeurs d'arête numériques sont affichées dans le journal).
`[GridProjectile] Hit` ne doit plus désigner cette porte à 9,7 cm.
La porte elle-même et sa collision 3D ne sont pas modifiées.

Tests : quatre directions × deux conventions de propriété de frontière,
et régression explicite (28,22 Nord / arête Sud). Filtre LOT02 existant.
Observer en PIE que les portes réellement placées devant bloquent
encore les flèches, y compris les portes secrètes fermées.
