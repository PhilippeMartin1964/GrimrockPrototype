# ARM-CLEAN06.5H.40 — Dégâts physiques et équipement ARM075 sans ancien DataAsset

## Migration des trois fixtures

Les tests `Grimrock.RPG.ARMEMENT01.10.3.1` passent par un monde de jeu transitoire avec `UGameInstance`, `UGridItemSubsystem` et l'inventaire d'un vrai `AGrimrockPartyPawn`. `Weapon_RoundShield_01` est toujours N1 et son profil d'armure physique de 5 points est vérifié directement par `ResolveItem`.

Le chargement de `DA_Weapon_RoundShield_01`, l'enregistrement `RegisterItemDefinition` et l'ancien `UGridWeaponCatalogueAsset` ne participent plus à ces tests. L'instance d'équipement est préparée avec `ApplyItemDefinitionToInstance`, puis équipée via les vraies fonctions d'inventaire. Aucun second modèle n'est autorité pour le calcul.

## Couverture conservée

Les trois scénarios gardent : absorption de dégâts physiques avant HP, comparaison UI, GUID stable, usure d'armure, retrait vers le sac, rééquipement sans refill gratuit, copie d'état dans un second inventaire du même monde, réserve de classe indépendante et restauration de quatre points d'armure sans dépassement. Les chiffres N1 d'origine et les contrôles de combat restent inchangés.

## Validation

Aucun code runtime, `.uasset`, `.umap`, `.sav`, effet Niagara ou HUD n'est modifié. Les 53 packages historiques H.23 restent intacts. Ne pas publier H.41 tant que les deux statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur ce SHA ne sont pas SUCCESS, avec compilations Editor / Shipping et tests `Grimrock.RPG.ARMEMENT01.10.3.1`. Les sept sauvegardes natives doivent encore être réauditées avant suppression.
