# FIREFX05 — Assets Grimrock versionnés, pack FireFX exclu

## Source de vérité

- **Versionnés sur GitHub** : `DA_GroundFire.uasset`, `BP_GridLevelRuntimeActor.uasset`, niveaux,
  profils VFX, sons dont Grimrock détient les droits de distribution, C++ et documentation.
- **Exclu** : `Content/F_FreeFlameFx_Pack/` (fichiers sources du pack tiers).
- Le Blueprint de runtime référence directement `DA_GroundFire`, et ce profil contient
  une référence **souple** (`TSoftObjectPtr`) vers le système Niagara du pack.
- Une référence souple peut exister sans son package cible dans le clone Git :
  le C++ refuse simplement de créer le système Niagara si l'asset externe manque.
  Le gameplay et les dégâts restent indépendants.
- L'ancien contournement `Saved/Config/GrimrockLocalVfx.ini` a été **supprimé** :
  il dupliquait les propriétés éditables du Blueprint et ne fonctionnait qu'en PIE.
- Le dossier `Effects/Fire/Local` conserve son chemin pour éviter une migration
  inutile de l'asset; son nom n'implique plus une exclusion Git.
- Le système de crépitements `FireCrackleLoopSound` appartient au profil versionné
  et le son « wouf », les sons de lancer/bris et le burst à `BP_GridLevelRuntimeActor`.

## Contraintes

Ne jamais ajouter au dépôt les `.uasset` et leurs dépendances du pack tiers.
Versionner les références d'assets Grimrock vers celui-ci est permis, mais **un clone
ne reproduira pas les flammes sans installer ce pack**. Les droits de redistribution
du pack dans un jeu final doivent être vérifiés séparément.

## Vérification

`Grimrock.RPG.FIREFX05.ProductionProfileWiring` charge **le vrai** Data Asset
versionné et la classe générée du Blueprint, vérifie le lien exact
et la présence de la référence souple du Niagara sans exiger le pack privé.
`UE5.5.4/LocalAuto` valide compilation et suite de tests en environnement Windows.
PIE/GPU reste obligatoire pour juger la taille, la couleur et le son du feu.

## Migration du README du pack

Le `README.txt` ajouté à la racine du dépôt décrit un pack qui annonce
`Content/FireFX` alors que l'exclusion Git du projet concerne
`Content/F_FreeFlameFx_Pack`. Cette différence doit être résolue à partir
des chemins d'assets visibles dans Unreal ; elle n'autorise pas à changer
silencieusement des chemins de références dans les .uasset.
