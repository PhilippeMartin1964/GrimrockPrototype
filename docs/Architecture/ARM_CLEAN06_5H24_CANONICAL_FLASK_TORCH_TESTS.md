# ARM-CLEAN06.5H.24 — Tests FLASK04 et LIGHT_CONFIG02 convertis au registre canonique

La flaque d'huile fonctionne dans le **bon exécutable Shipping**, et la prévisualisation jaune/verte reste un défaut de présentation différé (issue #221). Cette intervention poursuit seulement ARM-CLEAN06.5.

Deux tests de production n'exigent désormais plus les anciens assets `UGridItemDefinitionAsset` :

- `Grimrock.RPG.FLASK04.VersionedProjectileMeshes` : l'huile et le catalyseur sont cherchés par `ItemId` (`Item_Flask_Oil`, `Item_SurfaceCatalyst_01`) dans la seule racine `CanonicalStage/DA_ItemSystem`; leurs identités et leur famille non-armement sont vérifiées. Les assertions de présence des meshes de repli de l'acteur runtime et du vrai Blueprint sont conservées, sans modification d'asset ou d'effet.
- `Grimrock.Items.LIGHT_CONFIG02.SinglePointLightAuthority` : la torche est recherchée par `Item_Torch` dans cette racine. Les propriétés de lumière (usage, intensité, rayon et teinte), et toutes les vérifications du schéma de `FGridLightEmitterConfig`, restent actives.

**Non destructif :** aucune altération des 48 anciennes définitions, 2 catalogues historiques, 2 Readables protégés ou du redirecteur ThrowingKnife. Le rapport H.23 fournit l'inventaire statique privé pour achever la neutralisation des autres consommateurs, mais ne vaut pas autorisation de suppression.

La validation du lot requiert `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur son SHA précis (compilation Editor, Shipping et Automation). Le retrait natif devra faire l'objet d'un autre lot après sauvegarde privée et nouvelle preuve des sept sauvegardes.
