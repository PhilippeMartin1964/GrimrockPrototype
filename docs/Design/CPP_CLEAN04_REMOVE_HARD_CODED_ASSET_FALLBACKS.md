# CPP-CLEAN04 — Remove Hard-Coded Asset Fallbacks

Date : **27 septembre 2026**  
Parent : **CPP-AUDIT01 — Audit général du code C++**

## Objectif

Retirer les deux dépendances runtime à des chemins d'assets Unreal codés en dur :

1. les meshes de chaîne chargés par `AGridDoorActor` avec `ConstructorHelpers::FObjectFinder` ;
2. le fallback `LoadClass` de `UGrimrockMainMenuWidget` vers `WBP_CharacterCreationWizard`.

Aucun nouveau système de configuration n'est ajouté. Les propriétaires déjà présents deviennent simplement autoritaires.

## 1. Door chain

Avant CPP-CLEAN04, le constructeur natif de `AGridDoorActor` chargeait :

~~~text
/Game/GrimrockPrototype/Meshes/Door/SM_Door_Chain_Support_01
/Game/GrimrockPrototype/Meshes/Door/SM_Door_Chain_Moving_01
~~~

Cela signifiait qu'un `AGridDoorActor` C++ connaissait directement des assets de contenu du projet.

Après CPP-CLEAN04 :

~~~text
FGridDoorAnimationParams
    -> présence / distance / durée de la chaîne

BP_GridDoorActor class defaults
    -> ChainSupportMesh
    -> ChainMovingMesh
    -> ChainMaterial éventuel

AGridDoorActor
    -> logique et animation uniquement
~~~

Le CDO natif doit donc laisser les deux meshes à `nullptr`. Le Blueprint runtime canonique doit les fournir.

Cette frontière correspond à l'historique du projet : le commit qui a introduit
`SM_Door_Chain_Support_01` et `SM_Door_Chain_Moving_01` a également modifié
`BP_GridDoorActor.uasset`.

## 2. Character creation frontend

Avant CPP-CLEAN04 :

~~~text
CharacterCreationWidgetClass configurée
        ou
LoadClass("/Game/.../WBP_CharacterCreationWizard")
~~~

Après CPP-CLEAN04 :

~~~text
WBP_MainMenu
    -> CharacterCreationWidgetClass
        -> unique autorité

classe absente
    -> New Game échoue explicitement
    -> Reason=NoCharacterCreationWidgetClass
~~~

Le runtime ne masque plus une mauvaise configuration du frontend.

## 3. Audit Editor

Nouveau test :

~~~text
Grimrock.CppCleanup.CPP_CLEAN04.AssetConfiguration
~~~

Il vérifie :

- le CDO natif `AGridDoorActor` ne contient aucun mesh de chaîne implicite ;
- `BP_GridDoorActor` charge et fournit `ChainSupportMesh` + `ChainMovingMesh` ;
- toute Definition Door de production avec `bHasChainMechanism=true` résout vers une classe Door dont le CDO fournit ces meshes ;
- `WBP_MainMenu` charge ;
- son `CharacterCreationWidgetClass` est configuré ;
- la classe configurée dérive de `URPGCharacterCreationWidget` ;
- les deux fichiers C++ concernés ne contiennent plus les chemins ou appels de fallback supprimés.

Ce test sert aussi d'audit des `.uasset` réels sans les modifier à l'aveugle.

## 4. Non-objectifs

CPP-CLEAN04 ne modifie pas :

- la géométrie principale des Doors ;
- `MovingParts[]` ;
- les règles `FGridDoorAnimationParams` ;
- les instances de niveau ;
- le wizard de création de personnage ;
- STARTUP-FLOW01/02 ;
- les DataAssets ou Blueprints binaires.

## 5. Validation

D'abord :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.CppCleanup.CPP_CLEAN04"
~~~

Puis les chaînes de porte :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Doors"
~~~

et :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects.RECOVERY01.C12.DoorChainInstanceOverrides"
~~~

Puis le startup :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.StartupFlow"
~~~

Enfin, en PIE/frontend :

- **New Game** ouvre le wizard ;
- annuler revient au menu ;
- créer le personnage poursuit vers le donjon ;
- une Door avec chaîne affiche support + partie mobile ;
- la chaîne reste cliquable et anime correctement.

L'ajout du nouveau test porte la baseline attendue à **964 tests** si aucun autre test n'est ajouté entre-temps.
