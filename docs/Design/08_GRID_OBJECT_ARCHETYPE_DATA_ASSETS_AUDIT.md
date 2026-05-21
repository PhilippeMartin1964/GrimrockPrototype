# Audit des DataAssets GridObjectArchetype

Audit des `UGridObjectArchetypeAsset` existants, mis à jour après la normalisation de noms validée dans UE5.

Ce document reste un rapport de documentation. Il ne modifie pas le code C++, les enums, les DataAssets, les Blueprints, le runtime, la sérialisation ou les liens.

## 1. Objectif

Ce document audite les archétypes actuellement présents par rapport au design prévu dans :

- `docs/Design/01_GRID_OBJECT_SYSTEM.md`
- `docs/Design/02_OBJECT_ARCHETYPES.md`
- `docs/Design/06_GRID_EDITOR_UX_SPEC.md`
- `docs/Design/07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md`
- `docs/Design/09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md`

La Phase 5A a validé dans UE5 une première normalisation directe des noms de DataAssets et `ArchetypeId`. Les anciens noms orientés puzzle/test ont été remplacés par des noms génériques pour les archétypes principaux.

## 2. Inventaire des archétypes principaux

| Asset Path | ArchetypeId | DisplayName | SupportedType | ObjectCategory | Palette Category | PlacementKind | RuntimeActorClass | Notes |
|---|---|---|---|---|---|---|---|---|
| `Content/GrimrockPrototype/Core/DataAssets/DA_Button_Normal.uasset` | `Button_Normal` | Button / à vérifier | Button | Mechanism | Mechanisms / à vérifier | Wall | `AGridButtonActor` / BP dérivé à vérifier | Archétype générique de bouton normal, validé UE5. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Button_Secret.uasset` | `Button_Secret` | Secret Button / à vérifier | Button | Mechanism | Mechanisms / à vérifier | Wall | `AGridButtonActor` / BP dérivé à vérifier | Archétype générique de bouton secret, validé UE5. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Button_Wall.uasset` | `Button_Wall` | Wall Button | Button | Mechanism | Mechanisms | Wall | `AGridButtonActor` / BP dérivé | À créer. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Lever.uasset` | `Lever` | Lever | Lever | Mechanism | Mechanisms / à vérifier | Wall | `BP_GridLeverActor_C` ou équivalent | Le choix validé est la forme courte `Lever`, tant qu'il n'existe pas plusieurs variantes. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_PressurePlate.uasset` | `PressurePlate` | Pressure Plate | PressurePlate | Mechanism | Mechanisms / à vérifier | Floor ou Center | `BP_GridPressurePlateActor_C` ou équivalent | Le choix validé est `PressurePlate`, pas une forme longue. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Trigger.uasset` | `Trigger` | Trigger | Trigger | Mechanism | Triggers / à vérifier | Floor ou Center | `BP_GridTriggerActor_C` ou équivalent | Le choix validé est `Trigger`, pas une forme longue. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Door_Stone.uasset` | `Door_Stone` | Stone Door / à vérifier | Door | Mechanism | Doors / à vérifier | Edge | `AGridDoorActor` / BP dérivé à vérifier | Déjà canonique. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Door_Secret.uasset` | `Door_Secret` | Secret Door / à vérifier | Door | Mechanism | Doors / à vérifier | Edge ou Wall | `BP_GridSecretDoor_C` ou BP dérivé de `AGridDoorActor` | Nom canonique validé UE5. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.uasset` | `Receptacle_TorchHolder` | Torch Holder / à vérifier | Receptacle | Receptacle | Receptacles / à vérifier | Wall | `BP_Receptacle_TorchHolder_C` ou équivalent | Nom canonique validé UE5. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_Item_Torch.uasset` | `Item_Torch` | Torch / à vérifier | Item | Item | Items / à vérifier | Floor | `BP_Item_Torch_C` via `ItemActorClass` à vérifier | Déjà canonique. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_ItemSpawn_Torch.uasset` | `ItemSpawn_Torch` | Torch Spawn | ItemSpawn | Spawn | Spawns | Floor ou Center | Optionnel | Déjà canonique. Doit pointer vers `Item_Torch`. |
| `Content/GrimrockPrototype/Core/DataAssets/DA_WallInscription.uasset` | `WallInscription` | Wall Inscription / à vérifier | Decoration | Readable | Readable / à vérifier | Wall | Système readable existant | À préserver sous ce nom. |

## 3. Décorations et assets secondaires

Les décorations de sol peuvent conserver leurs IDs actuels. Elles ne bloquent pas la normalisation des archétypes gameplay.

| Asset | ArchetypeId | Status | Notes |
|---|---|---|---|
| `DA_A_FloorBloodStain` | `FloorBloodStain` | Conservé | Décoration de sol. |
| `DA_A_FloorBones` | `FloorBones` | Conservé | Vérifier seulement l'absence d'ancien alias `FloorBone` si besoin. |
| `DA_A_FloorCarpet` | `FloorCarpet` | Conservé | Décoration de sol. |
| `DA_A_FloorDebris` | `FloorDebris` | Conservé | Décoration de sol. |
| `DA_A_FloorDust` | `FloorDust` | Conservé | Décoration de sol. |
| `DA_A_FloorMoss` | `FloorMoss` | Conservé | Décoration de sol. |
| `DA_A_FloorRoots` | `FloorRoots` | Conservé | Corriger éventuellement la description, sans urgence. |
| `DA_A_FloorRubble` | `FloorRubble` | Conservé | Décoration de sol. |
| `DA_A_FloorRuneCircle` | `FloorRuneCircle` | Conservé comme Decoration | Ne pas confondre avec `Teleporter_Rune`. |

## 4. Archétypes attendus

| ArchetypeId attendu | Statut Phase 5A | Asset correspondant | Commentaire |
|---|---|---|---|
| `Button_Normal` | Présent / validé UE5 | `DA_Button_Normal` | Remplace l'ancien bouton orienté puzzle/test. |
| `Button_Secret` | Présent / validé UE5 | `DA_Button_Secret` | Nom canonique validé. |
| `Button_Wall` | À créer | `DA_Button_Wall` | Pas encore créé. |
| `Lever` | Présent / validé UE5 | `DA_Lever` | Forme courte choisie tant qu'il n'existe pas plusieurs variantes. |
| `PressurePlate` | Présent / validé UE5 | `DA_PressurePlate` | Forme courte choisie tant qu'il n'existe pas plusieurs variantes. |
| `Trigger` | Présent / validé UE5 | `DA_Trigger` | Forme courte choisie tant qu'il n'existe pas plusieurs variantes. |
| `Door_Stone` | Présent / déjà canonique | `DA_Door_Stone` | Conserver. |
| `Door_Secret` | Présent / validé UE5 | `DA_Door_Secret` | Remplace l'ancien nom de porte secrète. |
| `Receptacle_TorchHolder` | Présent / validé UE5 | `DA_Receptacle_TorchHolder` | Remplace l'ancien support de torche mural. |
| `Receptacle_Alcove` | Manquant | `DA_Receptacle_Alcove` | À créer plus tard. |
| `Receptacle_Altar` | Manquant | `DA_Receptacle_Altar` | À créer plus tard. |
| `Receptacle_OfferingBowl` | Manquant | `DA_Receptacle_OfferingBowl` | À créer plus tard. |
| `Lock_Keyhole` | Manquant | `DA_Lock_Keyhole` | À créer plus tard. |
| `Teleporter_Rune` | Manquant | `DA_Teleporter_Rune` | À créer séparément de `FloorRuneCircle`. |
| `Item_Torch` | Présent / déjà canonique | `DA_Item_Torch` | Conserver. |
| `Item_Key` | Manquant | `DA_Item_Key` | À créer plus tard. |
| `Item_Coin` | Manquant | `DA_Item_Coin` | À créer plus tard. |
| `ItemSpawn_Torch` | Présent / déjà canonique | `DA_ItemSpawn_Torch` | Conserver. |
| `WallInscription` | Présent / conservé | `DA_WallInscription` | Ne pas renommer. |
| `Spawn_Player` | Manquant | `DA_Spawn_Player` | À créer plus tard. |

## 5. Contrôles de cohérence

### Identité

- Les archétypes principaux validés suivent désormais la règle `DA_<ArchetypeId>`.
- Les anciens noms orientés puzzle/test ont été remplacés par des noms génériques pour les objets principaux.
- `Lever`, `PressurePlate` et `Trigger` sont volontairement courts : les formes longues seront utiles seulement si plusieurs variantes apparaissent.
- `WallInscription` reste l'exception historique explicitement conservée.

### Classification

- `Button_Normal`, `Button_Secret`, `Button_Wall`, `Lever`, `PressurePlate` et `Trigger` doivent rester `ObjectCategory=Mechanism`.
- `Door_Stone` et `Door_Secret` restent `SupportedType=Door`. `ObjectCategory=Mechanism` reste cohérent avec l'enum actuelle.
- `Receptacle_TorchHolder` reste `SupportedType=Receptacle` et `ObjectCategory=Receptacle`.
- `Item_Torch` reste `SupportedType=Item` et `ObjectCategory=Item`.
- `ItemSpawn_Torch` reste `SupportedType=ItemSpawn` et `ObjectCategory=Spawn`.
- `WallInscription` reste `SupportedType=Decoration`, `ObjectCategory=Readable` et `bIsReadable=true`.

### Placement

- Boutons, levier et support de torche : `PlacementKind=Wall`.
- Portes : `PlacementKind=Edge` ou `Wall` selon l'asset, mais elles doivent rester des objets de passage.
- Plaque et trigger : `PlacementKind=Floor` ou `Center`.
- Item et item spawn : `PlacementKind=Floor` ou `Center`.

### Runtime et behavior

- Les classes runtime doivent être revérifiées dans UE5 après renommage, mais aucun changement C++ n'a été nécessaire.
- Les connecteurs et niveaux doivent utiliser les `ArchetypeId` canoniques après migration.
- `ItemSpawn_Torch` doit continuer à référencer `Item_Torch`.
- `Receptacle_TorchHolder` doit continuer à accepter `Item_Torch`.

## 6. Problèmes restants

| Severity | Asset | Field | Problem | Suggested Fix |
|---|---|---|---|---|
| Warning | `DA_Button_Wall` | Asset | Archétype encore à créer. | Créer l'asset quand la variante murale visible est nécessaire. |
| Info | `DA_WallInscription` | Naming | Ne suit pas la forme `Readable_*`, mais c'est une exception volontaire. | Conserver. Ne pas renommer sans migration explicite. |
| Info | `DA_A_FloorRuneCircle` | Role | Reste une décoration, pas un téléporteur. | Créer `DA_Teleporter_Rune` séparément. |
| Info | Décorations de sol | Filename | Le préfixe `DA_A_` n'est pas strictement `DA_<ArchetypeId>`. | Reporter ce nettoyage ; priorité faible. |

## 7. Recommandations

### Priorité 1

- Vérifier dans UE5 que la palette pointe vers les noms canoniques.
- Vérifier que les niveaux chargent sans warnings d'archétype manquant.
- Vérifier que les connecteurs existants se résolvent.

### Priorité 2

- Créer `DA_Button_Wall`.
- Créer les réceptacles manquants : `DA_Receptacle_Alcove`, `DA_Receptacle_Altar`, `DA_Receptacle_OfferingBowl`.
- Créer `DA_Lock_Keyhole`.

### Priorité 3

- Créer `DA_Item_Key`, `DA_Item_Coin`, `DA_Teleporter_Rune` et `DA_Spawn_Player`.
- Nettoyer éventuellement les noms de fichiers des décorations de sol.

## 8. Recommandation finale

La normalisation principale est validée pour les archétypes gameplay existants.

Ne pas rouvrir les anciens noms orientés puzzle/test. À partir de maintenant, les nouveaux documents, palettes et niveaux doivent utiliser :

- `Button_Normal`
- `Button_Secret`
- `Button_Wall` à créer
- `Lever`
- `PressurePlate`
- `Trigger`
- `Door_Stone`
- `Door_Secret`
- `Receptacle_TorchHolder`
- `Item_Torch`
- `ItemSpawn_Torch`
- `WallInscription`
