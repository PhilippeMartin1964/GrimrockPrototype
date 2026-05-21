# Audit des DataAssets GridObjectArchetype

Audit Phase 4C des `UGridObjectArchetypeAsset` existants.

Ce document est un rapport uniquement. Aucun code C++, enum, asset, Blueprint, comportement runtime, lien ou format de sérialisation n'a été modifié.

## 1. Objectif

Ce document audite les DataAssets d'archétypes actuellement présents dans le projet par rapport au design prévu dans :

- `docs/Design/01_GRID_OBJECT_SYSTEM.md`
- `docs/Design/02_OBJECT_ARCHETYPES.md`
- `docs/Design/06_GRID_EDITOR_UX_SPEC.md`
- `docs/Design/07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md`

Un `UGridObjectArchetypeAsset` doit définir l'identité, la classification, le placement, les visuels, les classes runtime et les comportements par défaut d'un archétype concret utilisable dans l'éditeur de grille.

L'audit a été réalisé sans ouvrir ni sauvegarder les `.uasset`. Les valeurs listées ci-dessous proviennent des chemins d'assets, des chaînes lisibles dans les fichiers binaires Unreal, des documents de design et des règles de validation C++ existantes. Quand une valeur n'est pas lisible de façon fiable sans l'éditeur Unreal, elle est marquée `Non lisible` ou `À vérifier`.

## 2. Inventaire des archétypes trouvés

Dossiers inspectés :

- `Content/GrimrockPrototype/Core/DataAssets/`
- `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/`

Le dossier `Content/Grimrock/Core/DataAssets/` n'existe pas dans le checkout inspecté.

| Asset Path | ArchetypeId | DisplayName | SupportedType | ObjectCategory | Palette Category | PlacementKind | RuntimeActorClass | Notes |
|---|---|---|---|---|---|---|---|---|
| `Content/GrimrockPrototype/Core/DataAssets/DA_Arch_Button_ToggleDoor.uasset` | `Button_ToggleDoor` | Non lisible | Button, inféré | Mechanism | À vérifier | Wall | Non lisible | Archétype de bouton orienté puzzle, ne correspond pas au nom canonique `Button_Normal` / `Button_Wall`. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Arch_Lever_OpenSecret.uasset` | `Lever_OpenSecret` | `Lever / Open Secret` | Lever | Mechanism | À vérifier | Wall | `BP_GridLeverActor_C` | Archétype de levier orienté puzzle, pas un `Lever_Standard` générique. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Arch_Plate_HoldDoor.uasset` | `PressurePlate` ou non lisible précisément | `Pressure Plate / Hold Door` | PressurePlate | Mechanism | À vérifier | Center/Floor à vérifier | `BP_GridPressurePlateActor_C` | `TriggerMode=Hold` observé. Nom d'asset orienté puzzle. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Button_Secret_Stone.uasset` | `Button_Secret_Stone` | Non lisible | Button, inféré | Mechanism | À vérifier | Wall | Non lisible | Variante proche de `Button_Secret`, mais ID non canonique. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Door_Stone.uasset` | `Door_Stone` | Non lisible | Door, inféré | Mechanism | À vérifier | Edge | Non lisible | Mesh fixe et mesh mobile observés. La catégorie fonctionnelle `Mechanism` est cohérente avec la validation actuelle, même si le design parle de Passage. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Item_Torch.uasset` | `Item_Torch` | Non lisible | Item, inféré | Item | À vérifier | Floor | `BP_Item_Torch_C`, champ exact à vérifier | Des chaînes `GridItemSpawnBehaviorParams` / `SpawnedItemArchetypeId` apparaissent aussi ; vérifier qu'elles ne sont pas configurées inutilement sur l'item. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_ItemSpawn_Torch.uasset` | `ItemSpawn_Torch` | `Torch Spawn` | ItemSpawn | Spawn | `Spawns` | Floor | `BP_Item_Torch_C` observé | `SpawnedItemArchetypeId=Item_Torch` observé. RuntimeActorClass peut être optionnel pour ItemSpawn selon validation. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Receptacle_WallTorchHolder.uasset` | `Receptacle_WallTorchHolder` | Non lisible | Receptacle, inféré | Receptacle | À vérifier | Wall | `BP_Receptacle_WallTorchHolder_C` | Accepte `Item_Torch`. Variante proche de `Receptacle_TorchHolder`, mais ID non canonique. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_SecretDoor_Stone1.uasset` | `Secret_Door_Stone` | Non lisible | Door, inféré | Mechanism | À vérifier | Edge | `BP_GridSecretDoor_C` | Variante proche de `Door_Secret`, mais ID inversé et non canonique. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Trigger_Cell.uasset` | `Trigger_Cell` | `TriggerCelle` | Trigger | Mechanism | `Triggers` | Center/Floor à vérifier | `BP_GridTriggerActor_C` | DisplayName semble contenir une faute ou un nom temporaire. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_WallInscription.uasset` | `WallInscription` | Non lisible | Decoration, inféré | Readable | À vérifier | Wall | Non lisible | `bIsReadable` et `ReadableText` observés. Le design recommande `Readable_WallInscription`, mais note aussi de ne pas renommer l'existant sans migration. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorBloodStain.uasset` | `FloorBloodStain` | `Floor Blood Stain` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Décoration de sol avec mesh/material de decal. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorBones.uasset` | `FloorBones` / chaîne `FloorBone` aussi présente | `Floor Bones` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Deux chaînes proches `FloorBone` et `FloorBones` observées ; vérifier l'`ArchetypeId` exact. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorCarpet.uasset` | `FloorCarpet` | `Floor Carpet` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Décoration de sol. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorDebris.uasset` | `FloorDebris` | `Floor Debris` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Décoration de sol. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorDust.uasset` | `FloorDust` | `Floor Dust` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Décoration de sol avec material de decal. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorMoss.uasset` | `FloorMoss` | `Floor Moss` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Décoration de sol. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorRoots.uasset` | `FloorRoots` | `Floor Roots` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Description extraite : `Decorative floorroots placed on the floor.` à corriger en wording. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorRubble.uasset` | `FloorRubble` | `Floor Rubble` | Decoration, inféré | Decoration, inféré | `Floor Decorations` | Floor | Non lisible | Décoration de sol. |
| `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_A_FloorRuneCircle.uasset` | `FloorRuneCircle` | `Floor Rune Circle` | Decoration, inféré | Decoration / Light à vérifier selon usage | `Floor Decorations` | Floor | Non lisible | Visuel de rune de sol ; à ne pas confondre avec `Teleporter_Rune` tant que le comportement téléporteur n'est pas configuré. |

Assets non inclus dans l'inventaire d'archétypes :

- `DA_ObjectPalette_Default.uasset` : palette qui référence des archétypes.
- `DA_GridLevelAsset.uasset` et `GrimrockLevels/DA_GridLevel_00.uasset` : assets de niveau, pas des archétypes.

## 3. Archétypes attendus

Comparaison avec les archétypes concrets recommandés dans les documents de design.

| ArchetypeId attendu | Statut | Asset actuel correspondant | Commentaire |
|---|---|---|---|
| `Button_Normal` | Missing | Aucun | Aucun bouton standard générique trouvé. |
| `Button_Secret` | Present but inconsistent | `DA_Button_Secret_Stone` / `Button_Secret_Stone` | Variante présente, mais ID plus spécifique que l'ID canonique attendu. |
| `Button_Wall` | Missing | Aucun | `Button_ToggleDoor` est mural mais décrit un puzzle précis, pas une variante générique. |
| `Lever_Standard` | Present but inconsistent | `DA_Arch_Lever_OpenSecret` / `Lever_OpenSecret` | Levier présent, mais orienté puzzle. Prévoir un archétype générique. |
| `PressurePlate_Stone` | Present but inconsistent | `DA_Arch_Plate_HoldDoor` | Plaque présente, mais ID/nom orientés puzzle. |
| `Door_Stone` | Present | `DA_Door_Stone` | Archétype présent. |
| `Door_Secret` | Present but inconsistent | `DA_SecretDoor_Stone1` / `Secret_Door_Stone` | Porte secrète présente, mais ID non canonique. |
| `Receptacle_Alcove` | Missing | Aucun | Non créé. |
| `Receptacle_TorchHolder` | Present but inconsistent | `DA_Receptacle_WallTorchHolder` / `Receptacle_WallTorchHolder` | Support de torche présent, mais ID différent du design. |
| `Receptacle_Altar` | Missing | Aucun | Non créé. |
| `Receptacle_OfferingBowl` | Missing | Aucun | Non créé. |
| `Lock_Keyhole` | Missing | Aucun | Non créé. |
| `Teleporter_Rune` | Missing | Aucun | `FloorRuneCircle` existe mais reste une décoration de sol, pas un téléporteur confirmé. |
| `Item_Torch` | Present | `DA_Item_Torch` | Archétype présent. Vérifier `ItemActorClass`. |
| `Item_Key` | Missing | Aucun | Non créé. |
| `Item_Coin` | Missing | Aucun | Non créé. |
| `Readable_WallInscription` | Present but inconsistent | `DA_WallInscription` / `WallInscription` | L'existant est explicitement à préserver sans migration. |
| `Spawn_Player` | Missing | Aucun | Non créé comme archétype. |

Archétypes existants hors liste principale :

- `FloorBloodStain`
- `FloorBones`
- `FloorCarpet`
- `FloorDebris`
- `FloorDust`
- `FloorMoss`
- `FloorRoots`
- `FloorRubble`
- `FloorRuneCircle`
- `Button_ToggleDoor`
- `Lever_OpenSecret`
- `Trigger_Cell`
- `ItemSpawn_Torch`

Ces archétypes peuvent rester utiles, mais ils devraient être clairement distingués entre archétypes génériques, variantes visuelles et archétypes orientés puzzle/test.

## 4. Contrôles de cohérence

### Identité

- Les archétypes principaux ont un `ArchetypeId` lisible et non `None` d'après les chaînes extraites.
- Plusieurs IDs ne suivent pas les noms canoniques du design :
  - `Secret_Door_Stone` au lieu de `Door_Secret` ou `Door_Secret_Stone`
  - `Receptacle_WallTorchHolder` au lieu de `Receptacle_TorchHolder`
  - `Button_Secret_Stone` au lieu de `Button_Secret`
  - `Lever_OpenSecret` au lieu d'un `Lever_Standard` générique
- Certains `DisplayName` semblent absents ou non lisibles pour des assets importants (`Door_Stone`, `Button_Secret_Stone`, `Receptacle_WallTorchHolder`, `WallInscription`).

### Classification

- Les mécanismes (`Button`, `Lever`, `PressurePlate`, `Trigger`) utilisent `ObjectCategory=Mechanism`, ce qui est cohérent avec la validation C++.
- Les réceptacles utilisent `ObjectCategory=Receptacle`, cohérent.
- Les items utilisent `ObjectCategory=Item`, cohérent.
- Les spawns utilisent `ObjectCategory=Spawn`, cohérent.
- Le design emploie parfois la notion de `Passage`, mais `EGridObjectCategory` ne contient pas de catégorie `Passage`. Les portes sont donc actuellement classées `Mechanism`, ce qui est cohérent avec le code même si le wording design peut prêter à confusion.

### Placement

- Les boutons, leviers et réceptacles muraux observés sont en `PlacementKind=Wall`, cohérent.
- Les portes observées sont en `PlacementKind=Edge`, cohérent.
- Les décorations de sol sont en `PlacementKind=Floor`, cohérent.
- La plaque et le trigger doivent être `Floor` ou `Center`. Les chaînes extraites ne permettent pas toujours de distinguer une valeur par défaut `Center` d'une valeur explicitement configurée ; c'est acceptable selon la validation, mais à vérifier dans l'éditeur.
- Les flags legacy `bPlaceOnEdge` / `bPlaceAtCellCenter` ne sont pas audités précisément sans l'éditeur. Ils doivent être vérifiés avant toute migration.

### Visuel

- Les décorations de sol ont un `PreviewMesh` et parfois un `PreviewMaterial`, cohérent.
- `DA_Door_Stone` expose des chaînes de mesh fixe et mobile, cohérent pour une porte composite.
- `DA_Arch_Button_ToggleDoor` expose deux meshes de bouton, probablement fixe/mobile, cohérent pour un bouton animé.
- `DA_SecretDoor_Stone1` n'expose pas clairement les meshes fixe/mobile dans les chaînes extraites ; à vérifier, car le design de porte secrète attend une partie fixe et une partie mobile.

### Runtime

- Les classes Blueprint runtime observées sont cohérentes pour :
  - `BP_GridLeverActor_C`
  - `BP_GridPressurePlateActor_C`
  - `BP_GridTriggerActor_C`
  - `BP_Receptacle_WallTorchHolder_C`
  - `BP_GridSecretDoor_C`
- Les classes runtime de `Door_Stone`, `Button_ToggleDoor`, `Button_Secret_Stone` et `WallInscription` ne sont pas lisibles dans l'extraction, bien que le champ `RuntimeActorClass` soit présent. À vérifier dans l'éditeur.
- `Item_Torch` et `ItemSpawn_Torch` référencent `BP_Item_Torch_C`. Il faut vérifier si la référence est bien placée dans `ItemActorClass` pour l'item, et si l'item spawn utilise seulement `DefaultBehavior.ItemSpawn.SpawnedItemArchetypeId`.

### Interaction

- Les boutons, leviers, réceptacles et locks attendus devraient avoir `bIsInteractable=true`.
- Les chaînes binaires ne permettent pas de lire de façon fiable les booléens non textuels. Ces champs doivent être validés dans l'éditeur.
- `DA_WallInscription` a `bIsReadable` et `ReadableText`, cohérent pour un readable.
- Aucun autre readable explicite n'a été trouvé.
- Aucun archétype de lumière runtime explicite n'a été trouvé. `FloorRuneCircle` ne doit pas être traité comme lumière ou téléporteur sans configuration explicite.

### Behavior

- `DA_Arch_Lever_OpenSecret` a `TriggerMode=Toggle`, cohérent pour un levier.
- `DA_Arch_Plate_HoldDoor` a `TriggerMode=Hold`, cohérent pour une plaque de pression.
- `DA_Trigger_Cell` a `TriggerMode=OneShot`, cohérent pour un trigger ponctuel.
- `DA_Receptacle_WallTorchHolder` contient `AcceptedArchetypeIds` et `Item_Torch`, cohérent pour un support de torche.
- `DA_ItemSpawn_Torch` contient `SpawnedItemArchetypeId=Item_Torch`, cohérent.
- `DA_Item_Torch` contient aussi des chaînes liées à `GridItemSpawnBehaviorParams` / `SpawnedItemArchetypeId`. Cela peut être simplement dû au layout sérialisé de `DefaultBehavior`, mais il faut vérifier que l'item ne configure pas un comportement d'item spawn inutile.

## 5. Problèmes trouvés

| Severity | Asset | Field | Problem | Suggested Fix |
|---|---|---|---|---|
| Warning | `DA_SecretDoor_Stone1` | `ArchetypeId` | ID observé `Secret_Door_Stone`, différent du naming attendu `Door_Secret`. Le code de validation contient une règle spéciale pour `Door_Secret`, donc cette variante échappe probablement à cette règle. | Ne pas renommer directement. Prévoir migration ou créer un nouvel archétype canonique puis migrer les références. |
| Warning | `DA_Receptacle_WallTorchHolder` | `ArchetypeId` | ID différent du design `Receptacle_TorchHolder`. | Garder l'existant pour l'instant ; documenter alias/migration avant renommage. |
| Warning | `DA_Button_Secret_Stone` | `ArchetypeId` | Variante proche de `Button_Secret`, mais ID spécifique. | Décider si le canon doit être `Button_Secret` ou `Button_Secret_Stone`; ne pas renommer sans migration. |
| Warning | `DA_Arch_Button_ToggleDoor` | `ArchetypeId` / usage | Archétype orienté puzzle/test plutôt qu'archétype générique `Button_Normal` ou `Button_Wall`. | Créer plus tard un bouton générique et conserver celui-ci comme exemple/puzzle si utile. |
| Warning | `DA_Arch_Lever_OpenSecret` | `ArchetypeId` / usage | Archétype orienté puzzle/test plutôt qu'un `Lever_Standard`. | Créer `Lever_Standard`; conserver ou renommer/migrer l'asset puzzle plus tard. |
| Warning | `DA_Arch_Plate_HoldDoor` | `ArchetypeId` / usage | Plaque orientée puzzle/test plutôt que `PressurePlate_Stone`. | Créer un archétype générique ou migrer avec prudence. |
| Warning | `DA_Trigger_Cell` | `DisplayName` | DisplayName extrait `TriggerCelle`, probablement faute ou nom temporaire. | Corriger le DisplayName dans l'éditeur si confirmé. |
| Info | `DA_WallInscription` | `ArchetypeId` | Le design recommande `Readable_WallInscription`, mais l'existant est `WallInscription`. | Ne pas renommer sans migration ; éventuellement améliorer DisplayName et classification. |
| Info | `DA_A_FloorBones` | `ArchetypeId` | Chaînes `FloorBone` et `FloorBones` toutes deux présentes. | Vérifier l'ID réel dans l'éditeur et la palette. |
| Info | `DA_A_FloorRoots` | `Description` | Description extraite `Decorative floorroots placed on the floor.` | Corriger le texte plus tard si confirmé. |
| Warning | `DA_Item_Torch` | `ItemActorClass` / `DefaultBehavior` | `BP_Item_Torch_C` observé, mais le champ exact est à vérifier ; des chaînes item spawn apparaissent aussi. | Vérifier que `ItemActorClass` est défini et que le behavior ItemSpawn n'est pas configuré inutilement. |
| Info | Plusieurs assets principaux | `DisplayName` | Plusieurs noms éditeur ne sont pas lisibles dans l'extraction binaire. | Vérifier dans l'éditeur et renseigner des DisplayNames clairs pour les designers. |
| Info | Tous assets | `bPlaceOnEdge` / `bPlaceAtCellCenter` | Les flags legacy ne sont pas lisibles de façon fiable dans ce rapport. | Vérifier dans l'éditeur avant migration ; ne pas supprimer. |
| Info | Tous assets | `bIsInteractable` | Les booléens d'interaction ne sont pas lisibles de façon fiable par extraction de chaînes. | Vérifier boutons, leviers, réceptacles et locks dans l'éditeur. |

## 6. Corrections recommandées

### Priorité 1 : correction runtime / éditeur

- Vérifier dans l'éditeur que tous les archétypes `Door`, `Button`, `Lever`, `PressurePlate` et `Receptacle` qui exigent une classe runtime ont une classe cohérente.
- Vérifier `DA_SecretDoor_Stone1` : `SupportedType=Door`, `RuntimeActorClass` dérivée de `AGridDoorActor`, meshes fixe/mobile cohérents.
- Vérifier `DA_Item_Torch` : `SupportedType=Item`, `ItemActorClass=BP_Item_Torch_C` si c'est l'intention, pas de behavior ItemSpawn actif.
- Vérifier `DA_Receptacle_WallTorchHolder` : `bIsInteractable=true`, `SupportedType=Receptacle`, `AcceptedArchetypeIds` contient `Item_Torch`.

### Priorité 2 : cohérence UX / design

- Ajouter ou corriger les `DisplayName` manquants :
  - `Door_Stone`
  - `Button_Secret_Stone`
  - `Button_ToggleDoor`
  - `Receptacle_WallTorchHolder`
  - `WallInscription`
- Créer des archétypes génériques séparés des archétypes puzzle :
  - `Button_Normal`
  - `Button_Wall`
  - `Lever_Standard`
  - `PressurePlate_Stone`
- Clarifier les catégories palette pour éviter le mélange entre mécanismes, passages, décors et spawns.

### Priorité 3 : nettoyage optionnel

- Standardiser le naming des archétypes après migration documentée.
- Corriger les descriptions mineures (`FloorRoots`, `TriggerCelle`).
- Décider si `FloorRuneCircle` reste une simple décoration ou devient une base pour `Teleporter_Rune`.
- Ajouter progressivement :
  - `Item_Key`
  - `Item_Coin`
  - `Receptacle_Alcove`
  - `Receptacle_Altar`
  - `Receptacle_OfferingBowl`
  - `Lock_Keyhole`
  - `Spawn_Player`

## 7. Règles de migration sûres

- Ne jamais renommer un `ArchetypeId` sans migration des niveaux, palettes, links, behaviors et références Blueprint/DataAsset.
- Ne pas changer `RuntimeActorClass` à l'aveugle : vérifier la classe parent attendue et les Blueprints dérivés.
- Ne pas supprimer les flags legacy `bPlaceOnEdge` / `bPlaceAtCellCenter` tant que les assets n'ont pas été migrés et validés.
- Corriger une famille d'assets à la fois :
  - boutons
  - portes
  - réceptacles
  - triggers / plaques
  - items / spawns
  - readable / lumières
- Faire un commit après chaque famille d'assets.
- Après chaque correction d'asset, tester la palette éditeur, le placement, l'inspecteur contextuel, les connecteurs et le spawn runtime correspondant.
- Pour les IDs déjà utilisés dans des niveaux (`Door_Stone`, `Button_Secret_Stone`, `Button_ToggleDoor`, `Receptacle_WallTorchHolder`, `Secret_Door_Stone`, `Item_Torch`, `ItemSpawn_Torch`, `WallInscription`), préférer une migration explicite plutôt qu'un renommage direct.

## 8. Recommandation finale

Ne pas modifier tous les DataAssets d'un coup.

Valider d'abord ce rapport dans l'éditeur Unreal, car certains champs binaires ne sont pas auditables de manière fiable sans charger les assets.

Ensuite corriger les archétypes par petits lots :

1. Boutons
2. Portes
3. Réceptacles
4. Triggers et plaques
5. Items et spawns
6. Readable et lumières

La priorité immédiate n'est pas de renommer les IDs existants, mais de distinguer clairement les archétypes génériques des archétypes orientés puzzle/test, puis de préparer une migration contrôlée pour les noms non canoniques.
