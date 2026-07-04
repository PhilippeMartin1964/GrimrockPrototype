# Inventory and Item Ownership Design

## Objet

Ce document fixe la vision cible du système d'inventaire, de possession des objets et d'équipement pour **GrimrockPrototype**.

Il sert de référence avant toute tâche liée aux objets transportables, à l'équipement, aux personnages, au curseur, aux réceptacles et à la sauvegarde future du groupe.

## Principes validés

- Le jeu commence avec 1 personnage actif.
- Le groupe actif peut contenir de 1 à 6 personnages.
- Il n'existe pas d'inventaire de groupe principal.
- Chaque personnage possède son inventaire personnel.
- Chaque personnage possède son équipement.
- Le personnage sélectionné est le récepteur par défaut des objets ramassés.
- Le personnage sélectionné est celui dont la fiche, le paper doll et l'équipement sont affichés.
- L'inventaire utilise une grille simple à cases homogènes.
- L'écran Inventaire est un écran de logistique du groupe.

## Un seul propriétaire à la fois

Un item ne doit appartenir qu'à un seul propriétaire à la fois.

Un objet ne peut pas être simultanément dans le monde, dans un réceptacle, dans l'inventaire d'un personnage, équipé sur un personnage, tenu au curseur ou supprimé du niveau.

États possibles :

- `World` ;
- `Receptacle` ;
- `CharacterInventory` ;
- `EquipmentSlot` ;
- `Cursor` ;
- `HeldBySelectedCharacter` ;
- `Removed`.

## Structure de l'écran Inventaire

Le menu global est porté par `WBP_GrimrockMenu`. `WBP_GridInventory` est seulement le contenu de `Page_Inventory`.

`WBP_GridInventory` ne doit pas contenir le cadre global, les TopTabs, `WidgetSwitcher_MainContent`, `ScaleBox_DesignRoot`, `SizeBox_DesignSurface`, ni de logique DPI / viewport / scaling.

L'onglet Inventaire est organisé en trois zones :

1. Colonne gauche : personnages actifs.
2. Zone centrale : personnage sélectionné avec paper doll.
3. Zone droite : inventaire personnel, détails ou panneaux complémentaires selon l'étape UI.

## Zone centrale : personnage sélectionné

La zone centrale doit afficher le personnage sélectionné selon un layout paper doll :

- personnage vu de pied en cap au centre ;
- slots d'équipement placés autour du personnage ;
- mains placées en bas ;
- panneau de statistiques séparé ;
- aucun `Cursor` dans le paper doll.

## Liste officielle des slots paper doll

### Colonne gauche

```text
Head       -> Tête
Face       -> Visage
Amulet     -> Amulette
Shoulders  -> Épaules
Shirt      -> Chemise
Chest      -> Torse
Cloak      -> Cape
Bracers    -> Brassards
```

### Colonne droite

```text
Gloves     -> Gants
Belt       -> Ceinture
Legs       -> Jambes
Feet       -> Bottes
Ring1      -> Anneau I
Ring2      -> Anneau II
Earring1   -> Bijou d'oreille I
Earring2   -> Bijou d'oreille II
```

### Bas

```text
MainHand   -> Main principale
OffHand    -> Main secondaire
```

## Slots hors paper doll

Les éléments suivants ne font plus partie du panneau d'équipement central :

- `Talisman` ;
- `QuickSlot1` ;
- `QuickSlot2` ;
- `Accessory` ;
- `Cursor`.

Ils pourront être réutilisés plus tard pour un autre système : barre rapide, objets activables, talismans passifs, accessoires secondaires, consommables ou raccourcis gameplay.

`Cursor` reste un état temporaire de manipulation. Il ne doit jamais être documenté comme un slot d'équipement.

## État technique et dette C++

Le modèle visuel cible est plus large que le modèle C++ disponible au moment de UI-INV2.

Slots déjà disponibles ou alignés avec l'existant :

```text
MainHand
OffHand
Head
Chest
Legs
Feet
Amulet
Ring1
Ring2
Shoulders
Gloves
Belt
Cloak
```

Slots à ajouter dans une étape C++ suivante :

```text
Face
Shirt
Bracers
Earring1
Earring2
```

L'étape suivante devra aligner :

- `EGridEquipmentSlot` ;
- `FGridCharacterEquipmentState` ;
- `GetMutableSlot()` ;
- `GetSlot()` ;
- `ClearSlot()` ;
- les compatibilités `UGridItemDefinitionAsset::CompatibleEquipmentSlots` ;
- les mappings Blueprint `RegisterEquipmentSlotWidget`.

## CursorItem

Le `CursorItem` représente l'objet pris par la souris pendant une opération de déplacement, d'équipement ou de transfert.

Il peut provenir du monde, d'un réceptacle, de l'inventaire d'un personnage, d'un slot d'équipement ou d'un futur conteneur.

Le `CursorItem` n'est pas un slot paper doll.

## Flux principaux

### Monde vers personnage sélectionné

1. L'objet quitte le niveau.
2. L'objet est attribué au personnage sélectionné.
3. L'objet peut être placé au curseur, équipé ou rangé.
4. Le Runtime Dungeon State sait que l'objet n'est plus dans le niveau.

### Inventaire vers équipement

1. L'objet quitte son inventaire d'origine.
2. Le slot cible est vérifié.
3. Si l'objet est compatible, il est équipé.
4. Si un objet était déjà équipé, il doit être échangé ou déplacé selon les règles UX retenues.

### Équipement vers inventaire

1. Le slot d'équipement est vidé.
2. L'objet retourne au curseur ou dans l'inventaire personnel.
3. Les statistiques du personnage sont recalculées.

### Inventaire vers inventaire

1. L'objet quitte l'inventaire source.
2. La destination est vérifiée.
3. L'objet rejoint l'inventaire cible.
4. La charge des deux personnages est recalculée.

### Personnage vers réceptacle

1. L'objet quitte le personnage ou le curseur.
2. Le réceptacle vérifie s'il accepte l'objet.
3. L'objet devient propriété du réceptacle.
4. Le réceptacle déclenche ses événements éventuels.

## Charge, poids et Force

La charge d'un personnage est la somme des objets de son inventaire, des objets équipés et éventuellement de l'objet tenu.

La capacité maximale dépend principalement de la Force.

Règle validée : si la charge maximale est dépassée, le personnage subit un malus de déplacement.

## Item Definition

Une `UGridItemDefinitionAsset` décrit les données statiques d'un item : identifiant, nom, description, icône, mesh monde, mesh équipé, poids, type, tags, pile, slots compatibles, effets et comportement.

## Item Instance

Une `FGridItemInstance` représente l'objet réel dans une partie : RuntimeObjectId, ItemDefinitionId, quantité, propriétaire actuel, état spécifique, transform si dans le monde et données de sauvegarde.

## Character Equipment State cible

La cible paper doll est :

```text
Head
Face
Amulet
Shoulders
Shirt
Chest
Cloak
Bracers
Gloves
Belt
Legs
Feet
Ring1
Ring2
Earring1
Earring2
MainHand
OffHand
```

## Sauvegarde future

Le SaveGame devra contenir l'état du donjon, l'état du groupe actif, les personnages, inventaires, équipements, CursorItem ou règles de résolution du CursorItem, position du groupe et niveau courant.

## Plan recommandé

### Phase A — UI-INV2B paper doll

- afficher le personnage de pied en cap ;
- placer les slots autour du personnage ;
- sortir le Cursor du paper doll ;
- garder les slots non supportés comme placeholders visuels.

### Phase B — Alignement C++ des nouveaux slots

- ajouter `Face`, `Shirt`, `Bracers`, `Earring1`, `Earring2` à `EGridEquipmentSlot` ;
- ajouter les champs correspondants à `FGridCharacterEquipmentState` ;
- mettre à jour les fonctions d'accès ;
- adapter les compatibilités d'items ;
- brancher les `RegisterEquipmentSlotWidget` manquants.

### Phase C — Équipement visuel dynamique

- remplacer l'image statique par une image plein corps par race/classe/genre ;
- plus tard, composer ou afficher les équipements réellement portés.

### Phase D — Sauvegarde

- sérialiser les équipements ;
- restaurer le paper doll ;
- traiter le CursorItem lors de la fermeture ou du chargement.

## État d'implémentation résumé

Déjà appliqué :

- types de base d'inventaire et de possession ;
- `UGridPartyInventoryComponent` ;
- groupe actif minimal ;
- inventaires personnels ;
- `CursorItem` ;
- `UGridItemDefinitionAsset` ;
- compatibilités via `CompatibleEquipmentSlots` ;
- type UI générique `EGridInventoryUiSlotType::Equipment` ;
- documentation UI-INV2B du paper doll.

Reste à faire :

- alignement C++ complet des nouveaux slots paper doll ;
- modification finale du `WBP_GridInventory.uasset` ;
- validation runtime ;
- sauvegarde complète ;
- équipement visuel dynamique.
