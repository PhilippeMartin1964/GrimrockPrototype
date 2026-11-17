# ARM-CLEAN06.4X.21 — Réceptacles : ARM initial canonique sans pointeur historique

**Date :** 17 novembre 2026. **Parent :** `476fe10700d3ace56c1416ab9b5a762ac5564cee`. **C++ et tests, sans .uasset ni SaveGame.**

## Contrat et scénario

ARM-CLEAN06.4X.19 a confirmé que les 18 contenus initiaux réels de réceptacles possèdent déjà leurs `CanonicalItemId` et leurs niveaux physiques, mais conservent encore leurs anciens `ItemDefinition`. X.20 permet la présentation persistante canonique d'un objet ARM à N1–N20, sans garantir encore l'égalité visuelle avec les anciens packages.

X.21 traite seulement un **contenu initial explicitement canonique (`bHasPersistedCanonicalIdentity`) dépourvu de pointeur historique**. Pour cette provenance uniquement, `AddContainedItem` ne fait plus `ResolveRuntimeItemDefinition` pour chercher puis réattacher un ancien `DA_Weapon`. Il valide ItemId, niveau et quantité, puis initialise l'acteur par `InitializeFromCanonicalItem`. Le GUID de l'objet contenu reste conservé lors du capture runtime ; le spawn est refusé si l'identité canonique ne résout pas.

Les **ARM actuellement authorés avec un ancien `ItemDefinition` non nul conservent la branche visuelle historique**. TorchHolder/Gardien, les 18 items initiaux existants, les règles d'acceptation, les événements, la physique `PhysicalAtHit` et le parcours de sauvegarde v27 ne sont pas convertis.

La requête `ContainsItemTag` s'appuie désormais d'abord sur les tags du registre pour un ItemId enregistré, et ne retombe sur les tags d'une ancienne définition que pour une identité réellement hors-registre. Aucune traduction implicite des anciens `EGridItemType` n'est inventée.

## Tests ajoutés

`Grimrock.RPG.ARM.CLEAN06_4X21.PointerlessCanonicalArmInitialReceptacle` utilise un monde transitoire, le vrai registre du jeu et des réceptacles avec contenus initiaux `Weapon_Dagger_01` à N1, N20 et N0 sans aucune ancienne définition. Pour N1/N20 : présence de l'objet et de son acteur canonique sans pointeur, ID/niveau/GUID, mesh, Niagara permanent, lumière, tags et capture dans `FGridRuntimeReceptacleState`. Pour N0 : rejet sans inventer N1. Aucun asset ni SaveGame écrit.

Les tests précédents des anciens réceptacles restent dans la suite `Grimrock`; compilation et exécution par `VALIDATION-AUTO01` sur le SHA du commit.

## Reste

1. Migration native contrôlée des 22 profils ARM avant suppression de leurs pointeurs des 18 contenus historiques.
2. Restauration des ARM canoniques contenus : `GridLevelRuntimeActorPersistence.cpp` utilise encore une ancienne définition pour des ARM sauvegardés ; à convertir sans dégrader leurs effets.
3. `ContainsItemType` dépend encore de l'ancien enum et d'une définition : nécessite une politique sémantique explicite de classification canonique.
4. Éditeur et palette, quatre loots hors registre, sauvegardes v27 sur copies : non traités.

**06.4 partielle ; 06.5 bloquée.** Le jalon X.21 ne certifie pas la parité de rendu PIE des ARM natifs.
