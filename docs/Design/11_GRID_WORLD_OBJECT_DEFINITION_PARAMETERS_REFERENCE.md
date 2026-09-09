# 11 — Référence des paramètres GridWorldObjectDefinitionAsset

Statut : **document actif de référence**, 2026-09-09. Version cible : UE 5.5.4.

La déclaration de référence est `Source/GrimrockPrototype/Public/Core/GridWorldObjectDefinitionAsset.h`. Ce guide décrit l'authoring courant après MIG10. Les audits 07/08 et le plan 09 conservent leur vocabulaire historique et ne sont pas des références de schéma actuel.

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

`bDefaultInitiallyEnabled`, `bDefaultInitiallyActive` et `DefaultTag` fournissent les valeurs initiales au placement. `DefaultBehavior` porte les règles partagées, notamment les règles de poids, de réceptacle et les paramètres logiques des mécanismes.

Les seules configurations comportementales persistantes propres à une instance world-object sont `InstanceConfig.Teleporter`, `Transition`, `Pit`, `ReceptacleInitialContent` et `bStartsUnlocked`. Le resolver combine la définition avec ces données locales. Voir la [règle Definition / Instance](12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md).

## 4. Placement et comportement spatial

| Champ | Usage |
|---|---|
| `PlacementSurface` | Surface d'authoring : `Floor`, `Wall` ou `Ceiling`. |
| `DefaultLocalPosition` | Coordonnées locales `U`, `V`, `N` relativement à la surface. |
| `bBlocksMovement` | Blocage statique de la cellule. Le passage d'une porte dépend aussi de son état runtime. |
| `bOccupiesBoundary` | Possession de la frontière topologique entre cellules, pour un placement mural. |
| `bReplacesStandardWall` | Suppression visuelle du mur structurel sur cette frontière. |
| `bHideCellFloor` | Suppression du mesh de sol, sans modifier à elle seule la praticabilité. |

Au sol, `U/V` décrivent le plan et `N` la hauteur. Sur un mur, `U` suit le mur, `V` est vertical et `N` l'enfoncement vers la cellule. Au plafond, `N` mesure la distance sous le plafond.

Les propriétés transitoires de projection telles que `PlacementKind` et les anciens offsets ne sont pas des paramètres d'authoring sérialisés. Elles restent un détail d'implémentation préexistant ; MIG10 n'en crée ni n'en migre aucune.

## 5. Visuels et motion

`StaticPart` est optionnelle. `MovingParts` fournit exactement deux slots optionnels, `Part0` et `Part1` : zéro, une ou deux parties mobiles.

Les matériaux appartiennent aux Material Slots des Static Meshes. Chaque partie mobile porte sa `Motion` : type, axe, pivot, amplitude et durée. Une porte verticale, coulissante ou battante se configure par cette motion ; les volets de Pit utilisent les deux parties mobiles.

L'instance ne possède pas de copie de cette géométrie. Après modification de la définition, reconstruire l'aperçu ou le runtime pour observer la nouvelle présentation.

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
