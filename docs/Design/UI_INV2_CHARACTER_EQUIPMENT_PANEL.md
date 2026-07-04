# UI-INV2 Character Equipment Panel

## Objet

Ce document fixe la structure cible du panneau central personnage / equipement de `WBP_GridInventory`.

La direction validee est un layout **paper doll** : le personnage selectionne est vu de pied en cap au centre, et les `SlotWidget` d'equipement sont places autour de lui. L'equipement ne doit plus etre pense comme une simple grille separee du personnage.

`WBP_GridInventory` reste un widget enfant de `WBP_GrimrockMenu`. Il vit dans la surface logique 1920x1080 geree par `UGrimrockDesignSurfaceWidget` via le menu parent. Il ne doit donc jamais gerer la resolution ecran, le DPI ou le viewport.

## Decision canonique des slots d'equipement

La liste suivante devient la cible officielle du `Character Equipment Panel`.

### Colonne gauche

| Widget Blueprint cible | Libelle UI | Slot logique cible |
|---|---|---|
| `SlotWidget_Head` | Tete | `Head` |
| `SlotWidget_Face` | Visage | `Face` |
| `SlotWidget_Amulet` | Amulette | `Amulet` |
| `SlotWidget_Shoulders` | Epaules | `Shoulders` |
| `SlotWidget_Shirt` | Chemise | `Shirt` |
| `SlotWidget_Chest` | Torse | `Chest` |
| `SlotWidget_Cloak` | Cape | `Cloak` |
| `SlotWidget_Bracers` | Brassards | `Bracers` |

### Colonne droite

| Widget Blueprint cible | Libelle UI | Slot logique cible |
|---|---|---|
| `SlotWidget_Gloves` | Gants | `Gloves` |
| `SlotWidget_Belt` | Ceinture | `Belt` |
| `SlotWidget_Legs` | Jambes | `Legs` |
| `SlotWidget_Feet` | Bottes | `Feet` |
| `SlotWidget_Ring1` | Anneau I | `Ring1` |
| `SlotWidget_Ring2` | Anneau II | `Ring2` |
| `SlotWidget_Earring1` | Bijou d'oreille I | `Earring1` |
| `SlotWidget_Earring2` | Bijou d'oreille II | `Earring2` |

### Bas du personnage

| Widget Blueprint cible | Libelle UI | Slot logique cible |
|---|---|---|
| `SlotWidget_MainHand` | Main principale | `MainHand` |
| `SlotWidget_OffHand` | Main secondaire | `OffHand` |

## Slots hors cible du panneau paper doll

Les anciens slots `Talisman`, `QuickSlot1`, `QuickSlot2` et tout slot `Accessory` ne font pas partie du `Character Equipment Panel` valide ici.

Ils pourront etre reutilises plus tard dans un systeme separe : barre rapide, objets consommables, talismans actifs, accessoires secondaires ou raccourcis gameplay. Ils ne doivent pas etre places autour du personnage comme equipement principal.

`Cursor` n'est pas un equipement. C'est un etat temporaire de manipulation d'item. Il doit rester hors du panneau paper doll.

## Etat technique actuel

La cible visuelle contient plus de slots que le modele C++ actuellement disponible.

Slots deja alignes avec le modele actuel :

- `MainHand` ;
- `OffHand` ;
- `Head` ;
- `Chest` ;
- `Legs` ;
- `Feet` ;
- `Amulet` ;
- `Ring1` ;
- `Ring2` ;
- `Shoulders` ;
- `Gloves` ;
- `Belt` ;
- `Cloak`.

Slots a ajouter dans une etape C++ suivante :

- `Face` ;
- `Shirt` ;
- `Bracers` ;
- `Earring1` ;
- `Earring2`.

Tant que ces slots ne sont pas ajoutes a `EGridEquipmentSlot` et `FGridCharacterEquipmentState`, ils peuvent etre prepares visuellement dans le Blueprint, mais ne doivent pas etre enregistres comme slots fonctionnels via `RegisterEquipmentSlotWidget`.

## Structure cible du panneau SelectedCharacter

Structure recommandee :

```text
SizeBox_SelectedCharacterPanel
-> Border_SelectedCharacterPanel
   -> Overlay_SelectedCharacterRoot
      -> Text_SelectedCharacterTitle
      -> Overlay_PaperDollArea
         -> VerticalBox_LeftEquipmentColumn
            -> SlotWidget_Head
            -> SlotWidget_Face
            -> SlotWidget_Amulet
            -> SlotWidget_Shoulders
            -> SlotWidget_Shirt
            -> SlotWidget_Chest
            -> SlotWidget_Cloak
            -> SlotWidget_Bracers
         -> SizeBox_CharacterFigure
            -> Overlay_CharacterFigure
               -> Image_CharacterFullBody
               -> Image_CharacterClassIcon ou decoration de classe optionnelle
         -> VerticalBox_RightEquipmentColumn
            -> SlotWidget_Gloves
            -> SlotWidget_Belt
            -> SlotWidget_Legs
            -> SlotWidget_Feet
            -> SlotWidget_Ring1
            -> SlotWidget_Ring2
            -> SlotWidget_Earring1
            -> SlotWidget_Earring2
         -> HorizontalBox_BottomHandsRow
            -> SlotWidget_MainHand
            -> Spacer_BottomHands
            -> SlotWidget_OffHand
```

Le personnage doit etre vu de pied en cap. L'image centrale doit representer le personnage selectionne avec son apparence d'equipement. Dans un premier temps, `Image_CharacterFullBody` peut utiliser une image statique par race/classe/genre ; l'equipement dynamique complet pourra venir plus tard.

## Panneau de details et statistiques

Le panneau de statistiques doit rester separe du paper doll. Il peut etre place a droite du personnage et regrouper :

- `Details` : nom, race, classe, niveau, experience ;
- `Attributes` : FOR, DEX, CON, INT, SAG, CHA ;
- `DerivedStats` : PV, mana, charge ;
- `Combat` : armure physique, armure magique, degats, critique, precision, esquive ;
- `MobilityAndProgression` : deplacement, initiative, experience, niveau suivant ;
- `Resistances` : feu, eau, terre, air, poison, puis autres resistances si necessaire.

Les champs deja exposes par `UGridInventoryWidget` doivent etre reutilises plutot que recrées avec d'autres noms.

## Regles de scaling

- Ne pas ajouter de `ScaleBox` local pour compenser la resolution.
- Ne pas ajouter de `SizeBox_DesignSurface` local dans `WBP_GridInventory`.
- Ne pas calculer de DPI, viewport ou scaling global dans l'inventaire.
- Garder les slots a leur taille locale fixe de 132x132.
- Laisser `UGrimrockDesignSurfaceWidget` gerer le centrage et la limite physique via `WBP_GrimrockMenu`.

## Cablage Blueprint

Chaque slot fonctionnel doit etre un `WBP_InventorySlot` ou un widget derive de `UGridInventorySlotWidget`.

Mapping fonctionnel actuel recommande :

```text
SlotWidget_Head       -> Head
SlotWidget_Amulet     -> Amulet
SlotWidget_Shoulders  -> Shoulders
SlotWidget_Chest      -> Chest
SlotWidget_Cloak      -> Cloak
SlotWidget_Gloves     -> Gloves
SlotWidget_Belt       -> Belt
SlotWidget_Legs       -> Legs
SlotWidget_Feet       -> Feet
SlotWidget_Ring1      -> Ring1
SlotWidget_Ring2      -> Ring2
SlotWidget_MainHand   -> MainHand
SlotWidget_OffHand    -> OffHand
```

Placeholders visuels jusqu'a alignement C++ :

```text
SlotWidget_Face
SlotWidget_Shirt
SlotWidget_Bracers
SlotWidget_Earring1
SlotWidget_Earring2
```

Ces placeholders doivent etre clairement documentes comme non fonctionnels tant que le modele C++ n'est pas aligne.

## Checklist Blueprint

- Le personnage plein corps est au centre.
- Les slots sont autour du personnage, pas dans une grille separee.
- `SlotWidget_Cursor` est hors panneau paper doll.
- Aucun `.uasset` n'est modifie comme fichier texte.
- Aucun `ScaleBox` local n'est ajoute dans `WBP_GridInventory`.
- Aucun `SizeBox_DesignSurface` local n'est ajoute dans `WBP_GridInventory`.
- Aucun Blueprint ne decide de la compatibilite item/slot.
- Les slots fonctionnels appellent `RegisterEquipmentSlotWidget`.
- Les slots non encore supportes par C++ restent visuels ou desactives.

## Tests runtime attendus

- Ouvrir l'inventaire en jeu.
- Verifier que le personnage est affiche de pied en cap.
- Verifier que les slots sont visuellement autour du personnage.
- Verifier que `MainHand` et `OffHand` fonctionnent toujours.
- Verifier qu'un slot fonctionnel vide accepte un `CursorItem` compatible.
- Verifier qu'un slot fonctionnel occupe peut etre pris au cursor.
- Verifier qu'un drop incompatible est refuse sans perte.
- Verifier qu'aucun slot placeholder non supporte ne provoque de crash.
- Verifier qu'aucun `design surface scaling failed` n'apparait.
- Verifier qu'aucune erreur critique `BindWidget` n'apparait.
