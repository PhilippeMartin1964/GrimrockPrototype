# ARM-CLEAN06.2AD — Prévisualisation conservatrice des instances sauvegardées

## Pourquoi ce jalon

Les `FGridItemInstance` actuellement sérialisées dans
l'inventaire, l'équipement, le curseur et les SaveGame portent
encore `DisplayName` et `Weight` à côté des champs mutables.
Le modèle ARM-CLEAN06 approuvé exige que nom et poids ne soient
stockés que dans la définition d'objet canonique.

Retirer ces snapshots sans qualification pourrait faire perdre
des noms personnalisés ou changer la charge portée d'une ancienne
sauvegarde. Recharger une armure ou un bouclier à sa valeur
nominale en perdant `AbsorbedEquipmentPhysicalArmor` ou
`AbsorbedEquipmentMagicalArmor` serait une régression critique.

## Conversion de prévisualisation uniquement

L'outil **Editor-only** `GridArmClean06SaveInstancePreview`
est volontairement une structure C++ **non sérialisée** et
sans activation du runtime. Il vérifie la source historique
et la définition ARM canonique correspondante, puis conserve :

- `RuntimeObjectId` et `ItemId` exacts, `ItemLevel` N1–N20,
  `Quantity` et les deux compteurs d'armure consommée ;
- `OwnerType`, `OwnerGuid`, `OwnerCharacterIndex` et
  `EquipmentSlot`, y compris les transferts via curseur ;
- l'état de lumière, les informations de lecture et leurs
  overrides, ainsi que `LastWorldTransform`.

Seuls le nom et le poids **strictement égaux** à la définition
réelle sont exclus de la copie : ils seront recalculés via la
définition canonique. Le projecteur reconstitue intégralement
une `FGridItemInstance` de prévisualisation et compare
**tous les champs** à l'original.

La conversion refuse :
- un objet inconnu ou un `ItemId/ArchetypeId` incohérent ;
- `ItemLevel=0` (Legacy), sans inventer un niveau N1 ;
- une réserve d'armure négative ;
- un poids sauvegardé différent du poids actuel du DataAsset ;
- un nom propre à une instance ou une définition source/cible
  discordante ;
- une définition canonique invalide.

Chaque refus efface la destination pour empêcher la
réutilisation accidentelle d'une ancienne projection réussie.
Les champs de lecture conservent ici leurs références
`UObject` pour prouver le round-trip en mémoire ; une
future migration SaveGame devra sérialiser proprement une
identité stable et vérifier sa relecture avant de supprimer
le format historique.

## Automation

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionARMInstanceSaveRoundTrip`
charge **uniquement les 22 définitions du manifeste officiel**,
fabrique pour chacune une instance N1 et une instance N20,
puis vérifie 44 aller-retours exacts avec GUID et dette de
protection persistants. Les sources restent propres.

`RejectLossySaveInstanceMigration` couvre la relecture,
`ItemLevel=0`, le renommage historique, une masse
incohérente, la dette négative et une identité différente.

Le jalon ne lit ni n'écrit aucun vrai fichier de sauvegarde.
Il n'autorise pas le changement de version SaveGame ni la
suppression de ses lecteurs, et ne modifie aucun
`.uasset`, WBP, gameplay ou niveau. La preuve native UE5.5.4
doit réussir sur le SHA exact avant toute étape ultérieure.
