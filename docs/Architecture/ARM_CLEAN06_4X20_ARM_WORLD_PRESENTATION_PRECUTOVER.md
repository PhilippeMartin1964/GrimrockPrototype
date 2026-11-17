# ARM-CLEAN06.4X.20 — Préparer la présentation physique canonique des ARM

**Date :** 17 novembre 2026. **Parent :** 0e8da96346ed897b5844a340af8e65f3ff6ef061. **Statut : cutover C++ partiel, sans réécriture d'assets Unreal.**
Références : ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md et ARM_CLEAN06_4X19_FINAL_LEGACY_DEPENDENCY_AUDIT.md.

## Cause du découpage

La revue C++ confirme que le profil ARM du catalogue visuel canonique fournissait uniquement le mesh, le transform et le projectile. Son résolveur ne projetait pas encore les effets persistants, le scintillement ni la politique de pose et de masse Chaos qui restent dans les anciennes définitions. Une bascule aveugle des 44 objets libres du contenu réel aurait pu effacer ces effets, changer l'inclinaison des armes et rouvrir la régression PIT02.

Ce jalon répartit sans duplication la présentation physique : le profil FGridCanonicalWeaponVisual possède LightEmitter, VisualEffects (Niagara persistant, pas les VFX d'attaque) et WorldSparkle ; **l'archétype de DA_CanonicalWeaponCatalogue possède bUseItemWeightAsWorldPhysicsMass et WorldPhysicsInitialTiltDegrees**. Plusieurs ARM peuvent partager un VisualId sans imposer le même comportement physique. La masse numérique demeure exclusivement le WeightKg de la définition de l'item. Les attaques, sons et Niagara de frappes restent liés à DA_ActionPresentationCatalogue. Aucun profil binaire existant n'est réécrit ; ses nouveaux champs prennent des valeurs par défaut qui **ne prouvent aucune parité avec les anciennes armes**.

UGridItemSubsystem::ResolveItem projette cette configuration dans FGridResolvedItem transitoire pour les ARM N1–N20. Les non-ARM continuent à lire UGridNonArmamentDefinitionAsset.

## Chemin de placement progressif

Dans AddPlacedItemActor :

- Un ARM **actuel** portant CanonicalItemId **et un ancien ItemDefinition non nul** conserve exactement son chemin historique d'initialisation et d'effets, avec mesh/niveau canoniques. **Aucun des 44 loose du jeu n'est rebasculé dans ce jalon.**
- Un ARM canonique **sans ancien pointeur** peut désormais être initialisé avec InitializeFromCanonicalItem ; GUID, ItemId, quantité et N1–N20 sont prévalidés. Ce chemin est prévu pour les nouveaux placements canoniques et les copies transitoires d'automatisation ; sa parité de rendu avec les 22 armes existantes reste subordonnée à la migration native des champs du VisualCatalogue.
- Un non-ARM connu conserve son chemin canonique actuel.
- Un ItemId hors registre conserve provisoirement sa voie historique (quatre loots actifs sont encore hors registre).
- ARM N0, référence contradictoire ou GUID/quantité invalides : refus avant apparition de l'acteur.

## Preuves automatisées ajoutées

1. Grimrock.RPG.ARM.CLEAN06_4X20.ArmWorldPresentationProjection : duplication **transitoire** du vrai DA_ItemSystem et de son VisualCatalogue, application de valeurs non triviales à un profil cloné, résolution N1 et N20 et vérification de la lumière, Niagara, sparkle et physique. Le DataAsset natif n'est pas modifié.
2. Extension de Grimrock.RPG.ARM.CLEAN06_4X8.RealLooseItemCanonicalNonArmCutover : sur une copie en mémoire d'un placement ARM réel, suppression du pointeur historique et test d'apparition à N1 et N20, GUID/identité/niveau/mesh/Niagara/lumière inchangés. Le chemin du placement ARM avec son pointeur d'origine reste explicitement testé ; refus ARM N0 et source contradictoire maintenus.

La compilation et la suite de tests Grimrock sont à contrôler par l'agent Windows sur le SHA publié. NullRHI ne valide pas le rendu PIE ou Chaos.

## Suites et barrières de sécurité

**06.4 non terminée ; 06.5 BLOQUÉE.** Avant de généraliser le chemin pointerless aux vrais niveaux, migrer les champs persistants des 22 anciens ARM vers les 22 profils du catalogue visuel **dans Unreal** avec manifeste exact, copies SHA-256, parité de valeurs (lumière, Niagara, sparkle, tilt, masse), réouverture de packages et PIE. Ce travail est réservé à une intervention native ponctuelle, pas à une nouvelle enquête Codex générale. Ne pas relancer W/X.5 ni les migrations X.7–8 déjà validées.

Ensuite seulement : restauration des ARM dans les réceptacles, éditeur/palette, quatre loots hors registre et dix objets d'alchimie, copies de SaveGame v27, puis suppression 06.5 et recette 06.6. Aucun SaveGame, WBP, niveau ni asset .uasset touché dans X.20.
