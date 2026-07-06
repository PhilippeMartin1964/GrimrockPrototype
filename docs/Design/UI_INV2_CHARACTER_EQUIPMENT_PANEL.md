# UI-INV2 Character Equipment Panel

## Objet

Ce document fixe la structure cible du panneau central personnage / equipement de `WBP_GridInventory`.

La direction validee est un layout **paper doll** : le personnage selectionne est vu de pied en cap au centre, et les `SlotWidget` d'equipement sont places autour de lui. L'equipement ne doit plus etre pense comme une simple grille separee du personnage.

`WBP_GridInventory` reste un widget enfant de `WBP_GrimrockMenu`. Il vit dans la surface logique 1920x1080 geree par `UGrimrockDesignSurfaceWidget` via le menu parent. Il ne doit donc jamais gerer la resolution ecran, le DPI ou le viewport.

## Decision canonique des slots d'equipement

La liste suivante est la cible officielle du `Character Equipment Panel`.

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

UI-INV2C aligne le modele C++ sur la cible paper doll. Les slots suivants sont declares dans `EGridEquipmentSlot`, stockes dans `FGridCharacterEquipmentState` et accessibles via les fonctions generiques d'equipement :

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
- `Cloak` ;
- `Face` ;
- `Shirt` ;
- `Bracers` ;
- `Earring1` ;
- `Earring2`.

Le Blueprint paper doll reste a implementer dans UI-INV2D. Les nouveaux slots sont disponibles en C++, mais ils ne sont visibles dans l'interface que lorsque `WBP_GridInventory` expose ou construit les widgets correspondants.

## Structure definitive de SizeBox_SelectedCharacterPanel

La structure definitive de `SizeBox_SelectedCharacterPanel` doit separer clairement trois responsabilites :

1. titre du personnage selectionne ;
2. paper doll avec personnage plein corps et slots autour ;
3. panneau details / attributs / statistiques.

Structure cible complete :

```text
SizeBox_SelectedCharacterPanel
-> Border_SelectedCharacterPanel
   -> Overlay_SelectedCharacterRoot
      -> Text_SelectedCharacterTitle
      -> HorizontalBox_SelectedCharacterBody
         -> Border_PaperDollPanel
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
                     -> Image_CharacterClassIcon
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
         -> Border_CharacterStatsPanel
            -> VerticalBox_CharacterStatsRoot
               -> Border_DetailsSection
                  -> VerticalBox_DetailsSection
                     -> Text_DetailsTitle
                     -> HorizontalBox_Name
                        -> TextLabel_CharacterName
                        -> Text_CharacterName
                     -> HorizontalBox_Race
                        -> TextLabel_Race
                        -> Text_CharacterRace
                     -> HorizontalBox_Class
                        -> TextLabel_Class
                        -> Text_CharacterClass
                     -> HorizontalBox_Level
                        -> TextLabel_Level
                        -> Text_CharacterLevel
                     -> HorizontalBox_Experience
                        -> TextLabel_Experience
                        -> Text_CharacterExperience
               -> Border_AttributesSection
                  -> UniformGridPanel_Attributes
                     -> TextLabel_Strength
                     -> Text_CharacterStrength
                     -> TextLabel_Dexterity
                     -> Text_CharacterDexterity
                     -> TextLabel_Constitution
                     -> Text_CharacterConstitution
                     -> TextLabel_Intelligence
                     -> Text_CharacterIntelligence
                     -> TextLabel_Wisdom
                     -> Text_CharacterWisdom
                     -> TextLabel_Charisma
                     -> Text_CharacterCharisma
               -> Border_DerivedStatsSection
                  -> HorizontalBox_DerivedStats
                     -> VerticalBox_Health
                        -> TextLabel_Health
                        -> Text_CharacterHealth
                     -> VerticalBox_Mana
                        -> TextLabel_Mana
                        -> Text_CharacterMana
                     -> VerticalBox_CarryWeight
                        -> TextLabel_CarryWeight
                        -> Text_CharacterCarryWeight
```

## Regles de construction UMG

`VerticalBox_SelectedCharacter` et `UniformGrid_EquipmentSlots` correspondent a l'ancienne structure. Ils peuvent etre conserves temporairement pendant la transition, mais ils ne sont pas la cible definitive.

La cible definitive utilise :

- `Overlay_SelectedCharacterRoot` comme racine interne du panneau personnage ;
- `HorizontalBox_SelectedCharacterBody` pour placer le paper doll a gauche et les stats a droite ;
- `Border_PaperDollPanel` pour encadrer le personnage et ses slots ;
- `Overlay_PaperDollArea` pour permettre le placement libre des colonnes et de la ligne des mains autour du personnage ;
- `Border_CharacterStatsPanel` pour isoler les details, attributs et statistiques derivees.

Les slots sont des instances d'un widget de slot existant, par exemple `WBP_InventorySlot` ou tout widget derive de `UGridInventorySlotWidget`. `SlotWidget_Head`, `SlotWidget_Face`, etc. sont donc des **noms d'instances**, pas de nouveaux assets a chercher dans le Content Browser.

## Image du personnage

La cible definitive utilise `Image_CharacterFullBody` pour afficher le personnage de pied en cap.

`Image_CharacterPortrait` peut etre conserve temporairement pour compatibilite ou migration, mais il n'est plus l'image principale du paper doll. Ne pas le supprimer tant que le C++ ou le Blueprint y fait encore reference par `BindWidget`.

## Panneau de details et statistiques

Le panneau de statistiques doit rester separe du paper doll et etre place a droite du personnage.

Il regroupe au minimum :

- `Details` : nom, race, classe, niveau, experience ;
- `Attributes` : FOR, DEX, CON, INT, SAG, CHA ;
- `DerivedStats` : PV, mana, charge.

Il pourra ensuite accueillir :

- `Combat` : armure physique, armure magique, degats, critique, precision, esquive ;
- `MobilityAndProgression` : deplacement, initiative, experience, niveau suivant ;
- `Resistances` : feu, eau, terre, air, poison, puis autres resistances si necessaire.

Les champs deja exposes par `UGridInventoryWidget` doivent etre reutilises plutot que recrées avec d'autres noms.

## Regles de scaling

- Ne pas ajouter de `ScaleBox` local pour compenser la resolution.
- Ne pas ajouter de `SizeBox_DesignSurface` local dans `WBP_GridInventory`.
- Ne pas calculer de DPI, viewport ou scaling global dans l'inventaire.
- La taille visuelle des slots n'est pas pilotee par le C++. Elle doit etre definie dans les WBP, via le Designer UE5, les SizeBox, les containers et les parametres de layout UMG. `UGridInventorySlotWidget` ne fait que porter la logique d'interaction et d'etat.
- Aucun hardcode de taille de slot dans `UGridInventorySlotWidget`.
- Aucune propriete C++ de taille logique de slot.
- Aucun `SetWidthOverride` / `SetHeightOverride` depuis C++ pour les slots.
- Laisser `UGrimrockDesignSurfaceWidget` gerer le centrage et la limite physique via `WBP_GrimrockMenu`.

## Cablage Blueprint

Chaque slot fonctionnel doit etre un `WBP_InventorySlot` ou un widget derive de `UGridInventorySlotWidget`.

Mapping fonctionnel definitif :

```text
SlotWidget_Head       -> Head
SlotWidget_Face       -> Face
SlotWidget_Amulet     -> Amulet
SlotWidget_Shoulders  -> Shoulders
SlotWidget_Shirt      -> Shirt
SlotWidget_Chest      -> Chest
SlotWidget_Cloak      -> Cloak
SlotWidget_Bracers    -> Bracers
SlotWidget_Gloves     -> Gloves
SlotWidget_Belt       -> Belt
SlotWidget_Legs       -> Legs
SlotWidget_Feet       -> Feet
SlotWidget_Ring1      -> Ring1
SlotWidget_Ring2      -> Ring2
SlotWidget_Earring1   -> Earring1
SlotWidget_Earring2   -> Earring2
SlotWidget_MainHand   -> MainHand
SlotWidget_OffHand    -> OffHand
```

Tous les slots paper doll sont fonctionnels cote C++ depuis UI-INV2C. Ils doivent donc pouvoir etre enregistres via `RegisterEquipmentSlotWidget` dans UI-INV2D.

Note UI-INV2D4 : les slots paper doll manuels sont enregistres cote C++ via `BindWidgetOptional` sur les widgets `SlotWidget_*`. Le layout visuel reste possede par `WBP_GridInventory`; le C++ valide uniquement la presence et le cablage des widgets. `BuildPaperDollEquipmentPanel` est conserve comme outil provisoire, mais ne doit pas remplacer automatiquement le layout manuel.

Ne pas enregistrer dans le paper doll :

- `Talisman` ;
- `QuickSlot1` ;
- `QuickSlot2` ;
- `Accessory` ;
- `Cursor`.

## Checklist Blueprint

- Le personnage plein corps est au centre.
- Les slots sont autour du personnage, pas dans une grille separee.
- `SlotWidget_Cursor` est hors panneau paper doll.
- Aucun `.uasset` n'est modifie comme fichier texte.
- Aucun `ScaleBox` local n'est ajoute dans `WBP_GridInventory`.
- Aucun `SizeBox_DesignSurface` local n'est ajoute dans `WBP_GridInventory`.
- Aucun Blueprint ne decide de la compatibilite item/slot.
- Tous les slots paper doll appellent `RegisterEquipmentSlotWidget`.
- `Image_CharacterPortrait` n'est pas supprime tant que le C++ ou le Blueprint y fait reference.

## Tests runtime attendus

- Ouvrir l'inventaire en jeu.
- Verifier que le personnage est affiche de pied en cap.
- Verifier que les slots sont visuellement autour du personnage.
- Verifier que `MainHand` et `OffHand` fonctionnent toujours.
- Verifier qu'un slot fonctionnel vide accepte un `CursorItem` compatible.
- Verifier qu'un slot fonctionnel occupe peut etre pris au cursor.
- Verifier qu'un drop incompatible est refuse sans perte.
- Verifier qu'aucun slot paper doll ne provoque de crash.
- Verifier qu'aucun `design surface scaling failed` n'apparait.
- Verifier qu'aucune erreur critique `BindWidget` n'apparait.
