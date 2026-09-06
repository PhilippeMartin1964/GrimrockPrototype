# WORLDOBJ — Migration MIG00 à MIG10 et modèle de données cible

Statut : document directeur de migration — 2026-09-06.

Ce document accompagne l’effort de simplification des objets du monde entrepris sous les jalons `WORLDOBJ-MIG00` à `WORLDOBJ-MIG10`. Il décrit à la fois l’état de la migration, sa feuille de route et le modèle de données cible qui doit en résulter.

> **Référence architecturale prioritaire** : `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`.
>
> La mind map décrit **ce que l’architecture doit devenir**. Le présent document décrit **comment y arriver** et sert de vue d’ensemble opérationnelle. En cas de contradiction avec une ancienne documentation historique, la mind map et les décisions de migration les plus récentes prévalent.

---

## 1. But de la migration

La migration WORLDOBJ vise à remplacer progressivement un modèle d’archétype devenu trop large, redondant et dépendant de champs historiques par un modèle simple, modulaire et orienté données.

Les invariants cibles sont :

- **une définition = un concept** ;
- **définition ≠ instance** ;
- un item ramassable possède **une seule définition permanente** ;
- un objet du monde possède **une seule définition permanente** ;
- un monstre possède **une seule définition permanente** ;
- un niveau référence les définitions, il ne les duplique pas ;
- le Grid Editor et le runtime consomment la même définition ;
- les Data Assets sont des données d’authoring et ne sont jamais modifiés pour stocker un état runtime ;
- la grille, les cellules, les frontières et les coordonnées locales de surface restent la source de vérité spatiale ;
- le niveau reste la source de vérité pour les placements, la logique et l’état initial ;
- le SaveGame stocke des **deltas d’état mutable**, pas une copie des définitions.

Schéma directeur :

```text
GLOBAL DEFINITIONS
├── WorldObjectDefinition
├── ItemDefinition
├── MonsterDefinition
├── EnvironmentDefinition
├── ReadableContent
└── QuestDefinition
          │
          ▼ références
UGridLevelAsset
├── Grid / cells / walls / ceilings
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
├── LogicObjects
├── Links / variables / Lua / quests
└── Party start / transitions
          │
          ▼ instanciation
Runtime Actors / Components
          │
          ▼ état mutable
Runtime State / SaveGame
```

---

## 2. Hiérarchie documentaire

Pour éviter que les documents historiques ne deviennent une seconde architecture parallèle, les documents doivent être lus dans cet ordre :

1. **Architecture cible** : `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`.
2. **Migration et modèle cible** : le présent document.
3. **Architecture réellement implémentée** : par exemple `docs/Architecture/CORE_DUNGEON_LEVEL_GRID.md`.
4. **Notes de migration spécialisées** : `WORLDOBJ_MIG01_PLACEMENT_SURFACE.md`, `WORLDOBJ_MIG03_4B_AUTHORING_MIGRATION.md`, etc.
5. **Documents historiques / audits** : utiles pour comprendre l’origine du code, mais non normatifs lorsque la cible a changé.

La migration ne doit donc jamais « optimiser » le code existant en perdant de vue la structure décrite dans la mind map.

---

## 3. État actuel de la migration

Révision de référence au moment de la rédaction :

```text
a013f1f30f91c61d67fef4b1b6d5d854a46bc3ce
WORLDOBJ-MIG03.4D move inspector to visual composition
```

Le dernier filtre utilisé est :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects.MIG03"
```

La chaîne MIG03 est fonctionnellement validée avec zéro échec. Une exécution récente comporte encore un test classé « succeeded with warnings » ; ce warning n’est pas un échec fonctionnel mais doit disparaître ou être explicitement justifié avant la clôture finale de MIG10.

### 3.1. Synthèse des jalons

| Jalon | Statut | Résultat principal |
|---|---|---|
| `MIG00` | ✅ Validé | Caractérisation du contrat historique avant refactor. |
| `MIG01` | ✅ Validé | Placement cible `Floor / Wall / Ceiling` + coordonnées locales `U/V/N`. |
| `MIG02` | ✅ Validé | Comportement spatial réduit à `BlocksCellMovement`, `OccupiesBoundary`, `SuppressBaseWall` + frontière canonique. |
| `MIG03` | 🟨 Finalisation | Composition visuelle générique adoptée par runtime, mécanismes, preview, validation et inspecteur. Les derniers symboles legacy doivent encore être physiquement supprimés. |
| `MIG04` | ⬜ À faire | Unifier définitivement l’animation des mécanismes sur `MovingParts[].Motion`. |
| `MIG05` | ⬜ À faire | Un item = un `UGridItemDefinitionAsset`, sans WorldObjectDefinition compagnon. |
| `MIG06` | ⬜ À faire | Supprimer la copie intégrale de `Behavior` dans chaque instance de niveau. |
| `MIG07` | ⬜ À faire | Remplacer le gros `FGridLevelObjectData` par des structures d’instances typées. |
| `MIG08` | ⬜ À faire | Recréer/réenregistrer les `.uasset` avec le modèle cible et migrer la palette/contenu réel. |
| `MIG09` | ⬜ À faire | Purge de tous les ponts, fallback et symboles legacy restants. |
| `MIG10` | ⬜ À faire | Renommage final `UGridObjectArchetypeAsset` → `UGridWorldObjectDefinitionAsset` et clôture architecturale. |

---

## 4. Historique consolidé MIG00 → MIG03

### MIG00 — Caractériser avant de modifier

Objectif : figer le comportement historique utile avant toute suppression.

Points couverts :

- placement sol / mur / plafond ;
- offsets historiques ;
- orientation murale ;
- parité des transforms après introduction du nouveau schéma.

Repères Git principaux :

```text
2ba8c18  WORLDOBJ-MIG00 characterize current placement contract
440b181  Fix WORLDOBJ-MIG00 MIG01 unity test isolation
c70b530  Fix MIG00 historical placement parity after MIG01
7b643c7  Fix MIG00 wall rotation characterization
```

Le principe à retenir est que les tests de caractérisation protègent le comportement nécessaire pendant la migration, mais ne constituent pas le modèle final.

### MIG01 — Placement Surface

Le contrat d’authoring devient :

```text
PlacementSurface
├── Floor
├── Wall
└── Ceiling

DefaultLocalPosition
├── U
├── V
└── N
```

`Center` n’est plus un type de placement. Un objet centré est un objet de sol avec un offset local nul.

`Edge` n’est plus un type de placement. Une frontière est une notion topologique séparée.

Repère principal :

```text
5395a23  WORLDOBJ-MIG01 simplify world object placement surfaces
```

Des champs historiques restent temporairement sous forme de projections `Transient` pour les consommateurs C++ non encore migrés. Ils ne sont plus des données d’authoring.

### MIG02 — Comportement spatial et frontière

Le contrat spatial cible est réduit à :

```text
Spatial Behavior
├── BlocksCellMovement
├── OccupiesBoundary
└── SuppressBaseWall
```

Les notions vagues `CanShareCell` / `CanShareAnchor` ne font pas partie du modèle cible.

Une frontière est identifiée par une clé canonique de sorte que :

```text
Cell A / North == cellule voisine / South
Cell A / East  == cellule voisine / West
```

Repères Git :

```text
110d3c0  WORLDOBJ-MIG02 simplify spatial behavior schema
cae3d4b  WORLDOBJ-MIG02 wire boundary ownership placement
```

### MIG03 — Composition visuelle générique

Le modèle cible remplace tous les champs visuels spécialisés par :

```text
Visual
├── StaticPart (optionnelle)
│   ├── Mesh
│   └── LocalTransform
└── MovingParts
    ├── Part0 (optionnelle)
    │   ├── Mesh
    │   ├── LocalTransform
    │   └── Motion
    └── Part1 (optionnelle)
        ├── Mesh
        ├── LocalTransform
        └── Motion
```

`Motion` contient :

```text
Type     = Rotation | Translation
Axis     = X | Y | Z
Pivot    = X/Y/Z, utile pour Rotation
Amount   = degrés ou centimètres, signé
Duration = secondes
```

La preview du Grid Editor utilise les **mêmes parties** que le runtime. Il n’existe pas de `PreviewMesh` dans le modèle cible.

Repères Git de la première intégration :

```text
71b4223  WORLDOBJ-MIG03 add visual composition foundation
1f6a2fc  WORLDOBJ-MIG03 wire archetype visual composition
31af66e  WORLDOBJ-MIG03 wire composition into mechanisms and editor preview
23dee0f  WORLDOBJ-MIG03 drive mechanism states from visual motion
d58914d  WORLDOBJ-MIG03 remove legacy mesh spawn dependency
5218227  WORLDOBJ-MIG03 fix runtime spawn mesh assertion
8f0f1b2  WORLDOBJ-MIG03 retire legacy visual runtime paths
0319aa4  WORLDOBJ-MIG03.4B migrate editor visual authoring
a5a6aa1  WORLDOBJ-MIG03.4C retire legacy visual authoring schema
01a6f8b  Fix MIG03.4C UHT transient bridge warnings
1f1d15e  WORLDOBJ-MIG03.4D detach visual semantics from legacy fields
55924b6  WORLDOBJ-MIG03.4D migrate visual validation contract
c0fe5e9  WORLDOBJ-MIG03.4D detach tests from legacy visual fields
9bb3bfe  WORLDOBJ-MIG03.4D move editor provisioning to visual composition
f484ba1  WORLDOBJ-MIG03.4D remove dead legacy visual provisioning helpers
a013f1f  WORLDOBJ-MIG03.4D move inspector to visual composition
```

#### Reste à fermer dans MIG03

Les noms suivants existent encore comme ponts C++ `Transient` et doivent disparaître physiquement avant de considérer MIG03 complètement clos :

```text
PreviewMesh
FixedMesh
MovingMesh
PitLeftLeafMesh
PitRightLeafMesh
```

Critère de fermeture de MIG03 : aucune donnée d’authoring, aucun runtime, aucun test, aucun outil éditeur et aucun helper ne doit encore dépendre de ces symboles.

---

## 5. Roadmap détaillée MIG04 → MIG10

## MIG04 — Animation générique des mécanismes

### Objectif

Faire de `MovingParts[].Motion` l’unique contrat géométrique d’animation pour les objets du monde.

Le runtime doit uniquement manipuler un **alpha logique** de mouvement :

```text
Closed / Released / Off = 0
Open / Pressed / On     = 1
```

La géométrie exacte de l’animation appartient à la définition : rotation, translation, pivot, amplitude et durée.

### À éliminer progressivement

Les paramètres d’animation spécialisés qui dupliquent la présentation doivent quitter le contrat de données lorsque leur information existe dans `Motion` :

```text
DoorAnimation.OpenHeight
LeverAnimation.LeverOffPitch / LeverOnPitch
ButtonAnimation.ButtonPressDistance
PressurePlateAnimation.ReleasedHeightAboveFloor / PressedHeightAboveFloor
PitAnimation.LeftHingeLocation / RightHingeLocation
PitAnimation.OpenAngleDegrees
...et leurs durées de mouvement redondantes
```

Les paramètres qui sont réellement du **gameplay** restent séparés. Exemple : le temps pendant lequel un bouton reste enfoncé peut rester une règle comportementale ; la distance parcourue et la vitesse visuelle appartiennent à `Motion`.

### Critères de sortie

- les mécanismes utilisent le même moteur `Motion` ;
- aucun acteur n’a besoin de connaître la géométrie propre à « son » type de mécanisme ;
- une porte battante, coulissante ou verticale ne change que par ses données ;
- une trappe à deux volets ne nécessite aucun champ spécifique « PitLeft/PitRight ».

---

## MIG05 — Un collectible = une définition

### Objectif

Supprimer le modèle où un item peut nécessiter à la fois un `UGridItemDefinitionAsset` et un `UGridObjectArchetypeAsset` compagnon.

Règle cible :

```text
BlueGem
└── UGridItemDefinitionAsset

CopperKey
└── UGridItemDefinitionAsset

Stone
└── UGridItemDefinitionAsset
```

Le même item peut être :

- posé au sol ;
- placé dans une alcôve ;
- contenu dans un coffre ;
- dans l’inventaire ;
- équipé ;
- tenu ;
- lancé ;
- généré par un spawn.

Il ne change jamais de définition.

### Changements attendus

- la palette doit pouvoir placer un `ItemDefinition` directement ;
- un item au sol référence directement son `UGridItemDefinitionAsset` ;
- un réceptacle référence directement des ItemDefinitions pour son contenu ;
- `ItemSpawn` référence directement une ItemDefinition ;
- le runtime item utilise un acteur générique ou une factory, sans exiger un WorldObjectDefinition compagnon ;
- suppression des duplications `ItemDefinitionId` / `ArchetypeId` devenues inutiles lorsque la référence d’asset est autoritaire.

---

## MIG06 — Definition vs Instance : supprimer les copies de Behavior

### Problème actuel

`FGridLevelObjectData` transporte encore un bloc `Behavior` complet, souvent issu du `DefaultBehavior` de l’archétype. Cela crée deux sources de vérité :

```text
Definition.DefaultBehavior
        +
Instance.Behavior copié
```

Une modification de la définition n’est alors plus naturellement reflétée par les objets déjà placés.

### Cible

```text
WorldObjectDefinition
└── comportement par défaut réutilisable

PlacedWorldObjectInstance
├── InstanceId
├── DefinitionRef
├── Placement
├── InitialState
└── Overrides strictement nécessaires
```

Un override n’existe que lorsqu’une valeur est réellement spécifique au niveau.

Exemples de données naturellement **par instance** :

- destination d’un téléporteur ;
- destination d’une transition ;
- contenu initial d’un réceptacle ;
- texte lisible surchargé ;
- état initial activé / ouvert / verrouillé ;
- Tag local ;
- identifiants de quête ou de logique locaux.

Exemples de données naturellement **dans la définition** :

- mesh et motion ;
- son d’ouverture ;
- portée d’interaction ;
- règles génériques de réceptacle ;
- capacité de lecture ;
- lumière ;
- classe runtime ;
- comportement spatial.

---

## MIG07 — Scinder `FGridLevelObjectData`

### Objectif

Faire correspondre le stockage du niveau à la mind map cible :

```text
Placements
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
└── LogicObjects
```

Le gros `FGridLevelObjectData` actuel mélange des champs qui n’ont aucun sens pour la majorité des objets : définition d’item, définition de monstre, patrol, readable, receptacle, teleporter, pit, etc.

### Structures cibles logiques

Les noms C++ définitifs seront choisis pendant MIG07, mais le schéma doit suivre cette séparation.

```text
PlacedWorldObject
├── InstanceId
├── WorldObjectDefinitionRef
├── CellX / CellY
├── WallSide si Wall
├── LocalTransformOverride optionnel
├── InitialState
├── Tag / Notes
└── InstanceConfig typée minimale
```

```text
LooseItemInstance
├── InstanceId
├── ItemDefinitionRef
├── Quantity
├── CellX / CellY
├── Surface / position locale si nécessaire
└── état mutable initial spécifique à l’item
```

```text
MonsterSpawn
├── SpawnId
├── MonsterDefinitionRef
├── CellX / CellY
├── Facing
├── InitialMonsterState
├── Patrol
├── EncounterGroupId
└── WaveIndex
```

```text
ItemSpawn
├── SpawnId
├── ItemDefinitionRef
├── CellX / CellY
├── Quantity
└── règles de spawn
```

Important : **LooseItemInstance ≠ ItemSpawn**. Le premier est un objet réellement présent dans le niveau ; le second est un générateur.

Les liens logiques conservent des identifiants stables afin de référencer les instances sans dépendre des acteurs runtime.

---

## MIG08 — Migration des Data Assets réels

### Objectif

Une fois le schéma C++ stabilisé, migrer le contenu Unreal réel.

Cette étape doit se faire dans l’éditeur UE pour les `.uasset` et `.umap`, pas par modification binaire hors UE.

### Travaux

- réenregistrer/recréer les définitions d’objets du monde ;
- convertir toutes les présentations vers `StaticPart` / `MovingParts` ;
- convertir les items vers la définition unique ;
- migrer les placements du LevelAsset vers les structures typées ;
- mettre à jour `UGridObjectPaletteAsset` ;
- supprimer les anciennes entrées de palette devenues redondantes ;
- valider portes, boutons, leviers, plaques, pits, téléporteurs, triggers, réceptacles et décorations ;
- valider les niveaux réels et les transitions entre niveaux.

MIG08 est le moment où l’on accepte volontairement de recréer un asset si cela est plus simple et plus sûr que de maintenir une compatibilité historique inutile.

---

## MIG09 — Purge legacy

### Objectif

Une fois tous les consommateurs et assets migrés, retirer les ponts de compilation et les anciennes conventions.

La purge doit notamment rechercher et éliminer, selon leur état au moment de l’étape :

```text
PlacementKind transitoire
PlacementZOffset
WallInset
LocalOffsetAlongWall
LocalOffsetVertical
bCanShareCell
bCanShareAnchor
PreviewMesh
FixedMesh
MovingMesh
PitLeftLeafMesh
PitRightLeafMesh
GetObjectMesh() legacy fallback
Center / Edge comme valeurs de placement obsolètes
copies intégrales de Behavior
fallbacks item basés sur ArchetypeId
anciens helpers de provisioning
anciens tests qui ne décrivent plus la cible
```

Le principe est : **pas de legacy permanent pour un prototype encore en phase de conception**.

Un pont `Transient` peut être utile pendant une tranche de migration, mais il doit avoir une date de mort architecturale claire.

---

## MIG10 — Renommage final et clôture architecturale

### Objectif

Renommer enfin le concept historique :

```text
UGridObjectArchetypeAsset
        ↓
UGridWorldObjectDefinitionAsset
```

Et normaliser le vocabulaire associé :

```text
ArchetypeId              → DefinitionId / WorldObjectDefinitionId
ObjectArchetypes         → WorldObjectDefinitions
FindObjectArchetypeById  → FindWorldObjectDefinition...
Archetype Default        → Definition Default
```

Le renommage est volontairement le dernier jalon car renommer tôt une classe sérialisée augmente le risque sur les `.uasset` sans apporter de gain fonctionnel pendant la migration.

### Definition of Done MIG10

- le nom `Archetype` n’est plus utilisé pour le nouveau modèle d’objet du monde ;
- aucun champ legacy de MIG01/MIG02/MIG03/MIG05/MIG06 n’est encore requis ;
- les items n’ont qu’une définition ;
- les objets du monde n’ont qu’une définition ;
- les monstres n’ont qu’une définition ;
- le LevelAsset stocke des instances typées et des références ;
- le runtime ne contient aucune source de vérité d’authoring ;
- la preview et le runtime utilisent les mêmes données ;
- les tests automatisés couvrent les invariants cibles ;
- les niveaux réels sont jouables ;
- la documentation est réconciliée avec la mind map cible.

---

# 6. Modèle de données cible — vue d’ensemble

## 6.1. Règle fondamentale d’ownership

| Couche | Possède quoi ? | Ne doit pas posséder |
|---|---|---|
| Definition Data Asset | Identité et propriétés permanentes d’un concept réutilisable. | Position dans un niveau, état runtime mutable. |
| `UGridLevelAsset` | Layout, placements, état initial, logique, références aux définitions. | Copie complète des définitions, acteurs runtime. |
| Runtime Actor / Component | Exécution, animation, interaction, collisions, état courant. | Données d’authoring autoritaires. |
| SaveGame / RuntimeState | Deltas mutables : ouvert, ramassé, HP, contenu, variables, quêtes, etc. | Mesh, sons, stats permanentes, définition complète. |

Formule :

```text
Effective runtime object
    = Definition
    + Level Instance Configuration
    + Saved Runtime Delta
```

---

## 6.2. `UGridWorldObjectDefinitionAsset` — cible

Nom actuel transitoire : `UGridObjectArchetypeAsset`.

```text
UGridWorldObjectDefinitionAsset
├── Identity
│   ├── DefinitionId
│   ├── DisplayName
│   ├── Description
│   ├── Tags
│   ├── PaletteCategory
│   └── FunctionalCategory
│
├── Classification
│   └── WorldObjectType
│
├── Placement
│   ├── PlacementSurface = Floor | Wall | Ceiling
│   ├── DefaultLocalPosition U/V/N
│   ├── DefaultLocalRotation
│   └── DefaultLocalScale
│
├── Spatial Behavior
│   ├── BlocksCellMovement
│   ├── OccupiesBoundary
│   └── SuppressBaseWall
│
├── Visual
│   ├── StaticPart optional
│   │   ├── Mesh
│   │   └── LocalTransform
│   └── MovingParts 0..2
│       ├── Mesh
│       ├── LocalTransform
│       └── Motion
│           ├── Type
│           ├── Axis
│           ├── Pivot
│           ├── Amount
│           └── Duration
│
├── Interaction
│   ├── IsInteractable
│   ├── IsReadable
│   ├── ReadableContentRef
│   ├── UseDistance
│   └── InteractionPriority
│
├── Default State / Behavior
│   ├── InitiallyEnabled
│   ├── InitiallyActive
│   ├── defaults de comportement générique
│   └── capacités applicables
│
├── Audio
│   ├── Attenuation
│   └── AudioEvents[semantic event]
│
├── VFX
│   └── VFXEvents
│
├── Light optional
│   ├── Color
│   ├── Intensity
│   ├── Radius
│   └── FlickerProfile
│
└── Runtime
    ├── RuntimeActorClass
    └── ValidationRules
```

La définition ne contient **aucun** `PreviewMesh` séparé et aucun champ « spécial Pit », « spécial Door », etc. lorsque le même résultat peut être exprimé par la composition générique.

---

## 6.3. `UGridItemDefinitionAsset` — collectible unique

Cette classe existe déjà et constitue une base proche de la cible.

```text
UGridItemDefinitionAsset
├── Identity
│   ├── ItemDefinitionId
│   ├── DisplayName
│   ├── Description
│   ├── ItemType
│   └── ItemTags
│
├── Presentation
│   ├── Icon
│   ├── WorldMesh
│   ├── EquippedMesh
│   └── WorldSparkle optional
│
├── Inventory
│   ├── Weight
│   ├── Stackable
│   └── MaxStackSize
│
├── Manipulation
│   ├── HandUsage
│   ├── physical throw rules
│   └── Strength scaling
│
├── Equipment
│   ├── CompatibleEquipmentSlots
│   ├── StatBonus
│   ├── ResistanceBonus
│   ├── CombatActions
│   └── AttackPresentation
│
├── Quick Item / Combat
│   └── inventory-backed combat action
│
├── Throw
│   ├── CombatThrowWeapon
│   ├── Speed / Arc / Lifetime
│   └── visual Stable / Tumble / Spin
│
├── World Physics
│   ├── mass from Weight optional
│   └── initial tilt optional
│
├── Reading optional
│   └── ReadableContentRef / text
│
└── Portable Light optional
```

Règle absolue : un item ne reçoit jamais une seconde définition simplement parce qu’il est visible dans le monde.

---

## 6.4. `UGridMonsterDefinitionAsset`

Cette classe existe déjà et suit également le principe « définition permanente + spawn/état local ».

```text
UGridMonsterDefinitionAsset
├── Identity
├── Presentation
│   ├── Icon
│   ├── SkeletalMesh
│   ├── AnimationClass
│   ├── MonsterActorClass
│   └── visual offsets
├── Stats
├── Movement
├── Perception
├── AI profiles
├── Combat / attacks
├── Damage modifiers
├── Audio
├── VFX
├── Animation variations
└── Rewards / loot
```

Le niveau ne doit pas recopier ces paramètres. Un `MonsterSpawn` référence la définition et ne contient que les données propres à ce spawn : position, facing, état initial, patrol, encounter, etc.

---

## 6.5. `UGridEnvironmentDefinitionAsset` — cible

Ce type est prévu par la mind map et n’est pas encore le cœur de la migration WORLDOBJ actuelle.

But : définir une famille cohérente de présentation structurelle et d’ambiance sans transformer « pierre », « bois », « végétation », etc. en types de cellules.

Schéma cible indicatif :

```text
UGridEnvironmentDefinitionAsset
├── EnvironmentId
├── DisplayName
├── Floor set
├── Wall set
├── Ceiling set
├── Pillar / trim set
├── optional variants
├── lighting / ambience references
└── editor presentation
```

`UGridLevelAsset` peut avoir un environnement par défaut et, à terme, des overrides locaux si nécessaire.

---

## 6.6. `UGridReadableContentAsset`

Le contenu lisible est une définition de contenu, pas un type d’objet.

```text
UGridReadableContentAsset
├── ReadableId
├── Title
├── Text / pages
└── metadata narrative éventuelle
```

Il peut être référencé par :

- un objet du monde lisible ;
- un item lisible ;
- un élément narratif.

Le fait d’être lisible ne détermine pas si la chose est ramassable.

---

## 6.7. `UGridQuestDefinitionAsset`

```text
UGridQuestDefinitionAsset
├── QuestId
├── Title
├── Description
├── Objectives
├── prerequisites / conditions
└── presentation
```

La définition de quête est permanente. L’état de quête courant appartient au runtime/save : commencé, objectifs complétés, terminé, échoué, etc.

Le LevelAsset référence les quêtes utiles à ce niveau sans devenir propriétaire de leur progression runtime.

---

## 6.8. `UGridDungeonAsset`

`UGridDungeonAsset` est un **conteneur d’organisation**, pas une définition d’objet de gameplay.

Il organise plusieurs niveaux :

```text
UGridDungeonAsset
├── DungeonName
├── Author
├── Version
├── DefaultLevelId
└── Levels[]
    ├── LevelId
    ├── DisplayName
    ├── UGridLevelAsset ref
    ├── LogicalPosition
    └── Enabled
```

Le principe « un asset de niveau unique » signifie qu’un niveau donné possède une source de vérité `UGridLevelAsset`. Le DungeonAsset ne duplique pas la grille du niveau.

---

## 6.9. `UGridLevelAsset` — source de vérité d’un niveau

Le LevelAsset doit devenir le point d’assemblage de toutes les références et instances locales.

```text
UGridLevelAsset
├── Identity
│   ├── LevelId
│   ├── DisplayName
│   └── Description
│
├── Grid
│   ├── Width / Height
│   ├── CellSize
│   ├── Cells
│   ├── Walls / boundaries
│   ├── Ceilings
│   └── structural pits / stairs
│
├── Environment
│   └── DefaultEnvironmentDefinitionRef
│
├── Party Start
│   ├── StartCellX
│   ├── StartCellY
│   └── StartFacing
│
├── Placements
│   ├── WorldObjectInstances
│   ├── LooseItemInstances
│   ├── MonsterSpawns
│   ├── ItemSpawns
│   └── LogicObjects
│
├── Logic
│   ├── Links
│   ├── Variables
│   ├── LuaScripts
│   └── QuestDefinitionRefs
│
├── Transitions
└── Editor Metadata
```

Le LevelAsset ne sérialise jamais les acteurs runtime créés à partir de ces données.

---

## 6.10. `UGridObjectPaletteAsset` — catalogue éditeur uniquement

La palette du Grid Editor n’est pas une définition gameplay.

Elle sert à présenter des références sélectionnables par l’auteur du niveau :

```text
Palette Entry
├── Label / category / icon editor
└── DefinitionRef
    ├── WorldObjectDefinition
    ├── ItemDefinition
    ├── MonsterDefinition pour spawn
    └── autres outils autorisés
```

Elle ne doit pas dupliquer les propriétés de la définition référencée.

---

## 6.11. Définitions futures hors cœur WORLDOBJ

La mind map prévoit également, à terme :

```text
CharacterDefinitionAsset
PartyDefinitionAsset
```

Elles suivent la même règle : caractéristiques permanentes dans la définition, état courant dans le runtime/save.

Elles ne doivent pas bloquer la migration WORLDOBJ.

---

# 7. Placement cible des instances

Pour un objet du monde, la définition possède une **surface autorisée unique**. L’instance possède la cellule et les informations locales nécessaires.

```text
Definition
└── PlacementSurface = Floor | Wall | Ceiling

Instance
├── CellX
├── CellY
├── WallSide seulement si Wall
└── LocalTransformOverride optionnel
```

Les coordonnées locales sont exprimées dans le repère de surface :

```text
U = tangent horizontal
V = second tangent, vertical sur Wall
N = normale à la surface
```

Interprétation :

```text
Floor   : N = hauteur au-dessus du sol
Wall    : N = profondeur / inset
Ceiling : N = distance sous le plafond
```

Une frontière ne doit pas être confondue avec une surface :

```text
Placement Surface : où l’objet est ancré
Boundary           : quelle séparation topologique il occupe
```

Une porte est donc typiquement :

```text
PlacementSurface = Wall
WallSide          = South
OccupiesBoundary  = true
SuppressBaseWall  = true
```

---

# 8. État runtime et sauvegarde

La sauvegarde doit conserver uniquement ce qui peut changer.

Exemples :

```text
WorldObjectRuntimeState
├── InstanceId
├── Enabled / Active
├── Open / Locked
├── destroyed / consumed
└── custom state minimal

ItemRuntimeState
├── InstanceId
├── ItemDefinitionRef ou stable id
├── Quantity
├── durability / charges si applicable
└── location actuelle

MonsterRuntimeState
├── Spawn/InstanceId
├── MonsterDefinitionRef
├── Health
├── grid position / facing
├── AI state utile
└── dead / removed

LevelRuntimeState
├── Variables
├── object deltas
├── item deltas
├── monster deltas
└── logic state
```

Le SaveGame ne doit jamais recopier un mesh, un son, les stats de base d’un monstre ou les propriétés permanentes d’un item.

---

# 9. Règles de migration à respecter jusqu’à MIG10

1. **Ne pas maintenir une compatibilité historique par principe.** Le projet est un prototype ; lorsqu’un modèle cible est validé, le legacy doit être supprimé après migration du contenu.
2. **Les ponts transitoires doivent être `Transient`, non éditables et non sérialisés.**
3. **Une étape fonctionnelle doit rester petite et testable.**
4. **Ne pas modifier directement des `.uasset` binaires hors de l’éditeur Unreal.**
5. **Ne pas confondre une classe runtime avec une définition.** Une même classe runtime peut exécuter de nombreuses définitions.
6. **Ne pas dupliquer une définition pour changer le contexte d’une instance.** Une gemme au sol et la même gemme dans un coffre restent la même ItemDefinition.
7. **Ne pas coder une variation visuelle comme un nouveau type gameplay si une donnée suffit.**
8. **Conserver des identifiants d’instance stables pour les liens et la sauvegarde.**
9. **Maintenir la mind map à jour lorsqu’une décision cible est réellement changée.**
10. **À MIG10, les documents historiques doivent être clairement marqués comme historiques ou réconciliés avec la cible.**

---

# 10. Stratégie de validation

Pendant la migration :

```text
MIG00 : tests de caractérisation historique
MIG01 : placement cible
MIG02 : spatial / boundary
MIG03 : composition visuelle / spawn / mécanismes / editor preview
MIG04+ : nouveaux tests ciblés par invariant
```

Commande standard :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects.MIGXX"
```

Avant MIG10, il faudra en plus une validation de contenu réel couvrant au minimum :

- porte simple ;
- porte double ;
- porte secrète ;
- bouton ;
- levier ;
- plaque de pression ;
- pit/trappe ;
- trigger invisible ;
- téléporteur ;
- réceptacle ;
- décoration statique ;
- lumière ;
- item au sol ;
- item en réceptacle ;
- item lancé ;
- monstre ;
- liens logiques ;
- transition inter-niveaux ;
- sauvegarde/rechargement des états mutables.

---

# 11. Vue finale attendue à MIG10

```text
Grimrock Data Model
│
├── Dungeon
│   └── UGridDungeonAsset
│       └── references UGridLevelAsset
│
├── Level
│   └── UGridLevelAsset
│       ├── structural grid
│       ├── WorldObjectInstances ─────► UGridWorldObjectDefinitionAsset
│       ├── LooseItemInstances ───────► UGridItemDefinitionAsset
│       ├── MonsterSpawns ────────────► UGridMonsterDefinitionAsset
│       ├── ItemSpawns ───────────────► UGridItemDefinitionAsset
│       ├── Environment ──────────────► UGridEnvironmentDefinitionAsset
│       ├── Readable refs ────────────► UGridReadableContentAsset
│       ├── Quest refs ───────────────► UGridQuestDefinitionAsset
│       └── logic / links / Lua / variables
│
├── Editor
│   └── UGridObjectPaletteAsset
│       └── catalogue de références, jamais source de vérité gameplay
│
└── Runtime / Save
    ├── actors exécutent Definition + Instance
    └── save stocke uniquement les deltas mutables
```

Cette structure est celle à garder en tête pour toutes les décisions prises entre MIG04 et MIG10.

---

## 12. Documents associés

- `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md` — architecture cible globale.
- `docs/Architecture/CORE_DUNGEON_LEVEL_GRID.md` — architecture actuelle du noyau donjon/niveau/grille.
- `docs/Design/WORLDOBJ_MIG01_PLACEMENT_SURFACE.md` — détail de MIG01.
- `docs/Design/WORLDOBJ_MIG03_4B_AUTHORING_MIGRATION.md` — historique de la migration d’authoring visuel.
- `docs/Design/07_GRID_OBJECT_ARCHETYPE_ASSET_AUDIT.md` — audit historique du modèle d’archétype.
- `docs/Design/08_GRID_OBJECT_ARCHETYPE_DATA_ASSETS_AUDIT.md` — audit des Data Assets historiques.
- `docs/Design/09_GRID_OBJECT_ARCHETYPE_NAMING_NORMALIZATION_PLAN.md` — historique de normalisation des noms.

Le présent document doit être mis à jour à chaque changement de jalon majeur `MIG04` à `MIG10`. Une modification de la **cible elle-même** doit également être reportée dans la mind map.