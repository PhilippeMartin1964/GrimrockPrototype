# ARM-CLEAN06.5H.67 — Test du lancer physique Shuriken depuis l'autorité canonique

## Objet du lot

Le test `Grimrock.Items.ITEM_THROW_CLEAN01` utilisait une instance artificielle `UGridItemDefinitionAsset` pour vérifier `IsPhysicallyThrowable` et `IsCombatThrowable`, bien que le Shuriken réel N0 relève désormais de `DA_ItemSystem`. Le lot retire cette **fixture historique synthétique** et teste le vrai `FGridPhysicalThrowStrengthReadModel` avec `UGridItemSubsystem`, l'instance physique `Shuriken` et `nullptr` comme ancienne définition.

Les assertions vérifient le Shuriken à N0, sa politique de lancer canonique, son poids identique dans le résolveur physique, la possibilité de lancer avec Force suffisante, puis les refus de Force nulle, niveau N1 invalide et ItemId absent de la racine. Les identités reconnues mais invalides ne peuvent se rabattre sur l'ancienne autorité. La disponibilité de l'action de lancer récupérable reste testée par `GenericThrow.bCombatRecoverable` et reste indépendante de la contrainte physique.

Le contrôle de schéma `bThrowable` sur l'ancienne classe est maintenu tant que cette classe existe ; il devra être retiré au cutover final avec les tests encore dépendants de `UGridItemDefinitionAsset`.

## Limites et garanties

Aucun changement de runtime, gameplay, IA, projectile, caméra, Widget, assets Unreal, sauvegarde ou classe C++ exportée. Le test ne remplace ni la preuve d'authoring des anciennes données ni les assertions de combat et de ramassage d'autres scénarios. Les 53 packages de H.23 et les sept sauvegardes privées demeurent protégés.

Le passage à H.68 dépend des deux statuts Windows `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` à `SUCCESS` sur le SHA H.67 exact, incluant compilations Editor/Shipping et Automation `Grimrock.Items.ITEM_THROW_CLEAN01`. Une réussite `-NullRHI` ne constitue pas une recette visuelle PIE ni une autorisation de retrait d'asset.
