# WORLDOBJ — Roadmap MIG00 à MIG10 et modèle de données cible

Statut : document directeur de migration — mise à jour 2026-09-07.

> Référence architecturale prioritaire : `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md`.
>
> Ce document décrit la trajectoire d’implémentation. La mind map décrit la cible. En cas de contradiction avec une ancienne note historique, la cible et les décisions de migration les plus récentes prévalent.

## 1. Objectif général

WORLDOBJ remplace progressivement un modèle d’archétype trop large et redondant par une architecture simple, orientée données et fondée sur une séparation stricte entre définition, instance de niveau et état runtime.

Invariants :

```text
une définition = un concept permanent
une instance = placement + configuration locale minimale
le runtime = exécution + état mutable
le SaveGame = deltas mutables uniquement
```

En particulier :

- un item ramassable possède une seule `UGridItemDefinitionAsset` ;
- un objet du monde possède une seule définition permanente ;
- un monstre possède une seule `UGridMonsterDefinitionAsset` ;
- le niveau référence les définitions, il ne les duplique pas ;
- la preview et le runtime consomment les mêmes données ;
- les Data Assets ne servent jamais à stocker un état runtime ;
- la grille et les placements du `UGridLevelAsset` restent la source de vérité spatiale ;
- le SaveGame ne copie pas les meshes, sons ou paramètres permanents des définitions.

Schéma cible :

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
├── structure de grille
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
├── LogicObjects
├── Links / Lua / variables / quêtes
└── départ / transitions
          │
          ▼
Runtime Actors / Components
          │
          ▼
Runtime State / SaveGame
```

## 2. État actuel

Révision de travail avant la clôture MIG09-C :

```text
91aac59ed0b6a4ba959bfd82e5f2cee3e17a1718
WORLDOBJ-MIG09 remove legacy animation writes from generic motion tests
```

État des jalons :

| Jalon | Statut | Résultat |
|---|---|---|
| MIG00 | ✅ validé | Caractérisation du contrat historique. |
| MIG01 | ✅ validé | Placement `Floor / Wall / Ceiling` + coordonnées locales `U/V/N`. |
| MIG02 | ✅ validé | Spatial/boundary simplifié. |
| MIG03 | ✅ intégré | Composition visuelle générique `StaticPart` + `MovingParts`. |
| MIG04 | ✅ intégré | Moteur générique de motion pour les mécanismes. |
| MIG05 | ✅ intégré | Collectible direct : autorité `UGridItemDefinitionAsset`. |
| MIG06 | ✅ intégré | Résolution Definition + overrides strictement instance-owned. |
| MIG07 | ✅ intégré | Fondation des collections de placements typées. |
| MIG08 | ✅ intégré | Service de migration/réenregistrement et collections typées utilisées par le contenu migré. |
| MIG09 | 🟨 en cours | A à B2C-B validés ; C candidat ; D/E restent à fermer. |
| MIG10 | ⬜ à faire | Renommage final `UGridObjectArchetypeAsset` → `UGridWorldObjectDefinitionAsset` et clôture. |

MIG10 ne doit pas commencer avant la suppression complète des ponts MIG09-D/E.

## 3. Modèle spatial cible

Une définition indique sa surface :

```text
PlacementSurface
├── Floor
├── Wall
└── Ceiling
```

La position locale utilise :

```text
U = première tangente de surface
V = seconde tangente ; verticale sur Wall
N = normale à la surface
```

Interprétation :

```text
Floor   : N = hauteur au-dessus du sol
Wall    : N = profondeur / inset
Ceiling : N = distance sous le plafond
```

Une frontière reste une notion topologique distincte du placement de surface.

## 4. Composition visuelle et motion

Le contrat visuel est générique :

```text
Visual
├── StaticPart optional
│   ├── Mesh
│   └── LocalTransform
└── MovingParts
    ├── Part0 optional
    │   ├── Mesh
    │   ├── LocalTransform
    │   └── Motion
    └── Part1 optional
        ├── Mesh
        ├── LocalTransform
        └── Motion
```

`Motion` contient :

```text
Type     = Rotation | Translation
Axis     = X | Y | Z
Pivot    = X/Y/Z
Amount   = degrés ou centimètres, signé
Duration = secondes
```

Après MIG09-C, ce bloc est l’unique autorité de géométrie et de durée pour les mécanismes.

Les champs spécialisés historiques suivants ne font plus partie du modèle cible :

```text
DoorAnimation.OpenHeight
DoorAnimation.MoveDuration
LeverAnimation.LeverOffPitch / LeverOnPitch / ToggleDuration
ButtonAnimation.ButtonPressDistance / ButtonPressDuration / ButtonReleaseDuration
PressurePlateAnimation.ReleasedHeightAboveFloor / PressedHeightAboveFloor / MoveDuration
PitAnimation.LeftHingeLocation / RightHingeLocation / OpenAngleDegrees / MoveDuration
```

Les règles gameplay distinctes restent séparées, par exemple `ButtonHoldTime`, les règles de poids d’une plaque ou le mécanisme de chaîne d’une porte.

## 5. Définition vs instance

Formule runtime :

```text
Effective runtime object
    = Definition
    + Level Instance Configuration
    + Saved Runtime Delta
```

### Définition

Possède notamment :

- identité permanente ;
- placement autorisé ;
- comportement spatial permanent ;
- meshes et `MovingParts[].Motion` ;
- interaction générique ;
- audio/VFX ;
- lumière ;
- classe runtime ;
- comportement par défaut partagé.

### Instance de niveau

Possède seulement ce qui est propre à ce placement :

- `InstanceId` stable ;
- référence à la définition ;
- cellule / côté de mur ;
- état initial ;
- Tag / Notes ;
- destination de téléporteur/transition ;
- contenu initial local ;
- overrides explicitement nécessaires.

Une instance ne recopie pas une géométrie d’animation qui appartient à la définition.

## 6. Items

Règle absolue :

```text
CopperKey
└── UGridItemDefinitionAsset
```

La même définition est utilisée lorsque l’item est :

- au sol ;
- dans une alcôve ;
- dans l’inventaire ;
- équipé ;
- tenu ;
- lancé ;
- créé par un spawn.

Il n’existe pas de seconde définition WorldObject uniquement parce que l’item est visible dans le monde.

L’identité runtime/persistante canonique est `ItemDefinitionId` / `ItemDefinitionAsset`, pas un ancien `ArchetypeId` d’item.

## 7. Placements typés du niveau

La cible de `UGridLevelAsset` est :

```text
Placements
├── WorldObjectInstances
├── LooseItemInstances
├── MonsterSpawns
├── ItemSpawns
└── LogicObjects
```

`LooseItemInstance` et `ItemSpawn` sont deux concepts distincts : le premier est un item présent, le second est un générateur.

MIG07/MIG08 ont introduit et migré cette direction. MIG09-D/E doivent maintenant supprimer les projections de compatibilité restantes.

## 8. MIG09 — purge finale avant renommage

### Déjà validé

```text
MIG09-A       autorité Definition sans marqueur sparse              ✅
MIG09-B1      suppression AGridItemActor::ArchetypeId               ✅
MIG09-B2A     suppression miroirs ItemArchetypeId réceptacles       ✅
MIG09-B2B1    retrait ArchetypeId du SaveGame item                  ✅
MIG09-B2B2    migration consommateurs SaveGame                      ✅
MIG09-B2B3    suppression proxy/cache ItemArchetypeId               ✅
MIG09-B2C-A   audit Blueprint anciennes API Item                    ✅
MIG09-B2C-B   suppression InitializeItem/GetItemArchetypeId         ✅
```

### MIG09-C — mécanismes et animation spécialisée

Candidat actuel :

- aucun ancien initializer spécialisé de Button/Lever/Door n’est une API runtime de production ;
- les champs d’animation spécialisés sont physiquement supprimés ;
- `AGridDoorActor::OpenHeight` disparaît ;
- les fixtures automatisées authorent directement la motion générique ;
- l’Inspector n’édite plus la géométrie d’animation au niveau de l’instance.

Validation locale UE5.5.4 requise avant de marquer C validé.

### MIG09-D — derniers consommateurs de compatibilité

À éliminer après validation de C :

```text
SparseBehaviorOverrideObjectIds
CommitCompatibilityObjectEdit()
RefreshLegacyObjectMirrorFromTyped()
GetObjectCompatibilityView()
GridLevelPlacementCompatibility
```

MIG09-D doit migrer les consommateurs réels vers les collections typées, sans introduire un nouveau proxy.

### MIG09-E — suppression du gros modèle historique

Dernière purge :

```text
UGridLevelAsset::Objects
FGridLevelObjectData
compatibility projection
```

Critère de sortie : le Grid Editor, le runtime et les tests consomment uniquement les placements typés.

## 9. MIG10 — renommage et clôture

Une fois MIG09-E validé :

```text
UGridObjectArchetypeAsset
        ↓
UGridWorldObjectDefinitionAsset
```

Le vocabulaire associé doit suivre :

```text
ArchetypeId              -> DefinitionId / WorldObjectDefinitionId
ObjectArchetypes         -> WorldObjectDefinitions
FindObjectArchetypeById  -> FindWorldObjectDefinition...
Archetype Default        -> Definition Default
```

Le renommage est volontairement le dernier jalon afin de ne pas combiner migration de schéma, compatibilité de contenu et renommage d’une classe sérialisée.

## 10. Ownership cible

| Couche | Possède | Ne possède pas |
|---|---|---|
| Definition Data Asset | Identité et propriétés permanentes. | Position de niveau, état runtime mutable. |
| `UGridLevelAsset` | Layout, placements, logique, état initial, références. | Copie complète des définitions, acteurs runtime. |
| Runtime Actor/Component | Exécution, animation, interaction, collisions, état courant. | Source d’authoring permanente. |
| SaveGame/RuntimeState | Deltas mutables. | Meshes, sons, stats permanentes, définition complète. |

## 11. Definition of Done finale

Avant clôture de MIG10 :

```text
[ ] aucun pont MIG01/MIG02/MIG03/MIG05/MIG06 requis
[ ] aucun Behavior visuel spécialisé dupliquant Motion
[ ] items = une définition unique
[ ] objets du monde = une définition unique
[ ] monstres = une définition unique
[ ] placements LevelAsset typés uniquement
[ ] preview et runtime consomment les mêmes définitions
[ ] SaveGame stocke uniquement des deltas mutables
[ ] tests automatisés protègent les invariants
[ ] niveaux réels jouables après migration
[ ] documentation réconciliée avec la mind map
```

## 12. Validation

Commande standard pour la tranche courante :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Les sous-systèmes directement touchés par MIG09-C doivent également rester compatibles avec leurs filtres spécialisés Door/Pit/Monster lors d’une validation élargie.

## 13. Documents associés

- `docs/Architecture/Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md` — architecture cible globale.
- `docs/Architecture/WORLDOBJ_MIG09_LEGACY_PURGE.md` — état détaillé de la purge MIG09.
- `docs/Architecture/WORLDOBJ_MIG04_GENERIC_MOTION.md` — historique de l’introduction du moteur générique de motion.
- `docs/Design/12_GRID_OBJECT_INSTANCE_BEHAVIOR_RULE.md` — règle Definition/Instance actuelle.

Les anciennes notes d’audit et de paramètres restent utiles comme historique, mais ne doivent pas être utilisées pour réintroduire un champ supprimé par la migration.