# ARM-CLEAN06.5H.63 — Projectiles Dagger N20 et fosse sans pointeur hérité

## Conversion du scénario Issue181

`Grimrock.Pit.Issue181.FlyingProjectilePassage` créait un projectile `Weapon_Dagger_01` N20 en passant encore le vieux `DA_Weapon_Dagger_01` à `AGridLevelRuntimeActor::SpawnThrownItemProjectile`. Ce pointeur n'était déjà plus une autorité de jeu : lorsque `GetGameInstance` contient `UGridItemSubsystem` et que l'ItemId est enregistré, le runtime résout son profil canonique et ignore l'ancienne définition. H.63 passe donc explicitement **nullptr**.

Le test exige la résolution `ResolveItem(Weapon_Dagger_01, 20)`, la politique de lancer numérique authentique, et vérifie qu'un acteur Dagger existant dans la scène ne conserve aucun pointeur d'ancienne définition. L'include ancien `Runtime/GridItemDefinitionAsset.h` est retiré du fichier de test.

## Invariants conservés

Les propriétés de la fosse, l'initialisation du vrai projectile dans le monde Chaos, sa trajectoire, son transfert au niveau et la réapparition d'un unique pickup à l'emplacement attendu sont inchangés. Le GUID et le niveau N20 restent vérifiés ; aucun raccourci visuel ni modification du ciblage n'est introduit.

Aucune modification de gameplay, physique, Niagara, UMG, `.uasset`, `.umap` ou `.sav`. Les 53 packages historiques H.23 restent protégés.

H.64 interdit avant `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur ce SHA, avec compilations Editor/Shipping et tests `Grimrock.Pit.Issue181`. Les sept sauvegardes doivent encore subir un audit natif lié au HEAD avant suppression de packages.
