# ARM-CLEAN06.5H.57 — Dix attaques de mêlée N1 depuis ItemSystem

## Objectif

`Grimrock.RPG.ARMEMENT.LOT02.16.1.MeleeN1Execution` utilise désormais `UGridItemSubsystem` pour les dix armes physiques N1 : ARM001, ARM002, ARM003, ARM013, ARM014, ARM015, ARM020, ARM025, ARM031 et ARM038. Chaque monde de test reçoit son propre `FGridCanonicalLootTestScope`, attaché après la construction du monde et détaché avant sa destruction. Les identités stables sont `Weapon_<AssetName>_01`; aucun chemin `DA_Weapon_*` n'est ouvert dans ce scénario.

Pour chaque arme, le test exige le bon `ArchetypeId`, le niveau physique N1, l'autorisation explicite de produire une attaque dans le slot concerné et la politique de deux mains. L'instance équipée conserve un GUID et une propriété déterministes, sans enregistrement `RegisterItemDefinition` hérité.

## Non-régressions

Les dix transactions traversent toujours le véritable `UGridTurnManagerComponent`, le monstre, le HUD et le journal : action disponible, coût AP positif, activation, exécution, trace de jet d'attaque, montant facturé une fois, identités et slot du coup, absence de guérison fictive, puis rejet sans AP ni nouveau log.

Les autres tests MON12 de double maniement, audio, flèches et projectiles ne sont pas modifiés. Aucun gameplay, Blueprint, `.uasset`, `.umap`, `.sav`, effet ou son de production modifié. Les 53 packages H.23 restent protégés.

## Barrière de validation

H.58 ne peut être publié que si `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sont SUCCESS sur ce SHA, avec compilation UE5.5.4 Editor / Shipping et automation `Grimrock.RPG.ARMEMENT.LOT02.16.1.MeleeN1Execution`. Les sept SaveGames privés restent à réauditer nativement avant toute suppression.
