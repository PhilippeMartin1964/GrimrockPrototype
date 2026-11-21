# ARM-CLEAN06.5H.62 — Lancer physique des trois ARM N1 depuis le sac

## Migration de la seconde fixture MON12

Le test `Grimrock.RPG.ARMEMENT.LOT02.16.3.RealThrowingN1` ne charge plus aucune ancienne définition `DA_Weapon_*`. Les trois armes (`Weapon_ThrowingKnife_01`, `Weapon_Javelin_01`, `Weapon_Dart_01`) sont résolues N1 depuis `UGridItemSubsystem`.

Le scénario de lancer depuis le sac utilise désormais le véritable `AGrimrockPlayerController` de production, possédant `AGrimrockPartyPawn`. La source dans le sac est matérialisée par `ApplyItemDefinitionToInstance`. Le raccourci numéro 2 utilise `SetCharacterCombatHotbarBindingFromItem` et doit porter `FGridCombatHotbarBinding::MakePhysicalThrowItemActionId`. L'activation du raccourci démarre la **visée** sans dépenser d'objet, ni remplir le curseur ordinaire.

Un `FHitResult` explicite à 240 cm, au-delà de la portée de main, fait exécuter `HandlePhysicalThrowAimingHit`. Le test exige un projectile physique unique, de même ItemId/N1 avec un GUID nouveau, et la consommation d'une seule unité de l'inventaire. Il vérifie aussi que le ciblage se ferme et que le lancement physique n'invente ni coût en PA de combat ni entrée de journal d'attaque. Le chemin depuis une arme équipée (H.61) conserve au contraire le GUID de l'arme transférée vers son projectile.

## Sémantique corrigée

L'ancien test utilisait `BuildInventoryCombatActionDefinition` puis demandait une action `QuickItem` instantanée. Ce n'est plus le chemin canonique pour les armes physiques du sac. Aucune logique de gameplay n'est changée : ce jalon adapte uniquement les tests à la visée effectivement utilisée par le jeu. Les contrôles de lancer équipés, projectile récupérable, consommation unitaire et niveau physique sont préservés.

Aucun runtime, WBP, `.uasset`, `.umap` ni `.sav` modifié. Aucun des 53 packages historiques H.23 n'est supprimé.

H.63 est bloqué jusqu'à `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur le SHA H.62, avec compilations Editor/Shipping et test MON12 `Grimrock.RPG.ARMEMENT.LOT02.16.3.RealThrowingN1`. Audit des sept sauvegardes natives obligatoire avant suppression.
