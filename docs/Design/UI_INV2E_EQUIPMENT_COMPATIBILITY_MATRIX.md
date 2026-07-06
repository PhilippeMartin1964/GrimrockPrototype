# UI-INV2E Equipment Compatibility Matrix

## Objet

Ce document fixe la matrice officielle des compatibilites entre les familles d'items et les slots du paper doll.

Les compatibilites runtime sont portees par `UGridItemDefinitionAsset::CompatibleEquipmentSlots`. Les DataAssets d'items seront renseignes manuellement dans Unreal Editor pendant UI-INV2E-B. Cette etape ne modifie aucun `.uasset` et ne corrige pas automatiquement les assets.

## Matrice officielle

| Famille d'item | Slots compatibles |
|---|---|
| Torche | `MainHand`, `OffHand` |
| Arme une main | `MainHand` |
| Arme deux mains | `MainHand` |
| Bouclier | `OffHand` |
| Casque / capuche | `Head` |
| Masque / lunettes / visage | `Face` |
| Amulette | `Amulet` |
| Epaulieres | `Shoulders` |
| Chemise / sous-vetement haut | `Shirt` |
| Armure / plastron / robe cuirassee | `Chest` |
| Cape | `Cloak` |
| Brassards / poignets | `Bracers` |
| Gants | `Gloves` |
| Ceinture | `Belt` |
| Pantalon / jambieres | `Legs` |
| Bottes | `Feet` |
| Anneau | `Ring1`, `Ring2` |
| Bijou d'oreille | `Earring1`, `Earring2` |

Les armes deux mains sont documentees comme compatibles avec `MainHand`, mais la regle de blocage ou de liberation de `OffHand` reste a implementer dans un systeme ulterieur. Tant que cette logique n'existe pas, `CompatibleEquipmentSlots` ne suffit pas a exprimer la contrainte complete.

## Hors paper doll

`Talisman`, `QuickSlot1` et `QuickSlot2` ne font pas partie du paper doll actuel. Ils restent disponibles cote C++ pour compatibilite ou usage futur, mais ils ne doivent pas etre utilises pour le panneau d'equipement central.

`Cursor` n'est pas un slot d'equipement. C'est un etat temporaire de manipulation d'item et il ne doit jamais apparaitre dans une compatibilite d'item.

## Regles provisoires

- Un item compatible avec deux slots equivalents peut aller dans l'un ou l'autre.
- `Ring1` et `Ring2` sont equivalents pour les anneaux.
- `Earring1` et `Earring2` sont equivalents pour les bijoux d'oreille.
- `MainHand` et `OffHand` peuvent partager certains items, par exemple les torches.
- Les armes deux mains sont documentees, mais elles ne sont pas pleinement implementees tant qu'aucune logique ne bloque ou ne libere `OffHand`.
- Les compatibilites sont des autorisations de placement, pas des regles visuelles. Le layout reste porte par `WBP_GridInventory`.

## Travail manuel UE5

Pour chaque DataAsset d'item equipeable, verifier et renseigner :

- `CompatibleEquipmentSlots` ;
- `ItemType` ou categorie equivalente si applicable ;
- `bCanEmitLight` pour torches et autres sources lumineuses ;
- `bDefaultLightEnabled` si la source lumineuse doit etre active par defaut ;
- `Icon` ;
- `Weight` ;
- `DisplayName` en francais ;
- `Description` en francais si l'item a un tooltip detaille ;
- `WorldMesh` ;
- `EquippedMesh` ou mesh tenu si applicable ;
- `ItemTags` techniques si une logique future doit filtrer l'item.

## Diagnostic C++

UI-INV2E-A ajoute `UGridPartyInventoryComponent::LogEquipmentCompatibilityDiagnostics()`.

Ce diagnostic parcourt les definitions d'items enregistrees au runtime dans `RuntimeItemDefinitionsById` et logue sans bloquer le jeu :

- les items potentiellement equipeables dont `CompatibleEquipmentSlots` est vide ;
- les items lumineux sans compatibilite `MainHand` ou `OffHand` ;
- les items compatibles avec `Talisman`, `QuickSlot1` ou `QuickSlot2`, en warning informatif pour le contexte paper doll ;
- les items compatibles avec les nouveaux slots `Face`, `Shirt`, `Bracers`, `Earring1` ou `Earring2`, en log informatif.

Le diagnostic ne modifie aucun DataAsset et ne tente aucune auto-correction. Les corrections restent manuelles dans UE5.
