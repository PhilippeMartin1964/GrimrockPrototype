# ARM-CLEAN06.5H.54 — Kit dague N4 / bouclier N1 canonique

## Conversion

Le test `Grimrock.RPG.ARMEMENT01.9.StartingKits.DaggerAndShield` passe par le `FGridCanonicalWeightTestWorld` et un `UGridPartyInventoryComponent` rattaché au monde. Il résout la dague `Weapon_Dagger_01` N4 en MainHand et le bouclier `Weapon_RoundShield_01` N1 en OffHand, sans charger `DA_Weapon_Dagger_01` ou `DA_Weapon_RoundShield_01`.

L'ancienne règle de niveau implicite N0 de la définition est remplacée par un niveau de bouclier N1 explicite, conformément au contrat du registre physique. Le kit transient de test crée deux `FRPGStartingEquipmentEntry` canoniques sans pointeur historique.

## Contrôles

Les deux éléments doivent s'équiper atomiquement, rester dans leurs mains respectives avec des GUID valides différents, conserver N4 et N1, être possédés chacun une seule fois et préserver l'identité `Warrior_Defender`. Le bouclier conserve son profil de cinq points physiques, sans fausse réservation deux mains. La validation d'ownership et l'absence d'enregistrement de définitions historiques restent vérifiées.

`LegacyCreationUnchanged` et `RejectionIsAtomic` ne changent pas et seront traités dans un jalon distinct. Aucun code gameplay, `.uasset`, `.umap`, `.sav` ou package H.23 modifié.

## Barrière

H.55 requiert `UE5.5.4/SilentTask` ET `UE5.5.4/LocalAuto` SUCCESS sur le SHA H.54, avec Editor, Shipping et `Grimrock.RPG.ARMEMENT01.9.StartingKits`. Les sept sauvegardes doivent encore être validées nativement avant toute suppression.
