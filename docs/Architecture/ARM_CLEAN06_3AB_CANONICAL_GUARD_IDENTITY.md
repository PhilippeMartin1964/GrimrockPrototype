# ARM-CLEAN06.3AB — Identité canonique des réactions défensives

## Portée

`FGridEquippedGuardResolver` lit maintenant les identités enregistrées directement depuis le même `DA_ItemSystem` que le combat et l'armure. Le modèle en lecture seule `FGridEquippedGuardItemReadModel` classe une vraie instance de main secondaire à son **ItemId** et son **ItemLevel** effectifs : `ARM013` (main-gauche) est reconnu via `ArchetypeId`, `Kind=Weapon` et autorisation `OffHand`; les boucliers via `Kind=Shield` et autorisation `OffHand`. Les anciennes copies `ItemType`, `WeaponArchetypeId` et `bUseWeaponCatalogue` ne peuvent pas modifier ces décisions pour une identité canonique.

Le résolveur récupère le registre par le `GameInstance` du composant d'inventaire. Il ne requiert plus la présence d'un `DA_Weapon_*` pour un objet canonique réellement équipé. Un ItemId connu avec niveau invalide N0 ne réactive pas les copies. Les tests isolés sans monde, et les items encore hors `DA_ItemSystem`, conservent temporairement le lecteur historique jusqu'à 06.4.

Le préfiltrage des attaques (coup porté, non critique, physique, positif, au corps à corps), la chance de parade, le jet aléatoire du `TurnManager`, la trace des dégâts absorbés, la dette physique de bouclier et le combat log restent **strictement inchangés**. Une trace ne peut annoncer que l'absorption réelle par un bouclier équipé : aucun nouveau réservoir d'armure ni bonus n'est créé.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalGuardItemIdentity` vérifie le vrai ItemSystem ARM013/ARM014/ARM075/ARM081, niveaux N1/N20, face à une ancienne définition contradictoire ; identité du rôle, parades et boucliers, main incorrecte, N0 invalide et objets non enregistrés. Les tests d'intégration historiques `Grimrock.RPG.ARMEMENT.LOT02.19.3.MainGaucheParryEligibility` et `ShieldBlockActualAbsorption` restent actifs et inchangés.

Aucun `.uasset`, `.umap`, SaveGame, WBP ou texte UI n'est modifié. Le rendu des réactions réelles dans PIE reste un contrôle distinct de `-NullRHI`.

**Validation :** un seul commit atomique sur `master`, puis `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` tous deux verts sur son SHA exact.
