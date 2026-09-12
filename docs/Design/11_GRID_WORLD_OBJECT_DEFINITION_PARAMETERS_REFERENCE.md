# 11 — Référence des paramètres GridWorldObjectDefinitionAsset

Statut : **document actif de référence après ALIGN-B5.3 + WORLDOBJ-RECOVERY01 + états initiaux sémantiques**, 2026-09-12. Version cible : UE 5.5.4.

La déclaration de référence est `Source/GrimrockPrototype/Public/Core/GridWorldObjectDefinitionAsset.h`. Ce guide décrit l'authoring courant après MIG10 et ALIGN-B5.3. Les audits 07/08 et le plan 09 conservent leur vocabulaire historique et ne sont pas des références de schéma actuel.

## 1. Définition et placement

Une `UGridWorldObjectDefinitionAsset` décrit un objet du monde réutilisable. `DefinitionId` doit être non vide et unique. Un placement `FGridWorldObjectInstance` référence cet identifiant par `WorldObjectDefinitionId` ; il ne duplique pas les paramètres permanents.

Les collectibles utilisent directement `UGridItemDefinitionAsset`, et les monstres `UGridMonsterDefinitionAsset`. Ne pas créer de seconde définition world-object pour représenter un item ramassable.

## 2. Identité et classification

| Champ | Usage |
|---|---|
| `DefinitionId` | Identifiant stable de la définition, distinct de l'identité du placement. |
| `DisplayName`, `Description` | Présentation du concept. |
| `SupportedType` / Gameplay Type | Famille fonctionnelle de l'objet. |
| `ObjectCategory` / Functional Category | Classification éditeur et validation. |
| `Category` / Palette Category | Groupement dans la palette. |

Le type gameplay, la catégorie fonctionnelle et la catégorie de palette ont des rôles distincts. La catégorie ne choisit pas à elle seule le comportement runtime.

## 3. Defaults et overrides

`DefaultBehavior` porte les règles partagées, notamment les règles de poids, de réceptacle et les paramètres logiques des mécanismes. La définition ne porte plus de booléens génériques destinés à être copiés comme « enabled » ou « active » lors du placement.

Les états initiaux qui dépendent du puzzle appartiennent à la structure typée du placement :

| Placement | État initial authoré |
|---|---|
| Door | `InstanceConfig.bDoorInitiallyOpen` |
| Teleporter | `InstanceConfig.bTeleporterInitiallyEnabled` |
| Pit | `InstanceConfig.Pit.bInitiallyOpen` |
| Lock | `InstanceConfig.bStartsUnlocked` |
| MonsterSpawn | `bSpawnAtStart` |
| ItemSpawn | `bSpawnAtStart` |
| Lever | aucun override : démarre au repos / Off |
| PressurePlate | aucun état pressé authoré : état dérivé de l'occupation et du poids |

Les données naturellement locales restent `InstanceConfig.Teleporter`, `Transition`, `Pit`, `ReceptacleInitialContent` et `bStartsUnlocked`. RECOVERY01 autorise en plus deux familles d'exceptions **sparse** lorsque l'histoire du niveau exige une différence réelle : `MovingPartOverrides` (`LocalTransform`, `Motion.Amount`, `Motion.Duration`) et les overrides de chaîne (`DoorChainMode`, `bOverrideChainPullDuration`, `ChainPullDuration`). Le resolver combine la définition avec ces données locales sans recopier la définition. Il n'existe ni override de mesh/type/axe/pivot/`ReverseDuration`, ni `ChainPullDistance` d'instance. Voir la [règle Definition / Instance](12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md).

La présence d'un `FGridWorldObjectInstance`, `FGridLooseItemInstance` ou `FGridLogicObjectInstance` dans sa collection native signifie que ce placement existe. La présence initiale d'un générateur de monstre ou d'item est, elle, explicitement contrôlée par `bSpawnAtStart`.

## 4. Placement et comportement spatial

| Champ | Usage |
|---|---|
| `PlacementSurface` | Surface d'authoring : `Floor`, `Wall` ou `Ceiling`. |
| `DefaultLocalPosition` | Coordonnées locales `U`, `V`, `N` relativement à la surface. |
| `bBlocksMovement` | Blocage statique de la cellule. Le passage d'une porte dépend aussi de son état runtime. |
| `bOccupiesBoundary` | Possession de la frontière topologique entre cellules, pour un placement mural. |
| `bReplacesStandardWall` | Suppression visuelle du mur structurel sur cette frontière. |
| `bHideCellFloor` | Suppression du mesh de sol, sans modifier à elle seule la praticabilité. |

Depuis `WORLDOBJ-ALIGN-B5.3`, aucune projection de placement parallèle n'existe plus dans `UGridWorldObjectDefinitionAsset`. Le runtime et l'éditeur consomment directement `PlacementSurface` et `DefaultLocalPosition.U/V/N`. Les anciennes propriétés `PlacementKind`, `PlacementZOffset`, `WallInset`, `LocalOffsetAlongWall` et `LocalOffsetVertical`, ainsi que les helpers de projection associés, ont été supprimés.

Le repère local est défini ainsi :

- **Floor** : `U/V` décrivent le plan du sol et `N` la hauteur au-dessus du sol ;
- **Wall** : `U` suit le mur, `V` est vertical et `N` mesure l'inset depuis la frontière vers l'intérieur de la cellule ;
- **Ceiling** : `U/V` décrivent le plan du plafond et `N` la distance sous le plafond.

Le modèle de données prévoit donc `U/V/N` sur les trois surfaces. Le support runtime courant applique directement les trois composantes sur `Wall`. Pour `Floor` et `Ceiling`, `U/V` restent actuellement ignorés et le placement reste centré en XY ; seule la composante `N` affecte la position verticale. Le plan plafond courant reste fixé à 200 cm dans les chemins de résolution existants.

La face concrète d'un world-object mural placé est portée par l'instance via `WallSide` / `EGridEdge`. `EGridEdge` reste donc un concept actif ; ce n'est pas une surface d'authoring.

## 5. Visuels et motion

`StaticPart` est optionnelle. `MovingParts` fournit exactement deux slots optionnels, `Part0` et `Part1` : zéro, une ou deux parties mobiles.

Les matériaux appartiennent aux Material Slots des Static Meshes. Chaque partie mobile porte sa `Motion` : type, axe, pivot, amplitude, `Duration` (Alpha 0 -> 1) et `ReverseDuration` optionnelle (Alpha 1 -> 0). Lorsque `ReverseDuration <= 0`, le runtime retombe sur `Duration`, ce qui conserve un mouvement symétrique par défaut. Une porte verticale, coulissante ou battante se configure par cette motion ; les volets de Pit utilisent les deux parties mobiles.

La définition reste l'autorité. `MovingPartOverrides` permet seulement une exception locale sparse sur le transform de repos, l'amplitude ou la durée forward d'une partie existante. Après modification de la définition ou d'un override d'instance, reconstruire l'aperçu ou le runtime pour observer la nouvelle présentation.

## 6. Interaction et lumière

- `bIsInteractable` autorise l'interaction directe selon le chemin runtime concerné.
- `bIsReadable`, `ReadableText`, `bShowReadableOnlyOnce` décrivent la lecture ; un texte local explicitement autorisé peut être porté par le placement.
- `bIsLightSource`, `LightColor`, `LightIntensity`, `LightRadius` configurent la lumière.
- `bUseLightFlicker` dépend du support effectif du composant runtime.

## 7. Audio

`AudioEvents` associe des noms sémantiques (`Open`, `Close`, `Press`, etc.) à des `FGridObjectAudioEvent`. `DefaultAudioAttenuation` est le profil spatial unique de la définition.

Les champs audio dépréciés encore présents servent à une migration audio antérieure, pas à l'authoring courant. Utiliser le [contrat audio](GRID_OBJECT_AUDIO_SYSTEM.md) et le [guide des portes](DOOR_AUDIO_SETUP.md).

## 8. Classes runtime et palette

`RuntimeActorClass` choisit la classe d'acteur world-object. `ItemActorClass` reste déclaré, mais ne remplace pas l'autorité directe de `ItemDefinition` pour les collectibles.

Une entrée `FGridObjectPaletteEntry` référence `DefaultWorldObjectDefinition` pour un objet du monde ou `DefaultItemDefinition` pour un collectible. Le runtime reçoit les définitions world-object dans `WorldObjectDefinitions` ; la palette reste un outil d'authoring.

## 9. Validation et chemins de packages

Avant utilisation, vérifier `ValidateDefinition()` et `ValidateCurrentLevel()` : identifiant unique, surface valide, type/classe runtime cohérents, composition visuelle et références du niveau. Un warning de contenu doit être compris, pas masqué par une resauvegarde.

Le dossier historique `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/` reste inchangé. Son nom ne désigne plus une classe ni un concept architectural actif. Une réorganisation éventuelle des packages relève d'une tâche AssetTools distincte.

Les Core Redirects temporaires préservent le chargement des références externes utilisant les anciens noms ; voir le [rapport MIG10](../Architecture/WORLDOBJ_MIG10_FINAL.md).
