# WORLDOBJ-RECOVERY01 â€” Rapport final de rÃ©cupÃ©ration dâ€™authoring

Statut : **clÃ´ture proposÃ©e â€” validation D finale requise avant commit**  
Date : 2026-09-10  
Projet : GrimrockPrototype â€” Unreal Engine 5.5.4

## 1. Objet

`WORLDOBJ-RECOVERY01` a Ã©tÃ© ouvert aprÃ¨s la migration `GRID_OBJECT -> WORLDOBJ` afin de distinguer les donnÃ©es dâ€™authoring historiquement utiles qui avaient Ã©tÃ© perdues ou aplaties des anciens champs devenus sans objet dans le modÃ¨le courant.

La rÃ¨gle de rÃ©cupÃ©ration a Ã©tÃ© : **restaurer la sÃ©mantique utile, jamais lâ€™ancien schÃ©ma pour lui-mÃªme**.

Baseline historique :

```text
2ba8c18e9a6da6546c5df754247be2fcc5a9fcc7
```

Baseline avant rÃ©cupÃ©ration :

```text
21854ff5e7d1b9915b9754fbe1e4476e0a5c2be3
```

## 2. Ã‰tapes rÃ©alisÃ©es

| Ã‰tape | RÃ©sultat | Commit |
|---|---|---|
| B1.1 | Placement structurel et visuels statiques restaurÃ©s | `100fb072` |
| B1.2 | Motions canoniques des mÃ©canismes restaurÃ©es | `77f62899` |
| B1.3 | Pit contrÃ´lÃ© Ã  deux volets restaurÃ© | `04c54f52` |
| B2 | DÃ©corations, alcÃ´ves, escaliers et visuel Trigger restaurÃ©s | `7ae73c25` |
| C1.1 | Overrides sparse de parties mobiles par instance | `37dfad8a` |
| C1.2 | Overrides sparse de chaÃ®ne de porte par instance | `ae006c1e` |
| C2 | DurÃ©e inverse gÃ©nÃ©rique des motions | `62ec5350` |
| B3/P2 | `N=12` historique de MonsterSpawn / Logic objects | **non applicable â€” non restaurÃ©** |

## 3. Contrat final

### Definition

```text
UGridWorldObjectDefinitionAsset
â”œâ”€ PlacementSurface
â”œâ”€ DefaultLocalPosition U/V/N
â”œâ”€ StaticPart
â”œâ”€ MovingParts Part0/Part1
â”‚  â””â”€ Motion
â”œâ”€ DefaultBehavior
â”œâ”€ AudioEvents
â””â”€ RuntimeActorClass
```

### Instance world-object

```text
FGridWorldObjectInstance
â”œâ”€ identitÃ© / cellule / WallSide
â”œâ”€ Ã©tat initial / authoring
â””â”€ InstanceConfig
   â”œâ”€ Teleporter
   â”œâ”€ Transition
   â”œâ”€ Pit
   â”œâ”€ ReceptacleInitialContent
   â”œâ”€ bStartsUnlocked
   â”œâ”€ MovingPartOverrides[]
   â””â”€ Door chain overrides
```

`FGridWorldObjectMovingPartInstanceOverride` peut seulement surcharger `LocalTransform`, `Motion.Amount` et `Motion.Duration` dâ€™un `PartIndex` 0 ou 1. Il ne peut pas remplacer le mesh, le type, lâ€™axe, le pivot ou `ReverseDuration`.

Pour la chaÃ®ne de porte, lâ€™instance peut porter `DoorChainMode = Inherit | Enabled | Disabled` et un override optionnel de `ChainPullDuration`. `ChainPullDistance` reste exclusivement dans la Definition.

## 4. DurÃ©e inverse gÃ©nÃ©rique

```text
Duration         = Alpha 0 -> 1
ReverseDuration  = Alpha 1 -> 0
ReverseDuration <= 0  =>  Duration
```

Les boutons rÃ©cupÃ©rÃ©s utilisent :

```text
Button_Normal : 0.08 s forward / 0.10 s reverse
Button_Secret : 0.08 s forward / 0.10 s reverse
```

`ButtonHoldTime = 0.15 s` reste une rÃ¨gle logique.

## 5. DonnÃ©es volontairement non restaurÃ©es

Les six anciennes dÃ©finitions pickup suivantes ne doivent pas revenir :

```text
Item_BlueGem_Pickup
Item_CopperKey_Pickup
Item_Iron_Pickup
Item_TestNote_Pickup
ShurikenPickup
StonePickup
```

Les collectibles utilisent `UGridItemDefinitionAsset` + `FGridLooseItemInstance`.

Les anciens offsets `N=12` de `MonsterSpawn`, `CustomRecruiter_Service` et `StoryCompanion_Recruit` ne sont pas restaurÃ©s :

```text
MonsterSpawn
  -> FGridMonsterSpawnInstance
  -> transform centrÃ© sur GetCellCenterWorld()

StoryCompanion / CustomRecruiter
  -> FGridLogicObjectInstance
  -> cible logique/data-only centrÃ©e sur une cellule
```

RÃ©introduire `N`, `LocalOffset` ou `DefaultLocalPosition` dans ces structures crÃ©erait un canal spatial sans consommateur runtime.

## 6. DonnÃ©es rÃ©cupÃ©rÃ©es

RECOVERY01 a notamment restaurÃ© les positions `U/V/N` utiles des world objects, les meshes perdus de dÃ©corations/alcÃ´ves/escaliers/trigger, les motions canoniques des mÃ©canismes, le pit Ã  deux volets, 11 records de motion sparse sur 10 instances, 10 distinctions historiques de chaÃ®ne de porte et la distinction bouton `0.08 / 0.10`.

Les `LocalYaw` existants ont Ã©tÃ© conservÃ©s et nâ€™ont pas Ã©tÃ© normalisÃ©s arbitrairement.

## 7. AutoritÃ©s Ã  retenir

```text
Definition = ce quâ€™est lâ€™objet et ses defaults partagÃ©s
Instance   = oÃ¹ il est + exceptions rÃ©ellement propres Ã  ce placement
Runtime    = rÃ©solution Definition + Instance + Ã©tat courant
SaveGame   = deltas mutables nÃ©cessaires Ã  la restauration
```

Autres familles typÃ©es :

```text
Collectible  -> ItemDefinition + LooseItemInstance
Monster      -> MonsterDefinition + MonsterSpawnInstance
Logic/RPG    -> LogicObjectInstance
```

## 8. Validation D

Avant le commit de clÃ´ture :

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

Le test D protÃ¨ge notamment la dÃ©cision B3/P2 : `MonsterSpawn` et `LogicObject` restent des placements typÃ©s sans ancien champ spatial world-object.

## 9. ClÃ´ture

AprÃ¨s validation locale de ces trois filtres, `WORLDOBJ-RECOVERY01` peut Ãªtre considÃ©rÃ© comme terminÃ©. Toute future variation doit suivre le contrat courant et non rÃ©activer une structure historique simplement parce quâ€™elle existait avant la migration.
