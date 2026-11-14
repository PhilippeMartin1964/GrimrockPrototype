# ARM-CLEAN06.3M — Mesh et transformation de pickup canonique

**Base vérifiée :** ARM-CLEAN06.3L (`3908f1f`), compilation UE5.5.4 et Automation `Grimrock` vertes.

## Objectif

Les objets placés dans le niveau, déposés depuis le sac / le curseur et restaurés depuis l'état de donjon doivent utiliser le **même mesh et la même transformation visuelle** que `DA_ItemSystem`, sans relire les copies `WorldMesh`, `WeaponVisualCatalogue` du `DA_Weapon_*` lorsque l'`ItemId` est enregistré.

## Réalisation

`FGridResolvedItem::WorldVisualTransform` et `FGridCanonicalItemMeshReadView::WorldTransform` exposent la transformation `WorldTransform` du seul `DA_WeaponVisualCatalogue`. L'API `AGridItemActor::ApplyCanonicalWorldVisual(Instance)` s'applique **après** l'initialisation éventuelle du porteur historique et **avant** l'activation de Chaos quand le parcours le permet. Elle remplace le mesh et la transformation relative du composant uniquement pour une identité enregistrée ; les objets hors registre gardent intégralement leur comportement historique. Un ItemId connu mais à niveau invalide est vidé : aucun fallback depuis l'ancienne définition.

Les chemins migrés : `AddPlacedItemActor` (niveau du placement, fixé une seule fois) ; `TryDropItemInstanceAtCell` (instance déposée) ; `ApplyCurrentLevelRuntimeState` (acteur déjà existant, acteur de pickup créé pour état sauvegardé, acteur contenu dans un réceptacle). Le GUID, les quantités et les niveaux du runtime ne sont pas réécrits par la projection. Lorsqu'un pickup sauvegardé possède un `WorldMeshTransform`, ce transform réel est réappliqué **après** le choix du mesh, pour préserver son orientation et sa position sous Chaos.

Le code ne touche pas au choix des cellules, à la collision, à la rotation aléatoire initiale, à la masse physique, aux lumières, aux matériaux, aux recharges d'armure, aux sauvegardes ou aux `.uasset` ; l'image canonique et le format des instances restent transitoires. Les prévisualisations éditeur *sans instance* restent dans leur parcours historique jusqu'à la conversion du niveau ; aucune supposition N1 implicite.

## Preuve

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalWorldPickupMeshAndTransform` lit le vrai `DA_ItemSystem`, résout ARM014/ARM031/ARM075/ARM081 N1 et N20 et vérifie l'égalité des chemins de meshes, de `VisualId` et de la transformation, ainsi que l'absence de fallback pour N0 et le comportement conservateur hors registre. Les tests historiques de loot, chute, sauvegarde/reprise et physique sont conservés dans la suite complète `Grimrock`.

**Validation obligatoire** : `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` success sur le SHA exact. Le rendu réel du monde en PIE reste un contrôle visuel séparé de `-NullRHI`.
