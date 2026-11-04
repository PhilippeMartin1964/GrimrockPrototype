# ARMEMENT01.17.1.4 — Refonte du rendu de l’huile au sol

**4 novembre 2026 — Refonte de la présentation, à valider sous UE 5.5.4.**

## Pourquoi l’ancienne réalisation était incorrecte

La capture utilisateur montrait de larges rectangles blancs, flottant au-dessus du sol, avec les joints des cellules nettement apparents. L’implémentation ajoutait un plan `Plane` au moyen de `CombatOilSurfaceISM`, puis avait tenté successivement un décalage de 2 cm et un décalage calculé depuis l’épaisseur du mesh. Or **le plancher logique est construit à Z = 0** par `AGridLevelRuntimeActor::AddFloor()`. Une surface plane ne peut jamais épouser les reliefs et joints du sol.

Les anciens tests `-NullRHI` vérifiaient seulement l’existence d’instances, sans vérifier l’image réellement produite. Le correctif d’option « Used with Instanced Static Meshes » ne répondait pas à ce défaut de conception.

## Nouvelle architecture : projection sur les dalles réelles

- **Aucun mesh ajouté à la flaque.** Suppression de `CombatOilSurfaceISM`, `SurfaceOverlayMesh`, `OilSurfaceHeightOffset` et du calcul `FloorTopZ`.
- Les sols restent des instances `FloorISM`, positionnées à **Z = 0**, avec leur matériau de pierre intact ; `SetReceivesDecals(true)` autorise les projections.
- `UDecalComponent` projette le matériau `MI_Surface_Oil` directement sur les surfaces 3D du sol : la coloration et la rugosité suivent les reliefs, sans plan flottant ni décalage vertical.
- Le volume de projection traverse Z = 0 ; **sa profondeur n’est pas une hauteur de flaque**. La position locale des décalques reste exactement sur le plan de référence Z = 0.
- Les cases d’huile voisines sont regroupées en rectangles ne contenant que des cases d’huile. Une même projection peut couvrir plusieurs cases, évitant les contours artificiels à chaque joint.
- La silhouette est irrégulière et douce : un `SphereMask` aux contours perturbés par un bruit procédural. Teinte brun-noir et **rugosité très faible** pour foncer localement les pierres et conserver des reflets.
- Le matériau maître `M_Surface_Oil` passe au domaine **Deferred Decal**, avec mélange translucide et sorties BaseColor/Roughness/Opacity. `MI_Surface_Oil` conserve son parent et son identité de DataAsset.
- Les `UDecalComponent` sont créés et supprimés uniquement aux changements d’état ; ils n’ont pas de Tick. Ils ne sont pas sauvegardés.

L’autorité de combat **RPG03.6**, les consommables, les durées, les réactions et les sauvegardes ne sont pas modifiés. La présentation est reconstruite lors de l’application, de la réaction, de l’expiration, du chargement et du changement de niveau.

## Conversion native du matériau — sans opération manuelle

Le commandlet Unreal `GridOilSurfaceMaterialAuthoring -ConvertToDecal` reconstruit **uniquement le graphe de `M_Surface_Oil.uasset`**, le convertit en Deferred Decal et le sauvegarde. `MI_Surface_Oil.uasset` et tous les autres assets sont conservés.

L’agent Windows déclenche cette conversion seulement si le test `Grimrock.RPG.ARMEMENT01.17.1.4.ProductionMaterial` est **le seul test échoué**, et si le worktree dédié est propre. Il refuse tout autre fichier modifié, vérifie le dépôt et le commit, crée un commit Git LFS limité à `M_Surface_Oil.uasset` et pousse sans forcer `master`. Le commit d’asset sera ensuite validé par la suite complète.

L’ancien script de réparation `RepairOilInstancedMaterialUsage.ps1` et le contrôle `InstancedMaterialUsage` sont supprimés, puisqu’aucun mesh instancié n’est désormais utilisé pour l’huile.

## Tests et limites

- `OilCellProjection` : projection, regroupement des cases voisines, matériau et ancrage exact à Z = 0.
- `NoFloatingOverlay` : absence de plan et profondeur de projection autour du niveau de référence.
- `ReactionsAndConversions` : feu, conversion et remplacement par eau.
- `Expiration` : la flaque reste pendant la durée prévue puis disparaît.
- `SaveLoadRebuild` : sérialisation de l’état et reconstruction.
- `ProductionMaterial` : matériau natif Deferred Decal, liaisons du graphe et instance de production.
- Intégration combat/hotbar : utilisation réelle de la flasque jusqu’à l’existence d’une projection.

**L’acceptation visuelle exige une scène rendue avec GPU/RHI**. Les tests `-NullRHI` ne certifient ni les couleurs ni les reflets ni la qualité des contours. Après validation technique, il faudra obtenir une capture du vrai donjon sous les torches et examiner l’absence de surfaces blanches, de flottement et de joints de cellules.
