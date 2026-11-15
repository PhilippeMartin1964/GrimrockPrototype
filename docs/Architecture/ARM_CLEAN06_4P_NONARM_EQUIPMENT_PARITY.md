# ARM-CLEAN06.4P — Équipement non-ARM, destination et parité de données

## Références validées et décision

- UE5.5.4, `master`, norme prioritaire `ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`.
- **06.4O**, commit `20f641621cff0f2db0c12a94f9ff38176f0ca666`, trois checks Windows SUCCESS ; rapport [#148](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/148). Son audit a constaté 1 action équipement, 3 QuickItems valides et 1 présentation d'attaque. **Aucun mapping d'action n'a été approuvé**.
- **06.4K**, `4d5e7eb8b1406d29eeb9b0805f82507aa8a1bfc7` : 11 `ItemId` non-ARM, 40 références monde, 50 occurrences de propriétés actives dont **15** dans le domaine `NONARM_EQUIPMENT`.
- **06.4G**, `e0f877d17f39d913c585b55ba18110745c5385b6` : 84 sources binaires et 84 copies privées conservées sous reçus SHA-256.

Les **15** occurrences actives du domaine sont réparties entre cinq propriétés historiques possibles : `HandUsage`, `CompatibleEquipmentSlots`, `EquipmentStatBonus`, `EquipmentResistanceBonus` et `EquippedMesh`. Le rapport privé 06.4K fait autorité pour les valeurs et les noms réellement actifs. **Une occurrence de propriété n'est pas une identité d'objet** : le total de 15 n'implique pas 15 équipements.

## Propriétaire canonique

`UGridOtherEquipmentDefinitionAsset` reste la spécialisation pour les armures, bijoux et équipements **non-ARM**. Il possédait déjà `EquipmentSlots` et `Bonuses` ; le jalon ajoute `HandPolicy`, `Resistances` et `EquippedMesh`.

| Source exacte de l'ancien objet | Destination spécialisée (hors ARM) |
|---|---|
| `CompatibleEquipmentSlots` | `EquipmentSlots` — copie réfléchie stricte |
| `EquipmentStatBonus` | `Bonuses` — copie réfléchie stricte |
| `EquipmentResistanceBonus` | `Resistances` — copie réfléchie stricte |
| `EquippedMesh` | `EquippedMesh` — copie réfléchie stricte, avec repli visuel vers `WorldMesh` uniquement si vide |
| `HandUsage` | `HandPolicy` — transformation explicite de la valeur **effective** vers `NotHandHeld`, `OneHanded` ou `TwoHanded` |

La source historique `HandUsage=Auto` dépend de la compatibilité MainHand/OffHand. Le futur runtime n'a pas à réexécuter cette inférence : la conversion Editor ponctuelle résout `GetEffectiveHandUsage()` depuis les **valeurs réelles** et enregistre une politique de main explicite. La valeur originale reste vérifiable par le rapport 06.4K. La présence d'une option deux mains n'autorise pas à exposer OffHand comme deuxième case d'équipement disponible : `CanEquipToSlot` et `GetAvailableSlots` représentent **deux contrats distincts**.

Le noyau canonique de base conserve exactement cinq propriétés ; les définitions ARM ne reçoivent aucun nouveau champ. Le `DA_WeaponCatalogue` reste seul propriétaire des statistiques combattantes des armes. Pour une pièce d'armure/bijou non-ARM, les bonus et résistances appartiennent à sa définition spécialisée, sans profil ARM N1 inventé.

## Contrôle Editor UE5.5.4, lecture seule

Le sujet de commit déclenchant l'agent Windows est exactement :

`feat(ARM-CLEAN06.4P): preserve nonarm equipment hand slots resistances and meshes`

Le commandlet `GridArmClean064PNonArmEquipmentParity` et `Scripts/AuditARM_CLEAN06_4P_NonArmEquipmentParity.ps1` :

1. vérifient les reçus privés et SHA des jalons 06.4K et 06.4O, les portes de migration fermées, et la provenance des **84 binaires sources + 84 copies** ;
2. rechargent le `DA_ItemSystem` réel et les **11** `UGridItemDefinitionAsset` historiques, sans déduire d'identité depuis un nom ou une icône ;
3. recoupent **tous les 50** exports de propriétés réellement actifs du rapport 06.4K et leurs domaines ;
4. créent **11 objets transitoires** de type `UGridOtherEquipmentDefinitionAsset` pour éprouver les correspondances. Leur classe transitoire **n'impose pas** que les 11 sources deviennent des équipements spécialisés dans les vrais assets ;
5. réalisent **44 copies** strictes des quatre propriétés typées, y compris les valeurs par défaut, avec `FProperty::SameType`, `Identical` et `ExportText` ; vérifient **11** correspondances de politique de main à partir de `GetEffectiveHandUsage` ;
6. comparent **209 possibilités de placement direct** (les 19 valeurs d'emplacement, None incluse, sur 11 objets), et **11** listes ordonnées d'emplacements effectivement affichables ;
7. vérifient **33 lectures de bonus** pour les niveaux d'instance **N0, N1 et N20**, **11 replis de mesh Equipped/World** et le maintien de la masse unitaire comme source indépendante ;
8. garantissent l'absence de nouveau `Dirty` dans les packages chargés, aucune modification du worktree, aucun `SavePackage`, aucun accès aux SaveGames ;
9. conservent les `ItemId`, valeurs source et chemins privés uniquement sous `StateRoot/Runs`, avec reçu SHA-256 privé, et ne publient dans l'issue GitHub que les totaux.

**Pour valider 06.4P**, attendre les trois statuts Windows SUCCESS sur son SHA : `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto`, `UE5.5.4/ARM-CLEAN06.4P`. Les statuts des parents ne remplacent jamais celui du commit en cours.

## Limites strictes

- Ce jalon établit une **parité de données et de règles pures de lecture**, pas une équivalence de l'exécuteur d'équipement en PIE, de l'inventaire UMG ni de Save/Continue après conversion.
- Les **11 champs actions** restent sans destination sémantique entièrement approuvée malgré l'inventaire 06.4O. Les **6 champs d'empilement** nécessitent encore leur politique hors consommables/munitions.
- **17 occurrences de propriétés historiques actives** restent donc sans projection approuvée au terme des familles de données 06.4L/M/N/P, sous réserve du succès effectif de 06.4P.
- Aucun `.uasset`, `.umap`, `.sav` ni WBP modifié. `migration_authorized=false` et listes d'écriture vides.
- **Suite : 06.4Q** — empilement non-ARM et N0/Quantity/SaveGame, puis **06.4R** — mapping sémantique des 4 actions et 1 présentation relevées par 06.4O, avec vérification des effets réellement authorés, suivi des migrations Editor-only et régressions 06.6.
