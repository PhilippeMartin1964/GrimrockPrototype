# UI-INV2 Character Equipment Panel

## Objectif

UI-INV2 prepare le panneau central personnage / equipement de `WBP_GridInventory`.
Le but est de fournir une base fonctionnelle pour les slots d'equipement autour du
personnage sans refaire toute l'interface d'inventaire.

`WBP_GridInventory` reste un widget enfant de `WBP_GrimrockMenu`. Il vit deja dans
la surface logique 1920x1080 geree par `UGrimrockDesignSurfaceWidget` via le menu
parent. Il ne doit donc pas gerer la resolution ecran.

## Structure cible

Structure recommandee pour le panneau central :

```text
SizeBox_SelectedCharacterPanel
-> Border_SelectedCharacterPanel
   -> VerticalBox_SelectedCharacter
      -> Text_SelectedCharacterTitle
      -> VerticalBox_CharacterDetails
      -> Text_EquipmentTitle
      -> Border_EquipmentPanel
         -> UniformGrid_EquipmentSlots
            -> SlotWidget_Head
            -> SlotWidget_Amulet
            -> SlotWidget_Shoulders
            -> SlotWidget_Chest
            -> SlotWidget_Gloves
            -> SlotWidget_Belt
            -> SlotWidget_Legs
            -> SlotWidget_Feet
            -> SlotWidget_Cloak
            -> SlotWidget_Ring1
            -> SlotWidget_Ring2
            -> SlotWidget_MainHand
            -> SlotWidget_OffHand
            -> SlotWidget_Talisman
            -> SlotWidget_QuickSlot1
            -> SlotWidget_QuickSlot2
```

Les slots doivent etre des `WBP_InventorySlot` ou des widgets derives de
`UGridInventorySlotWidget`, puis etre enregistres avec
`RegisterEquipmentSlotWidget`.

## Slots supportes

Slots fonctionnels dans cette etape :

- `MainHand`
- `OffHand`
- `Head`
- `Chest`
- `Legs`
- `Feet`
- `Amulet`
- `Ring1`
- `Ring2`
- `Shoulders`
- `Gloves`
- `Belt`
- `Cloak`
- `Talisman`
- `QuickSlot1`
- `QuickSlot2`

Mapping Blueprint recommande :

```text
Slot_Head       -> EGridEquipmentSlot::Head
Slot_Amulet     -> EGridEquipmentSlot::Amulet
Slot_Chest      -> EGridEquipmentSlot::Chest
Slot_Shoulders  -> EGridEquipmentSlot::Shoulders
Slot_Gloves     -> EGridEquipmentSlot::Gloves
Slot_Belt       -> EGridEquipmentSlot::Belt
Slot_Legs       -> EGridEquipmentSlot::Legs
Slot_Feet       -> EGridEquipmentSlot::Feet
Slot_Cloak      -> EGridEquipmentSlot::Cloak
Slot_Ring1      -> EGridEquipmentSlot::Ring1
Slot_Ring2      -> EGridEquipmentSlot::Ring2
Slot_MainHand   -> EGridEquipmentSlot::MainHand
Slot_OffHand    -> EGridEquipmentSlot::OffHand
Slot_Talisman   -> EGridEquipmentSlot::Talisman
Slot_QuickSlot1 -> EGridEquipmentSlot::QuickSlot1
Slot_QuickSlot2 -> EGridEquipmentSlot::QuickSlot2
```

Le slot `Face` n'est pas traite dans UI-INV2, car `EGridEquipmentSlot` ne le
contient pas encore.

`Cursor` n'est pas un equipement. C'est un etat temporaire de manipulation
d'item, a placer hors de `UniformGrid_EquipmentSlots`.

## Regles de scaling

- Ne pas ajouter de `ScaleBox` local pour compenser la resolution.
- Ne pas ajouter de `SizeBox_DesignSurface` locale dans `WBP_GridInventory`.
- Ne pas calculer de DPI, viewport ou scaling global dans l'inventaire.
- Garder les slots a leur taille locale fixe de 132x132.
- Laisser `UGrimrockDesignSurfaceWidget` gerer le centrage et la limite physique
  de la surface via `WBP_GrimrockMenu`.

`WBP_GridInventory` vit dans `WBP_GrimrockMenu`. La surface 1920x1080 est
fournie par le parent ; l'inventaire organise seulement l'espace qu'il recoit.

## Structure finale du panneau SelectedCharacter

```text
SizeBox_SelectedCharacterPanel
-> Border_SelectedCharacterPanel
   -> VerticalBox_SelectedCharacter
      -> Text_SelectedCharacterTitle
      -> VerticalBox_CharacterDetails
         -> Border_CharacterClassAccent
            -> HorizontalBox_Identity
               -> SizeBox_Portrait
                  -> Overlay_PortraitSlot
                     -> Image_CharacterPortrait
                     -> Image_CharacterClassIcon
               -> VerticalBox_Identity
                  -> Text_CharacterName
                  -> HorizontalBox_Race
                  -> HorizontalBox_Class
                  -> HorizontalBox_Level
                  -> HorizontalBox_Experience
         -> UniformGridPanel_Attributes
         -> HorizontalBox_DerivedStats
      -> Text_EquipmentTitle
      -> Border_EquipmentPanel
         -> UniformGrid_EquipmentSlots
            -> tous les SlotWidget_* equipement UI-INV2
```

La structure actuelle est globalement conservee. Seul
`UniformGrid_EquipmentSlots` doit etre complete, et `SlotWidget_Cursor` doit etre
sorti du panneau equipement.

## Checklist Blueprint

- Aucun `.uasset` ne doit etre modifie comme fichier texte.
- Ne pas ajouter de `ScaleBox` local.
- Ne pas ajouter de `SizeBox_DesignSurface` local.
- Chaque `SlotWidget_*` equipement est un `UGridInventorySlotWidget`.
- Chaque slot equipement appelle `RegisterEquipmentSlotWidget`.
- `SlotWidget_Cursor` reste hors de `UniformGrid_EquipmentSlots`.
- `MainHand` et `OffHand` restent enregistres et fonctionnels.
- Aucun Blueprint ne decide de la compatibilite item/slot.

## Tests runtime attendus

- Ouvrir l'inventaire en jeu.
- Verifier que le panneau central est visible et stable.
- Verifier que les slots restent visuellement a 132x132 dans la surface design.
- Verifier que `MainHand` et `OffHand` fonctionnent toujours.
- Verifier qu'un slot `Equipment` vide accepte un `CursorItem` compatible.
- Verifier qu'un slot `Equipment` occupe peut etre pris au cursor.
- Verifier qu'un drop inventaire vers `Equipment` incompatible est refuse sans perte.
- Tester si possible l'equipement et le desequipement d'une arme.
- Tester si possible l'equipement et le desequipement d'une armure compatible.
- Verifier le changement de personnage si plusieurs personnages sont actifs.
- Tester si possible en 3840x2160, 1920x1080 et 1600x900.
- Verifier les logs : aucun `design surface scaling failed`, aucune erreur
  `BindWidget` critique, aucun crash au clic sur un slot equipement.
