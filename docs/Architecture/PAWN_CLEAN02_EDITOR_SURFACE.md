# PAWN-CLEAN02 — Clean Party Pawn Editor Surface

Statut : implémenté côté éditeur ; validation UE5.5.4 locale requise.

## Objectif

`PAWN-CLEAN02` nettoie la surface d'authoring de `BP_GrimrockPartyPawn` et de son composant `GridPartyIllumination` sans modifier leur comportement runtime, leur sérialisation ni leurs API Blueprint.

Le ticket utilise une `IDetailCustomization` dans `GrimrockPrototypeEditor`. Les propriétés C++ continuent d'exister ; seules les propriétés et catégories qui ne sont pas utiles à l'authoring courant sont masquées dans le panneau Details.

## BP_GrimrockPartyPawn

Les réglages de gameplay et de présentation restent accessibles, notamment :

```text
Grid
Movement
Camera
Held Item
Input / Input Buffer
Inventory / UI
Combat / UI
RPG
```

Les états ou références d'exécution suivants sont masqués du panneau d'authoring :

```text
SceneRoot
SpringArm
Camera
HeldItemRoot
PartyInventoryComponent
MenuWidgetInstance
bInventoryWidgetVisible
CombatHudWidgetInstance
CharacterCreationWidgetInstance
bCharacterCreationModalActive
StoryCompanionRecruitmentWidgetInstance
bIsFreeLooking
HeldItemActor
HeldItemDefinitionId
bNativeMovementAudioPlaybackEnabled
```

`bNativeMovementAudioPlaybackEnabled` reste disponible en C++ pour les tests automatisés, mais n'est pas un réglage d'auteur.

Les catégories génériques héritées d'`APawn`/`AActor` qui constituent du bruit pour ce Blueprint sont également masquées, par exemple Actor, Pawn, Replication, Collision, Physics, Navigation, Tags et Cooking.

### Ce qui reste volontairement éditable

`LevelRuntimeActor`, `CurrentCellX`, `CurrentCellY` et `Facing` ne sont pas reclassés en pur état runtime. `AGrimrockPartyPawn::BeginPlay()` utilise les valeurs configurées comme fallback lorsque l'application du start du LevelAsset est désactivée, invalide ou indisponible ; `LevelRuntimeActor` peut également servir d'override explicite avant la résolution automatique.

Le ticket ne change donc aucune règle de spawn ou de position initiale.

## GridPartyIllumination

La surface d'authoring du composant est réduite aux éléments utiles :

```text
Transform
Party Illumination
  Intensity Multiplier
  Radius Multiplier
  Cast Shadows
```

Sont masqués dans ce contexte :

```text
Runtime Config
Active Source Id
Light
Component Tick
Rendering
Sockets
Tags
Component Replication
Activation
Cooking
Events
Physics
LOD
Asset User Data
Replication
Navigation
```

`RuntimeConfig` reste la copie runtime calculée depuis la source (`DA_Item_Torch::LightEmitter`, puis future magie/effet). Il n'est pas une donnée d'authoring du groupe et ne doit pas concurrencer la définition source.

`ActiveSourceId` reste disponible au runtime via l'API C++/Blueprint mais n'encombre plus l'authoring normal.

## Non-objectifs

PAWN-CLEAN02 ne :

- supprime aucune propriété runtime ;
- ne déplace pas les systèmes UI, Save ou Input vers de nouveaux assets ;
- ne change aucune valeur par défaut de gameplay ;
- ne modifie pas `DA_Item_Torch` ;
- ne modifie pas la sélection de la source lumineuse PARTY-LIGHT01 ;
- ne crée aucune couche de compatibilité.

## Validation

Le changement se trouve entièrement dans le module Editor ; le build `GrimrockPrototypeEditor` valide donc directement les nouvelles customizations.

Validation ciblée recommandée :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Party.LIGHT01"
```

Puis vérification manuelle :

1. ouvrir `BP_GrimrockPartyPawn` ;
2. vérifier que les catégories d'authoring Grimrock restent disponibles ;
3. vérifier que les états runtime/test-only listés ci-dessus n'encombrent plus Class Defaults ;
4. sélectionner `GridPartyIllumination` ;
5. vérifier que `Runtime Config`, `Active Source Id` et les catégories héritées masquées ne sont plus affichés ;
6. vérifier que Transform, `Intensity Multiplier`, `Radius Multiplier` et `Cast Shadows` restent éditables.
