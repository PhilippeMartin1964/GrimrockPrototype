# ARM-CLEAN06.3AQ — Emplacements d'équipement dans les menus et infobulles

Parent : `c75b811` (06.3AP), `UE5.5.4/SilentTask` et `LocalAuto`
réussis au départ. Les paquets `.uasset` ne sont pas modifiés.

## Constats

`GridItemContextActionLibrary.cpp` offrait les actions « S'équiper »
depuis les champs historiques `HandUsage` / `CompatibleEquipmentSlots`,
même lorsque l'inventaire refusait ces slots pour un `ItemId`
enregistré. `GridInventorySlotWidget.cpp` et
`GridEquipmentTooltipGlobalProjection.cpp` exposaient la même
désynchronisation dans la chaîne d'infobulles et comparaisons.

## Un seul modèle de lecture pour les trois usages

`FGridCanonicalEquipmentSlotsReadModel` résout les mains possibles en
consultant `UGridItemSubsystem::ResolveItem(ItemId, ItemLevel)`.
Les entrées canoniques `ARM` utilisent uniquement
`FGridCanonicalResolvedWeapon::CanEquipInHand`, avec une unique
action physique en `MainHand` pour l'équipement à deux mains.
Les non-armements enregistrés, y compris les munitions N0, ne
deviennent jamais des équipements parce qu'une ancienne définition
le suggère. Un niveau ARM persisté N0 ou invalide N21 est
**refusé sans retour à DA_Weapon**. Une définition historique
absente n'empêche pas l'affichage des actions d'une arme canonique.

Les `ItemId` absents du registre conservent temporairement les
règles `CanEquipToSlot` et les autres slots explicites ; cela
préserve la compatibilité jusqu'à la conversion contrôlée des assets
en 06.4 et au retrait du legacy en 06.5.

Ce même résultat alimente :
- les choix `Equip` du menu contextuel de l'inventaire ;
- `CompatibleSlotsText` et `bEquippable` du tooltip ;
- les slots des scénarios `PreviewEquipFromInventorySlot`,
  et le chemin de comparaison synthétique existant.

La transaction d'équipement, les ressources, les GUID et les
données persistantes ne changent pas ; `CanEquipItemToSlot`
continue de valider toute exécution réelle.

## Vérifications

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalContextAndTooltipEquipmentSlots` :
monde de test avec vraie `GameInstance`, registre `DA_ItemSystem`
chargé, menu contextuel réel, widget de slot et projection
tooltip. Couvre ARM031 N12 deux mains, ARM075 N20 main
secondaire, ARM d'épée N4, munitions N0, ARM N0 invalide,
ancienne définition volontairement erronée et inconnus legacy.
Le test vérifie aussi l'absence de modification de l'instance.

Diagnostic continu : ce test est maintenant décliné en quatre exécutions indépendantes
`Policy`, `ContextMenu`, `TooltipModel` et `TooltipWidget`, afin que VALIDATION-AUTO01 publie
le nom exact de l'étage défaillant sans exposer des journaux privés.

### Checklist de localisation — début et fin du jalon

Aucune nouvelle chaîne visible : `S'équiper : deux mains` et
`S'equiper : {0}` restent des `NSLOCTEXT` existants ;
`Inventory.Tooltip.Equipment.TwoHands` reste une clé
`ST_UI` existante. Ce jalon change leur *sélection*, pas leur
autorité, contenu ou style.

| Étape | État 06.3AQ |
|---|---|
| `UI.csv` | Inchangé : aucune clé ajoutée |
| `ST_UI.uasset` | Inchangé : clé existante vérifiée par test |
| `WBP` | Inchangé : aucun authoring de texte ou designer |
| `Game/fr/en` | Inchangé : aucune nouvelle traduction requise |
| Texte utilisateur | Réutilisation stricte des clés et NSLOCTEXT déjà présents |
| PIE GPU | Non attesté par la validation `-NullRHI` |

Le flux `UI.csv → ST_UI → WBP → Game/fr/en` ne requiert
aucune nouvelle matérialisation : la preuve de localisation
concerne uniquement les clés déjà utilisées, sans inventer un
asset Unreal.

### Limites

Le rendu des scénarios numériques sans définition historique
reste du ressort de la migration 06.4 ; cette étape garantit
la cohérence des slots et du texte de compatibilité.

Le widget emploie également le monde de son composant d'inventaire
lorsque son propre `GetWorld()` ne fournit pas de GameInstance.
Ce repli remonte vers **le même** `UGridItemSubsystem`, sans nouvelle autorité.
