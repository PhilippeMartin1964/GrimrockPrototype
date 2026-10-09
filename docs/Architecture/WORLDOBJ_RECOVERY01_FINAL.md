# WORLDOBJ-RECOVERY01 — Rapport final de récupération d’authoring

> **DOC-ARCH01 — HISTORIQUE / SNAPSHOT.** Ce document décrit le jalon indiqué à sa date et peut mentionner des APIs, versions Save, compteurs ou états de roadmap désormais superseded. Il ne constitue pas le schéma courant. Références actuelles : `PROJECT_SYNTHESIS.md`, `ARCHITECTURE_INDEX.md` et `Maps/GRIMROCK_PROJECT_MAP.md`.


Statut : **clôture proposée — validation D finale requise avant commit**  
Date : 2026-09-10  
Projet : GrimrockPrototype — Unreal Engine 5.5.4

## 1. Objet

`WORLDOBJ-RECOVERY01` a été ouvert après la migration `GRID_OBJECT -> WORLDOBJ` afin de distinguer les données d’authoring historiquement utiles qui avaient été perdues ou aplaties des anciens champs devenus sans objet dans le modèle courant.

La règle de récupération a été : **restaurer la sémantique utile, jamais l’ancien schéma pour lui-même**.

Baseline historique :

```text
2ba8c18e9a6da6546c5df754247be2fcc5a9fcc7
```

Baseline avant récupération :

```text
21854ff5e7d1b9915b9754fbe1e4476e0a5c2be3
```

## 2. Étapes réalisées

| Étape | Résultat | Commit |
|---|---|---|
| B1.1 | Placement structurel et visuels statiques restaurés | `100fb072` |
| B1.2 | Motions canoniques des mécanismes restaurées | `77f62899` |
| B1.3 | Pit contrôlé à deux volets restauré | `04c54f52` |
| B2 | Décorations, alcôves, escaliers et visuel Trigger restaurés | `7ae73c25` |
| C1.1 | Overrides sparse de parties mobiles par instance | `37dfad8a` |
| C1.2 | Overrides sparse de chaîne de porte par instance | `ae006c1e` |
| C2 | Durée inverse générique des motions | `62ec5350` |
| B3/P2 | `N=12` historique de MonsterSpawn / Logic objects | **non applicable — non restauré** |

## 3. Contrat final

### Definition

```text
UGridWorldObjectDefinitionAsset
├─ PlacementSurface
├─ DefaultLocalPosition U/V/N
├─ StaticPart
├─ MovingParts Part0/Part1
│  └─ Motion
├─ DefaultBehavior
├─ AudioEvents
└─ RuntimeActorClass
```

### Instance world-object

```text
FGridWorldObjectInstance
├─ identité / cellule / WallSide
├─ état initial / authoring
└─ InstanceConfig
   ├─ Teleporter
   ├─ Transition
   ├─ Pit
   ├─ ReceptacleInitialContent
   ├─ bStartsUnlocked
   ├─ MovingPartOverrides[]
   └─ Door chain overrides
```

`FGridWorldObjectMovingPartInstanceOverride` peut seulement surcharger `LocalTransform`, `Motion.Amount` et `Motion.Duration` d’un `PartIndex` existant. Il ne peut pas remplacer le mesh, le type, l’axe, le pivot ou `ReverseDuration`.

Pour la chaîne de porte, l’instance peut porter `DoorChainMode = Inherit | Enabled | Disabled` et un override optionnel de `ChainPullDuration`. `ChainPullDistance` reste exclusivement dans la Definition.

## 4. Durée inverse générique

```text
Duration         = Alpha 0 -> 1
ReverseDuration  = Alpha 1 -> 0
ReverseDuration <= 0  =>  Duration
```

Les boutons récupérés utilisent :

```text
Button_Normal : 0.08 s forward / 0.10 s reverse
Button_Secret : 0.08 s forward / 0.10 s reverse
```

`ButtonHoldTime = 0.15 s` reste une règle logique.

## 5. Données volontairement non restaurées

Les six anciennes définitions pickup suivantes ne doivent pas revenir :

```text
Item_BlueGem_Pickup
Item_CopperKey_Pickup
Item_Iron_Pickup
Item_TestNote_Pickup
ShurikenPickup
StonePickup
```

Les collectibles utilisent `UGridItemDefinitionAsset` + `FGridLooseItemInstance`.

Les anciens offsets `N=12` de `MonsterSpawn`, `CustomRecruiter_Service` et `StoryCompanion_Recruit` ne sont pas restaurés :

```text
MonsterSpawn
  -> FGridMonsterSpawnInstance
  -> transform centré sur GetCellCenterWorld()

StoryCompanion / CustomRecruiter
  -> FGridLogicObjectInstance
  -> cible logique/data-only centrée sur une cellule
```

Réintroduire `N`, `LocalOffset` ou `DefaultLocalPosition` dans ces structures créerait un canal spatial sans consommateur runtime.

## 6. Données récupérées

RECOVERY01 a notamment restauré les positions `U/V/N` utiles des world objects, les meshes perdus de décorations/alcôves/escaliers/trigger, les motions canoniques des mécanismes, le pit à deux volets, 11 records de motion sparse sur 10 instances, 10 distinctions historiques de chaîne de porte et la distinction bouton `0.08 / 0.10`.

Les `LocalYaw` existants ont été conservés et n’ont pas été normalisés arbitrairement.

## 7. Autorités à retenir

```text
Definition = ce qu’est l’objet et ses defaults partagés
Instance   = où il est + exceptions réellement propres à ce placement
Runtime    = résolution Definition + Instance + état courant
SaveGame   = deltas mutables nécessaires à la restauration
```

Autres familles typées :

```text
Collectible  -> ItemDefinition + LooseItemInstance
Monster      -> MonsterDefinition + MonsterSpawnInstance
Logic/RPG    -> LogicObjectInstance
```

## 8. Validation D

Avant le commit de clôture :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects.RECOVERY01.D.FinalArchitectureContract"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Pit.PIT03_2"
```

Le test D protège notamment la décision B3/P2 : `MonsterSpawn` et `LogicObject` restent des placements typés sans ancien champ spatial world-object.

## 9. Clôture

Après validation locale de ces trois filtres, `WORLDOBJ-RECOVERY01` peut être considéré comme terminé. Toute future variation doit suivre le contrat courant et non réactiver une structure historique simplement parce qu’elle existait avant la migration.
