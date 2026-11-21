# ARM-CLEAN06.5H.64 — Trois pilotes ARM : exercice runtime canonique

## Périmètre migré

Le test `Grimrock.RPG.ARMEMENT01.6.Items.EquipAndFixedLevelCombat` n'utilise plus `RegisterItemDefinition` ni la définition historique pour créer des items ou lire l'attaque. Il crée un `FGridCanonicalWeightTestWorld`, un véritable `UGridItemSubsystem`, puis résout les trois pilotes `Weapon_Dagger_01` (ARM001), `Weapon_ShortBow_01` (ARM045) et `Weapon_RoundShield_01` (ARM075) au niveau physique N1.

Le scénario vérifie l'identité, l'archétype, le `VisualId`, la masse canonique finie, le bon slot de main, l'absence d'anciennes définitions enregistrées, la matérialisation de l'instance N1, la politique d'attaque, les dés, la portée et les coûts pour les armes, et le caractère passif/armure positive du bouclier.

## Preuve d'authoring délibérément conservée

`Grimrock.RPG.ARMEMENT01.6.Items.AuthoredCataloguesAndMeshes` continue à charger les trois anciens DataAssets afin de contrôler la **parité d'origine** avec les catalogues numériques et visuels : ID, type, poids, VisualId, mesh, transformation et actions anciennes. Cela constitue une preuve de migration, non une lecture runtime de production ; supprimer le test avant un remplacement durable effacerait cette preuve. Les anciens catalogues d'authoring restent protégés.

## Garanties de validation

Les tests existants des trois cas restent présents ; seul `EquipAndFixedLevelCombat` change de modèle d'autorité. Aucun runtime, asset Unreal, sauvegarde, contrôleur, HUD ou présentation GPU n'est modifié. Aucun des 53 packages H.23 n'est supprimé.

H.65 est bloqué jusqu'à `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur H.64, avec compilations Editor/Shipping et Automation `Grimrock.RPG.ARMEMENT01.6.Items`. Les sept sauvegardes natives doivent être réauditées sur le SHA de retrait avant toute suppression.
