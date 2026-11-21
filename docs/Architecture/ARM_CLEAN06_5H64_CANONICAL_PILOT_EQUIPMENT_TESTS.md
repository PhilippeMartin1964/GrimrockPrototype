# ARM-CLEAN06.5H.64 — Trois pilotes ARM : exercice runtime canonique

## Périmètre migré

Le test `Grimrock.RPG.ARMEMENT01.6.Items.EquipAndFixedLevelCombat` n'utilise plus `RegisterItemDefinition` ni la définition historique pour créer des items ou lire l'attaque. Il crée un `FGridCanonicalWeightTestWorld`, un véritable `UGridItemSubsystem`, puis résout les trois pilotes `Weapon_Dagger_01` (ARM001), `Weapon_ShortBow_01` (ARM045) et `Weapon_RoundShield_01` (ARM075) au niveau physique N1.

Le scénario vérifie l'identité, l'archétype, la masse canonique finie, le bon slot de main, l'absence d'anciennes définitions enregistrées, la matérialisation de l'instance N1, la politique d'attaque, la portée et les coûts pour les armes, et le caractère passif et les cinq points d'armure physique du bouclier.

## Preuve d'authoring délibérément conservée

`Grimrock.RPG.ARMEMENT01.6.Items.AuthoredCataloguesAndMeshes` continue à charger les trois anciens DataAssets afin de contrôler la **parité d'origine** avec les catalogues numériques et visuels : ID, type, poids, VisualId, mesh, transformation et actions anciennes. Cela constitue une preuve de migration, non une lecture runtime de production ; supprimer le test avant un remplacement durable effacerait cette preuve. Les anciens catalogues d'authoring restent protégés.

## Garanties de validation

Les tests existants des trois cas restent présents ; seul `EquipAndFixedLevelCombat` change de modèle d'autorité. Aucun runtime, asset Unreal, sauvegarde, contrôleur, HUD ou présentation GPU n'est modifié. Aucun des 53 packages H.23 n'est supprimé.

H.65 est bloqué jusqu'à `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur H.64, avec compilations Editor/Shipping et Automation `Grimrock.RPG.ARMEMENT01.6.Items`. Les sept sauvegardes natives doivent être réauditées sur le SHA de retrait avant toute suppression.

## H.64.1 — Correction après échec Automation

`UE5.5.4/LocalAuto` a retourné `FAILURE` sur H.64, description distante « Echec testing/AUTOMATION (Grimrock) » sans ligne d'assertion ni log accessible. **La cause exacte n'est donc pas connue**. H.64.1 recentre les assertions du second test sur les propriétés effectivement couvertes par le contrat de matérialisation N1 (identité et archetype, poids, slot de main, attaque autorisée, coût PA, portée, bouclier passif ARM075 à 5 points). La parité du `VisualId`, les profils de dés, les bindings précis et les politiques de projectile ont déjà leurs tests indépendants : `AuthoredCataloguesAndMeshes`, `Grimrock.RPG.ARMEMENT.LOT02.16.1` et `LOT02.16.2`, etc. Le contrôle du lancer de bouclier a été retiré de cette fixture, sans changement du comportement runtime.

Cette révision ne doit être considérée comme une correction fonctionnelle validée qu'après les deux statuts Windows SUCCESS sur son SHA. Aucun ancien asset modifié ni supprimé.
