# ARM-CLEAN06.2AG — Vérifier l'identité d'une instance équipée avant une attaque

## Objectif

Une attaque doit être autorisée par l'instance réelle occupant un
emplacement d'équipement, et non par une chaîne `ItemId` ou
l'appartenance d'une définition au catalogue.

La nouvelle méthode non activée
`UGridItemSubsystem::ResolveAttackFromEquippedInstance`
réutilise exclusivement le sous-système canonique existant. Elle
prend la `FGridItemInstance` persévérée, l'`ActionId`, la main,
et un **constat d'identité** lu depuis l'emplacement effectif :
`ObservedSlotRuntimeObjectId`, `ObservedCharacterId` et
`ObservedCharacterIndex`.

Le contrôle exige :
- registre canonique présent, instance avec GUID et niveau N1–N20
  valides, quantité positive et usures d'armure non négatives ;
- `OwnerType=EquipmentSlot` et `EquipmentSlot` identique
  à la main demandée (MainHand ou OffHand uniquement) ;
- égalité complète des GUID de l'objet et du personnage ainsi
  que de son index avec les observations fournies ;
- réussite du résolveur existant
  `ResolveBoundAttackForHand` : autorisation de main
  `AttackSourceSlots`, action de type `Attack` et présentation
  liée présentes dans les quatre catalogues.

Le résultat est entièrement réinitialisé après tout refus.
`InvalidSourceInstance` et `EquipmentSourceMismatch` sont
deux diagnostics distincts. Une épée dans le sac ou au curseur
n'est pas admissible même si sa définition peut être équipée.

**Important :** cette méthode compare les observations
fournies par l'appelant ; elle **n'interroge pas elle-même le
composant d'inventaire**. Lors du futur branchement gameplay,
l'appelant devra obtenir le GUID et le propriétaire du slot
courant et ne pas se fier à une hotbar ou sauvegarde périmée.
Cette vérification n'est pas une preuve cryptographique
d'appartenance et ne modifie aucune règle de combat historique.

## Test

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.EquippedInstanceAttackIdentity`
couvre l'attaque réelle N20, GUID d'objet obsolète, mauvais
personnage, index incorrect, objet dans sac/curseur,
mauvaise main, arme déplacée OffHand sans droit de frappe,
niveau 0, GUID invalide, dette d'armure négative,
action inconnue, bouclier passif, et remise à zéro après refus.

Aucun `.uasset`, `.umap`, SaveGame, WBP ni lecteur
du combat historique n'est modifié. Ce jalon exige
`SilentTask` et `LocalAuto` réussis sur le SHA exact.
