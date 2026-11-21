# ARM-CLEAN06.5H.59 — Armes magiques N1 sous ItemSystem

## Conversion

Le scénario `Grimrock.RPG.ARMEMENT.LOT02.16.4.MagicN1Execution` utilise les profils physiques canoniques de quatre armes : ARM061 `Weapon_ArcaneWand_01`, ARM062 `Weapon_FireWand_01`, ARM067 `Weapon_ElementalStaff_01`, ARM069 `Weapon_HolyScepter_01`. La fixture source valide le niveau N1, l'identité d'archétype, la politique deux mains et l'absence d'un type de munitions imposé.

Les deux scènes de test par arme (attaque acceptée et refus par mur) attachent un `FGridCanonicalLootTestScope` au monde MON12 et injectent une instance équipée N1 sans `RegisterItemDefinition` historique. Les chemins directs vers anciens `DA_Weapon_*` ont disparu **de ce scénario**.

## Invariants conservés

Le pipeline garde la recherche de l'action d'équipement, son type attaque magique à distance, la ligne de vue, l'absence de coût en objet consommable, le débit en PA et mana exactement une fois, la trace d'attaque et le maintien de l'arme en main. Un mur solide refuse l'attaque avec `PassageBlocked` sans PA, mana ni journal supplémentaire.

Les scénarios voisins MON12 (armes de lancer, passifs, audio) sont laissés inchangés. Aucun gameplay, Blueprint, `.uasset`, `.umap`, `.sav`, GPU, Niagara ou son de production n'est modifié. Les 53 packages historiques H.23 restent en place.

H.60 est bloqué jusqu'aux deux SUCCESS `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA H.59 (Editor, Shipping, `Grimrock.RPG.ARMEMENT.LOT02.16.4.MagicN1Execution`). Les sept SaveGames nécessitent toujours une validation native avant retrait.
