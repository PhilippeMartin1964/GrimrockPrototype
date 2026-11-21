# ARM-CLEAN06.5H.50 — Slot d'inventaire ARM002 N9 sans définition historique

## Test migré

`Grimrock.UI.WeaponTooltip04.InventorySlotProjection` n'ouvre plus `DA_Weapon_AssassinDagger_01` et n'appelle plus `RegisterItemDefinition`. La fixture `FGridCanonicalWeightTestWorld` crée un `UWorld` avec registre `DA_ItemSystem` configuré et un composant d'inventaire lié au monde. L'Assassin Dagger ARM002 N9 est résolu comme arme canonique et les deux instances N9 (survol) / N1 (équipée en MainHand) sont des valeurs physiques de `FGridItemInstance`.

Le widget d'inventaire temporaire peut retrouver `UGridItemSubsystem` depuis son composant parent. Son tooltip construit le profil réel N9, les dés d'attaque, le texte de fallback et la comparaison contre N1 sans jamais lire un ancien DataAsset d'armes.

## Assertions conservées

Validité de l'infobulle, niveau N9, ligne de dés, formule textuelle de fallback, scénario MainHand, formule de dés non numérique et texte de l'arme équipée. Aucun changement au code UMG ni au système d'armes de production.

Les autres tests de `GridUIWeaponTooltip04Tests.cpp` restent inchangés; leurs dépendances historiques sont traitées séparément.

Aucun runtime, `.uasset`, `.umap`, `.sav` modifié; les 53 packages H.23 demeurent. H.51 reste bloqué jusqu'aux statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` tous deux SUCCESS sur le SHA H.50, compilations Editor/Shipping et Automation `Grimrock.UI.WeaponTooltip04` comprises. Les sept SaveGames nécessitent toujours l'audit natif avant tout retrait.
