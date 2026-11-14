# ARM-CLEAN06.3AO — Autorité canonique des mains dans les kits de départ

**Base :** 06.3AN (`3fe7ad0`) — compilation UE5.5.4 et suite
`Grimrock` vertes sur ce SHA avant intervention.

## Problème résiduel après 06.3AM

La création des kits utilisait bien le niveau N1–N20 et les métadonnées
statiques de `DA_ItemSystem` ; toutefois la décision d'autoriser
`MainHand` / `OffHand` et de réserver les deux mains restait
`UGridItemDefinitionAsset::CanEquipToSlot` et
`GetEffectiveHandUsage`. Une ancienne copie contradictoire
pouvait produire un kit non équipable, placer un bouclier dans la
mauvaise main ou ignorer la réservation d'un arc à deux mains.

## Résolution

`FGridStartingKitEquipmentPolicyReadModel` classe tout `ItemId`
enregistré selon le **vrai niveau fixe** et
`FGridResolvedItem.Weapon.HandPolicy` du registre :
- une arme `TwoHanded` choisie depuis l'une des mains est stockée
  physiquement en `MainHand` **une seule fois**, `OffHand`
  étant réservée virtuellement ;
- `MainHand` et `OffHand` suivent exclusivement
  `Weapon.CanEquipInHand` ; aucun ancien `HandUsage` ou
  `CompatibleEquipmentSlots` ne peut autoriser un emplacement interdit ;
- un `ItemId` connu invalide échoue sans secours historique ;
- les munitions canoniques ne deviennent pas équipables ;
- les identités pas encore inscrites dans le registre restent
  provisoirement sur la politique historique jusqu'à 06.4/06.5.

`FRPGStartingEquipmentService::TryBuildKit` utilise cette vue pour
vérifier les conflits et réserver les mains **avant de publier** ses
copies transactionnelles. L'interface, la quantité, les GUID, le
choix de kit, la sémantique SaveGame et les statistiques de combat ne
sont pas modifiés. Aucune modification de `.uasset` ou `.umap`.

## Vérifications

Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitEquipmentHands`
charge la vraie racine et confronte ARM045 (arc, deux mains),
ARM075 (bouclier main secondaire) et ARM014 (épée principale)
en N4/N6/N7 à des définitions obsolètes volontairement fausses.
Il prouve la main physique, l'absence de double GUID, les deux
ordres de conflits arc/bouclier, l'atomicité du refus, la coexistence
épée/bouclier et le maintien des identités non enregistrées.

Suites historiques conservées : `Grimrock.RPG.ARMEMENT01.9.StartingKits.*`,
`Grimrock.RPG.ARMEMENT01.9.SixClassKits.*`, `Grimrock` complète.

**Recette obligatoire sur SHA :** `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` success. Rendu PIE non prouvé par `-NullRHI`.
Aucune chaîne visible joueur n'est créée :
`UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
