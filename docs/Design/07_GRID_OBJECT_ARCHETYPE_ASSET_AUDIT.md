# Audit de GridObjectArchetypeAsset

Audit Phase 4A de `UGridObjectArchetypeAsset`.

Ce document est uniquement documentaire. Il ne propose pas de refactor C++ immédiat et ne demande aucun changement d'enum, de logique runtime, de logique éditeur, de DataAsset ou de sérialisation.

## 1. Rôle de GridObjectArchetypeAsset

`UGridObjectArchetypeAsset` définit les données par défaut d'identité, de classification, de placement, de visuel, d'acteur runtime, de lumière, d'interaction et de comportement pour un archétype concret d'objet de grille.

Un archétype n'est pas la même chose qu'un type d'objet. `EGridLevelObjectType` décrit une grande famille de gameplay, par exemple Door, Button, Lever, Receptacle, Trigger ou Item. Un archétype concret décrit une variante utilisable et authorée, par exemple `Button_Secret`, `Door_Stone`, `Receptacle_TorchHolder` ou un archétype d'objet d'inventaire.

Les objets placés stockent leurs données d'instance dans `FGridLevelObjectData` : `ArchetypeId`, cellule, edge, état enabled/active, tag, notes, texte lisible surchargé, identifiant d'entrée palette et `FGridObjectBehaviorParams`. Au moment du placement, les valeurs sélectionnées dans l'éditeur sont copiées dans l'objet placé. Au runtime, `ArchetypeId` est résolu vers `UGridObjectArchetypeAsset` pour choisir les meshes, matériaux, transforms de placement, classes d'acteur, options de lumière/texte lisible et métadonnées d'item.

## 2. Tableau d'audit des champs

Les recommandations utilisent le vocabulaire suivant :

- `Essential` : donnée centrale requise par l'éditeur ou le runtime actuel.
- `Advanced` : donnée valide, mais à cacher ou afficher surtout dans des contextes expert/debug.
- `Archetype Only` : donnée à éditer dans les DataAssets, pas par instance placée.
- `Legacy` : conservée pour compatibilité ou migration.
- `Remove Later` : candidate à la suppression après migration et vérification.
- `Needs Clarification` : rôle valide, mais nommage, usage ou propriété fonctionnelle ambigus.

### Identité

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `ArchetypeId` | Archetype | `FName` | Validation palette, placement d'objet, affichage/édition via `FGridLevelObjectData::ArchetypeId`, labels overview. | Lookup d'archétype, spawn d'items, lookup readable/item/receptacle. | Tous les objets et items avec archétype. | Les objets placés stockent `ArchetypeId`; le champ du DataAsset reste archetype-only. | Essential | Identifiant stable. Le renommer casse les objets placés et références sans migration. |
| `DisplayName` | Archetype | `FText` | L'inspector et les résumés de connecteurs/objets le préfèrent quand il existe. | Pas utilisé directement par le comportement runtime. | Tous les archétypes affichables. | Non. | Archetype Only | Nom destiné aux designers. Peut être amélioré sans risque lourd de sérialisation. |
| `SupportedType` | Archetype | `EGridLevelObjectType` | Validation, affectation type/palette, attentes de l'inspector contextuel. | Les exigences de spawn/runtime et validation en dépendent, même si l'objet placé stocke aussi `Type`. | Tous les archétypes. | L'objet placé stocke `Type`; l'archétype reste la source du type attendu. | Essential | Confus avec `Type` placé, `Category` et `ObjectCategory`. |
| `Description` | Archetype | `FText` | Principalement documentation d'authoring. | Pas utilisé par le comportement runtime. | Tous les archétypes. | Non. | Archetype Only | Utile dans les DataAssets, encore peu exposé dans l'inspector. |

### Valeurs par défaut

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bDefaultInitiallyEnabled` | Defaults | `bool` | Défaut destiné à être copié dans l'état placé; l'inspector édite `bInitiallyEnabled`. | Le runtime vérifie `ObjectData.bInitiallyEnabled` pour spawn/activation. | Tous les objets placés. | Oui, via `FGridLevelObjectData::bInitiallyEnabled`. | Essential | Doit rester un défaut d'archétype, pas une source runtime vivante après placement. |
| `bDefaultInitiallyActive` | Defaults | `bool` | Défaut destiné à être copié dans l'état placé; l'inspector édite `bInitiallyActive`. | Les objets runtime, portes et receptacles lisent `ObjectData.bInitiallyActive`. | Doors, receptacles, mécanismes, triggers. | Oui, via `FGridLevelObjectData::bInitiallyActive`. | Essential | Le sens varie selon le type : ouvert, pressé, rempli, activé, etc. Labels UI contextuels nécessaires. |
| `DefaultTag` | Defaults | `FName` | Défaut prévu pour le tag de l'objet placé; l'inspector édite l'instance `Tag`. | Le receptacle peut utiliser `ObjectData.Tag` comme fallback de tag accepté si les listes sont vides. | Surtout receptacles et objets d'interaction taggés. | Oui, via `FGridLevelObjectData::Tag`. | Advanced | Utile mais technique. À garder hors UI primaire sauf advanced/debug/contextuel. |
| `DefaultBehavior` | Defaults | `FGridObjectBehaviorParams` | Copié vers les objets placés et utilisé par le helper de reset; l'inspector édite `Obj.Behavior`. | Le runtime lit `ObjectData.Behavior`, pas l'archétype directement, pour activation, trigger, teleporter, receptacle, button, item spawn. | Trigger, pressure plate, button, lever, receptacle, teleporter, item spawn. | Oui, via `FGridLevelObjectData::Behavior`. | Essential | Pont principal entre archétype et instance. Contient plusieurs groupes utiles seulement à certains types. |
| `ItemTags` | Item | `TArray<FName>` | Validation et authoring item/receptacle. | Les acteurs item sont initialisés avec ces tags; les receptacles inspectent les tags de l'archétype item. | Items et archétypes apparentés. | Pas d'override placé actuellement. | Essential | Runtime-relevant pour inventaire et matching receptacle. |

### Palette / Classification

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `Category` | Palette | `FName` | Groupement de palette et hints de validation; affiché read-only dans l'inspector. | Pas utilisé en gameplay. | Tous les archétypes/palette entries. | Non. | Archetype Only | Confus avec `ObjectCategory`; celui-ci sert uniquement à l'organisation UI. |
| `ObjectCategory` | Archetype | `EGridObjectCategory` | Validation, overview/inspector, attentes readable/light contextuelles. | Pas directement gameplay, mais les helpers l'utilisent pour déterminer la pertinence de certains paramètres. | Tous les archétypes. | Non. | Needs Clarification | Classification fonctionnelle/éditeur, différente de `SupportedType` et `Category`. |

### Placement

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `PlacementKind` | Placement | `EGridObjectPlacementKind` | Détermine placement edge/cell, validation, inspector, conflits, fallback de centre viewport. | La transform runtime choisit wall/edge/center/ceiling via helpers. | Tous les archétypes placés. | Pas d'override direct; l'objet placé stocke cell/edge et `LocalYaw`. | Essential | Source de vérité du placement. Remplace les booléens legacy. |
| `bPlaceOnEdge` | Placement\|Legacy | `bool` | Validation si incohérent; validation palette aussi. | Pas source runtime. | Assets legacy. | Non. | Legacy | Déjà legacy/advanced. À supprimer seulement après migration. |
| `bPlaceAtCellCenter` | Placement\|Legacy | `bool` | Validation si incohérent. | Pas source runtime. | Assets legacy. | Non. | Legacy | Candidat à suppression après migration. |
| `bCanShareCell` | Placement | `bool` | Le placement vérifie les conflits entre archétypes nouveau/existant. | Pas utilisé au runtime. | Objets partageables en cellule, decorations, floor objects, triggers. | Non. | Essential | Règle d'éditeur importante pour éviter les placements dangereux. |
| `bCanShareAnchor` | Placement | `bool` | Le placement vérifie les conflits sur même edge/ancre; validation warning pour doors. | Pas utilisé au runtime. | Objets edge/wall : doors, buttons, levers, receptacles. | Non. | Essential | Empêche les chevauchements d'ancre si nécessaire. |
| `bBlocksMovement` | Placement | `bool` | Affiché read-only; validation/tooltips précisent le cas non-door. | `AGridGenericObjectActor` règle la collision mesh avec ce champ. Les portes bloquent ailleurs. | Props/decorations génériques bloquants. | Non. | Needs Clarification | Confus pour les portes. UI à contextualiser comme blocage générique/non-door. |
| `PlacementZOffset` | Placement | `float` | Preview placement et centre de connecteur viewport. | Transform runtime pour objets wall/center. | Objets visuels placés, sauf doors qui ont transform edge spéciale. | Non. | Essential | Offset vertical visuel important. |
| `WallInset` | Placement\|Wall | `float` | Preview placement, centre de connecteur pour wall/edge, validation. | Transform runtime wall-mounted. | Wall/edge objects : buttons, levers, wall decor, receptacles, lights. | Non. | Essential | Pertinent seulement si `PlacementKind` est Wall ou Edge. |
| `LocalOffsetAlongWall` | Placement\|Wall | `float` | Preview placement, centre connecteur, validation. | Transform runtime wall-mounted. | Wall/edge objects. | Non. | Essential | Offset latéral le long du mur. |
| `LocalOffsetVertical` | Placement\|Wall | `float` | Preview placement, centre connecteur, validation. | Transform runtime wall-mounted. | Wall/edge objects. | Non. | Essential | Ajouté à `PlacementZOffset`. |
| `RotationStepYaw` | Placement\|Rotation | `float` | Le rotate selected utilise le step d'archétype si disponible. | La transform runtime utilise `LocalYaw` placé, pas ce champ directement. | Objets center/floor/ceiling ou supportant le yaw local. | Non; influence une action éditeur qui modifie `LocalYaw`. | Advanced | Paramètre d'outil/authoring, pas donnée gameplay directe. |

### Interaction

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bIsInteractable` | Interaction | `bool` | L'inspector et la validation affichent/vérifient les attentes d'interaction. | L'interaction runtime est surtout pilotée par acteurs/composants; ce champ n'est pas l'unique source d'autorité. | Buttons, levers, receptacles, decorations lisibles. | Non. | Needs Clarification | Classification utile, mais l'autorité runtime doit être mieux documentée. |
| `bIsReadable` | Interaction | `bool` | L'inspector affiche la section readable si true; validation de cohérence. | Generic object actor copie `ReadableText` et le flag only-once si true. | Decorations/props lisibles. | Non, mais le texte placé peut override. | Essential | Flag runtime readable. Confus avec `ObjectCategory == Readable`. |
| `ReadableText` | Interaction | `FText` | Texte DataAsset; l'inspector édite plutôt `OverrideReadableText` de l'instance. | Generic object actor l'utilise sauf override placé. | Objets lisibles. | Oui, via `FGridLevelObjectData::OverrideReadableText`. | Essential | Texte par défaut d'archétype. L'override instance doit rester le chemin inspector. |
| `bShowReadableOnlyOnce` | Interaction | `bool` | Validation seulement si readable. | Generic object actor le stocke comme comportement runtime. | Objets lisibles. | Pas d'override instance actuellement. | Advanced | Le flow activation/readable doit être documenté. |

### Lumière

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bIsLightSource` | Light | `bool` | L'inspector affiche une section Light si true; validation de cohérence. | Generic object actor active une point light si true. | Archétypes Light et props/receptacles lumineux. | Non. | Essential | Flag runtime light. Confus avec `SupportedType == Light` et `ObjectCategory == Light`. |
| `LightColor` | Light | `FLinearColor` | Affiché read-only dans l'inspector. | Generic object actor applique la couleur. | Light sources. | Non. | Essential | Réglage archetype-only visuel/runtime. |
| `LightIntensity` | Light | `float` | Affiché read-only; validation > 0 pour lights. | Generic object actor applique l'intensité. | Light sources. | Non. | Essential | Peut chevaucher les defaults de `GridLightEmitterComponent`. |
| `LightRadius` | Light | `float` | Affiché read-only; validation > 0 pour lights. | Generic object actor applique le radius. | Light sources. | Non. | Essential | À garder archetype-only pour l'instant. |
| `bUseLightFlicker` | Light | `bool` | Affiché read-only "Flicker"; validation de cohérence. | Pas appliqué par `AGridGenericObjectActor`; le flicker existe dans `GridLightEmitterComponent`. | Light sources. | Non. | Needs Clarification | Champ validé/affiché, mais pas câblé dans le chemin generic light. |

### Visuel

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `PreviewMesh` | Visual | `TObjectPtr<UStaticMesh>` | Mesh préféré pour preview éditeur; affichage advanced/debug. | Mesh préféré avant `MovingMesh` et `FixedMesh` dans le spawn runtime générique; validation de présence. | La plupart des objets visibles. | Non. | Essential | Le nom prête à confusion : ce n'est pas editor-only, c'est le mesh principal/simple. |
| `PreviewMaterial` | Visual | `TObjectPtr<UMaterialInterface>` | Matériau préféré de preview; peu exposé dans l'inspector. | Matériau préféré avant moving/fixed. | La plupart des objets visibles. | Non. | Essential | Même confusion que `PreviewMesh`. |
| `FixedMesh` | Visual | `TObjectPtr<UStaticMesh>` | Affichage advanced/debug; validation de pertinence. | Fallback mesh; partie fixe mechanism/door; items composites. | Doors, mécanismes composites, items. | Non. | Advanced | À garder advanced pour les objets simples. |
| `MovingMesh` | Visual | `TObjectPtr<UStaticMesh>` | Affichage advanced/debug; validation de pertinence. | Fallback mesh; partie mobile mechanism/door; visuels item/receptacle. | Doors, buttons, levers, receptacles, items. | Non. | Advanced | Important pour acteurs composites/animés. Distinction à clarifier face à `PreviewMesh`. |
| `FixedMaterial` | Visual | `TObjectPtr<UMaterialInterface>` | Validation de pertinence. | Matériau fallback; matériau fixed mechanism/door. | Doors, mécanismes composites, items. | Non. | Advanced | À associer à `FixedMesh`. |
| `MovingMaterial` | Visual | `TObjectPtr<UMaterialInterface>` | Validation de pertinence. | Matériau fallback; matériau moving mechanism/door; visuels item/receptacle. | Doors, buttons, levers, receptacles, items. | Non. | Advanced | À associer à `MovingMesh`. |

### Runtime

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `RuntimeActorClass` | Runtime | `TSubclassOf<AGridRuntimeObjectActor>` | Validation et affichage advanced/debug. | Classe de spawn pour objets runtime; requise pour plusieurs types; initialisation spécifique via casts. | Doors, buttons, levers, pressure plates, teleporters, receptacles, runtime objects. | Non. | Essential | Définit comment l'archétype est instancié. Confus avec `SupportedType`, qui définit ce qu'il est. |
| `ItemActorClass` | Runtime | `TSubclassOf<AGridItemActor>` | Validation des archétypes item. | Le helper de spawn item l'utilise avec fallback vers `AGridItemActor`. | Archétypes item et item spawns. | Non. | Essential | Séparé de `RuntimeActorClass`; utilisé pour les items monde/inventaire. |

### Validation / Helpers

`UGridObjectArchetypeAsset` expose aussi des helpers qui ne sont pas des UPROPERTY :

- `IsEdgePlaced`, `IsCenterPlaced`, `IsWallPlaced`, `IsCeilingPlaced`
- `IsReadable`, `IsLightSource`
- `ValidateArchetype`, `IsValidArchetype`, `GetValidationSummary`
- `RequiresEdgePlacement`, `SupportsCenterPlacement`, `SupportsWallPlacement`
- `RequiresRuntimeActorClass`, `AllowsInvisibleRuntimeObject`
- `UsesWallPlacementParams`, `UsesCenterPlacementParams`, `UsesReadableParams`, `UsesLightParams`
- `UsesItemParams`, `UsesItemSpawnParams`, `UsesReceptacleParams`, `UsesTeleporterParams`
- `UsesButtonAnimationParams`, `UsesTriggerParams`, `UsesMovingMeshParams`, `UsesFixedMeshParams`, `UsesRuntimeActorClass`

Ces helpers sont importants parce qu'ils encodent l'intention de design actuelle. Les nettoyages futurs de l'éditeur devraient les réutiliser au lieu de dupliquer des règles par type dans l'UI.

### Champs legacy

| Champ | Catégorie | Type | Utilisé dans l'éditeur | Utilisé au runtime | Types d'objets concernés | Override d'instance ? | Recommandation | Notes |
|---|---|---|---|---|---|---|---|---|
| `bPlaceOnEdge` | Placement\|Legacy | `bool` | Validation et warnings de compatibilité. | Non. | Archétypes legacy migrés. | Non. | Legacy | À conserver jusqu'à migration complète des assets et palette entries. |
| `bPlaceAtCellCenter` | Placement\|Legacy | `bool` | Validation et warnings de compatibilité. | Non. | Archétypes legacy migrés. | Non. | Legacy | Même stratégie de migration que `bPlaceOnEdge`. |

## 3. Champs groupés par responsabilité

### Identité

- `ArchetypeId`
- `DisplayName`
- `SupportedType`
- `Description`

### Valeurs par défaut

- `bDefaultInitiallyEnabled`
- `bDefaultInitiallyActive`
- `DefaultTag`
- `DefaultBehavior`
- `ItemTags`

### Palette / Classification

- `Category`
- `ObjectCategory`

### Placement

- `PlacementKind`
- `bCanShareCell`
- `bCanShareAnchor`
- `bBlocksMovement`
- `PlacementZOffset`
- `WallInset`
- `LocalOffsetAlongWall`
- `LocalOffsetVertical`
- `RotationStepYaw`

### Interaction

- `bIsInteractable`
- `bIsReadable`
- `ReadableText`
- `bShowReadableOnlyOnce`

### Lumière

- `bIsLightSource`
- `LightColor`
- `LightIntensity`
- `LightRadius`
- `bUseLightFlicker`

### Visuel

- `PreviewMesh`
- `PreviewMaterial`
- `FixedMesh`
- `MovingMesh`
- `FixedMaterial`
- `MovingMaterial`

### Runtime

- `RuntimeActorClass`
- `ItemActorClass`

### Validation / Helpers

- Les helpers listés plus haut.
- Les messages de validation utilisent l'état de l'archétype pour détecter les configurations incohérentes de type, catégorie, placement, visuel ou runtime.

### Champs legacy

- `bPlaceOnEdge`
- `bPlaceAtCellCenter`

## 4. Champs pouvant prêter à confusion

### `SupportedType` vs `ObjectCategory` vs `Category`

- `SupportedType` est le type gameplay large. Il répond à la question : "quelle famille de comportement ?"
- `ObjectCategory` est une catégorie fonctionnelle d'éditeur/validation. Elle aide à grouper les rôles sémantiques, mais ne pilote pas directement le runtime.
- `Category` sert uniquement à l'organisation de la palette et n'affecte pas le runtime.

L'éditeur devrait éviter de présenter ces trois champs comme équivalents dans l'UI principale. `SupportedType` est essentiel. `ObjectCategory` et `Category` sont surtout des métadonnées read-only ou des champs d'authoring DataAsset.

### `PlacementKind` vs `bPlaceOnEdge` vs `bPlaceAtCellCenter`

`PlacementKind` est la source de vérité actuelle. Les deux booléens sont des champs de compatibilité legacy. Les afficher ensemble dans l'UI normale donne l'impression de règles concurrentes.

### `PreviewMesh` vs `FixedMesh` vs `MovingMesh`

`PreviewMesh` n'est pas seulement un mesh de preview éditeur. C'est actuellement le mesh principal/simple utilisé par la sélection runtime avant fallback vers moving/fixed. `FixedMesh` et `MovingMesh` servent aux acteurs composites ou animés. Le nom `PreviewMesh` peut faire croire à tort qu'il est editor-only.

### `PreviewMaterial` vs `FixedMaterial` vs `MovingMaterial`

Même problème que pour les meshes. `PreviewMaterial` est le matériau principal/simple, pas seulement un matériau de preview.

### `RuntimeActorClass` vs `ItemActorClass`

`RuntimeActorClass` spawne les objets runtime de grille : doors, buttons, pressure plates, receptacles, generic objects. `ItemActorClass` spawne les items portés ou présents dans le monde. Selon le cas, un archétype item peut avoir besoin de l'un, de l'autre ou d'aucun.

### `bIsReadable` vs `ObjectCategory == Readable`

`bIsReadable` est le flag runtime readable. `ObjectCategory == Readable` est une classification. La validation tente de les garder cohérents pour les decorations lisibles, mais le runtime dépend de `bIsReadable`.

### `bIsLightSource` vs `SupportedType == Light`

`bIsLightSource` contrôle la point light runtime sur les objets génériques. `SupportedType == Light` est un type/classification large. Un objet non-Light peut quand même être source de lumière, par exemple un torch holder ou une decoration lumineuse.

### `bBlocksMovement` vs blocage des portes

`bBlocksMovement` contrôle la collision mesh d'un `AGridGenericObjectActor`. Le blocage de passage des portes est géré ailleurs par le door system. Pour les doors, ce champ peut induire les designers en erreur s'il est affiché sans contexte.

### `bUseLightFlicker`

Le champ est validé et affiché, mais le setup light générique applique actuellement couleur/intensité/radius seulement. Le flicker semble appartenir à `GridLightEmitterComponent`, pas encore au chemin generic light piloté par l'archétype. Il faut clarifier avant de l'exposer comme réglage gameplay important.

## 5. Archetype Only vs Instance Override

### Archetype Only

Ces champs devraient généralement être édités uniquement dans les DataAssets :

- Identité et classification : `ArchetypeId`, `DisplayName`, `SupportedType`, `Description`, `Category`, `ObjectCategory`
- Règles et offsets de placement : `PlacementKind`, `bCanShareCell`, `bCanShareAnchor`, `bBlocksMovement`, `PlacementZOffset`, `WallInset`, `LocalOffsetAlongWall`, `LocalOffsetVertical`, `RotationStepYaw`
- Capacités d'interaction : `bIsInteractable`, `bIsReadable`, `bShowReadableOnlyOnce`
- Defaults lumière : `bIsLightSource`, `LightColor`, `LightIntensity`, `LightRadius`, `bUseLightFlicker`
- Visuels : `PreviewMesh`, `PreviewMaterial`, `FixedMesh`, `MovingMesh`, `FixedMaterial`, `MovingMaterial`
- Classes runtime : `RuntimeActorClass`, `ItemActorClass`
- Métadonnées item : `ItemTags`
- Flags legacy de placement : `bPlaceOnEdge`, `bPlaceAtCellCenter`

### Instance Override

Ces champs d'archétype sont des defaults copiés ou reflétés dans les données d'objet placé :

- `bDefaultInitiallyEnabled` -> `FGridLevelObjectData::bInitiallyEnabled`
- `bDefaultInitiallyActive` -> `FGridLevelObjectData::bInitiallyActive`
- `DefaultTag` -> `FGridLevelObjectData::Tag`
- `DefaultBehavior` -> `FGridLevelObjectData::Behavior`
- `ReadableText` -> peut être surchargé par `FGridLevelObjectData::OverrideReadableText`
- `ArchetypeId` -> l'objet placé stocke l'`ArchetypeId` sélectionné, mais ne doit pas modifier le champ du DataAsset

Les champs d'instance qui ne sont pas des champs d'archétype incluent l'object id, la cellule, l'edge, le yaw local, les notes, l'id d'entrée palette et les connecteurs.

### Read-only dans l'inspector

Ces champs sont utiles à afficher dans le contexte de l'inspector d'objet, mais ne devraient pas être édités là à court terme :

- `DisplayName`
- `SupportedType`
- `Category`
- `ObjectCategory`
- `PlacementKind`
- `bIsInteractable`
- `bBlocksMovement`
- `bIsLightSource`
- `LightColor`
- `LightIntensity`
- `LightRadius`
- `bUseLightFlicker`
- `RuntimeActorClass`
- `PreviewMesh`
- `FixedMesh`
- `MovingMesh`

Les champs éditables de l'inspector doivent rester centrés sur les données d'instance : initially enabled/active, behavior params, override readable text, tag/notes/debug fields et édition des connecteurs.

## 6. Opportunités de nettoyage immédiates

Améliorations futures sûres, sans changer runtime ni sérialisation :

- Garder `bPlaceOnEdge` et `bPlaceAtCellCenter` en `AdvancedDisplay`; ne les supprimer qu'après une migration confirmant qu'aucun asset ou palette entry n'en dépend.
- Améliorer les tooltips de `SupportedType`, `Category` et `ObjectCategory` pour distinguer type gameplay, groupement palette et classification éditeur.
- Renommer les labels affichés, pas les propriétés C++, de `PreviewMesh` et `PreviewMaterial` vers "Main Mesh" et "Main Material" dans l'UI éditeur.
- Clarifier le label de `bBlocksMovement` comme blocage générique/non-door lorsqu'il est affiché.
- Clarifier que `bIsReadable` et `bIsLightSource` sont des flags de capacité runtime, alors que `ObjectCategory` est une classification.
- Documenter quels types d'objet utilisent chaque groupe de `DefaultBehavior` :
  - Activation : buttons, levers, pressure plates, triggers, receptacles, teleporters qui émettent des links.
  - Trigger : triggers et pressure plates.
  - Teleporter : teleporters.
  - Receptacle : receptacles.
  - ButtonAnimation : buttons.
  - ItemSpawn : item spawns.
- Éviter d'afficher les champs non pertinents par type dans l'inspector. Préférer des résumés contextuels read-only.
- Clarifier ou câbler `bUseLightFlicker` avant de le présenter comme comportement runtime actif.
- Envisager plus tard un layout DataAsset séparant "Bases designer", "Placement", "Visuels", "Classe runtime" et "Advanced/Validation".

## 7. Risques

Renommer ou supprimer ces champs est risqué, car ce sont des UPROPERTY de DataAssets et plusieurs sont référencés par Blueprint, l'éditeur et le runtime.

- Les DataAssets existants peuvent casser ou perdre des valeurs silencieusement si des noms UPROPERTY changent sans redirects/migration.
- Les références Blueprint peuvent casser pour `RuntimeActorClass`, `ItemActorClass`, meshes, matériaux, réglages de placement ou helpers exposés.
- Les level assets sérialisés peuvent perdre des données si les defaults copiés ou hypothèses de behavior changent.
- Le spawn runtime peut échouer si `RuntimeActorClass`, la sélection de mesh, le placement ou le lookup `ArchetypeId` changent.
- Le comportement receptacle/item peut régresser si `ItemTags`, `DefaultBehavior.Receptacle`, `MovingMesh` ou `MovingMaterial` changent de sémantique.
- Le comportement door peut régresser si `SupportedType`, `RuntimeActorClass` ou la sémantique de placement edge changent.
- Le placement éditeur peut régresser si `PlacementKind`, les flags de partage ou les offsets muraux sont renommés/supprimés.

## 8. Recommandation finale

Ne pas refactorer `UGridObjectArchetypeAsset` immédiatement.

D'abord documenter l'asset et ses frontières d'usage actuelles. Ensuite nettoyer seulement les labels affichés, tooltips, catégories et la présentation contextuelle de l'inspector. Traiter `PlacementKind`, `ArchetypeId`, les classes runtime, les champs visuels et les defaults de behavior comme une API sérialisée stable tant qu'un plan de migration explicite n'existe pas.

Ne supprimer les champs legacy de placement qu'après migration complète et validation des DataAssets, palette entries, objets placés, références Blueprint et validations runtime/éditeur.
