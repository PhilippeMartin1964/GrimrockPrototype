# ARM-CLEAN06.4X.34 — Lecture canonique du visuel en vol : niveau physique strict

**Date :** 17 novembre 2026. **Parent :** `df36e7836d491b9db92f8c56f68f18af3ecbaa4f`.
**Périmètre :** lecteur de présentation de projectile et test Automation existant.
Aucun asset Unreal binaire, Blueprint ni SaveGame modifié.

## Défaut

`FGridCanonicalProjectileVisualReadModel::Resolve` passe par
`UGridItemSubsystem::ResolveItem`, lequel conserve une lecture de
famille non-ARM N1–N20 pour compatibilité 06.3V. Pourtant le projectile
lancé est une instance *physique* et les munitions enregistrées
`Item_Arrows_01` sont uniquement N0. Une flèche N1 pouvait
être déclarée `bResolved=true`, recevoir un mesh canonique et être
classée comme munition, même si les lecteurs de poses au monde la
refuseraient ensuite.

## Contrat

Pour les identités enregistrées, conserver `bCanonicalIdentity=true`
même si le niveau physique est invalide. Après résolution générale,
refuser toute famille `!bIsArmament` à niveau différent de N0
**avant** d'exposer `bResolved`, `bIsAmmunition`, `WorldMesh`
et la politique de vol. ARM N0 reste déjà refusé par `ResolveItem`.
Aucun fallback vers le mesh d'une ancienne définition.

Le lecteur de famille N1 non-ARM n'est **pas supprimé** :
il reste disponible là où le contrat 06.3V l'exige.
Les vrais vols de munitions N0 et d'ARM N1–N20 conservent leur
apparence, orientation et paramètres exacts.

## Régression

Étendre `Grimrock.RPG.ARM.CLEAN06_3.UnlevelledAmmoFlightMesh` :
N0 et WorldMesh identiques au catalogue, N1 non-ARM encore résolu
par `ResolveItem` mais **jamais** par le lecteur de vol physique ;
conserver les contrôles négatif, GUID manquant, hors registre.
Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalProjectileVisualByActualLevel`
conserve le contrôle des trois ARM N1/N20.

Exiger `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS
sur le SHA final. Ce contrôle `-NullRHI` n'est ni PIE/Chaos
ni une autorisation de migration native.

**ARM-CLEAN06.4 partielle ; 06.5 BLOQUÉE ; anciens assets préservés.**
