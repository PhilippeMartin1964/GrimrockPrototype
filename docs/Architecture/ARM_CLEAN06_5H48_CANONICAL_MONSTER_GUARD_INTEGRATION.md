# ARM-CLEAN06.5H.48 — Parade et blocage MON11 depuis ItemSystem

## Conversion de la fixture d'intégration combat

`Grimrock.RPG.ARMEMENT.LOT02.19.3.MonsterDamageRuntimeIntegration` n'ouvre plus les anciens `DA_Weapon_RoundShield_01` et `DA_Weapon_MainGauche_01`. Chaque scène isolée MON11 attache un `FGridCanonicalLootTestScope` au `UWorld` réel avant d'exécuter l'attaque de monstre. Les deux identités `Weapon_RoundShield_01` (ARM075 N1) et `Weapon_MainGauche_01` (ARM013 N1) sont résolues depuis le `UGridItemSubsystem` de ce monde.

L'instance d'OffHand ne porte que le GUID, l'identité, le niveau physique N1 et l'emplacement. L'équipement n'enregistre aucune définition historique : `FGridEquippedGuardResolver` retrouve les propriétés canoniques de garde depuis le `GameInstance`.

## Invariants inchangés

Les deux scénarios conservent la véritable attaque du monstre, son composant `CombatComponent`, ses tirages déterministes, quatre dégâts absorbés sur cinq points de bouclier, aucune perte de HP pendant le blocage, la parade validée par callback une seule fois, aucun HP ni armure retirés après parade. L'intégralité des autres tests MON11 reste inchangée.

Les scopes sont détachés avant destruction de chaque monde. Aucun changement runtime/monstre, `.uasset`, `.umap`, `.sav`, animation, effet ou ciblage. Aucun des 53 packages historiques H.23 n'est supprimé.

H.49 exige au préalable les statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA H.48 (compilations Editor/Shipping et tests `Grimrock.RPG.ARMEMENT.LOT02.19.3` et `Grimrock.Monsters.MON11`). Les sept sauvegardes restent à valider nativement avant toute suppression.
